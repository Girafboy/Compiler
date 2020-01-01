#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/translate/tree.h"
#include "tiger/util/util.h"

namespace AS {
class InstrList;
class Proc;
}

namespace F {

extern const int wordsize;

class Access;
class AccessList;

class Frame {
  // Base class
 public:
  TEMP::Label *label;
  AccessList *formals;
  int s_offset;

  Frame(TEMP::Label *name, U::BoolList *escapes) : label(name) {}
  virtual Access *allocLocal(bool escape) = 0;
};

class X64Frame : public Frame {
  // TODO: Put your codes here (lab6).
 public:
  X64Frame(TEMP::Label *name, U::BoolList *escapes);
  Access *allocLocal(bool escape);
};

class Access {
 public:
  enum Kind { INFRAME, INREG };

  Kind kind;

  Access(Kind kind) : kind(kind) {}

  // Hints: You may add interface like
  //        `virtual T::Exp* ToExp(T::Exp* framePtr) const = 0`
  virtual T::Exp *ToExp(T::Exp *framePtr) const = 0;
};

class InFrameAccess : public Access {
 public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {assert(offset < 0);}
  T::Exp *ToExp(T::Exp *framePtr) const { return T::NewMemPlus_Const(framePtr, offset); }
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
  T::Exp *ToExp(T::Exp *framePtr) const { return new T::TempExp(reg); }
};

class AccessList {
 public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

/*
 * Fragments
 */

class Frag {
 public:
  enum Kind { STRING, PROC };

  Kind kind;

  Frag(Kind kind) : kind(kind) {}
};

class StringFrag : public Frag {
 public:
  TEMP::Label *label;
  std::string str;

  StringFrag(TEMP::Label *label, std::string str)
      : Frag(STRING), label(label), str(str) {}
};

class ProcFrag : public Frag {
 public:
  T::Stm *body;
  Frame *frame;

  ProcFrag(T::Stm *body, Frame *frame) : Frag(PROC), body(body), frame(frame) {}
};

class FragList {
 public:
  Frag *head;
  FragList *tail;

  FragList(Frag *head, FragList *tail) : head(head), tail(tail) {}
};

T::Exp *externalCall(std::string s, T::ExpList *args);
T::Stm *F_procEntryExit1(Frame *frame, T::Stm* stm);
AS::InstrList *F_procEntryExit2(AS::InstrList *ilist);
AS::Proc *F_procEntryExit3(Frame *frame, AS::InstrList *ilist);

static TEMP::Temp *rax = NULL;
static TEMP::Temp *rdi = NULL;
static TEMP::Temp *rsi = NULL;
static TEMP::Temp *rdx = NULL;
static TEMP::Temp *rcx = NULL;
static TEMP::Temp *r8 = NULL;
static TEMP::Temp *r9 = NULL;
static TEMP::Temp *r10 = NULL;
static TEMP::Temp *r11 = NULL;

static TEMP::Temp *rbx = NULL;
static TEMP::Temp *rbp = NULL;
static TEMP::Temp *r12 = NULL;
static TEMP::Temp *r13 = NULL;
static TEMP::Temp *r14 = NULL;
static TEMP::Temp *r15 = NULL;

static TEMP::Temp *fp = NULL;
static TEMP::Temp *rsp = NULL;

TEMP::Temp *RAX();
TEMP::Temp *RDI();
TEMP::Temp *RSI();
TEMP::Temp *RDX();
TEMP::Temp *RCX();
TEMP::Temp *R8();
TEMP::Temp *R9();
TEMP::Temp *R10();
TEMP::Temp *R11();

TEMP::Temp *RBX();
TEMP::Temp *RBP();
TEMP::Temp *R12();
TEMP::Temp *R13();
TEMP::Temp *R14();
TEMP::Temp *R15();

TEMP::Temp *SP();

TEMP::Temp *FP();
TEMP::Temp *RV();

TEMP::Temp *ARG_nth(int num);
TEMP::Map *RegMap();
TEMP::TempList *AllRegs();
TEMP::TempList *AllRegs_noRSP();
TEMP::TempList *CallerSavedRegs();
TEMP::TempList *CalleeSavedRegs();

}  // namespace F

#endif