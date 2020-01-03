#include "tiger/regalloc/regalloc.h"

namespace RA
{

Result RegAlloc(F::Frame* f, AS::InstrList* il)
{
  COL::Result result = COL::Color(f, il);
  return Result(result.coloring, RemoveMoveInstr(result.il, result.coloring));
}


AS::InstrList *RemoveMoveInstr(AS::InstrList *il, TEMP::Map *color)
{
  AS::InstrList *res = il;
  for(; il; il = il->tail)
    if(il->head->kind == AS::Instr::Kind::MOVE){
      AS::MoveInstr *i = (AS::MoveInstr *)il->head;
      if(color->Look(i->src->head) == color->Look(i->dst->head))
        i->assem = std::string("# remove ").append(i->assem);
    }
  return res;
}

} // namespace RA
