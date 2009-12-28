//
// transform_ixjy_df.cc
//
// Copyright (C) 2004 Edward Valeev
//
// Author: Edward Valeev <evaleev@vt.edu>
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
#pragma implementation
#endif

#include <stdexcept>
#include <cassert>

#include <util/misc/formio.h>
#include <util/group/memregion.h>
#include <util/state/state_bin.h>
#include <util/ref/ref.h>
#include <math/scmat/local.h>
#include <chemistry/qc/mbptr12/transform_ixjy_df.h>
#include <chemistry/qc/mbptr12/transform_ijR.h>
#include <chemistry/qc/mbptr12/transform_13inds.h>
#include <chemistry/qc/mbptr12/blas.h>
#include <chemistry/qc/df/df.h>

#include <chemistry/qc/mbptr12/distarray4_memgrp.h>
#include <chemistry/qc/mbptr12/distarray4_node0file.h>
#ifdef HAVE_MPIIO
#  include <chemistry/qc/mbptr12/distarray4_mpiiofile.h>
#endif

using namespace std;
using namespace sc;

inline int max(int a,int b) { return (a > b) ? a : b;}

/*-----------
  TwoBodyMOIntsTransform_ixjy_df
 -----------*/
static ClassDesc TwoBodyMOIntsTransform_ixjy_df_cd(
  typeid(TwoBodyMOIntsTransform_ixjy_df),"TwoBodyMOIntsTransform_ixjy_df",1,"public TwoBodyMOIntsTransform",
  0, 0, create<TwoBodyMOIntsTransform_ixjy_df>);

TwoBodyMOIntsTransform_ixjy_df::TwoBodyMOIntsTransform_ixjy_df(const std::string& name, const DensityFittingInfo* df_info,
                                                               const Ref<TwoBodyIntDescr>& tbintdescr,
                                                               const Ref<OrbitalSpace>& space1, const Ref<OrbitalSpace>& space2,
                                                               const Ref<OrbitalSpace>& space3, const Ref<OrbitalSpace>& space4) :
  TwoBodyMOIntsTransform(name,df_info->runtime()->moints_runtime()->factory(),tbintdescr,space1,space2,space3,space4),
  runtime_(df_info->runtime()), dfbasis12_(df_info->basis1()), dfbasis34_(df_info->basis2())
{
  // assuming for now that all densities will be fit in the same basis
  // TODO generalize to different fitting basis sets
  assert(dfbasis12_->equiv(dfbasis34_));

  // make sure Registries know about the fitting bases
  Ref<AOSpaceRegistry> aoreg = this->factory()->ao_registry();
  assert(aoreg->key_exists(dfbasis12_) &&
         aoreg->key_exists(dfbasis34_));

  init_vars();
}

TwoBodyMOIntsTransform_ixjy_df::TwoBodyMOIntsTransform_ixjy_df(StateIn& si) : TwoBodyMOIntsTransform(si)
{
  runtime_ << SavableState::restore_state(si);
  dfbasis12_ << SavableState::restore_state(si);
  dfbasis34_ << SavableState::restore_state(si);

  init_vars();
}

TwoBodyMOIntsTransform_ixjy_df::~TwoBodyMOIntsTransform_ixjy_df()
{
}

void
TwoBodyMOIntsTransform_ixjy_df::save_data_state(StateOut& so)
{
  TwoBodyMOIntsTransform::save_data_state(so);
  SavableState::save_state(runtime_.pointer(),so);
  SavableState::save_state(dfbasis12_.pointer(),so);
  SavableState::save_state(dfbasis34_.pointer(),so);
}

//////////////////////////////////////////////////////
// Compute required (dynamic) memory
// for a given batch size of the transformation
//
// Only arrays allocated before exiting the loop over
// i-batches are included here  - only these arrays
// affect the batch size.
//////////////////////////////////////////////////////
distsize_t
TwoBodyMOIntsTransform_ixjy_df::compute_transform_dynamic_memory_(int ni) const
{
  // integrals held by MemoryGrp
  // compute nij as nij on node 0, since nij on node 0 is >= nij on other nodes
  const int nproc = msg()->n();
  const int rank3 = space3()->rank();
  int nij = compute_nij(ni, rank3, nproc, 0);
  const distsize_t memsize_memgrp = num_te_types() * nij * (distsize_t) memgrp_blksize();

  // determine the peak memory requirements
  return memsize_memgrp;
}

const size_t
TwoBodyMOIntsTransform_ixjy_df::memgrp_blksize() const
{
  const int nbasis2 = space2_->basis()->nbasis();
  const int rank2 = space2_->rank();
  const int dim2 = (nbasis2 > rank2) ? nbasis2 : rank2;
  const int nbasis4 = space4_->basis()->nbasis();
  const int rank4 = space4_->rank();
  const int dim4 = (nbasis4 > rank4) ? nbasis4 : rank4;
  return dim2*dim4*sizeof(double);
}

void
TwoBodyMOIntsTransform_ixjy_df::init_acc()
{
  if (ints_acc_.nonnull())
    return;

  const int nij = compute_nij(batchsize_, space3_->rank(), msg_->n(), msg_->me());
  const size_t localmem = num_te_types() * nij * memgrp_blksize();

  switch (ints_method_) {

  case MOIntsTransform::StoreMethod::mem_only:
    if (npass_ > 1)
      throw std::runtime_error("TwoBodyMOIntsTransform_ixjy_df::init_acc() -- cannot use MemoryGrp-based accumulator in multi-pass transformations");
#if HAVE_R12IA_MEMGRP
    {
      // use a subset of a MemoryGrp provided by TransformFactory
      set_memgrp(new MemoryGrpRegion(mem(),localmem));
      ints_acc_ = new DistArray4_MemoryGrp(mem(), num_te_types(),
                                           space1_->rank(), space3_->rank(), space2_->rank(), space4_->rank(),
                                           memgrp_blksize());
    }
#else
    assert(false);
#endif
    break;

  case MOIntsTransform::StoreMethod::mem_posix:
    // if can do in one pass, use the factory hints about how data will be used
    if (npass_ == 1 && !factory()->hints().data_persistent()) {
#if HAVE_R12IA_MEMGRP
      // use a subset of a MemoryGrp provided by TransformFactory
      set_memgrp(new MemoryGrpRegion(mem(),localmem));
      ints_acc_ = new DistArray4_MemoryGrp(mem(), num_te_types(),
                                           space1_->rank(), space3_->rank(), space2_->rank(), space4_->rank(),
                                           memgrp_blksize());
#else
      assert(false);
#endif
      break;
    }
    // else use the next case

  case MOIntsTransform::StoreMethod::posix:
    ints_acc_ = new DistArray4_Node0File((file_prefix_+"."+name_).c_str(), num_te_types(),
                                         space1_->rank(), space3_->rank(), space2_->rank(), space4_->rank());
    break;

#if HAVE_MPIIO
  case MOIntsTransform::StoreMethod::mem_mpi:
    // if can do in one pass, use the factory hints about how data will be used
    if (npass_ == 1 && !factory()->hints().data_persistent()) {
#if HAVE_R12IA_MEMGRP
      // use a subset of a MemoryGrp provided by TransformFactory
      set_memgrp(new MemoryGrpRegion(mem(),localmem));
      ints_acc_ = new DistArray4_MemoryGrp(mem(), num_te_types(),
                                           space1_->rank(), space3_->rank(), space2_->rank(), space4_->rank(),
                                           memgrp_blksize());
#else
      assert(false);
#endif
      break;
    }
    // else use the next case

  case MOIntsTransform::StoreMethod::mpi:
#if HAVE_R12IA_MPIIO
    ints_acc_ = new DistArray4_MPIIOFile_Ind((file_prefix_+"."+name_).c_str(), num_te_types(),
                                             space1_->rank(), space3_->rank(), space2_->rank(), space4_->rank());
#else
    assert(false);
#endif
    break;
#endif

  default:
    throw std::runtime_error("TwoBodyMOIntsTransform_ixjy_df::init_acc() -- invalid integrals store method");
  }

  // now safe to use memorygrp
}

void
TwoBodyMOIntsTransform_ixjy_df::check_int_symm(double threshold) throw (ProgrammingError)
{
  Ref<DistArray4> iacc = ints_acc();
  iacc->activate();
  int num_te_types = iacc->num_te_types();
  int ni = iacc->ni();
  int nj = iacc->nj();
  int nx = iacc->nx();
  int ny = iacc->ny();
  vector<unsigned int> isyms = space1_->orbsym();
  vector<unsigned int> jsyms = space3_->orbsym();
  vector<unsigned int> xsyms = space2_->orbsym();
  vector<unsigned int> ysyms = space4_->orbsym();

  int me = msg_->me();
  vector<int> twi_map;
  int ntasks_with_ints = iacc->tasks_with_access(twi_map);
  if (!iacc->has_access(me))
    return;

  int ij=0;
  for(int i=0; i<ni; i++) {
    int isym = isyms[i];
    for(int j=0; j<nj; j++, ij++) {
      int jsym = jsyms[j];
      if (ij%ntasks_with_ints != twi_map[me])
        continue;

      for(int t=0; t<num_te_types; t++) {
        const double* ints = iacc->retrieve_pair_block(i,j,static_cast<DistArray4::tbint_type>(t));
        int xy=0;
        for(int x=0; x<nx; x++) {
          int xsym = xsyms[x];
          for(int y=0; y<ny; y++, xy++) {
            int ysym = ysyms[y];
            if ( (isym^jsym^xsym^ysym) != 0 && fabs(ints[xy]) > threshold) {
              ExEnv::outn() << scprintf("Integral type=%d i=%d x=%d j=%d y=%d should be zero\n",t,i,x,j,y);
              throw ProgrammingError("TwoBodyMOIntsTransform_ixjy_df::check_int_symm() -- nonzero nonsymmetric integrals are detected",
                                     __FILE__, __LINE__);
            }
          }
        }
        iacc->release_pair_block(i,j,static_cast<DistArray4::tbint_type>(t));
      }
    }
  }
}

void
TwoBodyMOIntsTransform_ixjy_df::compute() {

  init_acc();

  if (restart_orbital_ != 0)
    return;

  std::string tim_label("tbint_tform_ixjy_df ");
  tim_label += this->name();
  Timer tim(tim_label);

  const bool equiv_12_34 = (*space1() == *space3() && *space2() == *space4());

  // Prerequivisites:
  // 1) if computing Coulomb integrals only, need DensityFitting object only for robust fitting
  //    Wr = cC12 * C34 (if density fitting bases for 12 and 34 are different, this does not work)
  // 2) if computing other integrals, need DensityFitting, as well as 2- and 3-center matrices
  //    Wr = K12 * C34 + C12 * K34 - C12 * k * C34
  TwoBodyOperSet::type oset = intdescr()->operset();
  const bool coulomb_only = (oset == TwoBodyOperSet::ERI);

  const Ref<AOSpaceRegistry>& aoidxreg = this->factory()->ao_registry();

  Ref<DistArray4> C12, cC12;
  {
    const Ref<OrbitalSpace> dfspace12 = aoidxreg->value(dfbasis12());
    const std::string C12_key = ParsedDensityFittingKey::key(space1()->id(),
                                                             space2()->id(),
                                                             dfspace12->id());
    C12 = runtime()->get(C12_key);
    C12->activate();
  }

  Ref<DistArray4> C34, cC34;
  if (!equiv_12_34) {
    const Ref<OrbitalSpace> dfspace34 = aoidxreg->value(dfbasis34());
    const std::string C34_key = ParsedDensityFittingKey::key(space3()->id(),
                                                             space4()->id(),
                                                             dfspace34->id());
    C34 = runtime()->get(C34_key);
    C34->activate();
  }
  else {
    C34 = C12;
  }

  Ref<IntParams> params = intdescr()->params();
  Ref<TwoBodyThreeCenterIntDescr> descr = IntDescrFactory::make<3>(factory()->integral(),
      oset,
      params);
  const std::string descr_key = runtime()->moints_runtime()->runtime_3c()->descr_key(descr);

  // compute the 3-center operator matrices
  {
    const Ref<AOSpaceRegistry>& aoidxreg = this->factory()->ao_registry();
    const Ref<OrbitalSpace> dfspace12 = aoidxreg->value(dfbasis12());
    const std::string cC12_key = ParsedTwoBodyThreeCenterIntKey::key(space1()->id(),
                                                                     dfspace12->id(),
                                                                     space2()->id(),
                                                                     descr_key);
    Ref<TwoBodyThreeCenterMOIntsTransform> cC12_tform = runtime()->moints_runtime()->runtime_3c()->get(cC12_key);
    cC12_tform->compute();
    cC12 = cC12_tform->ints_acc();
    cC12->activate();

    if (!equiv_12_34) {
      const Ref<OrbitalSpace> dfspace34 = aoidxreg->value(dfbasis34());
      const std::string cC34_key = ParsedTwoBodyThreeCenterIntKey::key(space3()->id(),
                                                                       dfspace34->id(),
                                                                       space4()->id(),
                                                                       descr_key);
      Ref<TwoBodyThreeCenterMOIntsTransform> cC34_tform = runtime()->moints_runtime()->runtime_3c()->get(cC34_key);
      cC34_tform->compute();
      cC34 = cC34_tform->ints_acc();
      cC34->activate();
    }
    else
      cC34 = cC12;
  }

  double** kernel = 0;
  Ref<DistArray4> L12;   // L12 = C12 * kernel
  if (!coulomb_only) {

    // TODO convert to using two-body two-center ints runtime
    const int ntypes = num_te_types();
    kernel = new double*[TwoBodyOper::max_ntypes];
    // compute the kernel for each integral type
    {
      Ref<Integral> localints = factory()->integral();
      localints->set_basis(dfbasis12(), dfbasis34());
      Ref<IntParams> params = intdescr()->params();
      Ref<TwoBodyTwoCenterIntDescr> descr = IntDescrFactory::make<2>(localints, oset, params);
      Ref<TwoBodyTwoCenterInt> tbint = descr->inteval();

      Ref<GaussianBasisSet> b1 = dfbasis12();
      Ref<GaussianBasisSet> b2 = dfbasis34();
      const int n1 = b1->nbasis();
      const int n2 = b2->nbasis();
      const int n12 = n1 * n2;
      for(int te_type=0; te_type<ntypes; ++te_type) {
        kernel[te_type] = new double[n12];
        memset(kernel[te_type],0,n12*sizeof(double));
      }

      const double** buffer = new const double*[ntypes];
      for(int te_type=0; te_type<ntypes; ++te_type) {
        buffer[te_type] = tbint->buffer( descr->intset(te_type) );
      }
      for (int s1 = 0; s1 < b1->nshell(); ++s1) {
        const int s1offset = b1->shell_to_function(s1);
        const int nf1 = b1->shell(s1).nfunction();
        for (int s2 = 0; s2 < b2->nshell(); ++s2) {
          const int s2offset = b2->shell_to_function(s2);
          const int nf2 = b2->shell(s2).nfunction();

          // compute shell doublet
          tbint->compute_shell(s1, s2);

          // copy buffers into kernels
          const int s1off_x_n2 = s1offset*n2;
          for(int te_type=0; te_type<ntypes; ++te_type) {
            const double* bufptr = buffer[te_type];
            double* result_buf = kernel[te_type] + s1off_x_n2;
            for(int f1=0; f1<nf1; ++f1, result_buf+=n2) {
              int ff2 = s2offset;
              for(int f2=0; f2<nf2; ++f2, ++bufptr, ++ff2) {
                result_buf[ff2] = *bufptr;
              }
            }
          }

        }
      }

      delete[] buffer;
    }

    // now compute L12 = C12 * k
    DistArray4Dimensions L12_dims(this->num_te_types(),C12->ni(),C12->nj(),C12->nx(),C12->ny());
    // cloning C12 assumes that the fitting bases for 12 and 34 are same
    assert(dfbasis12()->nbasis() == dfbasis34()->nbasis());
    L12 = C12->clone(L12_dims);
    L12->activate();
    {
      // split the work between tasks who can write the integrals
      //
      // assume that the density fitting matrices are available from
      // all tasks that can store the target integrals
      std::vector<int> workers;
      const int nworkers = C12->tasks_with_access(workers);
      const int me = this->msg()->me();

      // loop over i
      const unsigned int rank1 = space1()->rank();
      const unsigned int rank2 = space2()->rank();
      const unsigned int rankF12 = dfbasis12()->nbasis();
      const unsigned int rankF34 = dfbasis34()->nbasis();
      double* L12_buf = new double[rank2 * rankF34];

      int task_count = 0;
      for(unsigned int te_type = 0; te_type<ntypes; ++te_type) {
        for(int i=0; i<rank1; ++i, ++task_count) {
          if (task_count % nworkers != workers[me])
            continue;

          const double* C12_buf = C12->retrieve_pair_block(0, i, 0);
          C_DGEMM('n', 'n', rank2, rankF12, rankF34, 1.0, C12_buf, rankF12, kernel[te_type], rankF34,
                  0.0, L12_buf, rankF34);
          L12->store_pair_block(0, i, te_type, L12_buf);
          C12->release_pair_block(0, i, 0);

        }
      }

      delete[] L12_buf;
    }
    if (L12->data_persistent()) L12->deactivate();
    L12->activate();
  }

  // split the work between tasks who can write the integrals
  //
  // assume that the density fitting matrices are available from
  // all tasks that can store the target integrals
  std::vector<int> workers;
  const int nworkers = ints_acc_->tasks_with_access(workers);
  const int me = this->msg()->me();

  // loop over ij
  const unsigned int rank1 = space1()->rank();
  const unsigned int rank2 = space2()->rank();
  const unsigned int rank3 = space3()->rank();
  const unsigned int rank4 = space4()->rank();
  const unsigned int rankF = dfbasis12()->nbasis();
  double* xy_buf = new double[rank2 * rank4];

  ints_acc_->activate();

  int task_count = 0;
  for(unsigned int te_type = 0; te_type<this->num_te_types(); ++te_type) {
    const TwoBodyOper::type opertype = descr->intset(te_type);
    for(int i=0; i<rank1; ++i) {
      const double* C12_buf = 0;
      const double* cC12_buf = 0;
      const double* L12_buf = 0;
      for(int j=0; j<rank3; ++j, ++task_count) {

        if (task_count % nworkers != workers[me])
          continue;

        if (C12_buf == 0) C12_buf = C12->retrieve_pair_block(0, i, 0);
        const double* cC34_buf = cC34->retrieve_pair_block(0, j, te_type);
        C_DGEMM('n', 't', rank2, rank4, rankF, 1.0, C12_buf, rankF, cC34_buf, rankF,
                0.0, xy_buf, rank4);
        cC34->release_pair_block(0, j, te_type);

        if (opertype != TwoBodyOper::eri) {
          if (cC12_buf == 0) cC12_buf = cC12->retrieve_pair_block(0, i, te_type);
          const double* C34_buf = C34->retrieve_pair_block(0, j, 0);
          C_DGEMM('n', 't', rank2, rank4, rankF, 1.0, cC12_buf, rankF, C34_buf, rankF,
                  1.0, xy_buf, rank4);
          if (L12_buf == 0) L12_buf = L12->retrieve_pair_block(0, i, te_type);
          C_DGEMM('n', 't', rank2, rank4, rankF, -1.0, L12_buf, rankF, C34_buf, rankF,
                  1.0, xy_buf, rank4);
          C34->release_pair_block(0, j, 0);
        }

#define DEBUG 0
#if DEBUG
        {
          ExEnv::out0() << indent << "TwoBodyMOIntsTransform_ixjy_df: name = " << this->name() << endl;
          ExEnv::out0() << indent << "te_type = " << te_type << " i = " << i << " j = " << j << endl;
          Ref<SCMatrixKit> kit = new LocalSCMatrixKit;
          RefSCMatrix xy_buf_mat = kit->matrix(new SCDimension(rank2),
                                               new SCDimension(rank4));
          xy_buf_mat.assign(xy_buf);
          xy_buf_mat.print("block");
        }
#endif

        ints_acc_->store_pair_block(i, j, te_type, xy_buf);

      }

      if (C12_buf) C12->release_pair_block(0, i, 0);  C12_buf = 0;
      if (cC12_buf) cC12->release_pair_block(0, i, te_type);  cC12_buf = 0;
      if (L12_buf) L12->release_pair_block(0, i, te_type);  L12_buf = 0;
    }
  }

  if (ints_acc_->data_persistent()) ints_acc_->deactivate();

  delete[] xy_buf;
  if (kernel != 0)
    for (int te_type=0; te_type<num_te_types(); ++te_type)
      delete[] kernel[te_type];

  restart_orbital_ = 1;
  tim.exit();
  ExEnv::out0() << indent << "Built TwoBodyMOIntsTransform_ixjy_df: name = " << this->name() << std::endl;
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
