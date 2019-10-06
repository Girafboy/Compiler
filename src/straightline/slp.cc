#include "straightline/slp.h"
#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  int argNum1 = stm1->MaxArgs();
  int argNum2 = stm2->MaxArgs();
  if(argNum1 > argNum2) 
    return argNum1;
  return argNum2;
}

Table *A::CompoundStm::Interp(Table *t) const {
  t = stm1->Interp(t);
  t = stm2->Interp(t);
  return t;
}

int A::AssignStm::MaxArgs() const {
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  IntAndTable *it = exp->Interp(t);
  t = it->t->Update(id, it->i);
  
 // delete it;
  return t;
}

int A::PrintStm::MaxArgs() const {
  int argNum1 = exps->NumExps();
  int argNum2 = exps->MaxArgs();
  if(argNum1 > argNum2) 
    return argNum1;
  return argNum2;
}

Table *A::PrintStm::Interp(Table *t) const {
  IntAndTable *it;
  ExpList *el = exps;
  do
  {
    it = el->Interp(t);
    std::cout << it->i << " ";
    t = it->t;
    //delete it;
  } while(el = el->PopOne());
  std::cout << std::endl;

  return t;
}

int A::IdExp::MaxArgs() const{
  return 0;
}

IntAndTable *A::IdExp::Interp(Table *t) const{
  return new IntAndTable(t->Lookup(id), t);
}

int A::NumExp::MaxArgs() const{
  return 0;
}

IntAndTable *A::NumExp::Interp(Table *t) const{
  return new IntAndTable(num, t);
}

int A::OpExp::MaxArgs() const{
  int argNum1 = left->MaxArgs();
  int argNum2 = right->MaxArgs();
  if(argNum1 > argNum2) 
    return argNum1;
  return argNum2;
}

IntAndTable *A::OpExp::Interp(Table *t) const{
  IntAndTable *t1 = left->Interp(t);
  IntAndTable *t2 = right->Interp(t1->t);
  int result;
  switch (oper)
  {
  case BinOp::PLUS:
    result = t1->i + t2->i;
    break;
  case BinOp::MINUS:
    result = t1->i - t2->i;
    break;
  case BinOp::TIMES:
    result = t1->i * t2->i;
    break;
  case BinOp::DIV:
    result = t1->i / t2->i;
    break;
  default:
    break;
  }
  t = t2->t;
  //delete t1,t2;
  return new IntAndTable(result, t);
}

int A::EseqExp::MaxArgs() const{
  int argNum1 = stm->MaxArgs();
  int argNum2 = exp->MaxArgs();
  if(argNum1 > argNum2) 
    return argNum1;
  return argNum2;
}

IntAndTable *A::EseqExp::Interp(Table *t) const{
  Table *t1 = stm->Interp(t);
  IntAndTable *t2 = exp->Interp(t1);
  int result = t2->i;
  t = t2->t;
  //delete t1,t2;
  return new IntAndTable(result, t);
}

int A::PairExpList::MaxArgs() const{
  int argNum1 = head->MaxArgs();
  int argNum2 = tail->MaxArgs();
  if(argNum1 > argNum2) 
    return argNum1;
  return argNum2;
}

int A::PairExpList::NumExps() const{
  return tail->NumExps() + 1;
}

IntAndTable *A::PairExpList::Interp(Table *t) const{
  IntAndTable *t1 = head->Interp(t);
  return t1;
}

ExpList *A::PairExpList::PopOne() const{
  return tail;
}

int A::LastExpList::MaxArgs() const{
  return last->MaxArgs();
}

int A::LastExpList::NumExps() const{
  return 1;
}

IntAndTable *A::LastExpList::Interp(Table *t) const{
  IntAndTable *t1 = last->Interp(t);
  return t1;
}

ExpList *A::LastExpList::PopOne() const{
  return NULL;
}

int Table::Lookup(std::string key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(std::string key, int value) const {
  return new Table(key, value, this);
}
}  // namespace A
