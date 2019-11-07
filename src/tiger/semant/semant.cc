#include "tiger/semant/semant.h"
#include "tiger/errormsg/errormsg.h"

extern EM::ErrorMsg errormsg;

using VEnvType = S::Table<E::EnvEntry> *;
using TEnvType = S::Table<TY::Ty> *;

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
  if (ty == nullptr){
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
  ty = ty->ActualTy();

  if(ty->kind != TY::Ty::Kind::RECORD){
    errormsg.Error(pos, "not a record type");
    return TY::IntTy::Instance();
  }

  TY::FieldList *fields = ((TY::RecordTy *)ty)->fields;
  while(fields){
    TY::Field *field = fields->head;
    if(field->name == sym)
      return field->ty->ActualTy();
    fields = fields->tail;
  }

  errormsg.Error(labelcount, "field %s doesn't exist", sym->Name().c_str());
  return TY::IntTy::Instance();
}

TY::Ty *SubscriptVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                                 int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *var_ty = var->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *exp_ty = subscript->SemAnalyze(venv, tenv, labelcount);
  var_ty = var_ty->ActualTy();

  if(var_ty->kind != TY::Ty::Kind::ARRAY){
    errormsg.Error(pos, "SubscriptVar check Array fail.");
    return TY::IntTy::Instance();
  }

  if(exp_ty->kind != TY::Ty::Kind::INT){
    errormsg.Error(labelcount, "SubscriptVar check Int fail.");
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
      errormsg.Error(pos, "CallExp check args_p fail (less).");
      break;
    }

    TY::Ty *ty = args_p->head->SemAnalyze(venv, tenv, labelcount);
    if(!ty->IsSameType(formals->head)){
      errormsg.Error(pos, "CallExp check type fail.");
      break;
    }
    
    args_p = args_p->tail;
    formals = formals->tail;
  }

  if(args_p)
    errormsg.Error(pos, "CallExp check args fail (more).");
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
        errormsg.Error(left->pos, "OpExp integer required.");
      if(right_ty->kind != TY::Ty::Kind::INT)
        errormsg.Error(right->pos, "OpExp integer required.");
      break;

    case Oper::LT_OP:
    case Oper::LE_OP:
    case Oper::GT_OP:
    case Oper::GE_OP:
      if(left_ty->kind != TY::Ty::Kind::INT && left_ty->kind != TY::Ty::Kind::STRING)
        errormsg.Error(left->pos, "OpExp integer/string required.");
      if(right_ty->kind != TY::Ty::Kind::INT && right_ty->kind != TY::Ty::Kind::STRING)
        errormsg.Error(right->pos, "OpExp integer/string required.");
      if(!left_ty->IsSameType(right_ty))
        errormsg.Error(pos, "same type required");
      break;

    case Oper::EQ_OP:
    case Oper::NEQ_OP:
      if(left_ty->kind != TY::Ty::Kind::INT && left_ty->kind != TY::Ty::Kind::STRING
      && left_ty->kind != TY::Ty::Kind::RECORD && left_ty->kind != TY::Ty::Kind::ARRAY)
        errormsg.Error(left->pos, "OpExp integer/string/record/array required.");
      if(right_ty->kind != TY::Ty::Kind::INT && right_ty->kind != TY::Ty::Kind::STRING
      && right_ty->kind != TY::Ty::Kind::RECORD && right_ty->kind != TY::Ty::Kind::ARRAY)
        errormsg.Error(right->pos, "OpExp integer/string/record/array required.");
      if(!left_ty->IsSameType(right_ty) 
      && !(left_ty->kind == TY::Ty::Kind::RECORD && right_ty->kind == TY::Ty::Kind::NIL))
        errormsg.Error(pos, "same type required");
      break;

    default:
      assert(0);
  }

  return TY::IntTy::Instance();
}

TY::Ty *RecordExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(typ);
  if(!ty){
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
    return new TY::RecordTy(NULL);
  }

  ty = ty->ActualTy();
  if(!ty){
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
    return TY::IntTy::Instance();
  }
  if(ty->kind != TY::Ty::Kind::RECORD){
    errormsg.Error(pos, "not record type %s", typ->Name().c_str());
    return TY::IntTy::Instance();
  }

  TY::FieldList *records = ((TY::RecordTy *)ty)->fields;
  EFieldList *fields_p = fields;
  while(fields_p && records){
    TY::Ty *field_ty = fields_p->head->exp->SemAnalyze(venv, tenv, labelcount);

    if(fields_p->head->name != records->head->name){
      errormsg.Error(pos, "field not defined");
      return TY::IntTy::Instance();
    }
    if(field_ty->IsSameType(records->head->ty)){
      errormsg.Error(pos, "field type mismatch");
      return TY::IntTy::Instance();
    }

    fields_p = fields_p->tail;
    records = records->tail;
  }

  if(fields_p || records)
    errormsg.Error(pos, "field amount mismatch");

  return ty->ActualTy();
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

  return ty->ActualTy();
}

TY::Ty *AssignExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  if(var->kind == Var::Kind::SIMPLE){
    E::EnvEntry *entry = venv->Look(((SimpleVar *)var)->sym);
    if(entry)
      errormsg.Error(pos, "loop variable can't be assigned");
  }

  TY::Ty *var_ty = var->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *exp_ty = exp->SemAnalyze(venv, tenv, labelcount);
  if(!var_ty->IsSameType(exp_ty))
    errormsg.Error(pos, "AssignExp unmatched");

  return var_ty->ActualTy();
}

TY::Ty *IfExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *test_ty = test->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *then_ty = then->SemAnalyze(venv, tenv, labelcount);
  if(test_ty->kind != TY::Ty::Kind::INT)
    errormsg.Error(test->pos, "integer required");
  
  if(elsee && elsee->kind != Exp::Kind::NIL){
    TY::Ty *elsee_ty = elsee->SemAnalyze(venv, tenv, labelcount);
    if(!then_ty->IsSameType(elsee_ty))
      errormsg.Error(then->pos, "then exp and else exp type mismatch");
  } else if(then_ty->kind != TY::Ty::Kind::VOID)
    errormsg.Error(then->pos, "if-then exp's body must produce no value");
  return TY::VoidTy::Instance();
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
  return ty->ActualTy();
}

TY::Ty *ArrayExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *size_ty = size->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *init_ty = init->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *typ_ty = tenv->Look(typ);
  if(!typ_ty){
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
    return TY::IntTy::Instance();
  }

  typ_ty = typ_ty->ActualTy();
  if(!typ_ty){
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
    return TY::IntTy::Instance();
  }

  if(typ_ty->kind != TY::Ty::Kind::ARRAY){
    errormsg.Error(pos, "not array type %d %d",typ_ty->kind , TY::Ty::Kind::ARRAY);
    return TY::IntTy::Instance();
  }

  if(size_ty->kind != TY::Ty::Kind::INT)
    errormsg.Error(pos, "size and init integer required");
  if(init_ty->IsSameType(typ_ty))
    errormsg.Error(pos, "init exp type mismatch");

  return typ_ty->ActualTy();
}

TY::Ty *VoidExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  FunDecList *functions_p = functions;
  while(functions_p){
    if(venv->Look(functions_p->head->name)){
      errormsg.Error(pos, "FunctionDec dup name.");
      continue;
    }

    TY::Ty *ty;
    if(functions_p->head->result){
      ty = tenv->Look(functions_p->head->result);
      if(!ty){
        errormsg.Error(functions->head->pos, "FunctionDec undefined result.");
        ty = TY::VoidTy::Instance();
      }
    } else
      ty = TY::VoidTy::Instance();

    FieldList *params = functions_p->head->params;
    TY::TyList *formals = make_formal_tylist(tenv, params);
    E::EnvEntry *entry = new E::FunEntry(formals, ty);
    entry->kind = E::EnvEntry::Kind::FUN;

    venv->Enter(functions_p->head->name, entry);

    functions_p = functions_p->tail;
  }

  functions_p = functions;
  while(functions_p){
    E::EnvEntry *entry = venv->Look(functions_p->head->name);
    
    FieldList *params = functions_p->head->params;
    TY::TyList *formals = ((E::FunEntry *)entry)->formals;

    venv->BeginScope();
    tenv->BeginScope();
    
    while(params){
      venv->Enter(params->head->name, new E::VarEntry(formals->head));
      params = params->tail;
      formals = formals->tail;
    }

    TY::Ty *ty = ((E::FunEntry *)entry)->result;
    TY::Ty *body_ty = functions_p->head->body->SemAnalyze(venv, tenv, labelcount);
    if(ty->kind == TY::Ty::Kind::VOID || body_ty->kind != TY::Ty::Kind::VOID)
      errormsg.Error(functions_p->head->body->pos, "procedure returns value");
    else if (ty->IsSameType(body_ty))
      errormsg.Error(functions_p->head->body->pos, "return type mismatch");

    tenv->EndScope();
    venv->EndScope();

    functions_p = functions_p->tail;
  }
}

void VarDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *init_ty = init->SemAnalyze(venv, tenv, labelcount);
  init_ty = init_ty->ActualTy();

  if(init_ty->IsSameType(TY::NilTy::Instance())){
    errormsg.Error(pos, "VarDec init nil.");
    return;
  }
  if(typ){
    TY::Ty *require = tenv->Look(typ);
    if(!require)
      errormsg.Error(init->pos,"undefined type %s", typ->Name().c_str());

    if(!init_ty->IsSameType(require))
      errormsg.Error(pos, "VarDec init type unmatched.");
  }
  venv->Enter(var, new E::VarEntry(init_ty));
}

bool Ty_hasCycle(TY::Ty *ty)
{
  if (ty->kind != TY::Ty::Kind::NAME)
    return false;
  TY::Ty *cur;
  for (cur = ((TY::NameTy *)ty)->ty; cur->kind == TY::Ty::Kind::NAME && ((TY::NameTy *)cur)->ty; cur = ((TY::NameTy *)cur)->ty)
    if (cur == ty)
      return true;
  return false;
}

void TypeDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  NameAndTyList *types_p = types;
  while(types_p){
    if(tenv->Look(types_p->head->name)){
      errormsg.Error(pos, "TypeDec dup name.");
      continue;
    }
    tenv->Enter(types_p->head->name, new TY::NameTy(types_p->head->name, NULL));

    types_p = types_p->tail;
  }

  types_p = types;
  while(types_p){
    TY::Ty *ty = tenv->Look(types_p->head->name);
    if(!ty)
      continue;
    
    TY::Ty *actual = types_p->head->ty->SemAnalyze(tenv);
    ((TY::NameTy *)ty)->ty = actual;
    
    if(Ty_hasCycle(ty))
      errormsg.Error(pos, "illegal type cycle");

    types_p = types_p->tail;
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
