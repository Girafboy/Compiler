#include "tiger/frame/frame.h"

#include <string>

namespace F {

const int wordsize = 8;

class InFrameAccess : public Access {
 public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {assert(offset < 0);}
  T::Exp *ToExp(T::Exp *framePtr) const
  {
    return new T::MemExp(new T::BinopExp(T::BinOp::PLUS_OP, framePtr, new T::ConstExp(offset)));
  }
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
  T::Exp *ToExp(T::Exp *framePtr) const
  {
    return new T::TempExp(reg);
  }
};

X64Frame::X64Frame(TEMP::Label *name, U::BoolList *escapes) : Frame(name, escapes)
{
  AccessList *formals = new AccessList(NULL, NULL);
  AccessList *ftail = formals;
  T::StmList *view_shift = new T::StmList(NULL, NULL);
  T::StmList *vtail = view_shift;

  int frame_offset = -8;
  int formal_offset = wordsize;
  int num = 1;
  for(; escapes; escapes = escapes->tail, num++){
    if(escapes->head){
      switch(num){
        case 1:
          vtail->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::ConstExp(-num*wordsize)), new T::TempExp(RDI())), NULL);
          ftail->tail = new AccessList(new InFrameAccess(-num*wordsize), NULL);
          frame_offset -= wordsize;
          ftail = ftail->tail;
          vtail = vtail->tail;
          break;
        case 2:
          vtail->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::ConstExp(-num*wordsize)), new T::TempExp(RSI())), NULL);
          ftail->tail = new AccessList(new InFrameAccess(-num*wordsize), NULL);
          frame_offset -= wordsize;
          ftail = ftail->tail;
          vtail = vtail->tail;
          break;
        case 3:
          vtail->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::ConstExp(-num*wordsize)), new T::TempExp(RDX())), NULL);
          ftail->tail = new AccessList(new InFrameAccess(-num*wordsize), NULL);
          frame_offset -= wordsize;
          ftail = ftail->tail;
          vtail = vtail->tail;
          break;
        case 4:
          vtail->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::ConstExp(-num*wordsize)), new T::TempExp(RCX())), NULL);
          ftail->tail = new AccessList(new InFrameAccess(-num*wordsize), NULL);
          frame_offset -= wordsize;
          ftail = ftail->tail;
          vtail = vtail->tail;
          break;
        case 5:
          vtail->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::ConstExp(-num*wordsize)), new T::TempExp(R8())), NULL);
          ftail->tail = new AccessList(new InFrameAccess(-num*wordsize), NULL);
          frame_offset -= wordsize;
          ftail = ftail->tail;
          vtail = vtail->tail;
          break;
        case 6:
          vtail->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::ConstExp(-num*wordsize)), new T::TempExp(R9())), NULL);
          ftail->tail = new AccessList(new InFrameAccess(-num*wordsize), NULL);
          frame_offset -= wordsize;
          ftail = ftail->tail;
          vtail = vtail->tail;
          break;
        default:
          ftail->tail = new AccessList(new InFrameAccess(formal_offset), NULL);
          formal_offset += wordsize;
          ftail = ftail->tail;
          break;
      }
    } else {
      TEMP::Temp *temp = TEMP::Temp::NewTemp();
      switch(num){
        case 1:
          view_shift = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(RDI())), view_shift);
          break;
        case 2:
          view_shift = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(RSI())), view_shift);
          break;
        case 3:
          view_shift = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(RDX())), view_shift);
          break;
        case 4:
          view_shift = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(RCX())), view_shift);
          break;
        case 5:
          view_shift = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(R8())), view_shift);
          break;
        case 6:
          view_shift = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(R9())), view_shift);
          break;
        default:
          printf("Frame: the 7-nth formal should be passed on frame.");
      }
      formals = new AccessList(new InRegAccess(temp), formals);
    }
  }
  
  formals = formals->tail;
  view_shift = view_shift->tail;
  
  this->s_offset = frame_offset;
  this->formals = formals;
  this->locals = NULL;
  this->view_shift = view_shift;
}

Access *X64Frame::allocLocal(bool escape)
{
  Access *local;
  if(escape){
    local = new InFrameAccess(s_offset);
    s_offset -= wordsize;
  }else{
    local = new InRegAccess(TEMP::Temp::NewTemp());
  }
  return local;
}

T::Exp *externalCall(std::string s, T::ExpList *args)
{
  return new T::CallExp(new T::NameExp(TEMP::NamedLabel(s)), args);
}

T::Stm *F_procEntryExit1(Frame *frame, T::Stm *stm){
	T::StmList *slist = frame->view_shift;
	T::Stm *bind = NULL;
	for(; slist; slist = slist->tail){
		if(bind)
			bind = new T::SeqStm(bind,slist->head);
		else
			bind = slist->head;
	}
	if(bind)
		bind = new T::SeqStm(bind,stm);
	else
		bind = stm;
	return bind;
}

AS::InstrList *F_procEntryExit2(AS::InstrList *body)
{
	static TEMP::TempList *retlist = NULL;
	if(!retlist)
		retlist = new TEMP::TempList(rsp, new TEMP::TempList(RV(),NULL));
	return AS::InstrList::Splice(body,new AS::InstrList(new AS::OperInstr("", NULL, retlist, new AS::Targets(NULL)), NULL));
}

AS::Proc * F_procEntryExit3(F::Frame * frame, AS::InstrList * body){
  // fix size! TODO:change fixsize 0x10000
  std::string prolog = "pushq %rbp\nmovq %rsp, %rbp\nsubq $0x10000,%rsp\n";
  std::string epilog = "\taddq $0x10000,%rsp\n\tpopq %rbp\n\tpopq %r15\n\tret\n";
  return new AS::Proc("",body,epilog);
}

TEMP::Temp *FP()
{
  if(!rbp)
    rbp = TEMP::Temp::NewTemp();
  return rbp;
}
TEMP::Temp *RV()
{
  if(!rax)
    rax = TEMP::Temp::NewTemp();
  return rax;
}
TEMP::Temp *RAX()
{
  if(!rax)
    rax = TEMP::Temp::NewTemp();
  return rax;
}
TEMP::Temp *RDI()
{
  if(!rdi)
    rdi = TEMP::Temp::NewTemp();
  return rdi;
}
TEMP::Temp *RSI()
{
  if(!rsi)
    rsi = TEMP::Temp::NewTemp();
  return rsi;
}
TEMP::Temp *RDX()
{
  if(!rdx)
    rdx = TEMP::Temp::NewTemp();
  return rdx;
}
TEMP::Temp *RCX()
{
  if(!rcx)
    rcx = TEMP::Temp::NewTemp();
  return rcx;
}
TEMP::Temp *R8()
{
  if(!r8)
    r8 = TEMP::Temp::NewTemp();
  return r8;
}
TEMP::Temp *R9()
{
  if(!r9)
    r9 = TEMP::Temp::NewTemp();
  return r9;
}
TEMP::Temp *R10()
{
  if(!r10)
    r10 = TEMP::Temp::NewTemp();
  return r10;
}
TEMP::Temp *R11()
{
  if(!r11)
    r11 = TEMP::Temp::NewTemp();
  return r11;
}
TEMP::Temp *RBX()
{
  if(!rbx)
    rbx = TEMP::Temp::NewTemp();
  return rbx;
}
TEMP::Temp *RBP()
{
  if(!rbp)
    rbp = TEMP::Temp::NewTemp();
  return rbp;
}
TEMP::Temp *R12()
{
  if(!r12)
    r12 = TEMP::Temp::NewTemp();
  return r12;
}
TEMP::Temp *R13()
{
  if(!r13)
    r13 = TEMP::Temp::NewTemp();
  return r13;
}
TEMP::Temp *R14()
{
  if(!r14)
    r14 = TEMP::Temp::NewTemp();
  return r14;
}
TEMP::Temp *R15()
{
  if(!r15)
    r15 = TEMP::Temp::NewTemp();
  return r15;
}
TEMP::Temp *SP()
{
  if(!rsp)
    rsp = TEMP::Temp::NewTemp();
  return rsp;
}

}  // namespace F
