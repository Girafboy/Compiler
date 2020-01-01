#include "tiger/translate/translate.h"

#include <cstdio>
#include <set>
#include <string>

#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/semant.h"
#include "tiger/semant/types.h"
#include "tiger/util/util.h"

#define LOG(format, args...) do{            \
  FILE *debug_log = fopen("translate.log", "a+"); \
  fprintf(debug_log, "%d,%s: ", __LINE__, __func__); \
  fprintf(debug_log, format, ##args);       \
  fclose(debug_log);\
} while(0)

extern EM::ErrorMsg errormsg;

namespace TR {

class Access {
 public:
  Level *level;
  F::Access *access;

  Access(Level *level, F::Access *access) : level(level), access(access) {}
  static Access *AllocLocal(Level *level, bool escape); 
};

class AccessList {
 public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

class Level {
 public:
  F::Frame *frame;
  Level *parent;

  Level(F::Frame *frame, Level *parent) : frame(frame), parent(parent) {}
  AccessList *Formals(Level *level) { return NULL; }

  static Level *NewLevel(Level *parent, TEMP::Label *name,
                         U::BoolList *formals);
};

class PatchList {
 public:
  TEMP::Label **head;
  PatchList *tail;

  PatchList(TEMP::Label **head, PatchList *tail) : head(head), tail(tail) {}
};

class Cx {
 public:
  PatchList *trues;
  PatchList *falses;
  T::Stm *stm;

  Cx(PatchList *trues, PatchList *falses, T::Stm *stm)
      : trues(trues), falses(falses), stm(stm) {}
};

class Exp {
 public:
  enum Kind { EX, NX, CX };

  Kind kind;

  Exp(Kind kind) : kind(kind) {}

  virtual T::Exp *UnEx() const = 0;
  virtual T::Stm *UnNx() const = 0;
  virtual Cx UnCx() const = 0;
};

class ExpAndTy {
 public:
  TR::Exp *exp;
  TY::Ty *ty;

  ExpAndTy(TR::Exp *exp, TY::Ty *ty) : exp(exp), ty(ty) {}
};

class ExExp : public Exp {
 public:
  T::Exp *exp;

  ExExp(T::Exp *exp) : Exp(EX), exp(exp) {}

  T::Exp *UnEx() const override;
  T::Stm *UnNx() const override;
  Cx UnCx() const override;
};

class NxExp : public Exp {
 public:
  T::Stm *stm;

  NxExp(T::Stm *stm) : Exp(NX), stm(stm) {}

  T::Exp *UnEx() const override;
  T::Stm *UnNx() const override;
  Cx UnCx() const override;
};

class CxExp : public Exp {
 public:
  Cx cx;

  CxExp(struct Cx cx) : Exp(CX), cx(cx) {}
  CxExp(PatchList *trues, PatchList *falses, T::Stm *stm)
      : Exp(CX), cx(trues, falses, stm) {}

  T::Exp *UnEx() const override;
  T::Stm *UnNx() const override;
  Cx UnCx() const override;
};

class ExpList {
 public:
  Exp *head;
  ExpList *tail;

  ExpList(Exp *head, ExpList *tail) : head(head), tail(tail) {}
};

void do_patch(PatchList *tList, TEMP::Label *label) {
  for (; tList && tList->head ; tList = tList->tail) *(tList->head) = label;
}

PatchList *join_patch(PatchList *first, PatchList *second) {
  if (!first) return second;
  for (; first->tail; first = first->tail)
    ;
  first->tail = second;
  return first;
}

Access *Access::AllocLocal(Level *level, bool escape)
{
  return new Access(level, level->frame->allocLocal(escape));
}

Level *Level::NewLevel(Level *parent, TEMP::Label *name, U::BoolList *formals)
{
  return new Level(new F::X64Frame(name, new U::BoolList(true, formals)), parent);
}

T::Exp *ExExp::UnEx() const
{
  return exp;
}

T::Stm *ExExp::UnNx() const
{
  return new T::ExpStm(exp);
}

Cx ExExp::UnCx() const
{
  T::CjumpStm *stm = new T::CjumpStm(T::RelOp::NE_OP, exp, new T::ConstExp(0), NULL, NULL);
  PatchList *trues = new PatchList(&(stm->true_label), NULL);
  PatchList *falses = new PatchList(&(stm->false_label), NULL);
  return Cx(trues, falses, stm);
}

T::Exp *NxExp::UnEx() const
{
  return new T::EseqExp(stm, new T::ConstExp(0));
}

T::Stm *NxExp::UnNx() const
{
  return stm;
}

Cx NxExp::UnCx() const
{
  printf("Error: nx can't be a test exp.");
}

T::Exp *CxExp::UnEx() const
{
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  TEMP::Label *t = TEMP::NewLabel(), *f = TEMP::NewLabel();
  do_patch(cx.trues, t);
  do_patch(cx.falses, f);
  return new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(1)),
    new T::EseqExp(cx.stm,
    new T::EseqExp(new T::LabelStm(f),
    new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(0)),
    new T::EseqExp(new T::LabelStm(t), new T::TempExp(r))))));
}

T::Stm *CxExp::UnNx() const
{
  TEMP::Label *label = TEMP::NewLabel();
  do_patch(cx.trues, label);
  do_patch(cx.falses, label);
  return new T::SeqStm(cx.stm, new T::LabelStm(label));
}

Cx CxExp::UnCx() const
{
  return cx;
}

Level *Outermost() 
{
  return new Level(new F::X64Frame(TEMP::NamedLabel("tigermain"), NULL), NULL);
}

static F::FragList * frags = new F::FragList(NULL, NULL);
static F::FragList * fragtail = frags;

F::FragList *TranslateProgram(A::Exp *root) 
{
  // TODO: Put your codes here (lab5).
  Level *mainframe = Outermost();
  TEMP::Label *mainlabel = TEMP::NamedLabel("tigermain");

  ExpAndTy mainexp = root->Translate(E::BaseVEnv(), E::BaseTEnv(), mainframe, mainlabel); 
  
  return frags->tail;
}

T::Exp *StaticLink(TR::Level *target, TR::Level *level)
{
  T::Exp *staticlink = new T::TempExp(F::FP());
  while(level != target){
    staticlink = level->frame->formals->head->ToExp(staticlink);
    level = level->parent;
  }
  return staticlink;
}

}  // namespace TR


namespace A {

TR::Exp *TranslateSimpleVar(TR::Access *access, TR::Level *level)
{
  T::Exp *staticlink = StaticLink(access->level, level);
  staticlink = access->access->ToExp(staticlink);
  return new TR::ExExp(staticlink);
}

TR::ExpAndTy SimpleVar::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  LOG("Translate SimpleVar level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::Exp *exp = NULL;
  TY::Ty *ty = TY::IntTy::Instance();

  E::EnvEntry *entry = venv->Look(sym);
  if(!entry || entry->kind != E::EnvEntry::VAR)
    errormsg.Error(pos, "undefined variable %s", sym->Name().c_str());

  E::VarEntry *var_entry = (E::VarEntry *)entry;
  exp = TranslateSimpleVar(var_entry->access, level);
  ty = var_entry->ty->ActualTy();
  
  return TR::ExpAndTy(exp, ty);
}

TR::ExpAndTy FieldVar::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  LOG("Translate FieldVar level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy check_var = var->Translate(venv, tenv, level, label);
  TY::Ty *actual_ty = check_var.ty->ActualTy();

  if(actual_ty->kind != TY::Ty::Kind::RECORD){
    errormsg.Error(pos, "not a record type");
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }else{
    TY::FieldList *fields = ((TY::RecordTy *)actual_ty)->fields;
    int order = 0;
    while(fields){
      TY::Field *field = fields->head;
      if(field->name == sym){
        if(check_var.exp->kind != TR::Exp::Kind::EX)
        	printf("Error: fieldVar's loc must be an expression");

        TR::Exp *exp = new TR::ExExp(T::NewMemPlus_Const(check_var.exp->UnEx(), order * F::wordsize));
        TY::Ty *ty = field->ty->ActualTy();
        return TR::ExpAndTy(exp, ty);
      }
      fields = fields->tail;
      order++;
    }

    errormsg.Error(pos, "field %s doesn't exist", sym->Name().c_str());
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }
}

TR::ExpAndTy SubscriptVar::Translate(S::Table<E::EnvEntry> *venv,
                                     S::Table<TY::Ty> *tenv, TR::Level *level,
                                     TEMP::Label *label) const {
  LOG("Translate SubscriptVar level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy check_var = var->Translate(venv, tenv, level, label);
  
  if(check_var.ty->ActualTy()->kind != TY::Ty::Kind::ARRAY){
    errormsg.Error(pos, "array type required");
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }

  TR::ExpAndTy check_subscript = subscript->Translate(venv, tenv, level, label);

  if(check_subscript.ty->ActualTy()->kind != TY::Ty::Kind::INT){
    errormsg.Error(pos, "array index must be integer");
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }

  if(check_var.exp->kind != TR::Exp::Kind::EX || check_subscript.exp->kind != TR::Exp::Kind::EX)
		printf("Error: subscriptVar's loc or subscript must be an expression");
  
  TR::Exp *exp = new TR::ExExp(new T::MemExp(new T::BinopExp(T::BinOp::PLUS_OP, check_var.exp->UnEx(), new T::BinopExp(T::BinOp::MUL_OP, check_subscript.exp->UnEx(), new T::ConstExp(F::wordsize)))));
  TY::Ty *ty = ((TY::ArrayTy *)check_var.ty)->ty->ActualTy();
  return TR::ExpAndTy(exp, ty);
}

TR::ExpAndTy VarExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  LOG("Translate VarExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  switch(var->kind){
    case Var::Kind::SIMPLE:
      return ((SimpleVar *)var)->Translate(venv, tenv, level, label);
    case Var::Kind::FIELD:
      return ((FieldVar *)var)->Translate(venv, tenv, level, label);
    case Var::Kind::SUBSCRIPT:
      return ((SubscriptVar *)var)->Translate(venv, tenv, level, label);
    default:
      assert(0);
  }
}

TR::Exp *TranslateNilExp()
{
  return new TR::ExExp(new T::ConstExp(0));
}

TR::ExpAndTy NilExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  LOG("Translate NilExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(TranslateNilExp(), TY::NilTy::Instance());
}

TR::ExpAndTy IntExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  LOG("Translate IntExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(new TR::ExExp(new T::ConstExp(i)), TY::IntTy::Instance());
}

TR::ExpAndTy StringExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  LOG("Translate StringExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TEMP::Label *string_label = TEMP::NewLabel();
  TR::fragtail->tail = new F::FragList(new F::StringFrag(string_label, s), NULL);
  TR::fragtail = TR::fragtail->tail;

  return TR::ExpAndTy(new TR::ExExp(new T::NameExp(string_label)), TY::StringTy::Instance());
}

TR::ExpAndTy CallExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  LOG("Translate CallExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::Exp *exp = NULL;
  TY::Ty *ty = TY::VoidTy::Instance();

  E::EnvEntry *entry = venv->Look(func);
  if(!entry || entry->kind != E::EnvEntry::Kind::FUN){
    errormsg.Error(pos, "undefined function %s", func->Name().c_str());
    return TR::ExpAndTy(exp, ty);
  }
	
  E::FunEntry *fun_entry = (E::FunEntry *)entry;
  if (fun_entry->result)
    ty = fun_entry->result->ActualTy();
  else
    ty = TY::VoidTy::Instance();

  T::ExpList *list = new T::ExpList(NULL, NULL);
  T::ExpList *tail = list;
  TY::TyList *formals_p = fun_entry->formals;
  ExpList *args_p = args;
  while (args_p && formals_p){
    TR::ExpAndTy check_arg = args_p->head->Translate(venv, tenv, level, label);
    if(!check_arg.ty->IsSameType(formals_p->head)){
      errormsg.Error(pos, "para type mismatch");
			return TR::ExpAndTy(exp, ty);
    }

    args_p = args_p->tail;
    formals_p = formals_p->tail;
    tail->tail = new T::ExpList(check_arg.exp->UnEx(), NULL);
    tail = tail->tail;
  }
	list = list->tail;

	if(!args_p && formals_p){
      errormsg.Error(pos, "too little params in function %s", this->func->Name().c_str());
      return TR::ExpAndTy(exp, ty);
  }
	if(args_p && !formals_p){
    errormsg.Error(pos, "too many params in function %s", func->Name().c_str());
    return TR::ExpAndTy(exp, ty);
  }

  if(!fun_entry->level->parent)
    exp = new TR::ExExp(F::externalCall(func->Name(), list));
  else
		exp = new TR::ExExp(new T::CallExp(new T::NameExp(func), new T::ExpList(StaticLink(fun_entry->level->parent, level), list)));

  return TR::ExpAndTy(exp, ty);
}

TR::ExpAndTy OpExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const {
  LOG("Translate OpExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy check_left = left->Translate(venv, tenv, level, label);
  TR::ExpAndTy check_right = right->Translate(venv, tenv, level, label);

  T::CjumpStm *stm = NULL;
  TR::Exp *exp = NULL;

  switch (oper)
  {
  case Oper::PLUS_OP:
  case Oper::MINUS_OP:
  case Oper::TIMES_OP:
  case Oper::DIVIDE_OP:
  {
    if (check_left.ty->kind != TY::Ty::Kind::INT)
      errormsg.Error(left->pos, "integer required");
    if (check_right.ty->kind != TY::Ty::Kind::INT)
      errormsg.Error(right->pos, "integer required");
    switch (oper)
    {
    case Oper::PLUS_OP:
      exp = new TR::ExExp(new T::BinopExp(T::BinOp::PLUS_OP, check_left.exp->UnEx(), check_right.exp->UnEx()));
      break;
    case Oper::MINUS_OP:
      exp = new TR::ExExp(new T::BinopExp(T::BinOp::MINUS_OP, check_left.exp->UnEx(), check_right.exp->UnEx()));
      break;
    case Oper::TIMES_OP:
      exp = new TR::ExExp(new T::BinopExp(T::BinOp::MUL_OP, check_left.exp->UnEx(), check_right.exp->UnEx()));
      break;
    case Oper::DIVIDE_OP:
      exp = new TR::ExExp(new T::BinopExp(T::BinOp::DIV_OP, check_left.exp->UnEx(), check_right.exp->UnEx()));
      break;
    }
    break;
  }
  case Oper::LT_OP:
  case Oper::LE_OP:
  case Oper::GT_OP:
  case Oper::GE_OP:
  {
    if (check_left.ty->kind != TY::Ty::Kind::INT && check_left.ty->kind != TY::Ty::Kind::STRING)
      errormsg.Error(left->pos, "integer or string required");
    if (check_right.ty->kind != TY::Ty::Kind::INT && check_right.ty->kind != TY::Ty::Kind::STRING)
      errormsg.Error(right->pos, "integer or string required");
    if (!check_left.ty->IsSameType(check_right.ty))
      errormsg.Error(pos, "same type required");

    T::CjumpStm *stm;
    switch (oper)
    {
    case Oper::LT_OP:
      stm = new T::CjumpStm(T::RelOp::LT_OP, check_left.exp->UnEx(), check_right.exp->UnEx(), NULL, NULL);
      break;
    case Oper::LE_OP:
      stm = new T::CjumpStm(T::RelOp::LE_OP, check_left.exp->UnEx(), check_right.exp->UnEx(), NULL, NULL);
      break;
    case Oper::GT_OP:
      stm = new T::CjumpStm(T::RelOp::GT_OP, check_left.exp->UnEx(), check_right.exp->UnEx(), NULL, NULL);
      break;
    case Oper::GE_OP:
      stm = new T::CjumpStm(T::RelOp::GE_OP, check_left.exp->UnEx(), check_right.exp->UnEx(), NULL, NULL);
      break;
    }
    TR::PatchList *trues = new TR::PatchList(&stm->true_label, NULL);
    TR::PatchList *falses = new TR::PatchList(&stm->false_label, NULL);
    exp = new TR::CxExp(trues, falses, stm);
    break;
  }
  case Oper::EQ_OP:
  case Oper::NEQ_OP:
  {
    T::CjumpStm *stm;
    switch (oper)
    {
    case Oper::EQ_OP:
      if (check_left.ty->kind == TY::Ty::STRING)
        stm = new T::CjumpStm(T::EQ_OP, F::externalCall("stringEqual", new T::ExpList(check_left.exp->UnEx(), new T::ExpList(check_right.exp->UnEx(), NULL))), new T::ConstExp(1), NULL, NULL);
      else
        stm = new T::CjumpStm(T::RelOp::EQ_OP, check_left.exp->UnEx(), check_right.exp->UnEx(), NULL, NULL);
      break;
    case Oper::NEQ_OP:
      stm = new T::CjumpStm(T::RelOp::NE_OP, check_left.exp->UnEx(), check_right.exp->UnEx(), NULL, NULL);
      break;
    }
    TR::PatchList *trues = new TR::PatchList(&stm->true_label, NULL);
    TR::PatchList *falses = new TR::PatchList(&stm->false_label, NULL);
    exp = new TR::CxExp(trues, falses, stm);
    break;
  }
  default:
    assert(0);
  }

  return TR::ExpAndTy(exp, TY::IntTy::Instance());
}

TR::ExpAndTy RecordExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  LOG("Translate RecordExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TY::Ty *ty = tenv->Look(typ);
  TR::ExExp *exp = NULL;
  T::ExpList *list = new T::ExpList(NULL, NULL);
  T::ExpList *tail = list;

  if (!ty){
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }

  ty = ty->ActualTy();
  if (ty->kind != TY::Ty::Kind::RECORD){
    errormsg.Error(pos, "not record type %s", typ->Name().c_str());
    return TR::ExpAndTy(NULL, ty);
  }

  TY::FieldList *records = ((TY::RecordTy *)ty)->fields;
  EFieldList *efields = fields;
  int count = 0;
  while (records && efields){
    count++;
    TR::ExpAndTy check_exp = efields->head->exp->Translate(venv, tenv, level, label);

    if(!check_exp.ty->IsSameType(records->head->ty)){
      errormsg.Error(this->pos, "record type unmatched");
    }

    records = records->tail;
    efields = efields->tail;

    tail->head = check_exp.exp->UnEx();
    if (records){
      tail->tail = new T::ExpList(NULL, NULL);
      tail = tail->tail;
    }
  }
  TEMP::Temp *reg = TEMP::Temp::NewTemp();

  T::Stm *stm = new T::MoveStm(new T::TempExp(reg), F::externalCall("allocRecord", new T::ExpList(new T::ConstExp(count * F::wordsize), NULL)));
  
  count = 0;
  while (list && list->head){
    stm = new T::SeqStm(stm, new T::MoveStm(T::NewMemPlus_Const(new T::TempExp(reg), count * F::wordsize), list->head));
    list = list->tail;
    count++;
    if (!list || !list->head)
      break;
  }

  exp = new TR::ExExp(new T::EseqExp(stm, new T::TempExp(reg)));

  return TR::ExpAndTy(exp, ty);
}

TR::Exp *TranslateSeqExp(TR::Exp *left, TR::Exp *right)
{
  if (right)
    return new TR::ExExp(new T::EseqExp(left->UnNx(), right->UnEx()));
  else
    return new TR::ExExp(new T::EseqExp(left->UnNx(), new T::ConstExp(0)));
}

TR::ExpAndTy SeqExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  LOG("Translate SeqExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  ExpList *seq_p = seq;
  TR::Exp *exp = TranslateNilExp();
  if(!seq_p)
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());

  TR::ExpAndTy check_exp(NULL, NULL);
  while(seq_p){
    check_exp = seq_p->head->Translate(venv, tenv, level, label);
    exp = TranslateSeqExp(exp, check_exp.exp);
    seq_p = seq_p->tail;
  }
  
  return TR::ExpAndTy(exp, check_exp.ty);
}

TR::Exp *TranslateAssignExp(TR::Exp *var, TR::Exp *exp)
{
	return new TR::NxExp(new T::MoveStm(var->UnEx(), exp->UnEx()));
}

TR::ExpAndTy AssignExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  LOG("Translate AssignExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  if(var->kind == Var::Kind::SIMPLE){
    E::EnvEntry *entry = venv->Look(((SimpleVar *)var)->sym);
    if(entry->readonly){
      errormsg.Error(pos, "loop variable can't be assigned");
      // return TY::IntTy::Instance();
    }
  }

  TR::ExpAndTy check_var = var->Translate(venv, tenv, level, label);
  TR::ExpAndTy check_exp = exp->Translate(venv, tenv, level, label);
  if(!check_var.ty->IsSameType(check_exp.ty))
    errormsg.Error(pos, "unmatched assign exp");

  TR::Exp *exp = TranslateAssignExp(check_var.exp, check_exp.exp);
  return TR::ExpAndTy(exp, TY::VoidTy::Instance());
}

TR::ExpAndTy IfExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const {
  LOG("Translate IfExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy check_test = test->Translate(venv, tenv, level, label);
  TR::ExpAndTy check_then = then->Translate(venv, tenv, level, label);

  TR::Exp *exp;
  if(elsee){
    TR::ExpAndTy check_elsee = elsee->Translate(venv, tenv, level, label);
    if (!check_then.ty->IsSameType(check_elsee.ty)) {
      errormsg.Error(pos, "then exp and else exp type mismatch");
      return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
    }
    
    TR::Cx testc = check_test.exp->UnCx();
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    TEMP::Label *true_label = TEMP::NewLabel();
    TEMP::Label *false_label = TEMP::NewLabel();
    TEMP::Label *meeting = TEMP::NewLabel();
    do_patch(testc.trues, true_label);
    do_patch(testc.falses, false_label);
    
    exp = new TR::ExExp(new T::EseqExp(testc.stm,
    new T::EseqExp(new T::LabelStm(true_label), 
    new T::EseqExp(new T::MoveStm(new T::TempExp(r), check_then.exp->UnEx()), 
    new T::EseqExp(new T::JumpStm(new T::NameExp(meeting), new TEMP::LabelList(meeting, NULL)), 
    new T::EseqExp(new T::LabelStm(false_label), 
    new T::EseqExp(new T::MoveStm(new T::TempExp(r), check_elsee.exp->UnEx()), 
    new T::EseqExp(new T::JumpStm(new T::NameExp(meeting), new TEMP::LabelList(meeting, NULL)), 
    new T::EseqExp(new T::LabelStm(meeting), new T::TempExp(r))))))))));
  } else {
    if(check_then.ty->kind != TY::Ty::VOID){
      errormsg.Error(pos, "if-then exp's body must produce no value");
      return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
    }

    TR::Cx testc = check_test.exp->UnCx();
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    TEMP::Label *true_label = TEMP::NewLabel();
    TEMP::Label *false_label = TEMP::NewLabel();
    TEMP::Label *meeting = TEMP::NewLabel();
    do_patch(testc.trues, true_label);
    do_patch(testc.falses, false_label);

    exp = new TR::NxExp(new T::SeqStm(testc.stm, new T::SeqStm(new T::LabelStm(true_label), new T::SeqStm(check_then.exp->UnNx(), new T::LabelStm(false_label)))));
  }

  return TR::ExpAndTy(exp, check_then.ty);
}

/*
 * test_label:
 *      if condition goto body_label else goto done_label
 * body_label:
 *      body
 * 			break -----------+
 *      goto test_label  |
 * done_label: <---------+
 */
TR::ExpAndTy WhileExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  LOG("Translate WhileExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::Exp *exp = NULL;
  TY::Ty *ty = TY::VoidTy::Instance();

  TEMP::Label *done_label = TEMP::NewLabel();
  TR::ExpAndTy check_test = test->Translate(venv, tenv, level, label);
  TR::ExpAndTy check_body = body->Translate(venv, tenv, level, done_label);
  if(check_test.ty->kind != TY::Ty::Kind::INT){
    errormsg.Error(test->pos, "integer required");
    return TR::ExpAndTy(exp, ty);
  }
  if(check_body.ty->kind !=TY::Ty::Kind::VOID){
    errormsg.Error(body->pos, "while body must produce no value");
    return TR::ExpAndTy(exp, ty);
  }

  TEMP::Label *test_label = TEMP::NewLabel();
  TEMP::Label *body_label = TEMP::NewLabel();
	TR::Cx condition = check_test.exp->UnCx();
  do_patch(condition.trues, body_label);
  do_patch(condition.falses, done_label);
  
  exp = new TR::NxExp(
		new T::SeqStm(new T::LabelStm(test_label), 
		new T::SeqStm(condition.stm, 
		new T::SeqStm(new T::LabelStm(body_label), 
		new T::SeqStm(check_body.exp->UnNx(), 
		new T::SeqStm(new T::JumpStm(new T::NameExp(test_label), new TEMP::LabelList(test_label, NULL)), 
		new T::LabelStm(done_label)))))));

  return TR::ExpAndTy(exp, ty);
}

/*
 * let 
 * 		var := lo
 * 		__limit_var__ := hi
 * in 
 * 		while var <= __limit_var__
 * 				body
 * 				if var == __limit_var__
 * 						break
 * 				var := var+1
 * end
 */
TR::ExpAndTy ForExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const 
{
  LOG("Translate ForExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::Exp *exp = NULL;
  TY::Ty *ty = TY::VoidTy::Instance();

  TR::ExpAndTy check_lo = lo->Translate(venv,tenv,level,label);
  TR::ExpAndTy check_hi = hi->Translate(venv,tenv,level,label);
  if(check_lo.ty->kind != TY::Ty::Kind::INT || check_hi.ty->kind != TY::Ty::Kind::INT){
    errormsg.Error(lo->pos, "for exp's range type is not integer");
		return TR::ExpAndTy(exp, ty);
	}

	venv->BeginScope();
  venv->Enter(var, new E::VarEntry(TR::Access::AllocLocal(level, escape), check_lo.ty));
	TR::ExpAndTy check_body = body->Translate(venv, tenv, level, label);
  if(check_body.ty->kind != TY::Ty::Kind::VOID){
    errormsg.Error(pos, "for body must produce no value");
		return TR::ExpAndTy(exp, ty);
	}
	venv->EndScope();

  A::Exp *forexp_to_letexp = new A::LetExp(0, 
    new A::DecList(new A::VarDec(0, var, S::Symbol::UniqueSymbol("int"), lo),
    new A::DecList(new A::VarDec(0, S::Symbol::UniqueSymbol("__limit_var__"), S::Symbol::UniqueSymbol("int"), hi), NULL)), 
    new A::WhileExp(0, new A::OpExp(0, A::Oper::LE_OP, new A::VarExp(0, new A::SimpleVar(0, var)), new A::VarExp(0, new A::SimpleVar(0, S::Symbol::UniqueSymbol("__limit_var__")))), 
		new A::SeqExp(0, new A::ExpList(body, 
		new A::ExpList(new A::IfExp(0, new A::OpExp(0, A::Oper::EQ_OP, new A::VarExp(0, new A::SimpleVar(0, var)), new A::VarExp(0, new A::SimpleVar(0, S::Symbol::UniqueSymbol("__limit_var__")))),
		new A::BreakExp(0), NULL), 
    new A::ExpList(new A::AssignExp(0, new A::SimpleVar(0,var), new A::OpExp(0, A::PLUS_OP, new A::VarExp(0, new A::SimpleVar(0,var)), new A::IntExp(0, 1))), NULL)
    )))));
  
  return forexp_to_letexp->Translate(venv, tenv, level, label);
}

TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  LOG("Translate BreakExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  T::Stm *stm = new T::JumpStm(new T::NameExp(label),new TEMP::LabelList(label,NULL)); 
  TR::Exp *nxexp = new TR::NxExp(stm);
  return TR::ExpAndTy(nxexp, TY::VoidTy::Instance());
}

TR::ExpAndTy LetExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  LOG("Translate LetExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
	TR::Exp *exp = NULL;
  TY::Ty *ty = TY::VoidTy::Instance();

  static bool first = true;
  bool isMain = false;
  if(first){
    isMain = true;
    first = false;
  }
  
  T::Exp * res = NULL;

  venv->BeginScope();
  tenv->BeginScope();
	T::Stm * stm = NULL;
  if(decs && decs->head){
    stm = decs->head->Translate(venv,tenv,level,label)->UnNx();
    for(DecList *decs_p = decs->tail; decs_p && decs_p->head; decs_p = decs_p->tail)
      stm = new T::SeqStm(stm,decs_p->head->Translate(venv,tenv,level,label)->UnNx());
  }
  
  TR::ExpAndTy check_body = body->Translate(venv, tenv, level, label);
  venv->EndScope();
  tenv->EndScope();
  if(stm)
    res = new T::EseqExp(stm, check_body.exp->UnEx());
  else
    res = check_body.exp->UnEx();
  stm = new T::ExpStm(res);

  if(isMain){
    TR::fragtail->tail = new F::FragList(new F::ProcFrag(stm,level->frame), NULL);
    TR::fragtail = TR::fragtail->tail;
    isMain = false;
  }
  return TR::ExpAndTy(new TR::ExExp(res), check_body.ty->ActualTy());
}

TR::ExpAndTy ArrayExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  LOG("Translate ArrayExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TY::Ty *ty = tenv->Look(typ)->ActualTy();
  if (!ty) {
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }
  if (ty->kind != TY::Ty::ARRAY) {
    errormsg.Error(pos, "not array type %d %d", ty->kind, TY::Ty::Kind::ARRAY);
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }

  TR::ExpAndTy check_size = size->Translate(venv, tenv, level, label);
  if (check_size.ty->kind != TY::Ty::INT) {
    errormsg.Error(pos, "type of size expression should be int");
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }

  TR::ExpAndTy check_init = init->Translate(venv, tenv, level, label);
  if (!check_init.ty->IsSameType(((TY::ArrayTy *)ty)->ty)) {
    errormsg.Error(pos, "type mismatch");
    return TR::ExpAndTy(NULL, TY::IntTy::Instance());
  }

  TR::Exp *exp = new TR::ExExp(F::externalCall("initArray", new T::ExpList(check_size.exp->UnEx(), new T::ExpList(check_init.exp->UnEx(), NULL))));
  return TR::ExpAndTy(exp, ty);
}

TR::ExpAndTy VoidExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  LOG("Translate VoidExp level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
}

static TY::TyList *make_formal_tylist(S::Table<TY::Ty> *tenv, A::FieldList *params)
{
  if (params == NULL){
    return NULL;
  }

  TY::Ty *ty = tenv->Look(params->head->typ);
  if (ty == NULL){
    errormsg.Error(params->head->pos, "undefined type %s",
                   params->head->typ->Name().c_str());
  }

  return new TY::TyList(ty->ActualTy(), make_formal_tylist(tenv, params->tail));
}

static TY::FieldList *make_fieldlist(S::Table<TY::Ty> *tenv, A::FieldList *fields)
{
  if (fields == NULL){
    return NULL;
  }

  TY::Ty *ty = tenv->Look(fields->head->typ);
  if (ty == NULL){
    errormsg.Error(fields->head->pos, "undefined type %s",
                   fields->head->typ->Name().c_str());
  }
  return new TY::FieldList(new TY::Field(fields->head->name, ty),
                           make_fieldlist(tenv, fields->tail));
}

U::BoolList *make_formal_esclist(A::FieldList *params)
{
  if(!params)
    return NULL;
  return new U::BoolList(params->head->escape, make_formal_esclist(params->tail));
}

TR::AccessList *make_formal_acclist(TR::Level *level)
{
  TR::AccessList *accesslist = new TR::AccessList(NULL, NULL);
  TR::AccessList *tail = accesslist;
  for (F::AccessList *f_accesslist = level->frame->formals; f_accesslist; f_accesslist = f_accesslist->tail)
  {
    tail->tail = new TR::AccessList(new TR::Access(level, f_accesslist->head), NULL);
    tail = tail->tail;
  }
  accesslist = accesslist->tail;
  return accesslist;
}

TR::Exp *FunctionDec::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  LOG("Translate FunctionDec level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).

  S::Table<int> *check_table = new S::Table<int>();
  for (A::FunDecList *fundec_list = functions; fundec_list; fundec_list = fundec_list->tail) {
    A::FunDec *fundec = fundec_list->head;

    if(check_table->Look(fundec->name)){
      errormsg.Error(fundec->pos, "two functions have the same name");
      continue;
    }
    check_table->Enter(fundec->name, (int *)1);

    TY::TyList *formaltys = make_formal_tylist(tenv, fundec->params);
    TR::Level *new_level = TR::Level::NewLevel(level, fundec->name, make_formal_esclist(fundec->params));

    if(!fundec->result){
      venv->Enter(fundec->name, new E::FunEntry(new_level, fundec->name, formaltys, TY::VoidTy::Instance()));
      continue;
    }
    TY::Ty *result = tenv->Look(fundec->result);
    if (!result){
      errormsg.Error(pos, "FunctionDec undefined result.");
      continue;
    }
    venv->Enter(fundec->name, new E::FunEntry(new_level, fundec->name, formaltys, result));
  }

  for (A::FunDecList *fundec_list = functions; fundec_list; fundec_list = fundec_list->tail) {
    A::FunDec *fundec = fundec_list->head;

    venv->BeginScope();
    E::FunEntry *funentry = (E::FunEntry *)venv->Look(fundec->name);
    
    TY::TyList *formaltys = funentry->formals;
    F::AccessList *formalaccs = funentry->level->frame->formals->tail;//staticlink
    for(FieldList *fields = fundec->params; fields; fields = fields->tail, formaltys = formaltys->tail, formalaccs = formalaccs->tail)
      venv->Enter(fields->head->name, new E::VarEntry(new TR::Access(funentry->level, formalaccs->head), formaltys->head));
    
    TR::ExpAndTy entry = fundec->body->Translate(venv, tenv, funentry->level, funentry->label);

    if (!entry.ty->IsSameType(TY::VoidTy::Instance()) && fundec->result == NULL)
      errormsg.Error(pos, "procedure returns value");
    if (fundec->result && !entry.ty->IsSameType(tenv->Look(fundec->result)->ActualTy()))
      errormsg.Error(pos, "function return value type incorrect");
    venv->EndScope();

    TR::fragtail->tail = new F::FragList(new F::ProcFrag(F::F_procEntryExit1(funentry->level->frame, new T::MoveStm(new T::TempExp(F::RV()), entry.exp->UnEx())), funentry->level->frame), NULL);
    TR::fragtail = TR::fragtail->tail;
  }

  return TranslateNilExp();
}

TR::Exp *VarDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                           TR::Level *level, TEMP::Label *label) const {
  LOG("Translate VarDec level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy check_init = init->Translate(venv,tenv,level,label);
  TR::Access *access;
  if (!typ) {
    if (check_init.ty->kind == TY::Ty::NIL)
      errormsg.Error(pos, "init should not be nil without type specified");
  }else{
    TY::Ty *ty = tenv->Look(typ);
    if(check_init.ty->kind == TY::Ty::Kind::NIL && ty->ActualTy()->kind != TY::Ty::Kind::RECORD)
      errormsg.Error(pos, "init should not be nil without type specified");
    if(ty && ty->kind != check_init.ty->kind)
      errormsg.Error(pos,"type mismatch");
  }
  access = TR::Access::AllocLocal(level, true);
  venv->Enter(var,new E::VarEntry(access, check_init.ty));
	
  return TranslateAssignExp(TranslateSimpleVar(access, level), check_init.exp);
}

TR::Exp *TypeDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                            TR::Level *level, TEMP::Label *label) const {
  LOG("Translate TypeDec level %s label %s\n", TEMP::LabelString(level->frame->label).c_str(), TEMP::LabelString(label).c_str());
  // TODO: Put your codes here (lab5).
  for (A::NameAndTyList *tyList = types; tyList; tyList = tyList->tail) {
    for (A::NameAndTyList *temp = tyList->tail; temp; temp = temp->tail)
      if (temp->head->name == tyList->head->name)
        errormsg.Error(pos, "two types have the same name");
    tenv->Enter(tyList->head->name, new TY::NameTy(tyList->head->name, NULL));
  }

  for (A::NameAndTyList *tyList = types; tyList; tyList = tyList->tail) {
    TY::NameTy *nameTy = (TY::NameTy *)tenv->Look(tyList->head->name);
    nameTy->ty = tyList->head->ty->Translate(tenv);
  }

  bool hasCycle = false;
  for (A::NameAndTyList *tyList = types; tyList; tyList = tyList->tail) {
    TY::Ty *ty = tenv->Look(tyList->head->name);
    if (ty->kind == TY::Ty::NAME) {
      TY::Ty *tyTy = ((TY::NameTy *)ty)->ty;
      while (tyTy->kind == TY::Ty::NAME) {
        TY::NameTy *nameTy = (TY::NameTy *)tyTy;
        if (nameTy->sym->Name() == tyList->head->name->Name()) {
          errormsg.Error(pos, "illegal type cycle");
          hasCycle = true;
          break;
        }
        tyTy = nameTy->ty;
      }
    }
    if (hasCycle)
      break;
  }
  return TranslateNilExp();
}


TY::Ty *NameTy::Translate(S::Table<TY::Ty> *tenv) const {
  LOG("Translate NameTy\n");
  // TODO: Put your codes here (lab5).
  TY::Ty *ty = tenv->Look(name);
  if(!ty){
    errormsg.Error(pos, "undefined type %s", name->Name().c_str());
		return TY::VoidTy::Instance();
  }
  return new TY::NameTy(name, ty);
}

TY::Ty *RecordTy::Translate(S::Table<TY::Ty> *tenv) const {
  LOG("Translate RecordTy\n");
  // TODO: Put your codes here (lab5).
  TY::FieldList *fields = make_fieldlist(tenv, record);
  return new TY::RecordTy(fields);
}

TY::Ty *ArrayTy::Translate(S::Table<TY::Ty> *tenv) const {
  LOG("Translate ArrayTy\n");
  // TODO: Put your codes here (lab5).
  TY::Ty *ty = tenv->Look(array);
  if(!ty){
    errormsg.Error(pos, "undefined type %s", array->Name().c_str());
    return new TY::ArrayTy(NULL);
  }
  return new TY::ArrayTy(ty);
}



}  // namespace A
