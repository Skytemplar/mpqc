//
// statein.h
//
// Copyright (C) 1998 Limit Point Systems, Inc.
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

#ifndef _util_state_statein_h
#define _util_state_statein_h

#ifdef __GNUC__
#pragma interface
#endif

#include <util/state/state.h>
#include <util/container/avlmap.h>
#include <util/keyval/keyval.h>

class StateInData {
  public:
    RefSavableState ptr;
    int size;
    int type;
    int offset;

    StateInData(): size(0), type(0), offset(0) {}
};

class StateClassData {
  public:
    int version;
    char *name;
    const ClassDesc *classdesc;
    int ninstance;
  public:
    StateClassData(int v=-1, const ClassDesc *c=0, char *name=0):
      version(v), classdesc(c), name(name), ninstance(0) {}
    StateClassData(const StateClassData &d) { operator=(d); }
    ~StateClassData();
    StateClassData &operator=(const StateClassData &d);
};

//. Objects of a type derived from \clsnmref{SavableState} can be
//. restored from a \clsnm{StateIn} object.
class StateIn:  public DescribedClass {
#   define CLASSNAME StateIn
#   include <util/class/classda.h>
    friend class SavableState;
    friend class TranslateDataIn;
  private:
    // do not allow copy constructor or assignment
    StateIn(const StateIn&);
    void operator=(const StateIn&);
    int have_cd_;
    int dir_loc_;
    char key_[KeyVal::MaxKeywordLength];
    int keylength_;
  protected:
    RefKeyVal override_;
    TranslateDataIn *translate_;
    AVLMap<int,StateInData> ps_;
    int expected_object_num_;
    AVLMap<ClassDescP,int> classidmap_;
    AVLMap<int,StateClassData> classdatamap_;
    int nextclassid_;
    int node_to_node_;
    int version_;
    int date_;
    char userid_[9];
    char format_;
    virtual int get_array_void(void*,int);

    int push_key(const char *key);
    void pop_key(int n) { key_[n] = '\0'; keylength_ = n; }
    const char *key() { return key_; }

    void get_directory();
    int directory_location() const { return dir_loc_; }
    void find_and_get_directory();

    // The following members are called by friend SavableState

    //. This is used to restore an object.  It is called with the
    //. reference to the reference being restored.  If the data being
    //. restored has previously been restored, then the pointer
    //. being restored is set to a reference to the previously
    //. restored object.
    virtual int getobject(RefSavableState &);

    //. This restores objects that are listed in the directory.
    virtual int dir_getobject(RefSavableState &, const char *name);

    //. When storage has been allocated during object restoration,
    //. this routine is called with the object reference number
    //. and the pointer to the new storage so \srccd{getpointer}
    //. can find the data if it is referenced again.
    virtual void haveobject(int,const RefSavableState &);

    //. A call to nextobject followed by havepointer(int) is equiv
    //. to havepointer(int,void**);
    virtual void nextobject(int);
    virtual void haveobject(const RefSavableState &);

    void have_classdesc() { have_cd_ = 1; }
    int need_classdesc() { int tmp = have_cd_; have_cd_ = 0; return !tmp; }

    //. This restores \srccd{ClassDesc}'s.  It will set the
    //. pointer to the address of the static \srccd{ClassDesc} for
    //. the class which has the same name as the class that had
    //. the \clsnm{ClassDesc} that was saved by \srccd{put(const
    //. \clsnmref{ClassDesc}*)}.
    virtual int get(const ClassDesc**);
  public:
    StateIn();
    virtual ~StateIn();

    //. Read in the header information.  Changes the translation
    //scheme if necessary.
    virtual void get_header();

    //. Returns the version of the ClassDesc in the persistent object
    //. or -1 if info on the ClassDesc doesn't exist
    virtual int version(const ClassDesc*);
    
    //. This restores strings saved with
    //. \srccd{\clsnmref{StateOut}::putstring}.
    virtual int getstring(char*&);

    //. These restore data saved with \clsnmref{StateOut}'s \srccd{put}.
    //members.
    virtual int get(char&r, const char *keyword = 0);
    virtual int get(int&r, const char *keyword = 0);
    virtual int get(float&r, const char *keyword = 0);
    virtual int get(double&r, const char *keyword = 0);
    //. These restore data saved with \clsnmref{StateOut}'s \srccd{put}.
    //members.  The data is allocated by StateIn.
    virtual int get(char*&);
    virtual int get(int*&);
    virtual int get(float*&);
    virtual int get(double*&);
    //. These restore data saved with \clsnmref{StateOut}'s \srccd{put}.
    //members.  The data must be preallocated by the user.
    virtual int get_array_char(char*p,int size);
    virtual int get_array_int(int*p,int size);
    virtual int get_array_float(float*p,int size);
    virtual int get_array_double(double*p,int size);

    //. True if this is a node to node save/restore.  This is
    //necessary for classes that try to avoid saving databases
    //to files that can otherwise be read in, but want to avoid
    //reading the database from disk on all nodes.
    int node_to_node() const { return node_to_node_; }

    //. Returns true of this object uses a directory.
    virtual int use_directory();

    //. Return the current position in the file.
    virtual int tell();
    //. Set the current position in the file.  The default implementation
    //does nothing.
    virtual void seek(int);
    //. Return non-zero if seek does anything sensible.  The
    //default implementation returns 0.
    virtual int seekable();

    //. List all the objects to the stream.  Only \clsnm{StateIn}
    //specializations with directories can list objects.
    virtual void list_objects(ostream& =cout);

    //. Give this \clsnm{StateIn} a \clsnmref{KeyVal} object
    //that is used to override values.
    void set_override(const RefKeyVal&kv) { override_ = kv; }
    //. Return the \clsnmref{KeyVal} used to override values.
    const RefKeyVal &override() const { return override_; }
  };
DescribedClass_REF_dec(StateIn);

#endif

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
