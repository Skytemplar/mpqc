//
// elemop.h
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Curtis Janssen <cljanss@limitpt.com>
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

#ifndef _math_scmat_elemop_h
#define _math_scmat_elemop_h

#ifdef __GNUC__
#pragma interface
#endif

#include <util/state/state.h>
#include <util/group/message.h>

class SCMatrixBlock;
class SCMatrixBlockIter;
class SCMatrixRectBlock;
class SCMatrixLTriBlock;
class SCMatrixDiagBlock;
class SCVectorSimpleBlock;
class SCMatrixRectSubBlock;
class SCMatrixLTriSubBlock;
class SCMatrixDiagSubBlock;
class SCVectorSimpleSubBlock;

class SCMatrix;
class SymmSCMatrix;
class DiagSCMatrix;
class SCVector;

class SSRefSCElementOp;
typedef class SSRefSCElementOp RefSCElementOp;

class SSRefSCElementOp2;
typedef class SSRefSCElementOp2 RefSCElementOp2;

class SSRefSCElementOp3;
typedef class SSRefSCElementOp3 RefSCElementOp3;

/** Objects of class SCElementOp are used to perform operations on the
    elements of matrices.  When the SCElementOp object is given to the
    element_op member of a matrix, each block the matrix is passed to one
    of the process, process_base, or process_base members. */
class SCElementOp: public SavableState {
#   define CLASSNAME SCElementOp
#   include <util/state/stated.h>
#   include <util/class/classda.h>
  public:
    SCElementOp();
    SCElementOp(StateIn&s): SavableState(s) {}
    virtual ~SCElementOp();
    /** If duplicates of the SCElementOp exist (that is, there is more than
        one node), then if has_collect returns nonzero then collect is
        called with a MessageGrp reference after all of the blocks have
        been processed.  The default return value of has_collect is 0 and
        collect's default action is do nothing.  If defer_collect member is
        called with nonzero, collect will do nothing (this is only used by
        the blocked matrices). */
    virtual int has_collect();
    virtual void defer_collect(int);
    virtual void collect(const RefMessageGrp&);
    /** By default this returns nonzero.  If the ElementOp specialization
        will change any elements of the matrix, then this must be
        overridden to return nonzero. */
    virtual int has_side_effects();

    /** This is the fallback routine to process blocks and is called
        by process_spec members that are not overridden. */
    virtual void process(SCMatrixBlockIter&) = 0;

    /** Lazy matrix implementors can call this member when the
        type of block specialization is unknown.  However, this
        will attempt to castdown block to a block specialization
        and will thus be less efficient. */
    void process_base(SCMatrixBlock*block);

    /** Matrices should call these members when the type of block is known.
        ElementOp specializations should override these when
        efficiency is important, since these give the most efficient access
        to the elements of the block. */
    virtual void process_spec_rect(SCMatrixRectBlock*);
    virtual void process_spec_ltri(SCMatrixLTriBlock*);
    virtual void process_spec_diag(SCMatrixDiagBlock*);
    virtual void process_spec_vsimp(SCVectorSimpleBlock*);
    virtual void process_spec_rectsub(SCMatrixRectSubBlock*);
    virtual void process_spec_ltrisub(SCMatrixLTriSubBlock*);
    virtual void process_spec_diagsub(SCMatrixDiagSubBlock*);
    virtual void process_spec_vsimpsub(SCVectorSimpleSubBlock*);
};
DCRef_declare(SCElementOp);
SSRef_declare(SCElementOp);

/** The SCElementOp2 class is very similar to the SCElementOp class except
    that pairs of blocks are treated simultaneously.  The two matrices
    involved must have identical storage layout, which will be the case if
    both matrices are of the same type and dimensions.  */
class SCElementOp2: public SavableState {
#   define CLASSNAME SCElementOp2
#   include <util/state/stated.h>
#   include <util/class/classda.h>
  public:
    SCElementOp2();
    SCElementOp2(StateIn&s): SavableState(s) {}
    virtual ~SCElementOp2();
    virtual int has_collect();
    virtual void defer_collect(int);
    virtual int has_side_effects();
    virtual int has_side_effects_in_arg();
    virtual void collect(const RefMessageGrp&);
    virtual void process(SCMatrixBlockIter&,SCMatrixBlockIter&) = 0;
    void process_base(SCMatrixBlock*,SCMatrixBlock*);
    virtual void process_spec_rect(SCMatrixRectBlock*,SCMatrixRectBlock*);
    virtual void process_spec_ltri(SCMatrixLTriBlock*,SCMatrixLTriBlock*);
    virtual void process_spec_diag(SCMatrixDiagBlock*,SCMatrixDiagBlock*);
    virtual void process_spec_vsimp(SCVectorSimpleBlock*,SCVectorSimpleBlock*);
};
DCRef_declare(SCElementOp2);
SSRef_declare(SCElementOp2);

/** The SCElementOp3 class is very similar to the SCElementOp class except
    that a triplet of blocks is treated simultaneously.  The three matrices
    involved must have identical storage layout, which will be the case if
    all matrices are of the same type and dimensions.  */
class SCElementOp3: public SavableState {
#   define CLASSNAME SCElementOp3
#   include <util/state/stated.h>
#   include <util/class/classda.h>
  public:
    SCElementOp3();
    SCElementOp3(StateIn&s): SavableState(s) {}
    virtual ~SCElementOp3();
    virtual int has_collect();
    virtual void defer_collect(int);
    virtual int has_side_effects();
    virtual int has_side_effects_in_arg1();
    virtual int has_side_effects_in_arg2();
    virtual void collect(const RefMessageGrp&);
    virtual void process(SCMatrixBlockIter&,
                         SCMatrixBlockIter&,
                         SCMatrixBlockIter&) = 0;
    void process_base(SCMatrixBlock*,SCMatrixBlock*,SCMatrixBlock*);
    virtual void process_spec_rect(SCMatrixRectBlock*,
                                   SCMatrixRectBlock*,
                                   SCMatrixRectBlock*);
    virtual void process_spec_ltri(SCMatrixLTriBlock*,
                                   SCMatrixLTriBlock*,
                                   SCMatrixLTriBlock*);
    virtual void process_spec_diag(SCMatrixDiagBlock*,
                                   SCMatrixDiagBlock*,
                                   SCMatrixDiagBlock*);
    virtual void process_spec_vsimp(SCVectorSimpleBlock*,
                                    SCVectorSimpleBlock*,
                                    SCVectorSimpleBlock*);
};
DCRef_declare(SCElementOp3);
SSRef_declare(SCElementOp3);

class SCElementScalarProduct: public SCElementOp2 {
#   define CLASSNAME SCElementScalarProduct
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    int deferred_;
    double product;
  public:
    SCElementScalarProduct();
    SCElementScalarProduct(StateIn&);
    ~SCElementScalarProduct();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&,SCMatrixBlockIter&);
    int has_collect();
    void defer_collect(int);
    void collect(const RefMessageGrp&);
    double result();
    void init() { product = 0.0; }
};
SavableState_REF_dec(SCElementScalarProduct);

class SCDestructiveElementProduct: public SCElementOp2 {
#   define CLASSNAME SCDestructiveElementProduct
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  public:
    SCDestructiveElementProduct();
    SCDestructiveElementProduct(StateIn&);
    ~SCDestructiveElementProduct();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&,SCMatrixBlockIter&);
};

class SCElementScale: public SCElementOp {
#   define CLASSNAME SCElementScale
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    double scale;
  public:
    SCElementScale(double a);
    SCElementScale(StateIn&);
    ~SCElementScale();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
};

class SCElementRandomize: public SCElementOp {
#   define CLASSNAME SCElementRandomize
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    double assign;
  public:
    SCElementRandomize();
    SCElementRandomize(StateIn&);
    ~SCElementRandomize();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
};

class SCElementAssign: public SCElementOp {
#   define CLASSNAME SCElementAssign
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    double assign;
  public:
    SCElementAssign(double a);
    SCElementAssign(StateIn&);
    ~SCElementAssign();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
};

class SCElementSquareRoot: public SCElementOp {
#   define CLASSNAME SCElementSquareRoot
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  public:
    SCElementSquareRoot();
    SCElementSquareRoot(double a);
    SCElementSquareRoot(StateIn&);
    ~SCElementSquareRoot();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
};

class SCElementInvert: public SCElementOp {
#   define CLASSNAME SCElementInvert
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    double threshold_;
    int nbelowthreshold_;
    int deferred_;
  public:
    SCElementInvert(double threshold = 0.0);
    SCElementInvert(StateIn&);
    ~SCElementInvert();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
    int has_collect();
    void defer_collect(int);
    void collect(const RefMessageGrp&);
    int result() { return nbelowthreshold_; }
};
SavableState_REF_dec(SCElementInvert);

class SCElementScaleDiagonal: public SCElementOp {
#   define CLASSNAME SCElementScaleDiagonal
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    double scale_diagonal;
  public:
    SCElementScaleDiagonal(double a);
    SCElementScaleDiagonal(StateIn&);
    ~SCElementScaleDiagonal();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
};

class SCElementShiftDiagonal: public SCElementOp {
#   define CLASSNAME SCElementShiftDiagonal
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    double shift_diagonal;
  public:
    SCElementShiftDiagonal(double a);
    SCElementShiftDiagonal(StateIn&);
    ~SCElementShiftDiagonal();
    int has_side_effects();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
};

class SCElementMaxAbs: public SCElementOp {
#   define CLASSNAME SCElementMaxAbs
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    int deferred_;
    double r;
  public:
    SCElementMaxAbs();
    SCElementMaxAbs(StateIn&);
    ~SCElementMaxAbs();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
    int has_collect();
    void defer_collect(int);
    void collect(const RefMessageGrp&);
    double result();
};
SavableState_REF_dec(SCElementMaxAbs);

class SCElementMinAbs: public SCElementOp {
#   define CLASSNAME SCElementMinAbs
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    int deferred_;
    double r;
  public:
    // rinit must be greater than the magnitude of the smallest element
    SCElementMinAbs(double rinit);
    SCElementMinAbs(StateIn&);
    ~SCElementMinAbs();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
    int has_collect();
    void defer_collect(int);
    void collect(const RefMessageGrp&);
    double result();
};
SavableState_REF_dec(SCElementMinAbs);

class SCElementSumAbs: public SCElementOp {
#   define CLASSNAME SCElementSumAbs
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    int deferred_;
    double r;
  public:
    SCElementSumAbs();
    SCElementSumAbs(StateIn&);
    ~SCElementSumAbs();
    void save_data_state(StateOut&);
    void process(SCMatrixBlockIter&);
    int has_collect();
    void defer_collect(int);
    void collect(const RefMessageGrp&);
    double result();
    void init() { r = 0.0; }
};
SavableState_REF_dec(SCElementSumAbs);

class SCElementDot: public SCElementOp {
#   define CLASSNAME SCElementDot
#   define HAVE_STATEIN_CTOR
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    double** avects;
    double** bvects;
    int length;
  public:
    SCElementDot(StateIn&);
    void save_data_state(StateOut&);
    SCElementDot(double**a, double**b, int length);
    void process(SCMatrixBlockIter&);
    int has_side_effects();
};

class SCElementAccumulateSCMatrix: public SCElementOp {
#   define CLASSNAME SCElementAccumulateSCMatrix
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    SCMatrix *m;
  public:
    SCElementAccumulateSCMatrix(SCMatrix *);
    int has_side_effects();
    void process(SCMatrixBlockIter&);
};

class SCElementAccumulateSymmSCMatrix: public SCElementOp {
#   define CLASSNAME SCElementAccumulateSymmSCMatrix
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    SymmSCMatrix *m;
  public:
    SCElementAccumulateSymmSCMatrix(SymmSCMatrix *);
    int has_side_effects();
    void process(SCMatrixBlockIter&);
};

class SCElementAccumulateDiagSCMatrix: public SCElementOp {
#   define CLASSNAME SCElementAccumulateDiagSCMatrix
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    DiagSCMatrix *m;
  public:
    SCElementAccumulateDiagSCMatrix(DiagSCMatrix *);
    int has_side_effects();
    void process(SCMatrixBlockIter&);
};

class SCElementAccumulateSCVector: public SCElementOp {
#   define CLASSNAME SCElementAccumulateSCVector
#   include <util/state/stated.h>
#   include <util/class/classd.h>
  private:
    SCVector *m;
  public:
    SCElementAccumulateSCVector(SCVector *);
    int has_side_effects();
    void process(SCMatrixBlockIter&);
};

#endif

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
