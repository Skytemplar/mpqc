//
// mstate.h
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

#ifdef __GNUC__
#pragma interface
#endif

#ifndef _util_group_mstate_h
#define _util_group_mstate_h

#include <util/state/state.h>
#include <util/state/statein.h>
#include <util/state/stateout.h>
#include <util/group/message.h>

//. The \clsnm{MsgStateSend} is an abstract base class that sends objects
//to nodes in a \clsnmref{MessageGrp}.
class MsgStateSend: public StateOut {
  private:
    // do not allow copy constructor or assignment
    MsgStateSend(const MsgStateSend&);
    void operator=(const MsgStateSend&);
  protected:
    RefMessageGrp grp;
    int nbuf; // the number of bytes used in the buffer
    int bufsize; // the allocated size of the data buffer
    char* buffer; // the data buffer
    char* send_buffer; // the buffer used to send data (includes nbuf)
    int nheader; // nbuf + nheader = the number of bytes in send_buffer to send
    int* nbuf_buffer; // the pointer to the nbuf stored in the buffer

    int put_array_void(const void*, int);
  public:
    MsgStateSend(const RefMessageGrp&);
    virtual ~MsgStateSend();

    //. Specializations must implement \srccd{flush()}.
    virtual void flush() = 0;

    //. The buffer size of statein and stateout objects that communicate
    //with each other must match.
    void set_buffer_size(int);

    //. I only need to override \srccd{put(const ClassDesc*)} but C++ will
    //hide all of the other put's so I must override everything.
    int put(const ClassDesc*);
    int put(char r);
    int put(int r);
    int put(float r);
    int put(double r);
    int put(const char*,int);
    int put(const int*,int);
    int put(const float*,int);
    int put(const double*,int);
};

//. The \clsnm{MsgStateBufRecv} is an abstract base class that
//buffers objects sent through a \clsnm{MessageGrp}.
class MsgStateBufRecv: public StateIn {
#   define CLASSNAME MsgStateBufRecv
#   include <util/class/classda.h>
  private:
    // do not allow copy constructor or assignment
    MsgStateBufRecv(const MsgStateBufRecv&);
    void operator=(const MsgStateBufRecv&);
  protected:
    RefMessageGrp grp;
    int nbuf; // the number of bytes used in the buffer
    int ibuf; // the current pointer withing the buffer
    int bufsize; // the allocated size of the buffer
    char* buffer; // the data buffer
    char* send_buffer; // the buffer used to send data (includes nbuf)
    int nheader; // nbuf + nheader = the number of bytes in send_buffer to send
    int* nbuf_buffer; // the pointer to the nbuf stored in the buffer

    int get_array_void(void*,int);

    //. Specializations must implement \srccd{next\_buffer()}.
    virtual void next_buffer() = 0;
  public:
    //. \clsnm{MsgStateBufRecv} can be initialized with a
    //\clsnmref{MessageGrp}.
    MsgStateBufRecv(const RefMessageGrp&);
    //. \clsnm{MsgStateBufRecv} will use the default \clsnmref{MessageGrp}.
    MsgStateBufRecv();

    virtual ~MsgStateBufRecv();

    //. The buffer size of statein and stateout objects that communicate
    //with each other must match.
    void set_buffer_size(int);
};

//. The \clsnm{MsgStateRecv} is an abstract base class that receives
//objects from nodes in a \clsnmref{MessageGrp}.
class MsgStateRecv: public MsgStateBufRecv {
  private:
    // do not allow copy constructor or assignment
    MsgStateRecv(const MsgStateRecv&);
    void operator=(const MsgStateRecv&);
  public:
    //. \clsnm{MsgStateRecv} must be initialized with a \clsnmref{MessageGrp}.
    MsgStateRecv(const RefMessageGrp&);

    virtual ~MsgStateRecv();

    //. Returns the version of the ClassDesc.  This assumes that
    // the version of the remote class is the same as that of
    // the local class.
    int version(const ClassDesc*);

    //. I only need to override \srccd{get(ClassDesc**)} but C++ will hide
    //all of the other put's so I must override everything.
    int get(const ClassDesc**);
    int get(char&r, const char *key = 0);
    int get(int&r, const char *key = 0);
    int get(float&r, const char *key = 0);
    int get(double&r, const char *key = 0);
    int get(char*&);
    int get(int*&);
    int get(float*&);
    int get(double*&);
};

//. \clsnm{StateSend} is a concrete specialization of
//\clsnmref{MsgStateSend} that does the send part of point to
//point communication in a \clsnmref{MessageGrp}.
class StateSend: public MsgStateSend {
  private:
    // do not allow copy constructor or assignment
    StateSend(const StateSend&);
    void operator=(const StateSend&);
  private:
    int target_;
  public:
    //. Create a \clsnm{StateSend} given a \clsnmref{MessageGrp}.
    StateSend(const RefMessageGrp&);

    ~StateSend();
    //. Specify the target node.
    void target(int);
    //. Flush the buffer.
    void flush();
};

//. \clsnm{StateRecv} is a concrete specialization of
//\clsnmref{MsgStateRecv} that does the receive part of point to
//point communication in a \clsnmref{MessageGrp}.
class StateRecv: public MsgStateRecv {
  private:
    // do not allow copy constructor or assignment
    StateRecv(const StateRecv&);
    void operator=(const StateRecv&);
  private:
    int source_;
  protected:
    void next_buffer();
  public:
    //. Create a \clsnm{StateRecv} given a \clsnmref{MessageGrp}.
    StateRecv(const RefMessageGrp&);
    //. Specify the source node.
    void source(int);
};

//.  \clsnm{BcastStateSend} does the send part of a broadcast of an object
//to all nodes.  Only one node uses a \clsnm{BcastStateSend} and the rest
//must use a \clsnmref{BcastStateRecv}.
class BcastStateSend: public MsgStateSend {
  private:
    // do not allow copy constructor or assignment
    BcastStateSend(const BcastStateSend&);
    void operator=(const BcastStateSend&);
  public:
    //. Create the \clsnm{BcastStateSend}.
    BcastStateSend(const RefMessageGrp&);

    ~BcastStateSend();
    //. Flush the data remaining in the buffer.
    void flush();
};

//.  \clsnm{BcastStateRecv} does the receive part of a broadcast of an
//object to all nodes.  Only one node uses a \clsnmref{BcastStateSend} and
//the rest must use a \clsnm{BcastStateRecv}.
class BcastStateRecv: public MsgStateRecv {
  private:
    // do not allow copy constructor or assignment
    BcastStateRecv(const BcastStateRecv&);
    void operator=(const BcastStateRecv&);
  protected:
    int source_;
    void next_buffer();
  public:
    //. Create the \clsnm{BcastStateRecv}.
    BcastStateRecv(const RefMessageGrp&, int source = 0);
    //. Set the source node.
    void source(int s);
};

//. This creates and forwards/retrieves data from either
// a \clsnmref{BcastStateRecv} or a \clsnmref{BcastStateSend}
// depending on the value of the \vrbl{source} argument to
// constructor.
class BcastState {
  private:
    BcastStateRecv *recv_;
    BcastStateSend *send_;
  public:
    //. Create a \clsnm{BcastState} object.  The default
    // source is node 0.
    BcastState(const RefMessageGrp &, int source = 0);

    ~BcastState();

    //. Broadcast data to all nodes.  After these are called
    // for a group of data the \srccd{flush} member must be called
    // to force the source node to actually write the data.
    void bcast(int &);
    void bcast(double &);
    void bcast(int *&, int);
    void bcast(double *&, int);
    void bcast(SSRefBase &);

    //. Force data to be written.  Data is not otherwise written
    // until the buffer is full.
    void flush();

    //. Call the \clsnmref{StateOut} or \clsnmref{StateIn}
    // \srccd{forget\_references} member.
    void forget_references();

    //. Controls the amount of data that is buffered before it is
    // sent.
    void set_buffer_size(int);
};

//.  \clsnm{BcastStateBin} reads a file in written by
//StateInBin on node 0 and broadcasts it to all nodes
//so state can be simultaneously restored on all nodes.
class BcastStateInBin: public MsgStateBufRecv {
#   define CLASSNAME BcastStateInBin
#   define HAVE_KEYVAL_CTOR
#   include <util/class/classd.h>
  private:
    // do not allow copy constructor or assignment
    BcastStateInBin(const BcastStateRecv&);
    void operator=(const BcastStateRecv&);
  protected:
    int opened_;
    int file_position_;
    streambuf *buf_;

    void next_buffer();
    int get_array_void(void*, int);
  public:
    //. Create the \clsnm{BcastStateRecv} using the default
    //\clsnmref{MessageGrp}.
    BcastStateInBin(const RefKeyVal &);
    //. Create the \clsnm{BcastStateRecv}.
    BcastStateInBin(const RefMessageGrp&, const char *filename);

    ~BcastStateInBin();

    virtual int open(const char *name);
    virtual void close();

    void seek(int loc);
    int seekable();
    int tell();
    int use_directory();
};

#endif

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
