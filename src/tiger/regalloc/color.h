#ifndef TIGER_REGALLOC_COLOR_H_
#define TIGER_REGALLOC_COLOR_H_

#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace COL {

class Result {
 public:
  TEMP::Map* coloring;
  AS::InstrList* il;
  Result(TEMP::Map * map, AS::InstrList * list):coloring(map),il(list){}
  // TEMP::TempList* spills;
};

// Result Color(G::Graph<TEMP::Temp>* ig, TEMP::Map* initial, TEMP::TempList* regs,
//              LIVE::MoveList* moves);

/* --------------- ^----------Above Not Used----------^ --------------------- */

using Node = G::Node<TEMP::Temp>;
using NodeList = G::NodeList<TEMP::Temp>;
template<typename T>
using Table = G::Table<TEMP::Temp, T>;

#define K 15

Result Color(F::Frame* f, AS::InstrList* il);

void Build();
void MakeWorklist();
void Simplify();
void Coalesce();
void Freeze();
void SelectSpill();
void AssignColors();
AS::InstrList * RewriteProgram(F::Frame *f, AS::InstrList *il);

TEMP::Map * AssignRegisters(LIVE::LiveGraph g);

}  // namespace COL

#endif