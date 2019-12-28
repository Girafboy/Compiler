#include "tiger/codegen/codegen.h"

namespace CG {

static char framesize[256];

static TEMP::Temp *saved_rbx = NULL;
static TEMP::Temp *saved_rbp = NULL;
static TEMP::Temp *saved_r12 = NULL;
static TEMP::Temp *saved_r13 = NULL;
static TEMP::Temp *saved_r14 = NULL;
static TEMP::Temp *saved_r15 = NULL;

AS::InstrList* Codegen(F::Frame* f, T::StmList* stmList) 
{
  // TODO: Put your codes here (lab6).
  AS::InstrList *i_list;
  
  saveCalleeRegs();
  sprintf(framesize, "%s_framesize", f->label->Name().c_str());
  for(T::StmList *s = stmList; s; s = s->tail)
    munchStm(s->head);
  restoreCalleeRegs();
  
  i_list = instr_list;
  instr_list = NULL;

  return F::F_procEntryExit2(i_list);
}

static void saveCalleeRegs()
{
  saved_rbx = TEMP::Temp::NewTemp();
  saved_rbp = TEMP::Temp::NewTemp();
  saved_r12 = TEMP::Temp::NewTemp();
  saved_r13 = TEMP::Temp::NewTemp();
  saved_r14 = TEMP::Temp::NewTemp();
  saved_r15 = TEMP::Temp::NewTemp();
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(saved_rbx, NULL), new TEMP::TempList(F::RBX(), NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(saved_rbp, NULL), new TEMP::TempList(F::RBP(), NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(saved_r12, NULL), new TEMP::TempList(F::R12(), NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(saved_r13, NULL), new TEMP::TempList(F::R13(), NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(saved_r14, NULL), new TEMP::TempList(F::R14(), NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(saved_r15, NULL), new TEMP::TempList(F::R15(), NULL)));
}

static void restoreCalleeRegs()
{
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::RBX(), NULL), new TEMP::TempList(saved_rbx, NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::RBP(), NULL), new TEMP::TempList(saved_rbp, NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::R12(), NULL), new TEMP::TempList(saved_r12, NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::R13(), NULL), new TEMP::TempList(saved_r13, NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::R14(), NULL), new TEMP::TempList(saved_r14, NULL)));
  emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::R15(), NULL), new TEMP::TempList(saved_r15, NULL)));
}

void emit(AS::Instr * instr){
  if(instr_list)
    instr_list = AS::InstrList::Splice(instr_list, new AS::InstrList(instr,NULL));
  else
    instr_list = new AS::InstrList(instr, NULL);
}

void munchStm(T::Stm *stm)
{
  switch(stm->kind){
    case T::Stm::Kind::MOVE:
    {
      T::MoveStm *s = (T::MoveStm *)stm;
      if(s->dst->kind == T::Exp::Kind::TEMP){
        TEMP::Temp *left = munchExp(s->src);
        emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(((T::TempExp *)s->dst)->temp, NULL), new TEMP::TempList(left, NULL)));
        return;
      }
      if(s->dst->kind == T::Exp::Kind::MEM){
        TEMP::Temp *left = munchExp(s->src);
        TEMP::Temp *right = munchExp(((T::MemExp *)s->dst)->exp);
        emit(new AS::OperInstr("movq `s0, (`s1)", NULL, new TEMP::TempList(left, new TEMP::TempList(right, NULL)), NULL));
        return;
      }
      break;
    }
    case T::Stm::Kind::LABEL:
    {
      TEMP::Label *label = ((T::LabelStm *)stm)->label;
      emit(new AS::LabelInstr(TEMP::LabelString(label), label));
      return;
    }
    case T::Stm::Kind::JUMP:
    {
      TEMP::Label *label = ((T::JumpStm *)stm)->exp->name;
      emit(new AS::OperInstr(std::string("jmp ").append(TEMP::LabelString(label)), NULL, NULL, new AS::Targets(((T::JumpStm *)stm)->jumps)));
      return;
    }
    case T::Stm::Kind::CJUMP:
    {
      T::CjumpStm *s = (T::CjumpStm *)stm;
      TEMP::Temp *left = munchExp(s->left);
      TEMP::Temp *right = munchExp(s->right);
      emit(new AS::OperInstr("cmp `s0, `s1", NULL, new TEMP::TempList(right, new TEMP::TempList(left, NULL)), NULL));

      std::string str;
      switch(s->op){
        case T::RelOp::EQ_OP: str = std::string("je ");  break;
        case T::RelOp::NE_OP: str = std::string("jne "); break;
        case T::RelOp::LT_OP: str = std::string("jl ");  break;
        case T::RelOp::GT_OP: str = std::string("jg ");  break;
        case T::RelOp::LE_OP: str = std::string("jle "); break;
        case T::RelOp::GE_OP: str = std::string("jge "); break;
      }
      emit(new AS::OperInstr(str.append(TEMP::LabelString(s->true_label)), NULL, NULL, new AS::Targets(new TEMP::LabelList(s->true_label, NULL))));
      return;
    }
    case T::Stm::Kind::EXP:
    {
      T::ExpStm *s = (T::ExpStm *)stm;
      munchExp(s->exp);
      return;
    }
  }
}

TEMP::Temp *munchExp(T::Exp *exp)
{
  static char temp[256];
  TEMP::Temp *reg = TEMP::Temp::NewTemp();
  switch (exp->kind)
  {
  case T::Exp::BINOP:
  {
    T::BinopExp *e = (T::BinopExp *)exp;
    switch (e->op)
    {
    case T::PLUS_OP:
    {
      TEMP::Temp *left = munchExp(e->left);
      TEMP::Temp *right = munchExp(e->right);
      emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(left, NULL)));
      emit(new AS::OperInstr("addq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(right, new TEMP::TempList(reg, NULL)), NULL));
      return reg;
    }
    case T::MINUS_OP:
    {
      TEMP::Temp *left = munchExp(e->left);
      TEMP::Temp *right = munchExp(e->right);
      emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(left, NULL)));
      emit(new AS::OperInstr("subq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(right, new TEMP::TempList(reg, NULL)), NULL));
      return reg;
    }
    case T::MUL_OP:
    {
      TEMP::Temp *left = munchExp(e->left);
      TEMP::Temp *right = munchExp(e->right);
      emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(left, NULL)));
      emit(new AS::OperInstr("imulq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(right, new TEMP::TempList(reg, NULL)), NULL));
      return reg;
    }
    case T::DIV_OP:
    {
      TEMP::Temp *left = munchExp(e->left);
      TEMP::Temp *right = munchExp(e->right);
      emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::RAX(), NULL), new TEMP::TempList(left, NULL)));
      emit(new AS::OperInstr("cltd", new TEMP::TempList(F::RAX(), 
        new TEMP::TempList(F::RDX(), NULL)), 
        new TEMP::TempList(F::RAX(), NULL), NULL));
      emit(new AS::OperInstr("idivq `s0", new TEMP::TempList(F::RAX(), 
        new TEMP::TempList(F::RDX(), NULL)), 
        new TEMP::TempList(right, 
        new TEMP::TempList(F::RAX(), 
        new TEMP::TempList(F::RDX(), NULL))), NULL));
      emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(F::RAX(), NULL)));
      return reg;
    }
    }
    break;
  }
  case T::Exp::MEM:
  {
    T::MemExp *e = (T::MemExp *)exp;

    TEMP::Temp *r = munchExp(e->exp);
    emit(new AS::OperInstr("movq (`s0), `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(r, NULL), NULL));
    return reg;
  }
  case T::Exp::TEMP:
    return ((T::TempExp *)exp)->temp;
  case T::Exp::ESEQ:
    assert(((T::EseqExp *)exp)->stm && ((T::EseqExp *)exp)->exp);
    munchStm(((T::EseqExp *)exp)->stm);
    return munchExp(((T::EseqExp *)exp)->exp);
  case T::Exp::NAME:
    sprintf(temp, "leaq %s(%%rip), `d0 ", ((T::NameExp *)exp)->name->Name().c_str());
    emit(new AS::MoveInstr(temp, new TEMP::TempList(reg, NULL), NULL));
    return reg;
  case T::Exp::CONST:
  {
    T::ConstExp *e = (T::ConstExp *)exp;
    sprintf(temp, "movq $%d, `d0", e->consti);
    emit(new AS::OperInstr(std::string(temp), new TEMP::TempList(reg, NULL), NULL, NULL));
    return reg;
  }
  case T::Exp::CALL:
  {
    T::CallExp *e = (T::CallExp *)exp;
    munchArgs(e->args);
    sprintf(temp, "call %s", TEMP::LabelString(((T::NameExp *)e->fun)->name).c_str());
    emit(new AS::OperInstr(std::string(temp), 
      new TEMP::TempList(F::RAX(), 
      new TEMP::TempList(F::RDI(), 
      new TEMP::TempList(F::RSI(), 
      new TEMP::TempList(F::RDX(), 
      new TEMP::TempList(F::RCX(), 
      new TEMP::TempList(F::R8(), 
      new TEMP::TempList(F::R9(), 
      new TEMP::TempList(F::R10(), 
      new TEMP::TempList(F::R11(), NULL))))))))), NULL, NULL));
    emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(reg, NULL), new TEMP::TempList(F::RAX(), NULL)));
    return reg;
  }
  }
}

void munchArgs(T::ExpList *list)
{
  for(int num = 1; list; num++, list = list->tail){
    TEMP::Temp *arg = munchExp(list->head);
    if(F::ARG_nth(num)){
      emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::ARG_nth(num), NULL), new TEMP::TempList(arg, NULL)));
    } else
      emit(new AS::OperInstr("pushq `s0", NULL, new TEMP::TempList(arg, NULL), NULL));
  }

  return;
}

}  // namespace CG
