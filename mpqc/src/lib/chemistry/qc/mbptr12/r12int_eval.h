//
// r12int_eval.h
//
// Copyright (C) 2004 Edward Valeev
//
// Author: Edward Valeev <edward.valeev@chemistry.gatech.edu>
// Maintainer: EV
//
// This file is part of the SC Toolkit.
//
// The SC Toolkit is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SC Toolkit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SC Toolkit; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
// The U.S. Government is granted a limited license as per AL 91-7.
//

#ifdef __GNUG__
#pragma interface
#endif

#ifndef _chemistry_qc_mbptr12_r12inteval_h
#define _chemistry_qc_mbptr12_r12inteval_h

#include <util/ref/ref.h>
#include <chemistry/qc/mbptr12/vxb_eval_info.h>
#include <chemistry/qc/mbptr12/linearr12.h>

namespace sc {

  /** R12IntEval is the top-level class which computes intermediates occuring in linear R12 theories.
      This class is used by all Wavefunction classes that implement linear R12 methods.
  */

class R12IntEval : virtual public SavableState {

  bool evaluated_;

  // Calculation information (number of basis functions, R12 approximation, etc.)
  Ref<R12IntEvalInfo> r12info_;

  RefSCMatrix Vaa_, Vab_, Xaa_, Xab_, Baa_, Bab_, Aaa_, Aab_, T2aa_, T2ab_;
  RefSCMatrix Raa_, Rab_;    // Not sure if I'll compute and keep these explicitly later
  RefSCVector emp2pair_aa_, emp2pair_ab_;
  RefSCDimension dim_ij_aa_, dim_ij_ab_, dim_ij_s_, dim_ij_t_;
  RefSCDimension dim_ab_aa_, dim_ab_ab_;

  bool gbc_;
  bool ebc_;
  LinearR12::ABSMethod abs_method_;
  LinearR12::StandardApproximation stdapprox_;
  bool spinadapted_;
  int debug_;

  /// Fock-weighted occupied space |i_f> = f_i^R |R>, where R is a function in RI-BS
  Ref<MOIndexSpace> focc_space_;
  /// Form Fock-weighted occupied space
  void form_focc_space_();
  /// Fock-weighted active occupied space |i_f> = f_i^R |R>, where R is a function in RI-BS
  Ref<MOIndexSpace> factocc_space_;
  /// Form Fock-weighted active occupied space
  void form_factocc_space_();
  /// Space of canonical virtual MOs
  Ref<MOIndexSpace> canonvir_space_;
  /// Form space of auxiliary virtuals
  void form_canonvir_space_();

  // (act.occ. OBS| act.occ. OBS) transform (used when OBS == VBS)
  Ref<TwoBodyMOIntsTransform> ipjq_tform_;
  // (act.occ. VBS| act.occ. VBS) transform (used when VBS != OBS)
  Ref<TwoBodyMOIntsTransform> iajb_tform_;

  /// Initialize transforms
  void init_tforms_();
  /// Set intermediates to zero + add the "diagonal" contributions
  void init_intermeds_();
  /// Compute r^2 contribution to X
  void r2_contrib_to_X_orig_();
  /// Compute r^2 contribution to X using compute_r2_()
  void r2_contrib_to_X_new_();
  /// Compute <space1 space1|r_{12}^2|space1 space2> matrix
  RefSCMatrix compute_r2_(const Ref<MOIndexSpace>& space1, const Ref<MOIndexSpace>& space2);
  /// Compute the Fock matrix between 2 spaces
  RefSCMatrix fock_(const Ref<MOIndexSpace>& occ_space, const Ref<MOIndexSpace>& bra_space,
                    const Ref<MOIndexSpace>& ket_space);
  /// Compute the coulomb matrix between 2 spaces
  RefSCMatrix coulomb_(const Ref<MOIndexSpace>& occ_space, const Ref<MOIndexSpace>& bra_space,
                       const Ref<MOIndexSpace>& ket_space);
  /// Compute the exchange matrix between 2 spaces
  RefSCMatrix exchange_(const Ref<MOIndexSpace>& occ_space, const Ref<MOIndexSpace>& bra_space,
                        const Ref<MOIndexSpace>& ket_space);
  /// Checkpoint the top-level molecular energy
  void checkpoint_() const;

  /** Compute the number tasks which have access to the integrals.
      map_to_twi is a vector<int> each element of which will hold the
      number of tasks with access to the integrals of lower rank than that task
      (or -1 if the task doesn't have access to the integrals) */
  const int tasks_with_ints_(const Ref<R12IntsAcc> ints_acc, vector<int>& map_to_twi);
  
  /** Compute contribution to V, X, and B of the following form:
      0.5 * \bar{g}_{ij}^{pq} * \bar{r}_{pq}^{kl}, where p and q span mospace
      Returns transform object referred to by tform_name which stores integrals in ijpq order
  */
  Ref<TwoBodyMOIntsTransform> contrib_to_VXB_a_symm_(const std::string& tform_name,
                                                     const Ref<MOIndexSpace>& mospace);

  /** Compute contribution to V, X, and B of the following form:
      \bar{g}_{ij}^{am} * \bar{r}_{am}^{kl}, where m and a span mospace1 and mospace2, respectively
      Returns transform object referred to by tform_name which stores integrals in ijma order
  */
  Ref<TwoBodyMOIntsTransform> contrib_to_VXB_a_asymm_(const std::string& tform_name,
                                                      const Ref<MOIndexSpace>& mospace1,
                                                      const Ref<MOIndexSpace>& mospace2);

  /// Compute OBS contribution to V, X, and B (these contributions are independent of the method)
  void obs_contrib_to_VXB_gebc_vbseqobs_();

  /// Compute 1-ABS contribution to V, X, and B (these contributions are independent of the method)
  void abs1_contrib_to_VXB_gebc_();

  /// Equiv to the sum of above, except for this doesn't assume that VBS is the same as OBS
  void contrib_to_VXB_gebc_vbsneqobs_();

  /// Compute A using the "simple" formula obtained using direct substitution alpha'->a'
  void compute_A_simple_();

  /// Compute MP2 T2
  void compute_T2_();

  /// Compute R "intermediate" (r12 integrals in occ-pair/vir-pair basis)
  void compute_R_();

  /// Compute A*T2 contribution to V (needed if EBC is not assumed)
  void AT2_contrib_to_V_();

  /// Compute -2*A*R contribution to B (needed if EBC is not assumed)
  void AR_contrib_to_B_();

  /** Compute the first (r<sub>kl</sub>^<sup>AB</sup> f<sub>A</sub><sup>m</sup> r<sub>mB</sub>^<sup>ij</sup>)
      contribution to B that vanishes under GBC */
  void compute_B_gbc_1_();
  
  /** Compute the second (r<sub>kl</sub>^<sup>AB</sup> r<sub>AB</sub>^<sup>Kj</sup> f<sub>K</sub><sup>i</sup>)
      contribution to B that vanishes under GBC */
  void compute_B_gbc_2_();

  /// Compute dual-basis MP2 energy (contribution from doubles to MP2 energy)
  void compute_dualEmp2_();
  
  /// Compute dual-basis MP1 energy (contribution from singles to HF energy)
  void compute_dualEmp1_();
  
  /** Sum contributions to the intermediates from all nodes and broadcast so
      every node has the correct matrices */
  void globally_sum_intermeds_();
  
public:
  R12IntEval(StateIn&);
  R12IntEval(const Ref<R12IntEvalInfo>&);
  ~R12IntEval();

  void save_data_state(StateOut&);
  virtual void obsolete();

  void set_gbc(const bool gbc);
  void set_ebc(const bool ebc);
  void set_absmethod(LinearR12::ABSMethod abs_method);
  void set_stdapprox(LinearR12::StandardApproximation stdapprox);
  void set_spinadapted(bool spinadapted);
  void set_debug(int debug);
  void set_dynamic(bool dynamic);
  void set_print_percent(double print_percent);
  void set_memory(size_t nbytes);

  const bool gbc() const { return gbc_; }
  const bool ebc() const { return ebc_; }
  const LinearR12::StandardApproximation stdapprox() const { return stdapprox_; }

  Ref<R12IntEvalInfo> r12info() const;
  RefSCDimension dim_oo_aa() const;
  RefSCDimension dim_oo_ab() const;
  RefSCDimension dim_oo_s() const;
  RefSCDimension dim_oo_t() const;
  RefSCDimension dim_vv_aa() const;
  RefSCDimension dim_vv_ab() const;

  /// This function causes the intermediate matrices to be computed.
  virtual void compute();

  /// Returns alpha-alpha block of the V intermediate matrix.
  RefSCMatrix V_aa();
  /// Returns alpha-alpha block of the X intermediate matrix.
  RefSCMatrix X_aa();
  /// Returns alpha-alpha block of the B intermediate matrix.
  RefSCMatrix B_aa();
  /// Returns alpha-alpha block of the A intermediate matrix.
  RefSCMatrix A_aa();
  /// Returns alpha-alpha block of the MP2 T2 matrix.
  RefSCMatrix T2_aa();
  /// Returns alpha-beta block of the V intermediate matrix.
  RefSCMatrix V_ab();
  /// Returns alpha-beta block of the X intermediate matrix.
  RefSCMatrix X_ab();
  /// Returns alpha-beta block of the B intermediate matrix.
  RefSCMatrix B_ab();
  /// Returns alpha-beta block of the A intermediate matrix.
  RefSCMatrix A_ab();
  /// Returns alpha-beta block of the MP2 T2 matrix.
  RefSCMatrix T2_ab();
  /// Returns alpha-alpha MP2 pair energies.
  RefSCVector emp2_aa();
  /// Returns alpha-beta MP2 pair energies.
  RefSCVector emp2_ab();

  RefDiagSCMatrix evals() const;  
};

}

#endif

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:


