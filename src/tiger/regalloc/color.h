#ifndef TIGER_REGALLOC_COLOR_H_
#define TIGER_REGALLOC_COLOR_H_

#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace COL {

using Node = G::Node<TEMP::Temp>;
using NodeList = G::NodeList<TEMP::Temp>;
template<typename T>
using Table = G::Table<TEMP::Temp, T>;

class Result {
 public:
  TEMP::Map* coloring;
  TEMP::TempList* spills;
};

Result Color(G::Graph<TEMP::Temp>* ig, TEMP::Map* initial, TEMP::TempList* regs,
             LIVE::MoveList* moves);


NodeList *simplifyWorklist;
NodeList *freezeWorklist;
NodeList *spillWorklist;

NodeList *spilledNodes;
NodeList *coalescedNodes;
NodeList *coloredNodes;

NodeList *selectStack;

LIVE::MoveList *coalescedMoves;
LIVE::MoveList *constrainedMoves;
LIVE::MoveList *frozenMoves;
LIVE::MoveList *worklistMoves;
LIVE::MoveList *activeMoves;

Table<int> degreeTab;
Table<LIVE::MoveList> *moveListTab;
Table<Node> *aliasTab;
Table<std::string> *colorTab;

}  // namespace COL

#endif