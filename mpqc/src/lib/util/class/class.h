
#ifndef _libQC_class_h
#define _libQC_class_h

#include <string.h>
#include <stdio.h>
#include <util/container/ref.h>
#include <util/container/array.h>
#include <util/container/set.h>

extern "C" void * sbrk(int);

class ClassKeyClassDescPMap;
class ClassKeySet;
class DescribedClass;
class ClassDesc;
typedef ClassDesc* ClassDescP;

class ClassKey {
  private:
    char* classname_;
  public:
    ClassKey();
    ClassKey(const char* name);
    ClassKey(const ClassKey&);
    ~ClassKey();
    ClassKey& operator=(const ClassKey&);
    int operator==(ClassKey& ck);
    int hash() const;
    int cmp(ClassKey&ck) const;
    char* name() const;
  };

class ClassDesc;

class ParentClass
{
  public:
    enum Access { Private, Protected, Public };
  private:
    Access _access;
    int _is_virtual;
    ClassDesc* _classdesc;
  public:
    ParentClass(ClassDesc*,Access access = Private,int is_virtual = 0);
    ParentClass(const ParentClass&);
    ~ParentClass();
    int is_virtual() const;
    Access access() const;
    const ClassDesc* classdesc() const;
    void change_classdesc(ClassDesc*n);
};

class ParentClasses
{
  private:
    int _n;
    ParentClass** _classes;
    void add(ParentClass*);
    // do not allow copy constructor or assignment
    ParentClasses(const ParentClasses&);
    operator=(const ParentClasses&);
  public:
    ParentClasses(const char*);
    ~ParentClasses();
    const ParentClass& parent(int i) const;
    const ParentClass& operator[](int i) const;
    int n() const;
    void change_parent(ClassDesc*oldcd,ClassDesc*newcd);
};
    

class KeyVal;
class StateIn;

class ClassDesc {
  private:
    char* classname_;
    int version_;
    ParentClasses parents_;
    ClassKeySet* children_;
    DescribedClass* (*ctor_)();
    DescribedClass* (*keyvalctor_)(KeyVal&);
    DescribedClass* (*stateinctor_)(StateIn&);

    void change_parent(ClassDesc*oldcd,ClassDesc*newcd);

    // do not allow copy constructor or assignment
    ClassDesc(const ClassDesc&);
    operator=(const ClassDesc&);
  public:
    ClassDesc(char*,int=1,char* p=0,
              DescribedClass* (*ctor)()=0,
              DescribedClass* (*keyvalctor)(KeyVal&)=0,
              DescribedClass* (*stateinctor)(StateIn&)=0);
    ~ClassDesc();
    static void list_all_classes();
    static const ClassDesc* name_to_class_desc(const char*);
    const ParentClasses& parents() const;
    const char* name() const;
    int version() const;
    DescribedClass* create_described_class() const;

    // create an object using the default constructor
    DescribedClass* create() const;

    // create an object using the keyval constructor
    DescribedClass* create(KeyVal&) const;

    // create an object using the statein constructor
    DescribedClass* create(StateIn&) const;

};

ARRAY_dec(ClassDescP);
SET_dec(ClassDescP);
ARRAYSET_dec(ClassDescP);

// This makes info about the class available.
class DescribedClass : public VRefCount {
  private:
    static ClassDesc class_desc_;
  public:
    DescribedClass();
    DescribedClass(const DescribedClass&);
    DescribedClass& operator=(const DescribedClass&);
    static DescribedClass* castdown(DescribedClass*);
    static const ClassDesc* static_class_desc();
    virtual ~DescribedClass();
    virtual const ClassDesc* class_desc() const;
    virtual void* _castdown(const ClassDesc*);
    const char* class_name() const;
    int class_version() const;
  };

class  RefDescribedClassBase {
  public:
    RefDescribedClassBase();
    RefDescribedClassBase(const RefDescribedClassBase&);
    RefDescribedClassBase& operator=(const RefDescribedClassBase&);
    virtual DescribedClass* parentpointer() = 0;
    virtual ~RefDescribedClassBase ();
    int operator==( RefDescribedClassBase &a);
    int operator>=( RefDescribedClassBase &a);
    int operator<=( RefDescribedClassBase &a);
    int operator>( RefDescribedClassBase &a);
    int operator<( RefDescribedClassBase &a);
};
#define DescribedClass_REF_dec(T)					      \
class  Ref ## T : public RefDescribedClassBase  {			      \
  private:								      \
    T* p;								      \
  public:								      \
    DescribedClass* parentpointer();					      \
    T* operator->();							      \
    const T* operator->() const;					      \
    T* pointer();							      \
    const T* pointer() const;						      \
    operator T*();							      \
    const operator T*() const;						      \
    T& operator *();							      \
    const T& operator *() const;					      \
    Ref ## T ();							      \
    Ref ## T (T*a);							      \
    Ref ## T ( Ref ## T &a);						      \
    Ref ## T ( RefDescribedClassBase &);				      \
    ~Ref ## T ();							      \
    int null();								      \
    int nonnull();							      \
    Ref ## T  operator=(T* cr);						      \
    Ref ## T  operator=( RefDescribedClassBase & c);			      \
    Ref ## T  operator=( Ref ## T & c);					      \
    void assign_pointer(T* cr);						      \
    void  ref_info(FILE*fp=stdout);					      \
    void warn(const char *);						      \
    void clear();							      \
    void check_pointer();						      \
}
#define DescribedClass_REF_def(T)					      \
T* Ref ## T :: operator->() { return p; };				      \
const T* Ref ## T :: operator->() const { return p; };			      \
T* Ref ## T :: pointer() { return p; };					      \
const T* Ref ## T :: pointer() const { return p; };			      \
Ref ## T :: operator T*() { return p; };				      \
Ref ## T :: const operator T*() const { return p; };			      \
T& Ref ## T :: operator *() { return *p; };				      \
const T& Ref ## T :: operator *() const { return *p; };			      \
int Ref ## T :: null() { return p == 0; };				      \
int Ref ## T :: nonnull() { return p != 0; };				      \
DescribedClass* Ref ## T :: parentpointer() { return p; }		      \
Ref ## T :: Ref ## T (): p(0) {}					      \
Ref ## T :: Ref ## T (T*a): p(a)					      \
{									      \
  if (REF_CHECK_STACK && (void*) p > sbrk(0)) {				      \
      warn("Ref" # T ": creating a reference to stack data");		      \
    }									      \
  if (p) p->reference();						      \
  if (REF_CHECK_POINTER) check_pointer();				      \
}									      \
Ref ## T :: Ref ## T ( Ref ## T &a): p(a.p)				      \
{									      \
  if (p) p->reference();						      \
  if (REF_CHECK_POINTER) check_pointer();				      \
}									      \
Ref ## T :: Ref ## T ( RefDescribedClassBase &a)			      \
{									      \
  p = T::castdown(a.parentpointer());					      \
  if (p) p->reference();						      \
  if (REF_CHECK_POINTER) check_pointer();				      \
}									      \
Ref ## T :: ~Ref ## T ()						      \
{									      \
  clear();								      \
}									      \
void									      \
Ref ## T :: clear()							      \
{									      \
  if (REF_CHECK_POINTER) check_pointer();				      \
  if (p && p->dereference()<=0) {					      \
      if (REF_CHECK_STACK && (void*) p > sbrk(0)) {			      \
          warn("Ref" # T ": skipping delete of object on the stack");	      \
        }								      \
      else {								      \
           delete p;							      \
         }								      \
    }									      \
  p = 0;								      \
}									      \
void									      \
Ref ## T :: warn ( const char * msg)					      \
{									      \
  fprintf(stderr,"WARNING: %s\n",msg);					      \
}									      \
Ref ## T  Ref ## T :: operator=( Ref ## T & c)				      \
{									      \
  if (c.p) c.p->reference();						      \
  clear();								      \
  p=c.p;								      \
  if (REF_CHECK_POINTER) check_pointer();				      \
  return *this;								      \
}									      \
Ref ## T  Ref ## T :: operator=(T* cr)					      \
{									      \
  if (cr) cr->reference();						      \
  clear();								      \
  p = cr;								      \
  if (REF_CHECK_POINTER) check_pointer();				      \
  return *this;								      \
}									      \
Ref ## T  Ref ## T :: operator=( RefDescribedClassBase & c)		      \
{									      \
  T* cr = T::castdown(c.parentpointer());				      \
  if (cr) cr->reference();						      \
  clear();								      \
  p = cr;								      \
  if (REF_CHECK_POINTER) check_pointer();				      \
  return *this;								      \
}									      \
void Ref ## T :: assign_pointer(T* cr)					      \
{									      \
  if (cr) cr->reference();						      \
  clear();								      \
  p = cr;								      \
  if (REF_CHECK_POINTER) check_pointer();				      \
}									      \
void Ref ## T :: check_pointer()					      \
{									      \
  if (p && p->nreference() <= 0) {					      \
      warn("Ref" # T ": bad reference count in referenced object\n");	      \
    }									      \
}									      \
void Ref ## T :: ref_info(FILE*fp=stdout)				      \
{									      \
  if (nonnull()) fprintf(fp,"nreference() = %d\n",p->nreference());	      \
  else fprintf(fp,"reference is null\n");				      \
}


DescribedClass_REF_dec(DescribedClass);
ARRAY_dec(RefDescribedClass);
SET_dec(RefDescribedClass);
ARRAYSET_dec(RefDescribedClass);

#endif

