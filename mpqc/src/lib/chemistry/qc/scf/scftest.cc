//
// scftest.cc --- test program
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Edward Seidl <seidl@janed.com>
// Maintainer: LPS
//
// This file is part of the SC Toolkit.
//
// The SC Toolkit is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SC Toolkit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SC Toolkit; see the file COPYING.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
// The U.S. Government is granted a limited license as per AL 91-7.
//

#ifdef HAVE_CONFIG_H
#include <scconfig.h>
#endif

#include <string.h>

#include <sys/stat.h>
#include <unistd.h>
#include <new.h>

#include <util/keyval/keyval.h>
#include <util/group/message.h>
#include <util/group/pregtime.h>
#include <util/group/picl.h>
#include <util/misc/bug.h>
#include <util/misc/formio.h>

#include <math/optimize/opt.h>

#include <chemistry/molecule/coor.h>
#include <chemistry/molecule/energy.h>

// Force linkages:
#include <chemistry/qc/scf/linkage.h>

RefRegionTimer tim;
RefMessageGrp grp;

static RefMessageGrp
init_mp(const RefKeyVal& keyval)
{
  // if we are on a paragon then use a ParagonMessageGrp
  // otherwise read the message group from the input file
#ifdef HAVE_NX_H
  grp = new ParagonMessageGrp;
#else
  grp = keyval->describedclassvalue("message");
#endif

  if (grp.nonnull()) MessageGrp::set_default_messagegrp(grp);
  else grp = MessageGrp::get_default_messagegrp();

  RefDebugger debugger = keyval->describedclassvalue(":debug");
  // Let the debugger know the name of the executable and the node
  if (debugger.nonnull()) {
    debugger->set_exec("scftest");
    debugger->set_prefix(grp->me());
    debugger->debug("curt is a hog");
  }
  
  tim = new ParallelRegionTimer(grp,"scftest",1,0);
  RegionTimer::set_default_regiontimer(tim);

  SCFormIO::set_printnode(0);
  SCFormIO::set_messagegrp(grp);
  //SCFormIO::set_debug(1);

  SCFormIO::setindent(cout, 2);
  SCFormIO::setindent(cerr, 2);
  
  {
    int nproc, me, host, top, ord, dir;
    open0_messagegrp(&nproc,&me,&host,grp);
    setarc0(&nproc,&top,&ord,&dir);
  }
  
  return grp;
}

main(int argc, char**argv)
{
  char *input =      (argc > 1)? argv[1] : SRCDIR "/mpqc.in";
  char *keyword =    (argc > 2)? argv[2] : "mole";
  char *optkeyword = (argc > 3)? argv[3] : "opt";

  // open keyval input
  RefKeyVal rpkv(new ParsedKeyVal(input));

  init_mp(rpkv);

  tim->enter("input");
  
  if (rpkv->exists("matrixkit"))
    SCMatrixKit::set_default_matrixkit(rpkv->describedclassvalue("matrixkit"));
  
  struct stat sb;
  RefMolecularEnergy mole;
  RefOptimize opt;

  if (stat("scftest.ckpt",&sb)==0 && sb.st_size) {
    StateInBin si("scftest.ckpt");
    opt.restore_state(si);
    mole = opt->function();
  } else {
    mole = rpkv->describedclassvalue(keyword);
    opt = rpkv->describedclassvalue(optkeyword);
    if (opt.nonnull()) {
      opt->set_checkpoint();
      opt->set_checkpoint_file("scftest.ckpt");
    }
  }

  tim->exit("input");

  if (mole.nonnull()) {
    if (mole->gradient_implemented()) {
      if (opt.nonnull()) {
        opt->optimize();
      } else {
        mole->gradient().print("gradient");
      }
    } else if (mole->value_implemented()) {
      cout << node0 << indent
           << scprintf("value of mole is %15.10f\n\n", mole->energy());
    }
  }

  mole->print(cout);

  StateOutBin so("scftest.wfn");
  mole.save_state(so);
  
  tim->print(cout);

  return 0;
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "ETS"
// End:
