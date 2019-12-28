#include "tiger/frame/frame.h"

#include <string>

namespace F {

const int wordsize = 8;

X64Frame::X64Frame(TEMP::Label *name, U::BoolList *escapes) : Frame(name, escapes)
{
  // this->formals = new AccessList(NULL, NULL);
  // AccessList *ftail = formals;
  // this->view_shift = new T::StmList(NULL, NULL);
  // T::StmList *vtail = view_shift;

  this->s_offset = -8;
  // int formal_offset = wordsize;
  // int num = 1;
  // for(; escapes; escapes = escapes->tail, num++){
  //   if(escapes->head){
  //     if(ARG_nth(num)){
  //       ftail->tail = new AccessList(new InFrameAccess(s_offset), NULL);
  //       vtail->tail = new T::StmList(new T::MoveStm(T::NewMemPlus_Const(new T::TempExp(FP()), s_offset), new T::TempExp(ARG_nth(num))), NULL);
  //       s_offset -= wordsize;
  //       ftail = ftail->tail;
  //       vtail = vtail->tail;
  //     } else {
  //       ftail->tail = new AccessList(new InFrameAccess(formal_offset), NULL);
  //       formal_offset += wordsize;
  //       ftail = ftail->tail;
  //     }
  //   } else {
  //     TEMP::Temp *temp = TEMP::Temp::NewTemp();
  //     if(ARG_nth(num))
  //       view_shift = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(ARG_nth(num))), view_shift);
  //     else
  //       printf("Frame: the 7-nth formal should be passed on frame.");
  //     formals = new AccessList(new InRegAccess(temp), formals);
  //   }
  // }
  
  // formals = formals->tail;
  // view_shift = view_shift->tail;
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
  std::string prolog = frame->label->Name();
  prolog.append(std::string(":\n.set "));
  prolog.append(frame->label->Name());
  prolog.append(std::string("_framesize,$0x10000\n"   \
                            "\tpushq %rcx\n"          \
                            "\tpushq %rbp\n"          \
                            "\tmovq %rsp, %rbp\n"     \
                            "\tsubq $0x10000, %rsp\n"));
  std::string epilog =  "\taddq $0x10000,%rsp\n"  \
                        "\tpopq %rbp\n"           \
                        "\tpopq %rcx\n"           \
                        "\tret\n";
  return new AS::Proc(prolog, body, epilog);
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

TEMP::Temp *ARG_nth(int num)
{
  switch (num) {
    case 1: return F::RDI(); 
    case 2: return F::RSI(); 
    case 3: return F::RDX(); 
    case 4: return F::RCX(); 
    case 5: return F::R8();
    case 6: return F::R9();
    default: return NULL;
  }
}

TEMP::Map *RegMap()
{
  static TEMP::Map *regmap = NULL;
  if(regmap)
    return regmap;
  regmap = TEMP::Map::Empty();
	regmap->Enter(F::RAX(), new std::string("%rax"));
	regmap->Enter(F::RDI(), new std::string("%rdi"));
	regmap->Enter(F::RSI(), new std::string("%rsi"));
	regmap->Enter(F::RDX(), new std::string("%rdx"));
	regmap->Enter(F::RCX(), new std::string("%rcx"));
	regmap->Enter(F::R8(), new std::string("%r8"));
	regmap->Enter(F::R9(), new std::string("%r9"));
	regmap->Enter(F::R10(), new std::string("%r10"));
	regmap->Enter(F::R11(), new std::string("%r11"));
	regmap->Enter(F::RBX(), new std::string("%rbx"));
	regmap->Enter(F::RBP(), new std::string("%rbp"));
	regmap->Enter(F::R12(), new std::string("%r12"));
	regmap->Enter(F::R13(), new std::string("%r13"));
	regmap->Enter(F::R14(), new std::string("%r14"));
	regmap->Enter(F::R15(), new std::string("%r15"));
	regmap->Enter(F::SP(), new std::string("%rsp"));
  return regmap;
}

TEMP::TempList *AllRegs()
{
  static TEMP::TempList *templist = 
    new TEMP::TempList(SP(), AllRegs_noRSP());
  return templist;
}

TEMP::TempList *AllRegs_noRSP()
{
  static TEMP::TempList *templist = 
    new TEMP::TempList(RAX(),
    new TEMP::TempList(RDI(),
    new TEMP::TempList(RSI(),
    new TEMP::TempList(RDX(),
    new TEMP::TempList(RCX(),
    new TEMP::TempList(R8(),
    new TEMP::TempList(R9(),
    new TEMP::TempList(R10(),
    new TEMP::TempList(R11(),
    new TEMP::TempList(RBX(),
    new TEMP::TempList(RBP(),
    new TEMP::TempList(R12(),
    new TEMP::TempList(R13(),
    new TEMP::TempList(R14(),
    new TEMP::TempList(R15(), NULL)))))))))))))));
  return templist;
}
}  // namespace F
