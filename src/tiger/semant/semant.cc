#include "tiger/semant/semant.h"
#include "tiger/errormsg/errormsg.h"

extern EM::ErrorMsg errormsg;

using VEnvType = S::Table<E::EnvEntry> *;
using TEnvType = S::Table<TY::Ty> *;

/*
 * S::Table is defined in tiger/symbol/symbol/h
 * TAB::Table is defined in tiger/util/table.h
 * useful functions:
 *  - void BeginScope()
 *  - void EndScope()
 *  - void Enter(KeyType *key, ValueType *value)
 *  - ValueType *Look(KeyType *key)
 */

namespace {
	static TY::TyList *make_formal_tylist(TEnvType tenv, A::FieldList *params) {
		if (params == nullptr) {
			return nullptr;
		}

		TY::Ty *ty = tenv->Look(params->head->typ);
		if (ty == nullptr) {
			errormsg.Error(params->head->pos, "undefined type %s",
				params->head->typ->Name().c_str());
		}

		return new TY::TyList(ty->ActualTy(), make_formal_tylist(tenv, params->tail));
	}

	static TY::FieldList *make_fieldlist(TEnvType tenv, A::FieldList *fields) {
		if (fields == nullptr) {
			return nullptr;
		}

		TY::Ty *ty = tenv->Look(fields->head->typ);
		if (ty == nullptr) {
			errormsg.Error(fields->head->pos, "undefined type %s",
				fields->head->typ->Name().c_str());
		}
		return new TY::FieldList(new TY::Field(fields->head->name, ty),
			make_fieldlist(tenv, fields->tail));
	}

}  // namespace

namespace A {

TY::Ty *SimpleVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  E::EnvEntry *entry = venv->Look(sym);
  if(entry && entry->kind == E::EnvEntry::VAR)
    return ((E::VarEntry *)entry)->ty->ActualTy();
  else{
    errormsg.Error(pos, "undefined variable %s", sym->Name().c_str());
    return TY::IntTy::Instance();
  }
}

TY::Ty *FieldVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4)
  TY::Ty *ty = var->SemAnalyze(venv, tenv, labelcount);
  // ty = ty->ActualTy();

  if(ty->kind != TY::Ty::Kind::RECORD){
    errormsg.Error(pos, "not a record type");
    return TY::IntTy::Instance();
  }

  TY::FieldList *fields = ((TY::RecordTy *)ty)->fields;
  while(fields){
    TY::Field *field = fields->head;
    if(field->name == sym)
      return field->ty;//->ActualTy();
    fields = fields->tail;
  }

  errormsg.Error(pos, "field %s doesn't exist", sym->Name().c_str());
  return TY::IntTy::Instance();
}

TY::Ty *SubscriptVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                                 int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *var_ty = var->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *exp_ty = subscript->SemAnalyze(venv, tenv, labelcount);
  // var_ty = var_ty->ActualTy();

  if(var_ty->kind != TY::Ty::Kind::ARRAY){
    errormsg.Error(pos, "array type required");
    return TY::IntTy::Instance();
  }

  if(exp_ty->kind != TY::Ty::Kind::INT){
    errormsg.Error(pos, "array index must be integer");
    return TY::IntTy::Instance();
  }

  return ((TY::ArrayTy *)var_ty)->ty->ActualTy();
}

TY::Ty *VarExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  switch(var->kind){
    case Var::Kind::SIMPLE:
      return ((SimpleVar *)var)->SemAnalyze(venv, tenv, labelcount);
    case Var::Kind::FIELD:
      return ((FieldVar *)var)->SemAnalyze(venv, tenv, labelcount);
    case Var::Kind::SUBSCRIPT:
      return ((SubscriptVar *)var)->SemAnalyze(venv, tenv, labelcount);
    default:
      assert(0);
  }
}

TY::Ty *NilExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::NilTy::Instance();
}

TY::Ty *IntExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::IntTy::Instance();
}

TY::Ty *StringExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::StringTy::Instance();
}

TY::Ty *CallExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const {
  // TODO: Put your codes here (lab4).
  E::EnvEntry *entry = venv->Look(func);
  if(!entry || entry->kind != E::EnvEntry::Kind::FUN){
    errormsg.Error(pos, "undefined function %s", this->func->Name().c_str());
    return TY::IntTy::Instance();
  }

  TY::TyList *formals = ((E::FunEntry *)entry)->formals;
  ExpList *args_p = args;
  while(formals){
    if(!args_p){
      errormsg.Error(pos, "too little params in function %s", this->func->Name().c_str());
      return TY::IntTy::Instance();
    }

    TY::Ty *ty = args_p->head->SemAnalyze(venv, tenv, labelcount);
    if(!ty->IsSameType(formals->head)){
      errormsg.Error(pos, "para type mismatch");
      return TY::IntTy::Instance();
    }
    
    args_p = args_p->tail;
    formals = formals->tail;
  }

  if(args_p)
    errormsg.Error(pos, "too many params in function %s", this->func->Name().c_str());
  return ((E::FunEntry *)entry)->result->ActualTy();
}

TY::Ty *OpExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *left_ty = left->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *right_ty = right->SemAnalyze(venv, tenv, labelcount);

  switch(oper){
    case Oper::PLUS_OP:
    case Oper::MINUS_OP:
    case Oper::TIMES_OP:
    case Oper::DIVIDE_OP:
      if(left_ty->kind != TY::Ty::Kind::INT)
        errormsg.Error(left->pos, "integer required");
      if(right_ty->kind != TY::Ty::Kind::INT)
        errormsg.Error(right->pos, "integer required");
      break;

    case Oper::LT_OP:
    case Oper::LE_OP:
    case Oper::GT_OP:
    case Oper::GE_OP:
      if(left_ty->kind != TY::Ty::Kind::INT && left_ty->kind != TY::Ty::Kind::STRING)
        errormsg.Error(left->pos, "integer or string required");
      if(right_ty->kind != TY::Ty::Kind::INT && right_ty->kind != TY::Ty::Kind::STRING)
        errormsg.Error(right->pos, "integer or string required");
      if(!left_ty->IsSameType(right_ty))
        errormsg.Error(pos, "same type required");
      break;

    case Oper::EQ_OP:
    case Oper::NEQ_OP:
      if(left_ty->kind != TY::Ty::Kind::INT && left_ty->kind != TY::Ty::Kind::STRING
      && left_ty->kind != TY::Ty::Kind::RECORD && left_ty->kind != TY::Ty::Kind::ARRAY)
        errormsg.Error(left->pos, "integer or string or record or array required");
      if(right_ty->kind != TY::Ty::Kind::INT && right_ty->kind != TY::Ty::Kind::STRING
      && right_ty->kind != TY::Ty::Kind::RECORD && right_ty->kind != TY::Ty::Kind::ARRAY)
        errormsg.Error(right->pos, "integer or string or record or array required");
      if (left_ty->kind == TY::Ty::NIL && right_ty->kind == TY::Ty::NIL)
				errormsg.Error(this->pos, "least one operand should not be NIL");
      if(!left_ty->IsSameType(right_ty) 
      && !(left_ty->kind == TY::Ty::Kind::RECORD && right_ty->kind == TY::Ty::Kind::NIL))
        errormsg.Error(pos, "same type required");
      break;

    default:
      assert(0);
  }

  return TY::IntTy::Instance();
}

TY::Ty *RecordExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(this->typ);
  if (!ty) {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
    return TY::IntTy::Instance();
  }
  ty = ty->ActualTy();
  if (ty->kind != TY::Ty::RECORD) {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
    return TY::IntTy::Instance();
  }
  if (ty->kind != TY::Ty::RECORD) {
    errormsg.Error(this->pos, "not record type %s", this->typ->Name().c_str());
    return TY::IntTy::Instance();
  }

  A::EFieldList *FIELDS = this->fields;
  TY::FieldList *RECORDS = ((TY::RecordTy *)ty)->fields;
  while (RECORDS && FIELDS) {
    A::EField *field = FIELDS->head;
    TY::Field *record = RECORDS->head;
    TY::Ty *field_ty = field->exp->SemAnalyze(venv, tenv, labelcount);
    if (field->name != record->name) {
      errormsg.Error(this->pos, "field not defined");
      return TY::IntTy::Instance();
    }
    if (field_ty->kind != record->ty->kind) {
      errormsg.Error(this->pos, "field type mismatch");
      return TY::IntTy::Instance();
    }
    FIELDS = FIELDS->tail;
    RECORDS = RECORDS->tail;
  }
  if (FIELDS || RECORDS) {
    errormsg.Error(this->pos, "field amount mismatch");
    return TY::IntTy::Instance();
  }

  return ty;
}

TY::Ty *SeqExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  ExpList *seq_p = seq;
  if(!seq_p)
    return TY::VoidTy::Instance();
  TY::Ty *ty;
  while(seq_p){
    ty = seq_p->head->SemAnalyze(venv, tenv, labelcount);
    seq_p = seq_p->tail;
  }

  return ty;//->ActualTy();
}

TY::Ty *AssignExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  if(var->kind == Var::Kind::SIMPLE){
    E::EnvEntry *entry = venv->Look(((SimpleVar *)var)->sym);
    if(entry){
      errormsg.Error(pos, "loop variable can't be assigned");
      return TY::IntTy::Instance();
    }
  }

  TY::Ty *var_ty = var->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *exp_ty = exp->SemAnalyze(venv, tenv, labelcount);
  if(!var_ty->IsSameType(exp_ty))
    errormsg.Error(pos, "unmatched assign exp");

  return var_ty;//->ActualTy();
}

TY::Ty *IfExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  if (this->test->SemAnalyze(venv, tenv, labelcount)->kind != TY::Ty::INT) {
    errormsg.Error(this->pos, "Wront type IfExp1");
    return TY::IntTy::Instance();
  }
  TY::Ty *ty = this->then->SemAnalyze(venv, tenv, labelcount);
  if (this->elsee == nullptr) {
    if (ty->kind != TY::Ty::VOID) {
      errormsg.Error(this->pos, "if-then exp's body must produce no value");
      return TY::IntTy::Instance();
    }
    return TY::VoidTy::Instance();
  } else {
    if (ty->IsSameType(this->elsee->SemAnalyze(venv, tenv, labelcount))) 
      return ty;
    else {
      errormsg.Error(this->pos, "then exp and else exp type mismatch");
      return TY::VoidTy::Instance();
    }
  }
}

TY::Ty *WhileExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *test_ty = test->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *body_ty = body->SemAnalyze(venv, tenv, labelcount);
  if(test_ty->kind != TY::Ty::Kind::INT)
    errormsg.Error(test->pos, "integer required");
  if(body_ty->kind !=TY::Ty::Kind::VOID)
    errormsg.Error(body->pos, "while body must produce no value");

  return TY::VoidTy::Instance();
}

TY::Ty *ForExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *lo_ty = lo->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *hi_ty = hi->SemAnalyze(venv, tenv, labelcount);

  if(lo_ty->kind != TY::Ty::Kind::INT)
    errormsg.Error(lo->pos, "for exp's range type is not integer");
  if(hi_ty->kind != TY::Ty::Kind::INT)
    errormsg.Error(hi->pos, "for exp's range type is not integer");

  venv->BeginScope();
  tenv->BeginScope();
  venv->Enter(var, new E::VarEntry(TY::IntTy::Instance()));
  TY::Ty *body_ty = body->SemAnalyze(venv, tenv, labelcount);
  venv->EndScope();
  tenv->EndScope();

  return TY::VoidTy::Instance();
}

TY::Ty *BreakExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

TY::Ty *LetExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  venv->BeginScope();
  tenv->BeginScope();
  DecList *decs_p = decs;
  while(decs_p){
    decs_p->head->SemAnalyze(venv, tenv, labelcount);
    decs_p = decs_p->tail;
  } 
  TY::Ty *ty = body->SemAnalyze(venv, tenv, labelcount);
  venv->EndScope();
  tenv->EndScope();
  return ty;//->ActualTy();
}

TY::Ty *ArrayExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(this->typ)->ActualTy();
  if (ty == nullptr) {
    errormsg.Error(this->pos, "Wront type ArrayExp1");
    return TY::VoidTy::Instance();
  }
  if (this->size->SemAnalyze(venv, tenv, labelcount)->kind != TY::Ty::INT) {
    errormsg.Error(this->pos, "Wront type ArrayExp2");
    return TY::VoidTy::Instance();
  }
  if (ty->kind != TY::Ty::ARRAY) {
    errormsg.Error(this->pos, "Wront type ArrayExp3");
    return TY::VoidTy::Instance();
  }
  TY::ArrayTy *arrayTy = (TY::ArrayTy *)ty;
  if (!this->init->SemAnalyze(venv, tenv, labelcount)->IsSameType(arrayTy->ty)) {
    errormsg.Error(this->pos, "type mismatch");
    return TY::VoidTy::Instance();
  }
  return arrayTy;
}

TY::Ty *VoidExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  for (A::FunDecList *funDecList = this->functions; funDecList != nullptr; funDecList = funDecList->tail) {
    A::FunDec *funDec = funDecList->head;
    for (A::FunDecList *checkName = funDecList; checkName->tail != nullptr; checkName = checkName->tail)
      if (checkName->tail->head->name->Name() == funDec->name->Name())
        errormsg.Error(funDec->pos, "two functions have the same name");

    TY::Ty *result;
    if (funDec->result != nullptr) {
      result = tenv->Look(funDec->result);
      if (result == nullptr)
        errormsg.Error(this->pos, "Wront type FuncDec");
    }
    else
      result = TY::VoidTy::Instance();
    venv->Enter(funDec->name, new E::FunEntry(make_formal_tylist(tenv, funDec->params), result));
  }

  for (A::FunDecList *funDecList = this->functions; funDecList != nullptr; funDecList = funDecList->tail) {
    A::FunDec *funDec = funDecList->head;
    venv->BeginScope();
    for (A::FieldList *fieldList = funDec->params; fieldList != nullptr; fieldList = fieldList->tail) {
      A::Field *field = fieldList->head;
      TY::Ty *ty = tenv->Look(field->typ);
      if (ty == nullptr)
        errormsg.Error(field->pos, "undefined type %s", field->typ->Name().c_str());
      venv->Enter(fieldList->head->name, new E::VarEntry(ty));
    }
    TY::Ty *bodyTy = funDec->body->SemAnalyze(venv, tenv, labelcount);
    TY::Ty *decTy = ((E::FunEntry *)venv->Look(funDec->name))->result;
    if (!bodyTy->IsSameType(decTy)) {
      if (decTy->kind == TY::Ty::VOID)
        errormsg.Error(funDec->body->pos, "procedure returns value");
      else
        errormsg.Error(funDec->body->pos, "Wrong type FuncDec");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  if (this->typ == nullptr) {
    TY::Ty *ty = this->init->SemAnalyze(venv, tenv, labelcount);
    if (ty->kind == TY::Ty::NIL)
      errormsg.Error(this->pos, "init should not be nil without type specified");
    venv->Enter(this->var, new E::VarEntry(ty->ActualTy()));
  }
  else {
    TY::Ty *ty = tenv->Look(this->typ);
    if (ty == nullptr) {
      errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
      return;
    }
    if (ty->IsSameType(this->init->SemAnalyze(venv, tenv, labelcount)))
      venv->Enter(this->var, new E::VarEntry(tenv->Look(this->typ)));
    else
      errormsg.Error(this->pos, "type mismatch");
  }
}

void TypeDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  for (A::NameAndTyList *tyList = this->types; tyList != nullptr; tyList = tyList->tail) {
    for (A::NameAndTyList *temp = tyList->tail; temp != nullptr; temp = temp->tail)
      if (temp->head->name == tyList->head->name)
        errormsg.Error(this->pos, "two types have the same name");
    tenv->Enter(tyList->head->name, new TY::NameTy(tyList->head->name, nullptr));
  }

  for (A::NameAndTyList *tyList = this->types; tyList != nullptr; tyList = tyList->tail) {
    TY::NameTy *nameTy = (TY::NameTy *)tenv->Look(tyList->head->name);
    nameTy->ty = tyList->head->ty->SemAnalyze(tenv);
  }

  bool hasCycle = false;
  for (A::NameAndTyList *tyList = this->types; tyList != nullptr; tyList = tyList->tail) {
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
}

TY::Ty *NameTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(name);
  if(!ty){
    errormsg.Error(pos, "undefined type %s", name->Name().c_str());
		return TY::VoidTy::Instance();
  }
  return new TY::NameTy(name, ty);
}

TY::Ty *RecordTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::FieldList *fields = make_fieldlist(tenv, record);
  return new TY::RecordTy(fields);
}

TY::Ty *ArrayTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(array);
  if(!ty){
    errormsg.Error(pos, "undefined type %s", array->Name().c_str());
    return TY::VoidTy::Instance();
  }
  return new TY::ArrayTy(ty);
}

}  // namespace A

namespace SEM {
	void SemAnalyze(A::Exp *root) {
		if (root) root->SemAnalyze(E::BaseVEnv(), E::BaseTEnv(), 0);
	}

}  // namespace SEM