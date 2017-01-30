//
// Created by Chong Peng on 3/2/16.
//

#include "mpqc/chemistry/qc/lcao/integrals/ao_factory_base.h"

namespace mpqc {
namespace lcao {
namespace gaussian {

namespace detail {

libint2::Operator to_libint2_operator(Operator::Type mpqc_oper) {
  TA_USER_ASSERT((Operator::Type::__first_1body_operator <= mpqc_oper &&
                  mpqc_oper <= Operator::Type::__last_1body_operator) ||
                     (Operator::Type::__first_2body_operator <= mpqc_oper &&
                      mpqc_oper <= Operator::Type::__last_2body_operator),
                 "invalid Operator::Type");

  libint2::Operator result;
  switch (mpqc_oper) {
    case Operator::Type::Overlap: {
      result = libint2::Operator::overlap;
    } break;
    case Operator::Type::Kinetic: {
      result = libint2::Operator::kinetic;
    } break;
    case Operator::Type::Nuclear: {
      result = libint2::Operator::nuclear;
    } break;
    case Operator::Type::Coulomb: {
      result = libint2::Operator::coulomb;
    } break;
    case Operator::Type::cGTG: {
      result = libint2::Operator::cgtg;
    } break;
    case Operator::Type::cGTG2: {
      result = libint2::Operator::cgtg;
    } break;
    case Operator::Type::cGTGCoulomb: {
      result = libint2::Operator::cgtg_x_coulomb;
    } break;
    case Operator::Type::DelcGTG2: {
      result = libint2::Operator::delcgtg2;
    } break;
    default: {
      TA_USER_ASSERT(
          false, "mpqc::Operator::Type not convertible to libint2::Operator");
    }
  }
  return result;
}

libint2::any to_libint2_operator_params(Operator::Type mpqc_oper,
                                        const AOFactoryBase &base) {
  TA_USER_ASSERT((Operator::Type::__first_1body_operator <= mpqc_oper &&
                  mpqc_oper <= Operator::Type::__last_1body_operator) ||
                     (Operator::Type::__first_2body_operator <= mpqc_oper &&
                      mpqc_oper <= Operator::Type::__last_2body_operator),
                 "invalid Operator::Type");

  libint2::any result;
  switch (mpqc_oper) {
    case Operator::Type::Nuclear: {
      result = make_q(base.molecule());
    } break;
    case Operator::Type::cGTG:
    case Operator::Type::cGTGCoulomb:
    case Operator::Type::DelcGTG2: {
      result = base.gtg_params();
    } break;
    case Operator::Type::cGTG2: {
      const auto &cgtg_params = base.gtg_params();
      const auto ng = cgtg_params.size();
      std::decay<decltype(cgtg_params)>::type cgtg2_params;
      cgtg2_params.reserve(ng * (ng + 1) / 2);
      for (auto b = 0; b < ng; ++b) {
        for (auto k = 0; k <= b; ++k) {
          const auto gexp = cgtg_params[b].first + cgtg_params[k].first;
          const auto gcoeff = cgtg_params[b].second * cgtg_params[k].second *
                              (b == k ? 1 : 2);  // if a != b include ab and ba
          cgtg2_params.push_back(std::make_pair(gexp, gcoeff));
        }
      }
      result = cgtg2_params;
    } break;
    default:;  // nothing to do
  }
  return result;
}
}

AOFactoryBase::AOFactoryBase(const KeyVal &kv)
    : world_(*kv.value<madness::World*>("$:world")),
      orbital_basis_registry_(),
      mol_(),
      gtg_params_() {

  std::string prefix = "";
  if(kv.exists("wfn_world") || kv.exists_class("wfn_world")){
    prefix = "wfn_world:";
  }
  /// Basis will come from wfn_world
  //  orbital_basis_registry_ = std::make_shared<basis::OrbitalBasisRegistry>(basis::OrbitalBasisRegistry(kv));
  mol_ = kv.class_ptr<Molecule>(prefix + "molecule");

  // if have auxilary basis
  if(kv.exists( prefix + "aux_basis")){
    int n_function = kv.value<int>(prefix + "corr_functions",6);
    double corr_param = kv.value<double>(prefix + "corr_param",0);
    f12::GTGParams gtg_params;
    if(corr_param != 0){
      gtg_params = f12::GTGParams(corr_param, n_function);
    } else{
      if(kv.exists(prefix + "vir_basis")){
        std::string basis_name = kv.value<std::string>(prefix + "vir_basis:name");
        gtg_params = f12::GTGParams(basis_name, n_function);
      }else{
        std::string basis_name = kv.value<std::string>(prefix + "basis:name");
        gtg_params = f12::GTGParams(basis_name, n_function);
      }
    }
    gtg_params_ = gtg_params.compute();
    if (world().rank() == 0) {
      std::cout << "F12 Correlation Factor: " << gtg_params.exponent
                << std::endl;
      std::cout << "NFunction: " << gtg_params.n_fit << std::endl;
      std::cout << "F12 Exponent Coefficient" << std::endl;
      for (auto &pair : gtg_params_) {
        std::cout << pair.first << " " << pair.second << std::endl;
      }
      std::cout << std::endl;
    }
  }
  // other initialization
  screen_ = kv.value<std::string>(prefix + "screen","");
  screen_threshold_ = kv.value<double>(prefix + "threshold",1.0e-10);
  auto default_precision = std::numeric_limits<double>::epsilon();
  precision_ = kv.value<double>(prefix + "precision",default_precision);
  detail::integral_engine_precision = precision_;

  utility::print_par(world_, "Screen: ", screen_, "\n");
  if (!screen_.empty()) {
    utility::print_par(world_, "Threshold: ", screen_threshold_, "\n");
  }
  utility::print_par(world_, "Precision: ", precision_, "\n");
  utility::print_par(world_, "\n");


}

libint2::Engine AOFactoryBase::make_engine(const Operator &oper,
                                                int64_t max_nprim,
                                                int64_t max_am) {
  auto op = detail::to_libint2_operator(oper.type());
  auto params = detail::to_libint2_operator_params(oper.type(), *this);
  libint2::Engine engine(op, max_nprim, static_cast<int>(max_am), 0);
  engine.set_params(std::move(params));

  return engine;
}

std::shared_ptr<Screener> AOFactoryBase::make_screener_three_center(
    ShrPool<libint2::Engine> &engine, Basis &basis1,
    Basis &basis2) {
  std::shared_ptr<Screener> p_screen;
  if (screen_.empty()) {
    p_screen = std::make_shared<Screener>(Screener{});
  } else if (screen_ == "qqr") {
    p_screen = std::make_shared<Screener>(Screener{});
  } else if (screen_ == "schwarz") {
    auto screen_builder = init_schwarz_screen(screen_threshold_);
    p_screen = std::make_shared<Screener>(
        screen_builder(world_, engine, basis1, basis2));
  } else {
    throw std::runtime_error("Wrong Screening Method!");
  }
  return p_screen;
}

std::shared_ptr<Screener> AOFactoryBase::make_screener_four_center(
    ShrPool<libint2::Engine> &engine, Basis &basis) {
  std::shared_ptr<Screener> p_screen;
  if (screen_.empty()) {
    p_screen = std::make_shared<Screener>(Screener{});
  } else if (screen_ == "qqr") {
    auto screen_builder = init_qqr_screen{};
    p_screen = std::make_shared<Screener>(
        screen_builder(world_, engine, basis, screen_threshold_));
  } else if (screen_ == "schwarz") {
    auto screen_builder = init_schwarz_screen(screen_threshold_);
    p_screen = std::make_shared<Screener>(
        screen_builder(world_, engine, basis));
  } else {
    throw std::runtime_error("Wrong Screening Method!");
  }
  return p_screen;
}

void AOFactoryBase::parse_one_body(
    const Formula &formula,
    std::shared_ptr<utility::TSPool<libint2::Engine>> &engine_pool,
    BasisVector &bases) {
  auto bra_indices = formula.bra_indices();
  auto ket_indices = formula.ket_indices();

  TA_ASSERT(bra_indices.size() == 1);
  TA_ASSERT(ket_indices.size() == 1);

  auto bra_index = bra_indices[0];
  auto ket_index = ket_indices[0];

  TA_ASSERT(bra_index.is_ao());
  TA_ASSERT(ket_index.is_ao());

  auto bra_basis = this->index_to_basis(bra_index);
  auto ket_basis = this->index_to_basis(ket_index);

  TA_ASSERT(bra_basis != nullptr);
  TA_ASSERT(ket_basis != nullptr);

  bases = BasisVector {{*bra_basis, *ket_basis}};

  auto oper_type = formula.oper().type();
  engine_pool = make_engine_pool(
      detail::to_libint2_operator(oper_type),
      utility::make_array_of_refs(*bra_basis, *ket_basis), libint2::BraKet::x_x,
      detail::to_libint2_operator_params(oper_type, *this));
}

void AOFactoryBase::parse_two_body_two_center(
    const Formula &formula,
    std::shared_ptr<utility::TSPool<libint2::Engine>> &engine_pool,
    BasisVector &bases) {
  TA_USER_ASSERT(formula.notation() == Formula::Notation::Chemical,
                 "Two Body Two Center Integral Must Use Chemical Notation");

  auto bra_indexs = formula.bra_indices();
  auto ket_indexs = formula.ket_indices();

  TA_ASSERT(bra_indexs.size() == 1);
  TA_ASSERT(ket_indexs.size() == 1);

  auto bra_index0 = bra_indexs[0];
  auto ket_index0 = ket_indexs[0];

  TA_ASSERT(ket_index0.is_ao());
  TA_ASSERT(bra_index0.is_ao());

  auto bra_basis0 = this->index_to_basis(bra_index0);
  auto ket_basis0 = this->index_to_basis(ket_index0);

  TA_ASSERT(bra_basis0 != nullptr);
  TA_ASSERT(ket_basis0 != nullptr);

  bases = BasisVector {{*bra_basis0, *ket_basis0}};

  auto oper_type = formula.oper().type();
  engine_pool = make_engine_pool(
      detail::to_libint2_operator(oper_type),
      utility::make_array_of_refs(*bra_basis0, *ket_basis0),
      libint2::BraKet::xs_xs,
      detail::to_libint2_operator_params(oper_type, *this));
}

void AOFactoryBase::parse_two_body_three_center(
    const Formula &formula,
    std::shared_ptr<utility::TSPool<libint2::Engine>> &engine_pool, BasisVector &bases,
    std::shared_ptr<Screener> &p_screener) {
  TA_USER_ASSERT(formula.notation() == Formula::Notation::Chemical,
                 "Three Center Integral Must Use Chemical Notation");

  auto bra_indexs = formula.bra_indices();
  auto ket_indexs = formula.ket_indices();

  TA_ASSERT(bra_indexs.size() == 1);
  TA_ASSERT(ket_indexs.size() == 2);

  TA_ASSERT(bra_indexs[0].is_ao());
  TA_ASSERT(ket_indexs[0].is_ao());
  TA_ASSERT(ket_indexs[1].is_ao());

  auto bra_basis0 = this->index_to_basis(bra_indexs[0]);
  auto ket_basis0 = this->index_to_basis(ket_indexs[0]);
  auto ket_basis1 = this->index_to_basis(ket_indexs[1]);

  TA_ASSERT(bra_basis0 != nullptr);
  TA_ASSERT(ket_basis0 != nullptr);
  TA_ASSERT(ket_basis1 != nullptr);

  bases = BasisVector {{*bra_basis0, *ket_basis0, *ket_basis1}};

  auto oper_type = formula.oper().type();
  engine_pool = make_engine_pool(
      detail::to_libint2_operator(oper_type),
      utility::make_array_of_refs(*bra_basis0, *ket_basis0, *ket_basis1),
      libint2::BraKet::xs_xx,
      detail::to_libint2_operator_params(oper_type, *this));

  if (!screen_.empty() && (ket_indexs[0] == ket_indexs[1])) {
    /// make another engine to screener!!!

    auto screen_engine_pool = make_engine_pool(
        detail::to_libint2_operator(oper_type),
        utility::make_array_of_refs(*bra_basis0, *ket_basis0, *ket_basis1),
        libint2::BraKet::xx_xx,
        detail::to_libint2_operator_params(oper_type, *this));

    p_screener = make_screener_three_center(screen_engine_pool, *bra_basis0,
                                            *ket_basis0);
  }
}

void AOFactoryBase::parse_two_body_four_center(
    const Formula &formula,
    std::shared_ptr<utility::TSPool<libint2::Engine>> &engine_pool, BasisVector &bases,
    std::shared_ptr<Screener> &p_screener) {
  auto bra_indexs = formula.bra_indices();
  auto ket_indexs = formula.ket_indices();

  TA_ASSERT(bra_indexs.size() == 2);
  TA_ASSERT(ket_indexs.size() == 2);

  TA_ASSERT(bra_indexs[0].is_ao());
  TA_ASSERT(ket_indexs[0].is_ao());
  TA_ASSERT(bra_indexs[1].is_ao());
  TA_ASSERT(ket_indexs[1].is_ao());

  auto bra_basis0 = this->index_to_basis(bra_indexs[0]);
  auto ket_basis0 = this->index_to_basis(ket_indexs[0]);
  auto bra_basis1 = this->index_to_basis(bra_indexs[1]);
  auto ket_basis1 = this->index_to_basis(ket_indexs[1]);

  TA_ASSERT(bra_basis0 != nullptr);
  TA_ASSERT(ket_basis0 != nullptr);
  TA_ASSERT(bra_basis1 != nullptr);
  TA_ASSERT(ket_basis1 != nullptr);

  if (formula.notation() == Formula::Notation::Chemical) {
    bases = {{*bra_basis0, *bra_basis1, *ket_basis0, *ket_basis1}};
  } else {
    bases = {{*bra_basis0, *ket_basis0, *bra_basis1, *ket_basis1}};
  }

  auto oper_type = formula.oper().type();
  engine_pool = make_engine_pool(
      detail::to_libint2_operator(oper_type),
      utility::make_array_of_refs(bases[0], bases[1], bases[2], bases[3]),
      libint2::BraKet::xx_xx,
      detail::to_libint2_operator_params(oper_type, *this));

  if ((bra_indexs[0] == bra_indexs[1]) && (ket_indexs[0] == ket_indexs[1]) &&
      (ket_indexs[0] == bra_indexs[0])) {
    p_screener = make_screener_four_center(engine_pool, *bra_basis0);
  }
}

std::array<std::wstring, 3> AOFactoryBase::get_df_formula(
    const Formula &formula) {
  std::array<std::wstring, 3> result;

  // chemical notation
  if (formula.notation() == Formula::Notation::Chemical) {
    std::wstring left = L"( Κ |" + formula.oper().oper_string() + L"| " +
                        formula.bra_indices()[0].name() + L" " +
                        formula.bra_indices()[1].name() + L" )";
    std::wstring right = L"( Κ |" + formula.oper().oper_string() + L"| " +
                         formula.ket_indices()[0].name() + L" " +
                         formula.ket_indices()[1].name() + L" )";
    std::wstring center =
        L"( Κ |" + formula.oper().oper_string() + L"| Λ)[inv]";
    result[0] = left;
    result[1] = center;
    result[2] = right;
  }
  // physical notation
  else {
    std::wstring left = L"( Κ |" + formula.oper().oper_string() + L"| " +
                        formula.bra_indices()[0].name() + L" " +
                        formula.ket_indices()[0].name() + L" )";
    std::wstring right = L"( Κ |" + formula.oper().oper_string() + L"| " +
                         formula.bra_indices()[1].name() + L" " +
                         formula.ket_indices()[1].name() + L" )";
    std::wstring center =
        L"( Κ |" + formula.oper().oper_string() + L"| Λ)[inv]";
    result[0] = left;
    result[1] = center;
    result[2] = right;
  }

  return result;
}

Formula AOFactoryBase::get_jk_formula(const Formula &formula,
                                           const std::wstring &obs) {
  Formula result;

  Operator oper(L"G");
  result.set_operator(oper);
  if (formula.notation() == Formula::Notation::Chemical) {
    result.set_notation(Formula::Notation::Chemical);
    if (formula.oper().type() == Operator::Type::J) {
      result.bra_indices().push_back(formula.bra_indices()[0]);
      result.bra_indices().push_back(formula.ket_indices()[0]);
      result.ket_indices().push_back(obs+L"4");
      result.ket_indices().push_back(obs+L"5");

    } else {
      result.bra_indices().push_back(formula.bra_indices()[0]);
      result.bra_indices().push_back(obs+L"4");
      result.ket_indices().push_back(formula.ket_indices()[0]);
      result.ket_indices().push_back(obs+L"5");
    }
  } else {
    result.set_notation(Formula::Notation::Physical);
    if (formula.oper().type() == Operator::Type::K) {
      result.bra_indices().push_back(formula.bra_indices()[0]);
      result.bra_indices().push_back(formula.ket_indices()[0]);
      result.ket_indices().push_back(obs+L"4");
      result.ket_indices().push_back(obs+L"5");

    } else {
      result.bra_indices().push_back(formula.bra_indices()[0]);
      result.bra_indices().push_back(obs+L"4");
      result.ket_indices().push_back(formula.ket_indices()[0]);
      result.ket_indices().push_back(obs+L"5");
    }
  }
  return result;
}

std::array<Formula, 3> AOFactoryBase::get_jk_df_formula(
    const Formula &formula, const std::wstring &obs) {
  std::array<Formula, 3> result;

  if (formula.oper().type() == Operator::Type::J) {
    std::wstring left = L"( Κ |G| " + formula.bra_indices()[0].name() + L" " +
                        formula.ket_indices()[0].name() + L" )";
    std::wstring right = L"( Κ |G| " + obs + L"4 " + obs + L"5 )";

    result[0] = Formula(left);
    result[2] = Formula(right);
  } else {
    std::wstring left =
        L"( Κ |G| " + formula.bra_indices()[0].name() + L" " + obs + L"4 )";
    std::wstring right =
        L"( Κ |G| " + formula.ket_indices()[0].name() + L" " + obs + L"5 )";

    result[0] = Formula(left);
    result[2] = Formula(right);
  }
  std::wstring center = L"( Κ |G| Λ)[inv]";
  result[1] = Formula(center);

  return result;
}

OrbitalIndex AOFactoryBase::get_jk_orbital_space(
    const Operator &operation) {
  if (operation.type() == Operator::Type::J ||
      operation.type() == Operator::Type::K) {
    return OrbitalIndex(L"m");
  } else if (operation.type() == Operator::Type::KAlpha) {
    return OrbitalIndex(L"m_α");
  } else if (operation.type() == Operator::Type::KBeta) {
    return OrbitalIndex(L"m_β");
  } else {
    assert(false);
    return OrbitalIndex{};
  }
}

std::array<Formula, 3> AOFactoryBase::get_fock_formula(
    const Formula &formula) {
  std::array<Formula, 3> result;
  Formula h(formula);
  h.set_operator(Operator(L"H"));
  decltype(h) j(formula);
  j.set_operator_type(Operator::Type::J);
  decltype(h) k(formula);
  if (formula.oper().type() == Operator::Type::Fock) {
    k.set_operator_type(Operator::Type::K);
  } else if (formula.oper().type() == Operator::Type::FockAlpha) {
    k.set_operator_type(Operator::Type::KAlpha);
  } else if (formula.oper().type() == Operator::Type::FockBeta) {
    k.set_operator_type(Operator::Type::KBeta);
  }

  result[0] = h;
  result[1] = j;
  result[2] = k;
  return result;
}

}  // namespace gaussian
}  // namespace lcao
}  // namespace mpqc