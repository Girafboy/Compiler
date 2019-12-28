#include "tiger/escape/escape.h"

namespace ESC {

void FindEscape(A::Exp* exp) {
  // TODO: Put your codes here (lab6).
  S::Table<EscapeEntry> *env = new S::Table<EscapeEntry>();
  traverseExp(env, 1, exp);
}

static void traverseExp(S::Table<EscapeEntry> *env, int depth, A::Exp *e)
{
  switch(e->kind){
    case A::Exp::Kind::VAR:
      traverseVar(env, depth, ((A::VarExp *)e)->var);
      break;
    case A::Exp::Kind::CALL:
      for(A::ExpList *args = ((A::CallExp *)e)->args; args; args = args->tail)
        traverseExp(env, depth, args->head);
      break;
    case A::Exp::Kind::OP:
      traverseExp(env, depth, ((A::OpExp *)e)->left);
      traverseExp(env, depth, ((A::OpExp *)e)->right);
      break;
    case A::Exp::Kind::RECORD:
      for(A::EFieldList *fields = ((A::RecordExp *)e)->fields; fields; fields = fields->tail)
        traverseExp(env, depth, fields->head->exp);
      break;
    case A::Exp::Kind::SEQ:
      for(A::ExpList *seq = ((A::SeqExp *)e)->seq; seq; seq = seq->tail)
        traverseExp(env, depth, seq->head);
      break;
    case A::Exp::Kind::ASSIGN:
      traverseVar(env, depth, ((A::AssignExp *)e)->var);
      traverseExp(env, depth, ((A::AssignExp *)e)->exp);
      break;
    case A::Exp::Kind::IF:
      traverseExp(env, depth, ((A::IfExp *)e)->test);
      traverseExp(env, depth, ((A::IfExp *)e)->then);
      if(((A::IfExp *)e)->elsee)
        traverseExp(env, depth, ((A::IfExp *)e)->elsee);
      break;
    case A::Exp::Kind::WHILE:
      traverseExp(env, depth, ((A::WhileExp *)e)->test);
      traverseExp(env, depth, ((A::WhileExp *)e)->body);
      break;
    case A::Exp::Kind::FOR:
      ((A::ForExp *)e)->escape = false;
      env->Enter(((A::ForExp *)e)->var, new EscapeEntry(depth, &((A::ForExp *)e)->escape));
      traverseExp(env, depth, ((A::ForExp *)e)->lo);
      traverseExp(env, depth, ((A::ForExp *)e)->hi);
      traverseExp(env, depth, ((A::ForExp *)e)->body);
      break;
    case A::Exp::Kind::LET:
      for(A::DecList *decs = ((A::LetExp *)e)->decs; decs; decs = decs->tail)
        traverseDec(env, depth, decs->head);
      traverseExp(env, depth, ((A::LetExp *)e)->body);
      break;
    case A::Exp::Kind::ARRAY:
      traverseExp(env, depth, ((A::ArrayExp *)e)->size);
      traverseExp(env, depth, ((A::ArrayExp *)e)->init);
      break;
    case A::Exp::Kind::VOID:
    case A::Exp::Kind::NIL:
    case A::Exp::Kind::INT:
    case A::Exp::Kind::STRING:
    case A::Exp::Kind::BREAK:
      break;
  }
}

static void traverseDec(S::Table<EscapeEntry> * env, int depth, A::Dec *d)
{
  switch(d->kind){
    case A::Dec::Kind::FUNCTION:
      for(A::FunDecList *functions = ((A::FunctionDec *)d)->functions; functions; functions = functions->tail){
        env->BeginScope();
        for(A::FieldList *params = functions->head->params; params; params = params->tail){
          params->head->escape = false;
          env->Enter(params->head->name, new EscapeEntry(depth+1, &params->head->escape));
        }
        traverseExp(env, depth+1, functions->head->body);
        env->EndScope();
      }
      break;
    case A::Dec::Kind::VAR:
      ((A::VarDec *)d)->escape = false;
      env->Enter(((A::VarDec *)d)->var, new EscapeEntry(depth, &((A::VarDec *)d)->escape));
      traverseExp(env, depth, ((A::VarDec *)d)->init);
      break;
    case A::Dec::Kind::TYPE:
      break;
  }
}

static void traverseVar(S::Table<EscapeEntry> * env, int depth, A::Var *v)
{
  switch(v->kind){
    case A::Var::Kind::SIMPLE:
      if(depth > env->Look(((A::SimpleVar *)v)->sym)->depth)
        *(env->Look(((A::SimpleVar *)v)->sym)->escape) = true;
      break;
    case A::Var::Kind::FIELD:
      traverseVar(env, depth, ((A::FieldVar *)v)->var);
      break;
    case A::Var::Kind::SUBSCRIPT:
      traverseVar(env, depth, ((A::SubscriptVar *)v)->var);
      traverseExp(env, depth, ((A::SubscriptVar *)v)->subscript);
      break;
  }
}

}  // namespace ESC