#include "tiger/liveness/flowgraph.h"

// #define DEBUG

#ifdef DEBUG
  #define LOG(format, args...) do{            \
    FILE *debug_log = fopen("register.log", "a+"); \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__); \
    fprintf(debug_log, format, ##args);       \
    fclose(debug_log);\
  } while(0)
#else
  #define LOG(format, args...) do{} while(0)
#endif

namespace FG {

TEMP::TempList* Def(G::Node<AS::Instr>* n) {
  // TODO: Put your codes here (lab6).
  AS::Instr *instr = n->NodeInfo();
  switch(instr->kind){
    case AS::Instr::Kind::OPER:
      return ((AS::OperInstr *)instr)->dst;
    case AS::Instr::Kind::LABEL:
      return NULL;
    case AS::Instr::Kind::MOVE:
      return ((AS::MoveInstr *)instr)->dst;
  }
}

TEMP::TempList* Use(G::Node<AS::Instr>* n) {
  // TODO: Put your codes here (lab6).
  AS::Instr *instr = n->NodeInfo();
  switch(instr->kind){
    case AS::Instr::Kind::OPER:
      return ((AS::OperInstr *)instr)->src;
    case AS::Instr::Kind::LABEL:
      return NULL;
    case AS::Instr::Kind::MOVE:
      return ((AS::MoveInstr *)instr)->src;
  }
}

bool IsMove(G::Node<AS::Instr>* n) {
  // TODO: Put your codes here (lab6).
  return n->NodeInfo()->kind == AS::Instr::Kind::MOVE;
}

bool IsJmp(G::Node<AS::Instr>* n) {
  AS::Instr *instr = n->NodeInfo();
  return instr->kind == AS::Instr::Kind::OPER && ((AS::OperInstr *)instr)->assem.find("jmp") != std::string::npos;
}

G::Graph<AS::Instr>* AssemFlowGraph(AS::InstrList* il, F::Frame* f) {
  // TODO: Put your codes here (lab6).
  LOG("AssemFlowGraph\n");
  G::Graph<AS::Instr> *flowgraph = new G::Graph<AS::Instr>();
  TAB::Table<TEMP::Label, G::Node<AS::Instr>> *labelnode = new TAB::Table<TEMP::Label, G::Node<AS::Instr>>();

  for(AS::InstrList *instrlist = il; instrlist; instrlist = instrlist->tail){
    AS::Instr *instr = instrlist->head;

    G::Node<AS::Instr> *prev = NULL;
    if(flowgraph->mylast)
      prev = flowgraph->mylast->head;
    G::Node<AS::Instr> *node = flowgraph->NewNode(instr);
    
    if(prev && !IsJmp(prev))
      flowgraph->AddEdge(prev, node);

    if(instr->kind == AS::Instr::Kind::LABEL)
      labelnode->Enter(((AS::LabelInstr *)instr)->label, node);
  }

  for(G::NodeList<AS::Instr> *nodelist = flowgraph->Nodes(); nodelist; nodelist = nodelist->tail){
    G::Node<AS::Instr> *node = nodelist->head;
    if(node->NodeInfo()->kind != AS::Instr::Kind::OPER || !((AS::OperInstr *)node->NodeInfo())->jumps)
      continue;
    
    for(TEMP::LabelList *labels = ((AS::OperInstr *)node->NodeInfo())->jumps->labels; labels; labels = labels->tail)
      flowgraph->AddEdge(node, labelnode->Look(labels->head));
  }

  return flowgraph;
}

}  // namespace FG
