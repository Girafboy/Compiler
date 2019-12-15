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
	static int get(StackVarList *root, TEMP::Temp *t);
	static int set(StackVarList *&root, TEMP::Temp *t, F::Frame *f, int off = 0);
};


Result RegAlloc(F::Frame* f, AS::InstrList* il);
AS::InstrList *StrongSplice(AS::InstrList *il, AS::Instr *i);
AS::InstrList *mapMapping(F::Frame *f, AS::InstrList **root, AS::InstrList *il, StackVarList *&varlist, TEMP::Map *destmap, TEMP::Map *regmap);

}  // namespace RA

#endif
