//
// elemop.cc
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Curtis Janssen <cljanss@ca.sandia.gov>
// Maintainer: LPS
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

#ifdef __GNUC__
#pragma implementation
#endif

#include <stdlib.h>
#include <math.h>

#include <util/misc/formio.h>
#include <math/scmat/block.h>
#include <math/scmat/blkiter.h>
#include <math/scmat/elemop.h>
#include <math/scmat/abstract.h>

/////////////////////////////////////////////////////////////////////////////
// SCElementOp member functions

SavableState_REF_def(SCElementOp);

#define CLASSNAME SCElementOp
#define PARENTS public SavableState
#include <util/state/statei.h>
#include <util/class/classia.h>

SCElementOp::SCElementOp()
{
}

void *
SCElementOp::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SavableState::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementOp::~SCElementOp()
{
}

int
SCElementOp::has_collect()
{
  return 0;
}

void
SCElementOp::defer_collect(int)
{
}

int
SCElementOp::has_side_effects()
{
  return 0;
}

void
SCElementOp::collect(const RefMessageGrp&)
{
}

void
SCElementOp::process_base(SCMatrixBlock* a)
{
  if (SCMatrixRectBlock::castdown(a))
    process_spec_rect(SCMatrixRectBlock::castdown(a));
  else if (SCMatrixLTriBlock::castdown(a))
    process_spec_ltri(SCMatrixLTriBlock::castdown(a));
  else if (SCMatrixDiagBlock::castdown(a))
    process_spec_diag(SCMatrixDiagBlock::castdown(a));
  else if (SCVectorSimpleBlock::castdown(a))
    process_spec_vsimp(SCVectorSimpleBlock::castdown(a));
  else if (SCMatrixRectSubBlock::castdown(a))
    process_spec_rectsub(SCMatrixRectSubBlock::castdown(a));
  else if (SCMatrixLTriSubBlock::castdown(a))
    process_spec_ltrisub(SCMatrixLTriSubBlock::castdown(a));
  else if (SCMatrixDiagSubBlock::castdown(a))
    process_spec_diagsub(SCMatrixDiagSubBlock::castdown(a));
  else if (SCVectorSimpleSubBlock::castdown(a))
    process_spec_vsimpsub(SCVectorSimpleSubBlock::castdown(a));
  else
    a->process(this);
}

// If specializations of SCElementOp do not handle a particle
// block type, then these functions will be called and will
// set up an appropiate block iterator which specializations
// of SCElementOp must handle since it is pure virtual.

void
SCElementOp::process_spec_rect(SCMatrixRectBlock* a)
{
  SCMatrixBlockIter*i = new SCMatrixRectBlockIter(a);
  SCMatrixBlockIter&r=*i;
  process(r);
  // this causes a SCMatrixRectBlock::operator int() to be
  // called with this = 0x0 using gcc 2.5.6
  // process(*i,b);
  delete i;
}
void
SCElementOp::process_spec_ltri(SCMatrixLTriBlock* a)
{
  SCMatrixBlockIter*i = new SCMatrixLTriBlockIter(a);
  process(*i);
  delete i;
}
void
SCElementOp::process_spec_diag(SCMatrixDiagBlock* a)
{
  SCMatrixBlockIter*i = new SCMatrixDiagBlockIter(a);
  process(*i);
  delete i;
}
void
SCElementOp::process_spec_vsimp(SCVectorSimpleBlock* a)
{
  SCMatrixBlockIter*i = new SCVectorSimpleBlockIter(a);
  process(*i);
  delete i;
}
void
SCElementOp::process_spec_rectsub(SCMatrixRectSubBlock* a)
{
  SCMatrixBlockIter*i = new SCMatrixRectSubBlockIter(a);
  SCMatrixBlockIter&r=*i;
  process(r);
  // this causes a SCMatrixRectBlock::operator int() to be
  // called with this = 0x0 using gcc 2.5.6
  // process(*i,b);
  delete i;
}
void
SCElementOp::process_spec_ltrisub(SCMatrixLTriSubBlock* a)
{
  SCMatrixBlockIter*i = new SCMatrixLTriSubBlockIter(a);
  process(*i);
  delete i;
}
void
SCElementOp::process_spec_diagsub(SCMatrixDiagSubBlock* a)
{
  SCMatrixBlockIter*i = new SCMatrixDiagSubBlockIter(a);
  process(*i);
  delete i;
}
void
SCElementOp::process_spec_vsimpsub(SCVectorSimpleSubBlock* a)
{
  SCMatrixBlockIter*i = new SCVectorSimpleSubBlockIter(a);
  process(*i);
  delete i;
}

/////////////////////////////////////////////////////////////////////////////
// SCElementOp2 member functions

SavableState_REF_def(SCElementOp2);

#define CLASSNAME SCElementOp2
#define PARENTS public SavableState
#include <util/state/statei.h>
#include <util/class/classia.h>

SCElementOp2::SCElementOp2()
{
}

void *
SCElementOp2::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SavableState::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementOp2::~SCElementOp2()
{
}

int
SCElementOp2::has_collect()
{
  return 0;
}

void
SCElementOp2::defer_collect(int)
{
}

int
SCElementOp2::has_side_effects()
{
  return 0;
}

int
SCElementOp2::has_side_effects_in_arg()
{
  return 0;
}

void
SCElementOp2::collect(const RefMessageGrp&)
{
}

void
SCElementOp2::process_base(SCMatrixBlock* a, SCMatrixBlock* b)
{
  a->process(this, b);
}

// If specializations of SCElementOp2 do not handle a particle
// block type, then these functions will be called and will
// set up an appropiate block iterator which specializations
// of SCElementOp2 must handle since it is pure virtual.

void
SCElementOp2::process_spec_rect(SCMatrixRectBlock* a,SCMatrixRectBlock* b)
{
  SCMatrixBlockIter*i = new SCMatrixRectBlockIter(a);
  SCMatrixBlockIter*j = new SCMatrixRectBlockIter(b);
  process(*i,*j);
  // this causes a SCMatrixRectBlock::operator int() to be
  // called with this = 0x0 using gcc 2.5.6
  // process(*i,b);
  delete i;
  delete j;
}
void
SCElementOp2::process_spec_ltri(SCMatrixLTriBlock* a,SCMatrixLTriBlock* b)
{
  SCMatrixBlockIter*i = new SCMatrixLTriBlockIter(a);
  SCMatrixBlockIter*j = new SCMatrixLTriBlockIter(b);
  process(*i,*j);
  delete i;
  delete j;
}
void
SCElementOp2::process_spec_diag(SCMatrixDiagBlock* a,SCMatrixDiagBlock* b)
{
  SCMatrixBlockIter*i = new SCMatrixDiagBlockIter(a);
  SCMatrixBlockIter*j = new SCMatrixDiagBlockIter(b);
  process(*i,*j);
  delete i;
  delete j;
}
void
SCElementOp2::process_spec_vsimp(SCVectorSimpleBlock* a,SCVectorSimpleBlock* b)
{
  SCMatrixBlockIter*i = new SCVectorSimpleBlockIter(a);
  SCMatrixBlockIter*j = new SCVectorSimpleBlockIter(b);
  process(*i,*j);
  delete i;
  delete j;
}

/////////////////////////////////////////////////////////////////////////////
// SCElementOp3 member functions

SavableState_REF_def(SCElementOp3);

#define CLASSNAME SCElementOp3
#define PARENTS public SavableState
#include <util/state/statei.h>
#include <util/class/classia.h>

SCElementOp3::SCElementOp3()
{
}

void *
SCElementOp3::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SavableState::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementOp3::~SCElementOp3()
{
}

int
SCElementOp3::has_collect()
{
  return 0;
}

void
SCElementOp3::defer_collect(int)
{
}

int
SCElementOp3::has_side_effects()
{
  return 0;
}

int
SCElementOp3::has_side_effects_in_arg1()
{
  return 0;
}

int
SCElementOp3::has_side_effects_in_arg2()
{
  return 0;
}

void
SCElementOp3::collect(const RefMessageGrp&)
{
}

void
SCElementOp3::process_base(SCMatrixBlock* a,
                           SCMatrixBlock* b,
                           SCMatrixBlock* c)
{
  a->process(this, b, c);
}

// If specializations of SCElementOp3 do not handle a particle
// block type, then these functions will be called and will
// set up an appropiate block iterator which specializations
// of SCElementOp3 must handle since it is pure virtual.

void
SCElementOp3::process_spec_rect(SCMatrixRectBlock* a,
                                SCMatrixRectBlock* b,
                                SCMatrixRectBlock* c)
{
  SCMatrixBlockIter*i = new SCMatrixRectBlockIter(a);
  SCMatrixBlockIter*j = new SCMatrixRectBlockIter(b);
  SCMatrixBlockIter*k = new SCMatrixRectBlockIter(c);
  process(*i,*j,*k);
  delete i;
  delete j;
  delete k;
}
void
SCElementOp3::process_spec_ltri(SCMatrixLTriBlock* a,
                                SCMatrixLTriBlock* b,
                                SCMatrixLTriBlock* c)
{
  SCMatrixBlockIter*i = new SCMatrixLTriBlockIter(a);
  SCMatrixBlockIter*j = new SCMatrixLTriBlockIter(b);
  SCMatrixBlockIter*k = new SCMatrixLTriBlockIter(c);
  process(*i,*j,*k);
  delete i;
  delete j;
  delete k;
}
void
SCElementOp3::process_spec_diag(SCMatrixDiagBlock* a,
                                SCMatrixDiagBlock* b,
                                SCMatrixDiagBlock* c)
{
  SCMatrixBlockIter*i = new SCMatrixDiagBlockIter(a);
  SCMatrixBlockIter*j = new SCMatrixDiagBlockIter(b);
  SCMatrixBlockIter*k = new SCMatrixDiagBlockIter(c);
  process(*i,*j,*k);
  delete i;
  delete j;
  delete k;
}
void
SCElementOp3::process_spec_vsimp(SCVectorSimpleBlock* a,
                                 SCVectorSimpleBlock* b,
                                 SCVectorSimpleBlock* c)
{
  SCMatrixBlockIter*i = new SCVectorSimpleBlockIter(a);
  SCMatrixBlockIter*j = new SCVectorSimpleBlockIter(b);
  SCMatrixBlockIter*k = new SCVectorSimpleBlockIter(c);
  process(*i,*j,*k);
  delete i;
  delete j;
  delete k;
}

/////////////////////////////////////////////////////////////////////////
// SCElementScale members

#define CLASSNAME SCElementScale
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCElementScale::SCElementScale(double a):scale(a) {}
SCElementScale::SCElementScale(StateIn&s):
  SCElementOp(s)
{
  s.get(scale);
}
void
SCElementScale::save_data_state(StateOut&s)
{
  s.put(scale);
}
void *
SCElementScale::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementScale::~SCElementScale() {}
void
SCElementScale::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      i.set(scale*i.get());
    }
}

int
SCElementScale::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementScalarProduct members

#define CLASSNAME SCElementScalarProduct
#define PARENTS   public SCElementOp2
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
SCElementScalarProduct::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp2::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementScalarProduct::SCElementScalarProduct():
  product(0.0), deferred_(0)
{
}

SCElementScalarProduct::SCElementScalarProduct(StateIn&s):
  SCElementOp2(s)
{
  s.get(product);
  s.get(deferred_);
}

void
SCElementScalarProduct::save_data_state(StateOut&s)
{
  s.put(product);
  s.put(deferred_);
}

SCElementScalarProduct::~SCElementScalarProduct()
{
}

void
SCElementScalarProduct::process(SCMatrixBlockIter&i,
                                SCMatrixBlockIter&j)
{
  for (i.reset(),j.reset(); i; ++i,++j) {
      product += i.get()*j.get();
    }
}

int
SCElementScalarProduct::has_collect()
{
  return 1;
}

void
SCElementScalarProduct::defer_collect(int h)
{
  deferred_=h;
}

void
SCElementScalarProduct::collect(const RefMessageGrp&grp)
{
  if (!deferred_)
    grp->sum(product);
}

double
SCElementScalarProduct::result()
{
  return product;
}

/////////////////////////////////////////////////////////////////////////
// SCDestructiveElementProduct members

#define CLASSNAME SCDestructiveElementProduct
#define PARENTS   public SCElementOp2
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCDestructiveElementProduct::SCDestructiveElementProduct() {}
SCDestructiveElementProduct::SCDestructiveElementProduct(StateIn&s):
  SCElementOp2(s)
{
}
void
SCDestructiveElementProduct::save_data_state(StateOut&s)
{
}
void *
SCDestructiveElementProduct::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp2::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCDestructiveElementProduct::~SCDestructiveElementProduct() {}
void
SCDestructiveElementProduct::process(SCMatrixBlockIter&i,
                                     SCMatrixBlockIter&j)
{
  for (i.reset(),j.reset(); i; ++i,++j) {
      i.set(i.get()*j.get());
    }
}

int
SCDestructiveElementProduct::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementInvert members

#define CLASSNAME SCElementInvert
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCElementInvert::SCElementInvert(double threshold):threshold_(threshold) {}
SCElementInvert::SCElementInvert(StateIn&s):
  SCElementOp(s)
{
  s.get(threshold_);
}
void
SCElementInvert::save_data_state(StateOut&s)
{
  s.put(threshold_);
}
void *
SCElementInvert::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementInvert::~SCElementInvert() {}
void
SCElementInvert::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      double val = i.get();
      if (fabs(val) > threshold_) val = 1.0/val;
      else val = 0.0;
      i.set(val);
    }
}

int
SCElementInvert::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementSquareRoot members

#define CLASSNAME SCElementSquareRoot
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCElementSquareRoot::SCElementSquareRoot() {}
SCElementSquareRoot::SCElementSquareRoot(double a) {}
SCElementSquareRoot::SCElementSquareRoot(StateIn&s):
  SCElementOp(s)
{
}
void
SCElementSquareRoot::save_data_state(StateOut&s)
{
}
void *
SCElementSquareRoot::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementSquareRoot::~SCElementSquareRoot() {}
void
SCElementSquareRoot::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      i.set(sqrt(i.get()));
    }
}

int
SCElementSquareRoot::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementMaxAbs members

SavableState_REF_def(SCElementMaxAbs);
#define CLASSNAME SCElementMaxAbs
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>

SCElementMaxAbs::SCElementMaxAbs():r(0.0), deferred_(0) {}
SCElementMaxAbs::SCElementMaxAbs(StateIn&s):
  SCElementOp(s)
{
  s.get(r);
  s.get(deferred_);
}
void
SCElementMaxAbs::save_data_state(StateOut&s)
{
  s.put(r);
  s.put(deferred_);
}
void *
SCElementMaxAbs::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementMaxAbs::~SCElementMaxAbs() {}
void
SCElementMaxAbs::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      if (fabs(i.get()) > r) r = fabs(i.get());
    }
}
double
SCElementMaxAbs::result()
{
  return r;
}
int
SCElementMaxAbs::has_collect()
{
  return 1;
}
void
SCElementMaxAbs::defer_collect(int h)
{
  deferred_=h;
}
void
SCElementMaxAbs::collect(const RefMessageGrp&msg)
{
  if (!deferred_)
    msg->max(r);
}

/////////////////////////////////////////////////////////////////////////
// SCElementAssign members

#define CLASSNAME SCElementAssign
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCElementAssign::SCElementAssign(double a):assign(a) {}
SCElementAssign::SCElementAssign(StateIn&s):
  SCElementOp(s)
{
  s.get(assign);
}
void
SCElementAssign::save_data_state(StateOut&s)
{
  s.put(assign);
}
void *
SCElementAssign::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementAssign::~SCElementAssign() {}
void
SCElementAssign::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      i.set(assign);
    }
}

int
SCElementAssign::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementRandomize members

#define CLASSNAME SCElementRandomize
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCElementRandomize::SCElementRandomize() {}
SCElementRandomize::SCElementRandomize(StateIn&s):
  SCElementOp(s)
{
}
void
SCElementRandomize::save_data_state(StateOut&s)
{
}
void *
SCElementRandomize::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementRandomize::~SCElementRandomize() {}
void
SCElementRandomize::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
#ifdef HAVE_DRAND48
      i.set(drand48()*(drand48()<0.5?1.0:-1.0));
#else
      int r=rand();
      double dr = (double) r / 32767.0;
      i.set(dr*(dr<0.5?1.0:-1.0));
#endif
    }
}

int
SCElementRandomize::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementShiftDiagonal members

#define CLASSNAME SCElementShiftDiagonal
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCElementShiftDiagonal::SCElementShiftDiagonal(double a):shift_diagonal(a) {}
SCElementShiftDiagonal::SCElementShiftDiagonal(StateIn&s):
  SCElementOp(s)
{
  s.get(shift_diagonal);
}
void
SCElementShiftDiagonal::save_data_state(StateOut&s)
{
  s.put(shift_diagonal);
}
void *
SCElementShiftDiagonal::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementShiftDiagonal::~SCElementShiftDiagonal() {}
void
SCElementShiftDiagonal::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      if (i.i() == i.j()) i.set(shift_diagonal+i.get());
    }
}

int
SCElementShiftDiagonal::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementScaleDiagonal members

#define CLASSNAME SCElementScaleDiagonal
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
SCElementScaleDiagonal::SCElementScaleDiagonal(double a):scale_diagonal(a) {}
SCElementScaleDiagonal::SCElementScaleDiagonal(StateIn&s):
  SCElementOp(s)
{
  s.get(scale_diagonal);
}
void
SCElementScaleDiagonal::save_data_state(StateOut&s)
{
  s.put(scale_diagonal);
}
void *
SCElementScaleDiagonal::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}
SCElementScaleDiagonal::~SCElementScaleDiagonal() {}
void
SCElementScaleDiagonal::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      if (i.i() == i.j()) i.set(scale_diagonal*i.get());
    }
}

int
SCElementScaleDiagonal::has_side_effects()
{
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// SCElementDot members

#define CLASSNAME SCElementDot
#define PARENTS   public SCElementOp
#define HAVE_STATEIN_CTOR
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
SCElementDot::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementDot::SCElementDot(double**a, double**b, int n):
  avects(a),
  bvects(b),
  length(n)
{
}

SCElementDot::SCElementDot(StateIn&s)
{
  cerr << indent << "SCElementDot does not permit StateIn CTOR\n";
  abort();
}

void
SCElementDot::save_data_state(StateOut&s)
{
  cerr << indent << "SCElementDot does not permit save_data_state\n";
  abort();
}

int
SCElementDot::has_side_effects()
{
  return 1;
}

void
SCElementDot::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      double tmp = i.get();
      double* a = avects[i.i()];
      double* b = bvects[i.j()];
      for (int j = length; j; j--, a++, b++) {
          tmp += *a * *b;
        }
      i.accum(tmp);
    }
}

/////////////////////////////////////////////////////////////////////////
// SCElementAccumulateSCMatrix members

#define CLASSNAME SCElementAccumulateSCMatrix
#define PARENTS   public SCElementOp
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
SCElementAccumulateSCMatrix::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementAccumulateSCMatrix::SCElementAccumulateSCMatrix(SCMatrix*a):
  m(a)
{
}

int
SCElementAccumulateSCMatrix::has_side_effects()
{
  return 1;
}

void
SCElementAccumulateSCMatrix::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      i.accum(m->get_element(i.i(), i.j()));
    }
}

/////////////////////////////////////////////////////////////////////////
// SCElementAccumulateSymmSCMatrix members

#define CLASSNAME SCElementAccumulateSymmSCMatrix
#define PARENTS   public SCElementOp
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
SCElementAccumulateSymmSCMatrix::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementAccumulateSymmSCMatrix::SCElementAccumulateSymmSCMatrix(
    SymmSCMatrix*a):
  m(a)
{
}

int
SCElementAccumulateSymmSCMatrix::has_side_effects()
{
  return 1;
}

void
SCElementAccumulateSymmSCMatrix::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      i.accum(m->get_element(i.i(), i.j()));
    }
}

/////////////////////////////////////////////////////////////////////////
// SCElementAccumulateDiagSCMatrix members

#define CLASSNAME SCElementAccumulateDiagSCMatrix
#define PARENTS   public SCElementOp
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
SCElementAccumulateDiagSCMatrix::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementAccumulateDiagSCMatrix::SCElementAccumulateDiagSCMatrix(
    DiagSCMatrix*a):
  m(a)
{
}

int
SCElementAccumulateDiagSCMatrix::has_side_effects()
{
  return 1;
}

void
SCElementAccumulateDiagSCMatrix::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      i.accum(m->get_element(i.i()));
    }
}

/////////////////////////////////////////////////////////////////////////
// SCElementAccumulateSCVector members

#define CLASSNAME SCElementAccumulateSCVector
#define PARENTS   public SCElementOp
#include <util/state/statei.h>
#include <util/class/classi.h>
void *
SCElementAccumulateSCVector::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = SCElementOp::_castdown(cd);
  return do_castdowns(casts,cd);
}

SCElementAccumulateSCVector::SCElementAccumulateSCVector(SCVector*a):
  m(a)
{
}

int
SCElementAccumulateSCVector::has_side_effects()
{
  return 1;
}

void
SCElementAccumulateSCVector::process(SCMatrixBlockIter&i)
{
  for (i.reset(); i; ++i) {
      i.accum(m->get_element(i.i()));
    }
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// eval: (c-set-style "CLJ")
// End:
