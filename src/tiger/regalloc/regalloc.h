#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/codegen/codegen.h"
#include "tiger/regalloc/color.h"

namespace RA {

class Result {
 public:
  TEMP::Map* coloring;
  AS::InstrList* il;
  Result(TEMP::Map * map, AS::InstrList * list):coloring(map),il(list){}
};

Result RegAlloc(F::Frame* f, AS::InstrList* il);

AS::InstrList *RemoveMoveInstr(AS::InstrList *il, TEMP::Map *color);

}  // namespace RA

#endif
