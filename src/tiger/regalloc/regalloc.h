#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"

namespace RA {

class Result {
 public:
  TEMP::Map* coloring;
  AS::InstrList* il;
  Result(TEMP::Map * map, AS::InstrList * list):coloring(map),il(list){}
};

class StackVarList
{
public:
	TEMP::Temp *tmp;
	int offset;
	StackVarList *next;

	StackVarList(TEMP::Temp *t, int off, StackVarList *li = NULL) : tmp(t), offset(off), next(li) {}

	int get(TEMP::Temp *t);
	int set(TEMP::Temp *t, F::Frame *f, int off = 0);
};


Result RegAlloc(F::Frame* f, AS::InstrList* il);
AS::InstrList *StrongSplice(AS::InstrList *il, AS::Instr *i);
AS::InstrList *mapMapping(AS::InstrList *il, AS::InstrList *cur, F::Frame *f, TEMP::Map *destmap);

}  // namespace RA

#endif
