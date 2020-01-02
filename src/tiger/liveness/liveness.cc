#include "tiger/liveness/liveness.h"

#define DEBUG

#define LOG(format, args...) do{            \
  FILE *debug_log = fopen("register.log", "a+"); \
  fprintf(debug_log, "%d,%s: ", __LINE__, __func__); \
  fprintf(debug_log, format, ##args);       \
  fclose(debug_log);\
} while(0)

namespace LIVE {

using NodeTempListTable = TAB::Table<G::Node<AS::Instr>, TEMP::TempList>;
using TempNodeTable = TAB::Table<TEMP::Temp, G::Node<TEMP::Temp>>;

void InstrGraphShow(FILE *out, AS::Instr *instr)
{
  instr->Print(out, TEMP::Map::LayerMap(F::RegMap(), TEMP::Map::Name()));
  fprintf(out, "\t\t^-------");
}

void TempGraphShow(FILE *out, TEMP::Temp *t)
{
  fprintf(out, "Livegraph:  t%d ->", t->Int());
}

void NodeTempListTableShow(G::Node<AS::Instr> *key, TEMP::TempList *value)
{
  FILE *debug_log = fopen("register.log", "a+");
  fprintf(debug_log, "Node (%d): ", key->Key());
  for(; value; value = value->tail)
    fprintf(debug_log, "t%d ", value->head->Int());
  fprintf(debug_log, "\n");
  fclose(debug_log);
}

LiveGraph Liveness(G::Graph<AS::Instr>* flowgraph) {
  // TODO: Put your codes here (lab6).
#ifdef DEBUG
  {
    LOG("Liveness Begin\n");
    FILE *debug_log = fopen("register.log", "a+");
    flowgraph->Show(debug_log, flowgraph->Nodes(), InstrGraphShow);
    fclose(debug_log);
  }
#endif
  NodeTempListTable *in = new NodeTempListTable();
  NodeTempListTable *out = new NodeTempListTable();
  
  for(G::NodeList<AS::Instr> *nodelist = flowgraph->Nodes(); nodelist; nodelist = nodelist->tail){
    in->Enter(nodelist->head, NULL);
    out->Enter(nodelist->head, NULL);
  }

  /*
   * repeat:
   *    for each node:
   *        in[node] <- use[node] U (out[node] - def[node])
   *        for n in succ[node]:
   *            out[node] <- out[node] U in[n]
   * until fixedpoint
   */
  LOG("Liveness: Find Fixed Point\n");
  bool fixedpoint = false;
  while(!fixedpoint){
    LOG("\tRound\n");
    // in->Dump(NodeTempListTableShow);
    // out->Dump(NodeTempListTableShow);
    fixedpoint = true;
    for(G::NodeList<AS::Instr> *nodelist = flowgraph->Nodes()->Reverse(); nodelist; nodelist = nodelist->tail){
      G::Node<AS::Instr> *node = nodelist->head;
      
      TEMP::TempList *oldin_templist = in->Look(node);
      TEMP::TempList *oldout_templist = out->Look(node);
      for(G::NodeList<AS::Instr> *succ = node->Succ(); succ; succ = succ->tail)
        out->Set(node, Union(out->Look(node), in->Look(succ->head)));
      in->Set(node, Union(FG::Use(node), Difference(out->Look(node), FG::Def(node))));

      if(!Equal(oldin_templist, in->Look(node)) || !Equal(oldout_templist, out->Look(node)))
        fixedpoint = false;
    }
  }

  /*
   * Interference Graph
   * 1. register interference
   * 2. non-move instr: addEdge(def, out)
   *    move instr    : addEdge(def, out-use)
   */
  LOG("Liveness: Build Interference Graph\n");
  LiveGraph livegraph = LiveGraph();
  livegraph.graph = new G::Graph<TEMP::Temp>();
  livegraph.moves = NULL;

  // step 1
  TempNodeTable *temptab = new TempNodeTable();
  for(TEMP::TempList *list = F::AllRegs_noRSP(); list; list = list->tail){
    G::Node<TEMP::Temp> *node = livegraph.graph->NewNode(list->head);
    temptab->Enter(list->head, node);
  }

  for(TEMP::TempList *from = F::AllRegs_noRSP(); from; from = from->tail)
    for(TEMP::TempList *to = from->tail; to; to = to->tail){
      livegraph.graph->AddEdge(temptab->Look(from->head), temptab->Look(to->head));
      livegraph.graph->AddEdge(temptab->Look(to->head), temptab->Look(from->head));
    }
  
  // step 2
  for(G::NodeList<AS::Instr> *nodelist = flowgraph->Nodes(); nodelist; nodelist = nodelist->tail)
    for(TEMP::TempList *list = Union(out->Look(nodelist->head), FG::Def(nodelist->head)); list; list = list->tail){
      TEMP::Temp *temp = list->head;
      if(temp == F::SP())
        continue;
      if(!temptab->Look(temp)){
        G::Node<TEMP::Temp> *node = livegraph.graph->NewNode(temp);
        temptab->Enter(temp, node);
      }
    }

  for(G::NodeList<AS::Instr> *nodelist = flowgraph->Nodes(); nodelist; nodelist = nodelist->tail){
    G::Node<AS::Instr> *node = nodelist->head;
    // non-move instr
    if(!FG::IsMove(node)){
      for(TEMP::TempList *def = FG::Def(node); def; def = def->tail)
        for(TEMP::TempList *outt = out->Look(node); outt; outt = outt->tail){
          if(def->head == F::SP() || outt->head == F::SP())
            continue;

          livegraph.graph->AddEdge(temptab->Look(def->head), temptab->Look(outt->head));
          livegraph.graph->AddEdge(temptab->Look(outt->head), temptab->Look(def->head));
        }
    }
    // move instr 
    else {
      for(TEMP::TempList *def = FG::Def(node); def; def = def->tail){
        for(TEMP::TempList *outt = Difference(out->Look(node), FG::Use(node)); outt; outt = outt->tail){
          if(def->head == F::SP() || outt->head == F::SP())
            continue;
          
          livegraph.graph->AddEdge(temptab->Look(def->head), temptab->Look(outt->head));
          livegraph.graph->AddEdge(temptab->Look(outt->head), temptab->Look(def->head));
        }
      
        for(TEMP::TempList *use = FG::Use(node); use; use = use->tail){
          if(def->head == F::SP() || use->head == F::SP())
            continue;
          
          if(!livegraph.moves->contain(temptab->Look(def->head), temptab->Look(use->head)))
            livegraph.moves = new MoveList(temptab->Look(def->head), temptab->Look(use->head), livegraph.moves);
        }
      }
    }
  }

#ifdef DEBUG
  {
    LOG("Liveness End\n");
    FILE *debug_log = fopen("register.log", "a+");
    livegraph.graph->Show(debug_log, livegraph.graph->Nodes(), TempGraphShow);
    livegraph.moves->Print(debug_log);
    fclose(debug_log);
  }
#endif
  return livegraph;
}

TEMP::TempList *Union(TEMP::TempList *left, TEMP::TempList *right)
{
  TEMP::TempList *unio = NULL;
  for(; left; left = left->tail)
    unio = new TEMP::TempList(left->head, unio);
  for(; right; right = right->tail)
    if(!Contain(unio, right->head))
      unio = new TEMP::TempList(right->head, unio);
  return unio;
}

TEMP::TempList *Difference(TEMP::TempList *left, TEMP::TempList *right)
{
  TEMP::TempList *diff = NULL;
  for(; left; left = left->tail)
    if(!Contain(right, left->head))
      diff = new TEMP::TempList(left->head, diff);
  return diff;
}

bool Equal(TEMP::TempList *left, TEMP::TempList *right)
{
  for(TEMP::TempList *list = left; list; list = list->tail)
    if(!Contain(right, list->head))
      return false;
  for(; right; right = right->tail)
    if(!Contain(left, right->head))
      return false;
  return true;
}

bool Contain(TEMP::TempList *container, TEMP::Temp *temp)
{
  for(; container; container = container->tail)
    if(container->head == temp)
      return true;
  return false;
}

bool MoveList::contain(G::Node<TEMP::Temp>* src, G::Node<TEMP::Temp>* dst)
{
  for(MoveList *movelist = this; movelist; movelist = movelist->tail)
    if((movelist->src == src && movelist->dst == dst)
      || (movelist->src == dst && movelist->dst == src))
      return true;
  return false;
}

MoveList *MoveList::Intersect(MoveList *left, MoveList *right)
{
  MoveList *list = NULL;
  for(; left; left = left->tail)
    if(right->contain(left->src, left->dst))
      list = new MoveList(left->src, left->dst, list);
  return list;
}

MoveList *MoveList::Union(MoveList *left, MoveList *right)
{
  MoveList *list = NULL;
  for(; left; left = left->tail)
    list = new MoveList(left->src, left->dst, list);
  for(; right; right = right->tail)
    if(!list->contain(right->src, right->dst))
      list = new MoveList(right->src, right->dst, list);
  return list;
}

MoveList *MoveList::Difference(MoveList *left, MoveList *right)
{
  MoveList *list = NULL;
  for(; left; left = left->tail)
    if(!right->contain(left->src, left->dst))
      list = new MoveList(left->src, left->dst, list);
  return list;
}

}  // namespace LIVE