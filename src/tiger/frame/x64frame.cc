#include "tiger/frame/frame.h"

#include <string>

namespace F {

const int wordsize = 8;

X64Frame::X64Frame(TEMP::Label *name, U::BoolList *escapes) : Frame(name, escapes)
{
  this->s_offset = -8;
  this->formals = new AccessList(NULL, NULL);
  AccessList *ftail = this->formals;

  int num = 1;
  for(; escapes; escapes = escapes->tail, num++){
    ftail->tail = new AccessList(allocLocal(escapes->head), NULL);
    ftail = ftail->tail;
  }
  
  this->formals = this->formals->tail;
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

  int num = 1;
  T::Stm *viewshift = new T::ExpStm(new T::ConstExp(0));
  for(AccessList *formals = frame->formals; formals; formals = formals->tail, num++)
    if (F::ARG_nth(num))
      viewshift = new T::SeqStm(viewshift, new T::MoveStm(formals->head->ToExp(new T::TempExp(F::FP())), new T::TempExp(F::ARG_nth(num))));

	return new T::SeqStm(viewshift, stm);
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
  static char instr[256];

  std::string prolog;
  sprintf(instr, ".set %s_framesize, %d\n", frame->label->Name().c_str(), -frame->s_offset);
  prolog = std::string(instr);
  sprintf(instr, "%s:\n", frame->label->Name().c_str());
  prolog.append(std::string(instr));
  sprintf(instr, "\tsubq $%s_framesize, %%rsp\n", frame->label->Name().c_str());
  prolog.append(std::string(instr));

  sprintf(instr, "\taddq $%s_framesize, %%rsp\n", frame->label->Name().c_str());
  std::string epilog = std::string(instr);
  epilog.append(std::string("\tret\n"));
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

TEMP::TempList *CallerSavedRegs()
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
    new TEMP::TempList(R11(), NULL)))))))));
  return templist;
}

TEMP::TempList *CalleeSavedRegs()
{
  static TEMP::TempList *templist = 
    new TEMP::TempList(RBX(),
    new TEMP::TempList(RBP(),
    new TEMP::TempList(R12(),
    new TEMP::TempList(R13(),
    new TEMP::TempList(R14(),
    new TEMP::TempList(R15(), NULL))))));
  return templist;
}

}  // namespace F
