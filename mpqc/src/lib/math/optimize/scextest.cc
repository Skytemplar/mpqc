//
// scextest.cc
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

#include <util/keyval/keyval.h>
#include <util/state/stateio.h>
#include <math/scmat/local.h>
#include <math/optimize/diis.h>
#include <math/optimize/scextrap.h>
#include <math/optimize/scextrapmat.h>

// Force linkages:
#ifndef __PIC__
const ClassDesc &fl0 = DIIS::class_desc_;
#endif

int
main()
{
  int i;
  
  RefKeyVal keyval = new ParsedKeyVal( SRCDIR "/scextest.in");

  RefSelfConsistentExtrapolation extrap
      = keyval->describedclassvalue("scextrap");

  RefSCDimension dim = new LocalSCDimension(3, "test_dim");

  RefSymmSCMatrix datamat(dim);
  datamat.assign(0.0);
  datamat->shift_diagonal(2.0);

  RefDiagSCMatrix val(dim);
  RefSCMatrix vec(dim,dim);

  // solve f(x) = x

  i = 0;
  while (i < 100 && !extrap->converged()) {
      datamat.diagonalize(val,vec);
      for (int j=0; j<datamat.dim().n(); j++) {
          double v = val.get_element(j);
          val.set_element(j, sqrt(v));
        }
      RefSymmSCMatrix newdatamat(dim);
      newdatamat.assign(0.0);
      newdatamat.accumulate_transform(vec, val);
      RefSymmSCMatrix errormat = newdatamat - datamat;

      datamat.assign(newdatamat);
      RefSCExtrapData data = new SymmSCMatrixSCExtrapData(datamat);
      RefSCExtrapError error = new SymmSCMatrixSCExtrapError(errormat);

      cout << "Iteration " << i << ":" << endl;

      datamat.print("Datamat:");
      errormat.print("Errormat:");

      extrap->extrapolate(data, error);

      datamat.print("Extrap Datamat");

      i++;
    }

  StateOutText s("scextest.ckpt");
  extrap.save_state(s);
  s.close();

  StateInText si("scextest.ckpt");
  RefSelfConsistentExtrapolation e2(si);
  
  si.close();
  
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
