//
// Created by Chong Peng on 11/2/17.
//

#ifndef SRC_MPQC_CHEMISTRY_QC_LCAO_CC_CCSD_R1_R2_H_
#define SRC_MPQC_CHEMISTRY_QC_LCAO_CC_CCSD_R1_R2_H_

#include <tiledarray.h>
#include "mpqc/chemistry/qc/lcao/cc/ccsd_intermediates.h"

namespace mpqc {
namespace lcao {
namespace cc {

/**
 * @todo need docs
 * @tparam Array
 */
template <typename Array>
struct Integrals {
  Integrals() = default;
  ~Integrals() = default;

  Array Fia;
  Array Fij;
  Array Fab;

  Array FIJ;
  Array FAB;

  Array Gabcd;
  Array Gijab;
  Array Gijkl;
  Array Giajb;
  Array Giabc;
  Array Gijka;

  Array Xai;
  Array Xij;
  Array Xab;

  Array Ci;
  Array Ca;
};

/**
 * @todo need docs
 * @tparam Array
 * @param t1
 * @param t2
 * @param tau
 * @param ints
 * @return
 */
template <typename Array>
Array compute_cs_ccsd_r1(const Array& t1, const Array& t2, const Array& tau,
                         cc::Integrals<Array>& ints, const Array& u = Array()) {
  Array r1;
  {
    // intermediates for t1
    // external index i and a
    // vir index a b c d
    // occ index i j k l
    Array h_kc, h_ki, h_ac;

    // compute residual r1(n) (at convergence r1 = 0)
    // external index i and a
    r1("a,i") =
        ints.Fia("i,a") - 2.0 * (ints.Fia("k,c") * t1("c,i")) * t1("a,k");

    Array g_ijab_bar;
    g_ijab_bar("i,j,a,b") = 2.0 * ints.Gijab("i,j,a,b") - ints.Gijab("j,i,a,b");
    {
      h_ac = compute_cs_ccsd_F_vv(ints.Fab, g_ijab_bar, tau);
      ints.FAB = h_ac;
      r1("a,i") += h_ac("a,c") * t1("c,i");
    }

    {
      h_ki = compute_cs_ccsd_F_oo(ints.Fij, g_ijab_bar, tau);
      ints.FIJ = h_ki;
      r1("a,i") -= t1("a,k") * h_ki("k,i");
    }

    {
      h_kc = compute_cs_ccsd_F_ov(ints.Fia, g_ijab_bar, t1);

      r1("a,i") += h_kc("k,c") * (2.0 * t2("c,a,k,i") - t2("c,a,i,k") +
                                  t1("c,i") * t1("a,k"));
    }

    r1("a,i") +=
        (2.0 * ints.Gijab("k,i,c,a") - ints.Giajb("k,a,i,c")) * t1("c,k");

    if (!u.is_initialized()) {
      Array g_iabc_bar;
      g_iabc_bar("k,a,c,d") =
          2.0 * ints.Giabc("k,a,c,d") - ints.Giabc("k,a,d,c");
      r1("a,i") += g_iabc_bar("k,a,c,d") * tau("c,d,k,i");
    } else {
      r1("a,i") +=
          (2.0 * u("p,r,k,i") - u("p,r,i,k")) * ints.Ci("p,k") * ints.Ca("r,a");
    }

    r1("a,i") -=
        (2.0 * ints.Gijka("l,k,i,c") - ints.Gijka("k,l,i,c")) * tau("c,a,k,l");
  }
  return r1;
};

/**
 * @todo need docs
 * @tparam Array
 * @param t1
 * @param t2
 * @param tau
 * @param ints
 * @return
 */
template <typename Array>
Array compute_cs_ccsd_r1_df(const Array& t1, const Array& t2, const Array& tau,
                            cc::Integrals<Array>& ints) {
  Array r1;
  {
    // intermediates for t1
    // external index i and a
    // vir index a b c d
    // occ index i j k l
    Array h_kc, h_ki, h_ac;

    Array X_ai_tau;

    X_ai_tau("X,a,i") = 2.0 * ints.Xai("X,b,j") * tau("a,b,i,j") -
                        ints.Xai("X,b,j") * tau("a,b,j,i");

    // compute residual r1(n) (at convergence r1 = 0)
    // external index i and a
    r1("a,i") = ints.Fia("i,a") - 2.0 * ints.Fia("k,c") * t1("c,i") * t1("a,k");

    {
      h_ac("a,c") = ints.Fab("a,c") - X_ai_tau("K,a,l") * ints.Xai("K,c,l");
      ints.FAB = h_ac;
      r1("a,i") += h_ac("a,c") * t1("c,i");
    }

    {
      h_ki("k,i") = ints.Fij("k,i") + X_ai_tau("K,c,i") * ints.Xai("K,c,k");
      ints.FIJ = h_ki;
      r1("a,i") -= t1("a,k") * h_ki("k,i");
    }

    {
      h_kc("k,c") =
          ints.Fia("k,c") +
          (2.0 * ints.Gijab("k,l,c,d") - ints.Gijab("k,l,d,c")) * t1("d,l");
      r1("a,i") += h_kc("k,c") * (2.0 * t2("c,a,k,i") - t2("c,a,i,k") +
                                  t1("c,i") * t1("a,k"));
    }

    r1("a,i") +=
        (2.0 * ints.Gijab("k,i,c,a") - ints.Giajb("k,a,i,c")) * t1("c,k");

    r1("a,i") += X_ai_tau("K,c,i") * ints.Xab("K,a,c");

    r1("a,i") -= X_ai_tau("K,a,l") * ints.Xij("K,l,i");
  }
  return r1;
}

/**
 *  @todo need docs
 * @tparam Array
 * @param t1
 * @param t2
 * @param tau
 * @param ints
 * @return
 */
template <typename Array>
Array compute_cs_ccsd_r2(const Array& t1, const Array& t2, const Array& tau,
                         const cc::Integrals<Array>& ints,
                         const Array& u = Array()) {
  Array r2;

  // compute residual r2(n) (at convergence r2 = 0)
  // permutation part
  {
    r2("a,b,i,j") =
        (ints.Giabc("i,c,a,b") - ints.Giajb("k,b,i,c") * t1("a,k")) * t1("c,j");

    r2("a,b,i,j") -=
        (ints.Gijka("j,i,k,a") + ints.Gijab("i,k,a,c") * t1("c,j")) * t1("b,k");
  }

  {
    // compute g intermediate
    Array g_ki, g_ac;

    g_ki("k,i") =
        ints.FIJ("k,i") + ints.Fia("k,c") * t1("c,i") +
        (2.0 * ints.Gijka("k,l,i,c") - ints.Gijka("l,k,i,c")) * t1("c,l");

    g_ac("a,c") =
        ints.FAB("a,c") - ints.Fia("k,c") * t1("a,k") +
        (2.0 * ints.Giabc("k,a,d,c") - ints.Giabc("k,a,c,d")) * t1("d,k");

    r2("a,b,i,j") += g_ac("a,c") * t2("c,b,i,j") - g_ki("k,i") * t2("a,b,k,j");
  }

  {
    Array j_akic;
    Array k_kaic;
    // compute j and k intermediate
    {
      Array T;

      T("d,b,i,l") = 0.5 * t2("d,b,i,l") + t1("d,i") * t1("b,l");

      j_akic("a,k,i,c") =
          ints.Gijab("i,k,a,c") - ints.Gijka("l,k,i,c") * t1("a,l") +
          ints.Giabc("k,a,c,d") * t1("d,i") -
          ints.Gijab("k,l,c,d") * T("d,a,i,l") +
          0.5 * (2.0 * ints.Gijab("k,l,c,d") - ints.Gijab("k,l,d,c")) *
              t2("a,d,i,l");

      k_kaic("k,a,i,c") = ints.Giajb("k,a,i,c") -
                          ints.Gijka("k,l,i,c") * t1("a,l") +
                          ints.Giabc("k,a,d,c") * t1("d,i") -
                          ints.Gijab("k,l,d,c") * T("d,a,i,l");
    }

    r2("a,b,i,j") += 0.5 * (2.0 * j_akic("a,k,i,c") - k_kaic("k,a,i,c")) *
                     (2.0 * t2("c,b,k,j") - t2("b,c,k,j"));

    r2("a,b,i,j") += -0.5 * k_kaic("k,a,i,c") * t2("b,c,k,j") -
                     k_kaic("k,b,i,c") * t2("a,c,k,j");
  }

  // perform the permutation
  r2("a,b,i,j") = r2("a,b,i,j") + r2("b,a,j,i");

  r2("a,b,i,j") += ints.Gijab("i,j,a,b");

  {
    Array a_klij;
    // compute a intermediate
    a_klij("k,l,i,j") = ints.Gijkl("k,l,i,j") +
                        ints.Gijka("k,l,i,c") * t1("c,j") +
                        ints.Gijka("l,k,j,c") * t1("c,i") +
                        ints.Gijab("k,l,c,d") * tau("c,d,i,j");

    r2("a,b,i,j") += a_klij("k,l,i,j") * tau("a,b,k,l");
  }

  {
    Array b_abij;
    if (!u.is_initialized()) {
      b_abij("a,b,i,j") = tau("c,d,i,j") * ints.Gabcd("a,b,c,d");
      Array tmp;
      tmp("k,a,i,j") = ints.Giabc("k,a,c,d") * tau("c,d,i,j");
      b_abij("a,b,i,j") -= tmp("k,a,j,i") * t1("b,k");

      b_abij("a,b,i,j") -= tmp("k,b,i,j") * t1("a,k");
    } else {
      b_abij("a,b,i,j") =
          (u("p,r,i,j") * ints.Ca("r,b") -
           ints.Ci("r,k") * t1("b,k") * u("p,r,i,j")) *
              ints.Ca("p,a") -
          u("p,r,i,j") * ints.Ci("p,k") * ints.Ca("r,b") * t1("a,k");
    }

    r2("a,b,i,j") += b_abij("a,b,i,j");
  }

  return r2;
}

/**
 * @todo docs, more refactor
 * @tparam Array
 * @param t1
 * @param t2
 * @param tau
 * @param ints
 * @return
 */
template <typename Array>
Array compute_cs_ccsd_r2_df(const Array& t1, const Array& t2, const Array& tau,
                            const cc::Integrals<Array>& ints, const Array& u = Array()) {
  // compute residual r2(n) (at convergence r2 = 0)

  Array r2;
  // permutation part
  Array X_ab_t1;
  X_ab_t1("K,a,i") = ints.Xab("K,a,b") * t1("b,i");

  {
    r2("a,b,i,j") =
        X_ab_t1("K,b,j") * (ints.Xai("K,a,i") - ints.Xij("K,k,i") * t1("a,k")) -
        (ints.Gijka("j,i,k,a") + ints.Gijab("i,k,a,c") * t1("c,j")) * t1("b,k");
  }
  {
    // compute g intermediate
    Array g_ki, g_ac;

    g_ki("k,i") =
        ints.FIJ("k,i") + ints.Fia("k,c") * t1("c,i") +
        (2.0 * ints.Gijka("k,l,i,c") - ints.Gijka("l,k,i,c")) * t1("c,l");

    g_ac("a,c") = ints.FAB("a,c") - ints.Fia("k,c") * t1("a,k") +
                  2.0 * ints.Xai("K,d,k") * t1("d,k") * ints.Xab("K,a,c") -
                  X_ab_t1("K,a,k") * ints.Xai("K,c,k");

    r2("a,b,i,j") += g_ac("a,c") * t2("c,b,i,j") - g_ki("k,i") * t2("a,b,k,j");
  }
  {
    Array j_akic;
    Array k_kaic;
    // compute j and k intermediate
    {
      Array T;

      T("d,b,i,l") = 0.5 * t2("d,b,i,l") + t1("d,i") * t1("b,l");

      j_akic("a,k,i,c") =
          ints.Gijab("i,k,a,c") - ints.Gijka("l,k,i,c") * t1("a,l") +
          X_ab_t1("K,a,i") * ints.Xai("K,c,k") -
          ints.Xai("x,d,l") * T("d,a,i,l") * ints.Xai("x,c,k") +
          0.5 * (2.0 * ints.Gijab("k,l,c,d") - ints.Gijab("k,l,d,c")) *
              t2("a,d,i,l");

      k_kaic("k,a,i,c") = ints.Giajb("k,a,i,c") -
                          ints.Gijka("k,l,i,c") * t1("a,l") +
                          ints.Xai("K,d,k") * t1("d,i") * ints.Xab("K,a,c") -
                          ints.Gijab("k,l,d,c") * T("d,a,i,l");
    }

    r2("a,b,i,j") += 0.5 * (2.0 * j_akic("a,k,i,c") - k_kaic("k,a,i,c")) *
                     (2.0 * t2("c,b,k,j") - t2("b,c,k,j"));

    Array tmp;
    tmp("a,b,i,j") = k_kaic("k,a,i,c") * t2("c,b,j,k");
    r2("a,b,i,j") -= 0.5 * tmp("a,b,i,j") + tmp("b,a,i,j");
  }

  // perform the permutation
  r2("a,b,i,j") = r2("a,b,i,j") + r2("b,a,j,i");
  r2("a,b,i,j") += ints.Gijab("i,j,a,b");

  {
    Array a_klij;
    // compute a intermediate
    a_klij("k,l,i,j") = ints.Gijkl("k,l,i,j") +
                        ints.Gijka("k,l,i,c") * t1("c,j") +
                        ints.Gijka("l,k,j,c") * t1("c,i") +
                        ints.Gijab("k,l,c,d") * tau("c,d,i,j");

    r2("a,b,i,j") += a_klij("k,l,i,j") * tau("a,b,k,l");
  }
  {
    // compute b intermediate
    // avoid store b_abcd
    Array b_abij;

    if (!u.is_initialized()) {
      //    if (!reduced_abcd_memory_) {
      b_abij("a,b,i,j") = tau("c,d,i,j") * ints.Gabcd("a,b,c,d");
      Array tmp;
      tmp("k,a,i,j") = ints.Giabc("k,a,c,d") * tau("c,d,i,j");
      b_abij("a,b,i,j") -= tmp("k,a,j,i") * t1("b,k");
      b_abij("a,b,i,j") -= tmp("k,b,i,j") * t1("a,k");

      //    } else {
      //      Array X_ab_t1;
      //      X_ab_t1("K,a,b") = 0.5 * X_ab("K,a,b") - X_ai("K,b,i") *
      //      t1("a,i");

      //      auto g_abcd_iabc_direct = gaussian::df_direct_integrals(
      //          X_ab, X_ab_t1, Formula::Notation::Physical);

      //      b_abij("a,b,i,j") = tau("c,d,i,j") *
      //      g_abcd_iabc_direct("a,b,c,d");

      //      b_abij("a,b,i,j") += b_abij("b,a,j,i");
      //    }
    } else {
      b_abij("a,b,i,j") =
          (u("p,r,i,j") * ints.Ca("r,b") -
           ints.Ci("r,k") * t1("b,k") * u("p,r,i,j")) *
              ints.Ca("p,a") -
          u("p,r,i,j") * ints.Ci("p,k") * ints.Ca("r,b") * t1("a,k");
    }

    r2("a,b,i,j") += b_abij("a,b,i,j");
  }

  return r2;
}

}  // namespace cc
}  // namespace lcao
}  // namespace mpqc

#endif  // SRC_MPQC_CHEMISTRY_QC_LCAO_CC_CCSD_R1_R2_H_
