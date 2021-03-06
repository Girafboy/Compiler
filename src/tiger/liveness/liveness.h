#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"

namespace LIVE {

void InstrGraphShow(AS::Instr *instr);

class MoveList {
 public:
  G::Node<TEMP::Temp>*src, *dst;
  MoveList* tail;

  MoveList(G::Node<TEMP::Temp>* src, G::Node<TEMP::Temp>* dst, MoveList* tail)
      : src(src), dst(dst), tail(tail) {}

  bool contain(G::Node<TEMP::Temp>* src, G::Node<TEMP::Temp>* dst);
  void Print(FILE *out){
    fprintf(out, "Move:  t%d ---> t%d\n", src->NodeInfo()->Int(), dst->NodeInfo()->Int());
    if(tail)
      tail->Print(out);
  }

  static MoveList *Intersect(MoveList *left, MoveList *right);
  static MoveList *Union(MoveList *left, MoveList *right);
  static MoveList *Difference(MoveList *left, MoveList *right);
};

class LiveGraph {
 public:
  G::Graph<TEMP::Temp>* graph;
  MoveList* moves;
};

LiveGraph Liveness(G::Graph<AS::Instr>* flowgraph);

TEMP::TempList *Union(TEMP::TempList *left, TEMP::TempList *right);
TEMP::TempList *Difference(TEMP::TempList *left, TEMP::TempList *right);
bool Equal(TEMP::TempList *left, TEMP::TempList *right);
bool Contain(TEMP::TempList *container, TEMP::Temp *temp);
}  // namespace LIVE

#endif