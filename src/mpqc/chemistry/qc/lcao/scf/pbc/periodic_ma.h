#ifndef MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_PBC_PERIODIC_MA_H_
#define MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_PBC_PERIODIC_MA_H_

#include "mpqc/chemistry/qc/lcao/factory/periodic_ao_factory.h"
#include "mpqc/chemistry/qc/lcao/scf/pbc/util.h"

#include <boost/math/special_functions/factorials.hpp>
#include <boost/math/special_functions/legendre.hpp>

namespace mpqc {
namespace pbc {

namespace detail {

/*!
 * @brief Class to hold information for basis shell pairs, including total
 * number of shell pairs, extents (radii) and centers of shell pairs, etc.
 */
class BasisPairInfo {
 public:
  using Shell = ::mpqc::lcao::gaussian::Shell;
  using Basis = ::mpqc::lcao::gaussian::Basis;

  BasisPairInfo() = default;

  /*!
   * @brief This constructs BasisPairInfo between two basis sets
   * @param bs0 first basis set
   * @param bs1 second basis set
   * @param thresh threshold of the first-order multipole expansion error
   * @param small_extent a small value that is used when no real solution exists
   * for any shell pair
   */
  BasisPairInfo(std::shared_ptr<const Basis> bs0,
                std::shared_ptr<const Basis> bs1, const double thresh = 1.0e-6,
                const double small_extent = 0.01);

 private:
  const double thresh_;        ///> threshold of multipole expansion error
  const double small_extent_;  ///> a small extent will be used in case no real
                               /// solution exists

  std::shared_ptr<const Basis> bs0_;
  std::shared_ptr<const Basis> bs1_;
  size_t nshells0_;
  size_t nshells1_;
  size_t npairs_;
  RowMatrixXd pair_extents_;
  std::vector<std::vector<Vector3d>> pair_centers_;

 public:
  /*!
   * @brief This returns the extent (radius) of a pair of shells (shell \em i
   * and shell \em j)
   * @param i index of shell \em i
   * @param j index of shell \em j
   * @return the extent (radius)
   */
  double extent(int64_t i, int64_t j) const { return pair_extents_(i, j); }

  /*!
   * @brief This returns the extent (radius) of a pair of shells (shell \em i
   * and shell \em j)
   * @param ij the ordinal index of the shell pair (\em i, \em j)
   * @return the extent (radius)
   */
  double extent(int64_t ij) const;

  /*!
   * @brief This returns the weighted center of a pair of shells (shell \em i
   * and shell \em j)
   * @param i index of shell \em i
   * @param j index of shell \em j
   * @return the weighted center
   */
  Vector3d center(int64_t i, int64_t j) const { return pair_centers_[i][j]; }

  /*!
   * @brief This returns the weighted center of a pair of shells (shell \em i
   * and shell \em j)
   * @param ij the ordinal index of the shell pair (\em i, \em j)
   * @return the weighted center
   */
  Vector3d center(int64_t ij) const;

  /*!
   * @brief This returns the total number of shell pairs
   */
  size_t npairs() const { return npairs_; }

  /*!
   * \brief This computes maximum distance between \c ref_point and all charge
   * centers comprising the product density
   */
  double max_distance_to(const Vector3d &ref_point);

 private:
  /*!
   * \brief This computes the center and the extent of the product of two shells
   * \param sh0
   * \param sh1
   * \return
   */
  std::pair<Vector3d, double> shell_pair_center_extent(const Shell &sh0,
                                                       const Shell &sh1);

  /*!
   * \brief This computes the extent (radius) of the product of two primitives
   * with exponents \c exp0 and \c exp1, respectively. \param exp0 exponent of
   * the first primitive function \param exp1 exponent of the second primitive
   * function \param rab distance between centers of two primitives \return the
   * extent (radius)
   */
  double prim_pair_extent(const double exp0, const double exp1,
                          const double rab);
};

}  // namespace detail

namespace ma {

/*!
 * \brief This class computes the contribution to Coulomb interaction from unit
 * cells in Crystal Far Field (CFF) using multipole approximation.
 */
template <typename Factory,
          libint2::Operator Oper = libint2::Operator::sphemultipole>
class PeriodicMA {
 public:
  using TArray = TA::DistArray<TA::TensorD, TA::SparsePolicy>;
  using Shell = ::mpqc::lcao::gaussian::Shell;
  using ShellVec = ::mpqc::lcao::gaussian::ShellVec;
  using Basis = ::mpqc::lcao::gaussian::Basis;
  using Ord2lmMap = std::unordered_map<unsigned int, std::pair<int, int>>;
  using UnitCellList = std::vector<std::pair<Vector3i, Vector3d>>;
  using Sphere2UCListMap = std::unordered_map<size_t, UnitCellList>;
  template <typename T, unsigned int nops = libint2::operator_traits<Oper>::nopers>
  using MultipoleMoment = std::array<T, nops>;
  /*!
   * @brief This constructs PeriodicMA using a \c PeriodicAOFactory object
   * @param ao_factory a \c PeriodicAOFactory object
   * @param ma_thresh threshold of multipole expansion error
   * @param ws well-separateness criterion
   */
  PeriodicMA(Factory &ao_factory, double ma_thresh = 1.0e-6, double bs_extent_thresh = 1.0e-6, double ws = 3.0)
      : ao_factory_(ao_factory), ma_thresh_(ma_thresh), bs_extent_thresh_(bs_extent_thresh), ws_(ws) {
    auto &world = ao_factory_.world();
    obs_ = ao_factory_.basis_registry()->retrieve(OrbitalIndex(L"λ"));
    dfbs_ = ao_factory_.basis_registry()->retrieve(OrbitalIndex(L"Κ"));

    dcell_ = ao_factory_.unitcell().dcell();
    R_max_ = ao_factory_.R_max();
    RJ_max_ = ao_factory_.RJ_max();
    RD_max_ = ao_factory_.RD_max();
    R_size_ = ao_factory_.R_size();
    RJ_size_ = ao_factory_.RJ_size();
    RD_size_ = ao_factory_.RD_size();

    // determine dimensionality of crystal
    dimensionality_ = 0;
    for (auto dim = 0; dim <= 2; ++dim) {
      if (RJ_max_(dim) > 0) {
        dimensionality_++;
      }
    }
    ExEnv::out0() << "\nCrystal dimensionality : " << dimensionality_
                  << std::endl;

    // compute centers and extents of product density between the reference
    // unit cell and its neighbours
    ref_pairs_ = construct_basis_pairs();
    // compute maximum distance between the center of mass of unit cell atoms
    // and all charge centers
    const auto &ref_com = ao_factory_.unitcell().com();
    max_distance_to_refcenter_ = ref_pairs_->max_distance_to(ref_com);

    // determine CFF boundary
    cff_boundary_ = compute_CFF_boundary(RJ_max_);

    ExEnv::out0() << "\nThe boundary of Crystal Far Field is "
                  << cff_boundary_.transpose() << std::endl;

    // compute spherical multipole moments (the chargeless version, before being
    // contracted with density matrix)
    sphemm_ = ao_factory_.template compute_array<Oper>(L"<κ|O|λ>");

    // make a map from ordinal indices of (l, m) to (l, m) pairs
    O_ord_to_lm_map_ = make_ord_to_lm_map<MULTIPOLE_MAX_ORDER>();
    M_ord_to_lm_map_ = make_ord_to_lm_map<2 * MULTIPOLE_MAX_ORDER>();

  }

 private:
  Factory &ao_factory_;
  const double ma_thresh_;  /// multipole approximation is considered converged when the energy difference between two iterations of unit cell spheres is below this value
  const double bs_extent_thresh_;  /// threshold used in computing extent of a pair of primitives
  const double ws_;         /// well-separateness criterion

  MultipoleMoment<TArray> sphemm_;

  static constexpr unsigned int nopers_ = libint2::operator_traits<Oper>::nopers;
  static constexpr unsigned int nopers_doubled_lmax_ = (2 * MULTIPOLE_MAX_ORDER + 1) * (2 * MULTIPOLE_MAX_ORDER + 1);

  Ord2lmMap O_ord_to_lm_map_;
  Ord2lmMap M_ord_to_lm_map_;

  Sphere2UCListMap cff_sphere_to_unitcells_map_;
  int dimensionality_;  // dimensionality of crystal

  std::shared_ptr<Basis> obs_;
  std::shared_ptr<Basis> dfbs_;
  Vector3d dcell_;
  Vector3i R_max_;
  Vector3i RJ_max_;
  Vector3i RD_max_;
  int64_t R_size_;
  int64_t RJ_size_;
  int64_t RD_size_;

  std::vector<Vector3i> uc_near_list_;
  std::shared_ptr<detail::BasisPairInfo> ref_pairs_;
  double max_distance_to_refcenter_;
  Vector3i cff_boundary_;

 public:
  const Vector3i &CFF_boundary() { return cff_boundary_; }

  double compute_energy(const TArray &D, double target_precision) {
    // compute electronic multipole moments for the reference unit cell
    auto O_elec = compute_multipole_moments(sphemm_, D);

    double e_total = 0.0;
    double e_sphere = 0.0;
    double e_diff = 0.0;
    bool converged = false;
    bool out_of_rjmax = false;
    size_t cff_sphere_idx = 0;
    Vector3i sphere_thickness_next = {0, 0, 0};

    do {
      auto e_old = e_sphere;

      auto sphere_iter = cff_sphere_to_unitcells_map_.find(cff_sphere_idx);
      // build a list of unit cells for a sphere if does not exist
      if (sphere_iter == cff_sphere_to_unitcells_map_.end()) {
        auto unitcell_list = build_unitcells_on_a_sphere(cff_boundary_, cff_sphere_idx);
        sphere_iter = cff_sphere_to_unitcells_map_.insert({cff_sphere_idx, unitcell_list}).first;
      }

      ExEnv::out0() << "\nSphere index in CFF: " << sphere_iter->first << std::endl;

      e_sphere = 0.0;
      for (const auto &unitcell : sphere_iter->second) {
        const auto &unitcell_idx = unitcell.first;
        const auto &unitcell_vec = unitcell.second;

        // compute interaction kernel between multipole moments centered at the
        // reference unit cell and a remote unit cell
        // TODO attach each M to a unit cell in UnitCellList to avoid recomputing though it may cause memory issues
        auto M = build_interaction_kernel<2 * MULTIPOLE_MAX_ORDER>(Vector3d::Zero() - unitcell_vec);

        // compute local potential created by a remote unit cell
        auto L = build_local_potential(O_elec, M);

        // compute multipole interaction energy per unit cell
        double e_unitcell = compute_energy_per_unitcell(O_elec, L);

        e_sphere += e_unitcell;

        ExEnv::out0() << "\tmultipole interaction energy for unit cell (" << unitcell_idx.transpose() << ") = " << e_unitcell << std::endl;
      }
      ExEnv::out0() << "multipole interaction energy for sphere [" << cff_sphere_idx << "] = " << e_sphere << std::endl;

      e_diff = e_sphere - e_old;
      ExEnv::out0() << "Delta(E_ma_per_sphere) = " << e_diff << std::endl;

      e_total += e_sphere;

      // determine if the energy is converged
      if (std::abs(e_diff) < ma_thresh_) {
        converged = true;
      }

      // determine if the next sphere is out of rjmax range
      for (auto dim = 0; dim <= 2; ++dim) {
        if (RJ_max_(dim) > 0) {
          sphere_thickness_next(dim) = cff_sphere_idx + 1;
        }
      }
      Vector3i corner_idx_next = cff_boundary_ + sphere_thickness_next;
      if (corner_idx_next(0) > RJ_max_(0) || corner_idx_next(1) > RJ_max_(1)
          || corner_idx_next(2) > RJ_max_(2)) {
        out_of_rjmax = true;
      }

      cff_sphere_idx++;
    } while (!converged && !out_of_rjmax);

    if (!converged) {
      ExEnv::out0() << "\n!!!!!! Warning !!!!!!"
                    << "\nMultipole approximation is not converged to the given threshold!"
                    << std::endl;
    }

    return e_total;
  }

  /// compute electronic multipole moments for the reference unit cell
  MultipoleMoment<double> compute_multipole_moments(const TArray &D) {
    return compute_multipole_moments(sphemm_, D);
  }

 private:
  /*!
   * \brief This constructs a \c BasisPairInfo object between basis sets of unit
   * cell \c ref_uc and its neighbours
   */
  std::shared_ptr<detail::BasisPairInfo> construct_basis_pairs(
      const Vector3i &ref_uc = {0, 0, 0}) {
    using ::mpqc::lcao::gaussian::detail::shift_basis_origin;

    Vector3d uc_vec = ref_uc.cast<double>().cwiseProduct(dcell_);
    auto basis = shift_basis_origin(*obs_, uc_vec);
    auto basis_neighbour = shift_basis_origin(*obs_, uc_vec, R_max_, dcell_);

    return std::make_shared<detail::BasisPairInfo>(basis, basis_neighbour,
                                                   bs_extent_thresh_);
  }

  /*!
   * \brief This determines if a unit cell \c uc_ket is in the crystal far
   * field of the bra unit cell \c uc_bra.
   */
  bool is_uc_in_CFF(const Vector3i &uc_ket,
                    const Vector3i &uc_bra = {0, 0, 0}) {
    using ::mpqc::lcao::gaussian::detail::shift_basis_origin;

    Vector3d vec_bra = uc_bra.cast<double>().cwiseProduct(dcell_);
    Vector3d vec_ket = uc_ket.cast<double>().cwiseProduct(dcell_);
    const auto vec_rel = vec_ket - vec_bra;

    const auto npairs = ref_pairs_->npairs();
    // CFF condition #1: all charge distributions are well separated
    auto condition1 = true;
    for (auto p0 = 0ul; p0 != npairs; ++p0) {
      const auto center0 = ref_pairs_->center(p0) + vec_bra;
      const auto extent0 = ref_pairs_->extent(p0);
      for (auto p1 = 0ul; p1 != npairs; ++p1) {
        const auto center1 = ref_pairs_->center(p1) + vec_ket;
        const auto extent1 = ref_pairs_->extent(p1);

        const double rab = (center1 - center0).norm();
        if (rab < (extent0 + extent1)) {
          condition1 = false;
          break;
        }
      }
      if (!condition1) break;
    }

    // CFF condition #2: |L| >= ws * (r0_max + r1_max)
    const auto L = vec_rel.norm();
    bool condition2 = (L >= ws_ * (max_distance_to_refcenter_ * 2.0));

    // test:
    //    {
    //      ExEnv::out0() << "Vector bra corner = " << vec_bra.transpose()
    //                    << std::endl;
    //      ExEnv::out0() << "Vector ket corner = " << vec_ket.transpose()
    //                    << std::endl;
    //      ExEnv::out0() << "Vector bra center = " << uc_center_bra.transpose()
    //                    << std::endl;
    //      ExEnv::out0() << "Distance between bra and ket = " << L <<
    //      std::endl; ExEnv::out0() << "r0_max = " <<
    //      max_distance_to_refcenter_ << std::endl; auto cond1_val =
    //      (condition1) ? "true" : "false"; auto cond2_val = (condition2) ?
    //      "true" : "false"; ExEnv::out0() << "Is Condition 1 true? " <<
    //      cond1_val << std::endl; ExEnv::out0() << "Is Condition 2 true? " <<
    //      cond2_val << std::endl;
    //    }

    return (condition1 && condition2);
  }

  /*!
   * @brief This computes Crystal Far Field (CFF) boundary (bx, by, bz)
   * @param limit3d the range limit (lx, ly, lz) of CFF boundary so that
   * bx <= lx, by <= ly, bz <= lz. The computation of the boundary in one
   * dimension will be skipped if its limit is zero.
   * @return CFF boundary
   */
  Vector3i compute_CFF_boundary(const Vector3i &limit3d) {
    Vector3i cff_bound({0, 0, 0});

    for (auto dim = 0; dim <= 2; ++dim) {
      Vector3i uc_idx({0, 0, 0});
      bool is_in_CFF = false;
      auto idx1 = 0;
      if (limit3d(dim) > 0) {
        do {
          uc_idx(dim) = idx1;
          is_in_CFF = is_uc_in_CFF(uc_idx);
          idx1++;
        } while (idx1 <= limit3d(dim) && !is_in_CFF);

        cff_bound(dim) = uc_idx(dim);

        if (!is_in_CFF) {
          throw AlgorithmException(
              "Insufficient range limit for the boundary of Crystal Far Field",
              __FILE__, __LINE__);
        }
      }
    }

    return cff_bound;
  }

  /*!
   * @brief This makes a map from the ordinal index of a (\em l, \em m) pair to
   * the corresponding \em l and \em m
   * @tparam lmax max value of l
   * @return \c unordered_map with
   * key: ordinal index
   * mapped value: (\em l, \em m) pair
   */
  template <unsigned int lmax>
  Ord2lmMap make_ord_to_lm_map() {
    Ord2lmMap result;
    result.reserve((lmax + 1) * (lmax + 1));

    auto ord_idx = 0u;
    for (auto l = 0; l <= lmax; ++l) {
      for (auto m = -l; m <= l; ++m) {
        result[ord_idx] = std::make_pair(l, m);
        ord_idx++;
      }
    }

    return result;
  }

  /*!
   * @brief This builds the real interaction kernel \latexonly M$_{l, m}$
   * \endlatexonly between two distant multipole moments centered at P and Q,
   * respectively
   *
   * \latexonly
   * \begin{eqnarray*}
   * M_{l,m}(\mathbf{r}) =
   * \begin{cases}
   *  \frac{(l-m)!}{|\mathbf{r}|^{l+1}} P_l^m(cos\theta) cos(m \phi),
   *    \text{ if m $\geq$ 0} \\
   *  \frac{(l-m)!}{|\mathbf{r}|^{l+1}} P_l^m(cos\theta) sin(m \phi),
   *    \text{ if m $<$ 0}
   * \end{cases}
   * \end{eqnarray*}
   * \endlatexonly
   *
   * @tparam lmax max value of l
   * @param c12 non-zero vector from center Q to center P
   * @return an array of doubles with array size = number of components in
   * \c Oper
   */
  template<unsigned int lmax>
  MultipoleMoment<double, (lmax + 1) * (lmax + 1)> build_interaction_kernel(const Vector3d &c12) {
    const auto c12_norm = c12.norm();
    MPQC_ASSERT(c12_norm > 0.0);

    using namespace boost::math;
    MultipoleMoment<double, (lmax + 1) * (lmax + 1)> result;

    const auto cos_theta = c12(2) / c12_norm;
    const auto c12_xy_norm = std::sqrt(c12(0) * c12(0) + c12(1) * c12(1));

    // When Rx^2 + Ry^2 = 0, φ cannot computed. We use the fact that in this
    // case cosθ = 1, and then associated Legendre polynomials
    // P_{l, m}(cosθ) = 0 if m != 0
    // P_{l, m}(cosθ) = 1 if m == 0
    if (c12_xy_norm < std::numeric_limits<double>::epsilon()) {
      for (auto l = 0, ord_idx = 0; l <= lmax; ++l) {
        auto frac = factorial<double>(l) / std::pow(c12_norm, l + 1);
        for (auto m = -l; m <= l; ++m, ++ord_idx) {
          result[ord_idx] = (m == 0) ? frac : 0.0;
        }
      }
      return result;
    } else {
      const auto inv_x2py2 = 1.0 / c12_xy_norm;
      const auto cos_phi = c12(0) * inv_x2py2;
      const auto sin_phi = c12(1) * inv_x2py2;
      const auto sin_theta = c12_xy_norm / c12_norm;

      std::unordered_map<int, double> cos_m_phi_map, sin_m_phi_map, legendre_map;
      cos_m_phi_map.reserve(2 * lmax + 1);
      sin_m_phi_map.reserve(2 * lmax + 1);
      legendre_map.reserve((lmax + 1) * (lmax + 1));

      // build a map that returns cos(m * phi) for each m using recurrence relation
      // build a map that returns sin(m * phi) for each m using recurrence relation
      cos_m_phi_map[0] = 1.0;
      sin_m_phi_map[0] = 0.0;
      if (lmax >= 1) {
        cos_m_phi_map[1] = cos_phi;
        sin_m_phi_map[1] = sin_phi;
        cos_m_phi_map[-1] = cos_phi;
        sin_m_phi_map[-1] = -sin_phi;
      }
      if (lmax >= 2) {
        for (auto m = 2; m <= lmax; ++m) {
          cos_m_phi_map[m] = 2.0 * cos_m_phi_map[m - 1] * cos_phi - cos_m_phi_map[m - 2];
          sin_m_phi_map[m] = 2.0 * sin_m_phi_map[m - 1] * cos_phi - sin_m_phi_map[m - 2];
          cos_m_phi_map[-m] = cos_m_phi_map[m];
          sin_m_phi_map[-m] = -sin_m_phi_map[m];
        }
      }

      // build a map that returns P_{l, m)(cosθ) for any ordinal index of (l, m)
      // using recurrence relation
      legendre_map[0] = 1.0;  // (l = 0, m = 0)
      if (lmax >= 1) {
        // compute P_{l, m} for all l >= 1 and m = 0
        legendre_map[2] = cos_theta;  // (l = 1, m = 0)
        if (lmax >= 2) {
          for (auto l = 2; l <= lmax; ++l) {
            const auto ord_l_0 = l * l + l;  // (l, 0)
            const auto ord_lm1_0 = l * l - l;  // (l - 1, 0)
            const auto ord_lm2_0 = l * l - 3 * l + 2;  // (l - 2, 0)
            legendre_map[ord_l_0] = legendre_next(l - 1, 0, cos_theta, legendre_map[ord_lm1_0], legendre_map[ord_lm2_0]);
          }
        }

        // compute P_{l, m} for all l >= 1 and m != 0
        for (auto m = 1; m <= lmax; ++m) {
          const auto sign = (m % 2 == 0) ? 1 : -1;
          const auto m2 = m * m;

          const auto ord_m_m = m2 + m + m;  // (l = m, m)
          const auto ord_mm1_mm1 = m2 - 1;  // (l = m - 1, m - 1)
          legendre_map[ord_m_m] = -1.0 * (2.0 * m - 1.0) * sin_theta * legendre_map[ord_mm1_mm1];
          legendre_map[m2] = sign * legendre_map[ord_m_m] / factorial<double>(m + m);  // (l = m, -m)

          if (lmax >= 2 && lmax >= m + 1) {
            const auto ord_mp1_m = m2 + 4 * m + 2;  // (l = m + 1, m)
            legendre_map[ord_mp1_m] = (2.0 * m + 1.0) * cos_theta * legendre_map[ord_m_m];
            legendre_map[ord_mp1_m - 2 * m] = sign * legendre_map[ord_mp1_m] / factorial<double>(2 * m + 1);  // (l = m + 1, -m)
          }

          if (lmax >= 3 && lmax >= m + 2) {
            for (auto l = m + 2; l <= lmax; ++l) {
              const auto l2 = l * l;
              const auto ord_l_m = l2 + l + m;  // (l, m)
              const auto ord_lm1_m = l2 - l + m;  // (l - 1, m)
              const auto ord_lm2_m = l2 - 3 * l + 2 + m;  // (l - 2, m)
              legendre_map[ord_l_m] = legendre_next(l - 1, m, cos_theta, legendre_map[ord_lm1_m], legendre_map[ord_lm2_m]);
              legendre_map[ord_l_m - 2 * m] = sign * factorial<double>(l - m) * legendre_map[ord_l_m] / factorial<double>(l + m);  // (l, -m)
            }
          }
        }
      }

      // fill the result
      for (auto l = 0, ord_idx = 0; l <= lmax; ++l) {
        const auto inv_denom = 1.0 / std::pow(c12_norm, l + 1);
        for (auto m = -l; m <= l; ++m, ++ord_idx) {
          const auto num = factorial<double>(l - m);
          const auto phi_part = (m >= 0) ? cos_m_phi_map[m] : sin_m_phi_map[m];
          result[ord_idx] = num * inv_denom * legendre_map[ord_idx] * phi_part;
        }
      }

      return result;
    }
  }

  /*!
   * @brief This forms real multipole moments with cos(m φ) and sin(m φ) parts,
   * respectively.
   * @tparam nopers total number of componments in multipole expansion operator
   * @param M multipole moments with mixed cosine and sine parts
   * @param ord_to_lm_map a map from ordinal index of (l, m) to a specific
   * (l, m) pair
   * @return a pair of multipole moments with cos(m φ) and sin(m φ) parts,
   * respectively.
   */
  template <unsigned int nopers>
  std::pair<MultipoleMoment<double, nopers>, MultipoleMoment<double, nopers>>
  form_symm_and_antisymm_moments(const MultipoleMoment<double, nopers> &M, Ord2lmMap &ord_to_lm_map) {
    assert(M.size() == nopers);
    assert(ord_to_lm_map.size() == nopers);
    MultipoleMoment<double, nopers> Mplus, Mminus;
    for (auto op = 0u; op != nopers; ++op) {
      auto m = ord_to_lm_map[op].second;
      auto sign = (m > 0) ? 1 : ((m < 1) ? -1 : 0);
      auto nega1_m = (m % 2 == 0) ? 1.0 : -1.0;  // (-1)^m
      switch (sign) {
        case 1:
          Mplus[op] = M[op];
          Mminus[op] = -nega1_m * M[op - 2 * m];
          break;
        case 0:
          Mplus[op] = M[op];
          Mminus[op] = 0.0;
          break;
        case -1:
          Mplus[op] = nega1_m * M[op - 2 * m];
          Mminus[op] = M[op];
          break;
        default:
          throw ProgrammingError("Invalid sign of m.", __FILE__, __LINE__);
      }
    }
    return std::make_pair(Mplus, Mminus);
  };

  /*!
   * @brief This builds the local multipole expansion of the potential at the origin
   * created by a remote unit cell
   * @param O multipole moments of the charge distribution of the remote unit cell
   * @param M multipole interaction kernel between the origin and the remote unit cell
   * @return
   */
  MultipoleMoment<double> build_local_potential(const MultipoleMoment<double>& O,
                                                const MultipoleMoment<double, nopers_doubled_lmax_> &M) {
    MultipoleMoment<double> result;

    // form O+, O-
    auto O_pm = form_symm_and_antisymm_moments<nopers_>(O, O_ord_to_lm_map_);
    auto &O_plus = O_pm.first;
    auto &O_minus = O_pm.second;

    // form M+, M-
    auto M_pm = form_symm_and_antisymm_moments<nopers_doubled_lmax_>(M, M_ord_to_lm_map_);
    auto &M_plus = M_pm.first;
    auto &M_minus = M_pm.second;

    // form multipole expansion of local potential created by the remote unit cell
    for (auto op1 = 0; op1 != nopers_; ++op1) {
      auto l1 = O_ord_to_lm_map_[op1].first;
      auto m1 = O_ord_to_lm_map_[op1].second;
      auto accu = 0.0;
      for (auto op2 = 0; op2 != nopers_; ++op2) {
        auto l1pl2 = l1 + O_ord_to_lm_map_[op2].first;
        auto m1pm2 = m1 + O_ord_to_lm_map_[op2].second;
        auto ord_M = l1pl2 * l1pl2 + l1pl2 + m1pm2;
        if (m1 >= 0) {
          accu += M_plus[ord_M] * O_plus[op2];
          accu += M_minus[ord_M] * O_minus[op2];
        } else {
          accu += M_minus[ord_M] * O_plus[op2];
          accu -= M_plus[ord_M] * O_minus[op2];
        }
      }
      result[op1] = accu;
    }

    return result;
  }

  double compute_energy_per_unitcell(const MultipoleMoment<double> &O,
                                     const MultipoleMoment<double> &L) {
    double result = 0.0;
    for (auto op = 0; op != nopers_; ++op) {
      auto l = O_ord_to_lm_map_[op].first;
      auto m = O_ord_to_lm_map_[op].second;
      auto sign = (l % 2 == 0) ? 1.0 : -1.0;
      auto delta = (m == 0) ? 1.0 : 2.0;
      // scale by 0.5 because energy per cell is half the interaction energy
      result += 0.5 * sign * delta * O[op] * L[op];
    }

    return result;
  }

  UnitCellList build_unitcells_on_a_sphere(const Vector3i &sphere_start,
                                           size_t sphere_idx) {
    UnitCellList result;

    // compute thickness of the sphere to CFF boundary
    Vector3i sphere_thickness = {0, 0, 0};
    for (auto dim = 0; dim <= 2; ++dim) {
      if (RJ_max_(dim) > 0) {
        sphere_thickness(dim) = sphere_idx;
      }
    }

    // index (x, y, z) of the unit cell at the corner with positive x, y, z
    Vector3i corner_idx = sphere_start + sphere_thickness;

    // a lambda that returns a (idx, vector) pair for a given idx of a unit cell
    auto make_pair = [](const Vector3i &idx, const Vector3d &dcell) {
      return std::make_pair(idx, idx.cast<double>().cwiseProduct(dcell));
    };

    // build the unit cell list depending on the crystal dimensionality
    if (dimensionality_ == 0) {
      result.emplace_back(std::make_pair(Vector3i::Zero(), Vector3d::Zero()));
    } else if (dimensionality_ == 1) {
      auto dim = (RJ_max_(0) > 0) ? 0 : ((RJ_max_(1) > 0) ? 1 : 2);
      MPQC_ASSERT(corner_idx(dim) > 0);
      Vector3i neg1_idx = -1 * corner_idx;
      result.emplace_back(make_pair(corner_idx, dcell_));
      result.emplace_back(make_pair(neg1_idx, dcell_));
    } else if (dimensionality_ == 2) {
      auto dim_a = (RJ_max_(0) == 0) ? 1 : 0;
      auto dim_b = (RJ_max_(2) == 0) ? 1 : 2;
      const auto a_max = corner_idx(dim_a);
      const auto b_max = corner_idx(dim_b);
      MPQC_ASSERT(a_max > 0 && b_max > 0);
      Vector3i pos2_idx = Vector3i::Zero();
      Vector3i neg2_idx = Vector3i::Zero();
      pos2_idx(dim_a) = a_max;
      neg2_idx(dim_a) = -a_max;
      for (auto b = -b_max; b <= b_max; ++b) {
        pos2_idx(dim_b) = b;
        neg2_idx(dim_b) = b;
        result.emplace_back(make_pair(pos2_idx, dcell_));
        result.emplace_back(make_pair(neg2_idx, dcell_));
      }

      pos2_idx.setZero();
      neg2_idx.setZero();
      pos2_idx(dim_b) = b_max;
      neg2_idx(dim_b) = -b_max;
      for (auto a = -a_max + 1; a <= a_max - 1; ++a) {
        pos2_idx(dim_a) = a;
        neg2_idx(dim_a) = a;
        result.emplace_back(make_pair(pos2_idx, dcell_));
        result.emplace_back(make_pair(neg2_idx, dcell_));
      }
    } else if (dimensionality_ == 3) {
      const auto x_max = corner_idx[0];
      const auto y_max = corner_idx[1];
      const auto z_max = corner_idx[2];
      MPQC_ASSERT(x_max > 0 && y_max > 0 && z_max > 0);
      Vector3i pos3_idx = Vector3i::Zero();
      Vector3i neg3_idx = Vector3i::Zero();
      pos3_idx(0) = x_max;
      neg3_idx(0) = -x_max;
      for (auto y = -y_max; y <= y_max; ++y) {
        pos3_idx(1) = y;
        neg3_idx(1) = y;
        for (auto z = -z_max; z <= z_max; ++z) {
          pos3_idx(2) = z;
          neg3_idx(2) = z;
          result.emplace_back(make_pair(pos3_idx, dcell_));
          result.emplace_back(make_pair(neg3_idx, dcell_));
        }
      }

      pos3_idx.setZero();
      neg3_idx.setZero();
      pos3_idx(1) = y_max;
      neg3_idx(1) = -y_max;
      for (auto x = -x_max + 1; x <= x_max - 1; ++x) {
        pos3_idx(0) = x;
        neg3_idx(0) = x;
        for (auto z = -z_max; z <= z_max; ++z) {
          pos3_idx(2) = z;
          neg3_idx(2) = z;
          result.emplace_back(make_pair(pos3_idx, dcell_));
          result.emplace_back(make_pair(neg3_idx, dcell_));
        }
      }

      pos3_idx.setZero();
      neg3_idx.setZero();
      pos3_idx(2) = z_max;
      neg3_idx(2) = -z_max;
      for (auto x = -x_max + 1; x <= x_max - 1; ++x) {
        pos3_idx(0) = x;
        neg3_idx(0) = x;
        for (auto y = -y_max + 1; y <= y_max - 1; ++y) {
          pos3_idx(1) = y;
          neg3_idx(1) = y;
          result.emplace_back(make_pair(pos3_idx, dcell_));
          result.emplace_back(make_pair(neg3_idx, dcell_));
        }
      }
    } else {
      throw ProgrammingError("Invalid crystal dimensionality.", __FILE__, __LINE__);
    }

    return result;
  }

  /// compute electronic multipole moments for the reference unit cell
  MultipoleMoment<double> compute_multipole_moments(
      const MultipoleMoment<TArray> &sphemm, const TArray &D) {
    MultipoleMoment<double> result;
    using ::mpqc::pbc::detail::dot_product;
    for (auto op = 0; op != nopers_; ++op) {
      result[op] = -2.0 * dot_product(sphemm[op], D, R_max_, RD_max_);
    }

    return result;
  }

};

}  // namespace ma
}  // namespace pbc
}  // namespace mpqc

#endif  // MPQC4_SRC_MPQC_CHEMISTRY_QC_SCF_PBC_PERIODIC_MA_H_
