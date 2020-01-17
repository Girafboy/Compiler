#include "tiger/regalloc/color.h"

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

namespace COL {

void StringBoolTableShow(std::string *s, bool *b)
{
  LOG("%s true?%d\n", s->c_str(), *b);
}

void NodeStringTableShow(Node *n, std::string *s)
{
  if(s)
    LOG("t%d -> %s\n", n->NodeInfo()->Int(), s->c_str());
  else
    LOG("t%d -> NULL\n", n->NodeInfo()->Int());
}

void NodeIntTableShow(Node *n, int *s)
{
  if(s)
    LOG("t%d -> Num: %d\n", n->NodeInfo()->Int(), *s);
  else
    LOG("t%d -> NULL\n", n->NodeInfo()->Int());
}

void NodeNodeTableShow(Node *n, Node *s)
{
  if(s)
    LOG("t%d -> t%d\n", n->NodeInfo()->Int(), s->NodeInfo()->Int());
  else
    LOG("t%d NULL\n", n->NodeInfo()->Int());
}

void NodeMoveTableShow(Node *n, LIVE::MoveList *s)
{
  if(s){
    LOG("t%d: ---v\n", n->NodeInfo()->Int());
    FILE *debug_log = fopen("register.log", "a+");
    s->Print(debug_log);
    fclose(debug_log);
  }
  else
    LOG("t%d NULL\n", n->NodeInfo()->Int());
}

void TempGraphShow(FILE *out, TEMP::Temp *t)
{
  fprintf(out, "Livegraph:  t%d ->", t->Int());
}

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

Table<int> *degreeTab;
Table<LIVE::MoveList> *moveListTab;
Table<Node> *aliasTab;
Table<std::string> *colorTab;

TEMP::TempList *noSpillTemps;

static LIVE::LiveGraph livegraph;

// Result Color(G::Graph<TEMP::Temp> ig, TEMP::Map initial, TEMP::TempList regs,
//              LIVE::MoveList* moves) {
//   // TODO: Put your codes here (lab6).
//   return Result();
// }

void ShowStatus()
{
#ifdef DEBUG
  FILE *debug_log = fopen("register.log", "a+");
  fprintf(debug_log, "simplifyWorklist: \n");
  livegraph.graph->Show(debug_log, simplifyWorklist, TempGraphShow);
  fprintf(debug_log, "freezeWorklist: \n");
  livegraph.graph->Show(debug_log, freezeWorklist, TempGraphShow);
  fprintf(debug_log, "spillWorklist: \n");
  livegraph.graph->Show(debug_log, spillWorklist, TempGraphShow);
  fprintf(debug_log, "spilledNodes: \n");
  livegraph.graph->Show(debug_log, spilledNodes, TempGraphShow);
  fprintf(debug_log, "coalescedNodes: \n");
  livegraph.graph->Show(debug_log, coalescedNodes, TempGraphShow);
  fprintf(debug_log, "coloredNodes: \n");
  livegraph.graph->Show(debug_log, coloredNodes, TempGraphShow);
  fprintf(debug_log, "selectStack: \n");
  livegraph.graph->Show(debug_log, selectStack, TempGraphShow);

  fprintf(debug_log, "coalescedMoves: \n");
  if(coalescedMoves)
    coalescedMoves->Print(debug_log);
  fprintf(debug_log, "constrainedMoves: \n");
  if(constrainedMoves)
    constrainedMoves->Print(debug_log);
  fprintf(debug_log, "frozenMoves: \n");
  if(frozenMoves)
    frozenMoves->Print(debug_log);
  fprintf(debug_log, "worklistMoves: \n");
  if(worklistMoves)
    worklistMoves->Print(debug_log);
  fprintf(debug_log, "activeMoves: \n");
  if(activeMoves)
    activeMoves->Print(debug_log);
  fclose(debug_log);

  LOG("degreeTab: \n");
  if(degreeTab)
    degreeTab->Dump(NodeIntTableShow);
  LOG("aliasTab: \n");
  if(aliasTab)
    aliasTab->Dump(NodeNodeTableShow);
  LOG("moveListTab: \n");
  if(moveListTab)
    moveListTab->Dump(NodeMoveTableShow);
  LOG("colorTab: \n");
  if(colorTab)
    colorTab->Dump(NodeStringTableShow);
#endif
}

Result Color(F::Frame* f, AS::InstrList* il)
{
  LOG("RegAlloc\n");
  bool done = true;
  while(done){
    done = false;

    livegraph = LIVE::Liveness(FG::AssemFlowGraph(il, f));
    Build();
    MakeWorklist();

    while(simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist){
      if(simplifyWorklist)
        Simplify();
      else if(worklistMoves)
        Coalesce();
      else if(freezeWorklist)
        Freeze();
      else if(spillWorklist)
        SelectSpill();
    }

    AssignColors();

    if(spilledNodes){
      il = RewriteProgram(f, il);
      done = true;
    }
  }
  
  return Result(AssignRegisters(livegraph), il);
}

void Build()
{
  LOG("Build\n");
  simplifyWorklist = NULL;
  freezeWorklist = NULL;
  spillWorklist = NULL;

  spilledNodes = NULL;
  coalescedNodes = NULL;
  coloredNodes = NULL;

  selectStack = NULL;

  coalescedMoves = NULL;
  constrainedMoves = NULL;
  frozenMoves = NULL;
  worklistMoves = livegraph.moves;
  activeMoves = NULL;

  degreeTab = new Table<int>();
  moveListTab = new Table<LIVE::MoveList>();
  aliasTab = new Table<Node>();
  colorTab = new Table<std::string>();

  for(NodeList *nodes = livegraph.graph->Nodes(); nodes; nodes = nodes->tail){
    degreeTab->Enter(nodes->head, new int(nodes->head->OutDegree()));

    colorTab->Enter(nodes->head, F::RegMap()->Look(nodes->head->NodeInfo()));
    
    aliasTab->Enter(nodes->head, nodes->head);

    LIVE::MoveList *movelist = NULL;
    for(LIVE::MoveList *list = worklistMoves; list; list = list->tail)
      if(list->src == nodes->head || list->dst == nodes->head)
        movelist = new LIVE::MoveList(list->src, list->dst, movelist);
    moveListTab->Enter(nodes->head, movelist);
  }
}

void AddEdge(Node *u, Node *v)
{
  if(!u->Succ()->InNodeList(v) && u != v){
    if(!colorTab->Look(u)){
      livegraph.graph->AddEdge(u, v);
      (*degreeTab->Look(u))++;
    }
    if(!colorTab->Look(v)){
      livegraph.graph->AddEdge(v, u);
      (*degreeTab->Look(v))++;
    }
  }
}

NodeList *Adjacent(Node *n)
{
  return NodeList::Difference(n->Succ(), NodeList::Union(selectStack, coalescedNodes));
}

LIVE::MoveList *NodeMoves(Node *n)
{
  return LIVE::MoveList::Intersect(moveListTab->Look(n), LIVE::MoveList::Union(activeMoves, worklistMoves));
}

bool MoveRelated(Node *n)
{
  return NodeMoves(n);
}

void MakeWorklist()
{
  LOG("MakeWorklist\n");
  for(NodeList *nodes = livegraph.graph->Nodes(); nodes; nodes = nodes->tail){
    Node *node = nodes->head;

    if(colorTab->Look(node))
      continue;
    
    if(*degreeTab->Look(node) >= K)
      spillWorklist = new NodeList(node, spillWorklist);
    else if (MoveRelated(node))
      freezeWorklist = new NodeList(node, freezeWorklist);
    else
      simplifyWorklist = new NodeList(node, simplifyWorklist);
  }
}

void EnableMoves(NodeList *nodes)
{
  for(; nodes; nodes = nodes->tail)
    for(LIVE::MoveList *m = NodeMoves(nodes->head); m; m = m->tail)
      if(activeMoves->contain(m->src, m->dst)){
        activeMoves = LIVE::MoveList::Difference(activeMoves, new LIVE::MoveList(m->src, m->dst, NULL));
        worklistMoves = new LIVE::MoveList(m->src, m->dst, worklistMoves);
      }
}

void DecrementDegree(Node *n)
{
  (*degreeTab->Look(n))--;
  if(*degreeTab->Look(n) == K-1 && !colorTab->Look(n)){
    EnableMoves(new NodeList(n, n->Succ()));
    spillWorklist = NodeList::Difference(spillWorklist, new NodeList(n, NULL));
    if(MoveRelated(n))
      freezeWorklist = new NodeList(n, freezeWorklist);
    else
      simplifyWorklist = new NodeList(n, simplifyWorklist);
  }
}

void Simplify()
{
  LOG("***************************Before Simplify***************************\n");
  ShowStatus();

  Node *node = simplifyWorklist->head;
  simplifyWorklist = simplifyWorklist->tail;
  selectStack = new NodeList(node, selectStack);
  for(NodeList *nodes = node->Succ(); nodes; nodes = nodes->tail)
    DecrementDegree(nodes->head);
  
  LOG("***************************After Simplify****************************\n");
  ShowStatus();
}

void AddWorkList(Node *n)
{
  if(!colorTab->Look(n) && !MoveRelated(n) && *degreeTab->Look(n) < K){
    freezeWorklist = NodeList::Difference(freezeWorklist, new NodeList(n, NULL));
    simplifyWorklist = new NodeList(n, simplifyWorklist);
  }
}

// George
bool OK(Node *v, Node *u)
{
  bool ok = true;
  for(NodeList *nodes = v->Succ(); nodes; nodes = nodes->tail){
    Node *t = nodes->head;
    if(!(*degreeTab->Look(t) < K || colorTab->Look(t) || u->Succ()->InNodeList(t))){
      ok = false;
      break;
    }
  }
  return ok;
}

bool Conservative(NodeList *nodes)
{
  int k = 0;
  for(; nodes; nodes = nodes->tail)
    if(*degreeTab->Look(nodes->head) >= K)
      k++;
  return k < K;
}

Node *GetAlias(Node *n)
{
  if(coalescedNodes->InNodeList(n))
    return GetAlias(aliasTab->Look(n));
  else
    return n;
}

void Combine(Node *u, Node *v)
{
  if(freezeWorklist->InNodeList(v))
    freezeWorklist = NodeList::Difference(freezeWorklist, new NodeList(v, NULL));
  else
    spillWorklist = NodeList::Difference(spillWorklist, new NodeList(v, NULL));
  coalescedNodes = new NodeList(v, coalescedNodes);
  aliasTab->Set(v, u);

  moveListTab->Set(u, LIVE::MoveList::Union(moveListTab->Look(u), moveListTab->Look(v)));

  EnableMoves(new NodeList(v, NULL));

  for(NodeList *nodes = v->Succ(); nodes; nodes = nodes->tail){
    AddEdge(nodes->head, u);
    DecrementDegree(nodes->head);
  } 
  if(*degreeTab->Look(u) >= K && freezeWorklist->InNodeList(u)){
    freezeWorklist = NodeList::Difference(freezeWorklist, new NodeList(u, NULL));
    spillWorklist = new NodeList(u, spillWorklist);
  }
}

void Coalesce()
{
  LOG("Coalesce\n");
  LOG("***************************Before Coalesce***************************\n");
  ShowStatus();
  
  Node *x, *y, *u, *v;
  x = worklistMoves->src;
  y = worklistMoves->dst;
  if(colorTab->Look(GetAlias(y))){
    u = GetAlias(y);
    v = GetAlias(x);
  } else {
    u = GetAlias(x);
    v = GetAlias(y);
  }
  worklistMoves = worklistMoves->tail;

  if(u == v){
    coalescedMoves = new LIVE::MoveList(x, y, coalescedMoves);
    AddWorkList(u);
  }else if(colorTab->Look(v) || v->Succ()->InNodeList(u)){
    constrainedMoves = new LIVE::MoveList(x, y, constrainedMoves);
    AddWorkList(u);
    AddWorkList(v);
  }else if((colorTab->Look(u) && OK(v, u)) || (!colorTab->Look(u) && Conservative(NodeList::Union(u->Succ(), v->Succ())))){
    coalescedMoves = new LIVE::MoveList(x, y, coalescedMoves);
    Combine(u, v);
    AddWorkList(u);
  }else
    activeMoves = new LIVE::MoveList(x, y, activeMoves);
  
  LOG("***************************After Coalesce****************************\n");
  ShowStatus();
}

void FreezeMoves(Node *u)
{
  for(LIVE::MoveList *m = NodeMoves(u); m; m = m->tail){
    Node *x = m->src;
    Node *y = m->dst;

    Node *v;
    if(GetAlias(y) == GetAlias(u))
      v = GetAlias(x);
    else
      v = GetAlias(y);

    activeMoves = LIVE::MoveList::Difference(activeMoves, new LIVE::MoveList(x, y, NULL));
    frozenMoves = new LIVE::MoveList(x, y, frozenMoves);
    if(!NodeMoves(v) && *degreeTab->Look(v) < K){
      freezeWorklist = NodeList::Difference(freezeWorklist, new NodeList(v, NULL));
      simplifyWorklist = new NodeList(v, simplifyWorklist);
    }
  }
}

void Freeze()
{
  LOG("Freeze\n");
  LOG("***************************Before Freeze***************************\n");
  ShowStatus();

  Node *u = freezeWorklist->head;
  freezeWorklist = freezeWorklist->tail;
  simplifyWorklist = new NodeList(u, simplifyWorklist);
  FreezeMoves(u);

  LOG("***************************After Freeze****************************\n");
  ShowStatus();
}

void SelectSpill()
{
  LOG("***************************Before SelectSpill***************************\n");
  ShowStatus();

  Node *m = NULL;
  int maxweight = 0;
  for(NodeList *nodes = spillWorklist; nodes; nodes = nodes->tail){
    if(LIVE::Contain(noSpillTemps, nodes->head->NodeInfo()))
      continue;
    if(*degreeTab->Look(nodes->head) > maxweight){
      m = nodes->head;
      maxweight = *degreeTab->Look(nodes->head);
    }
  }
  assert(m);
  if(!m)
    m = spillWorklist->head;
  
  spillWorklist = NodeList::Difference(spillWorklist, new NodeList(m, NULL));
  simplifyWorklist = new NodeList(m, simplifyWorklist);
  FreezeMoves(m);
  LOG("***************************After SelectSpill****************************\n");
  ShowStatus();
}

void AssignColors()
{
  LOG("AssignColors\n");
  LOG("***************************Before AssignColors***************************\n");
  ShowStatus();

  spilledNodes = NULL;//?
  TAB::Table<std::string, bool> okColors;
  for(TEMP::TempList *t = F::AllRegs_noRSP(); t; t = t->tail)
    okColors.Enter(F::RegMap()->Look(t->head), new bool(true));

  while(selectStack){
    Node *n = selectStack->head;
    selectStack = selectStack->tail;
    for(TEMP::TempList *t = F::AllRegs_noRSP(); t; t = t->tail)
      okColors.Set(F::RegMap()->Look(t->head), new bool(true));
    
    for(NodeList *nodes = n->Succ(); nodes; nodes = nodes->tail){
      //TODO
      if(colorTab->Look(GetAlias(nodes->head)))
        okColors.Set(colorTab->Look(GetAlias(nodes->head)), new bool(false));
    }

    bool realspill = true;
    TEMP::TempList *t;
    for(t = F::AllRegs_noRSP(); t; t = t->tail)
      if(*okColors.Look(F::RegMap()->Look(t->head))){
        realspill = false;
        break;
      }
    
    if(realspill){
      spilledNodes = new NodeList(n, spilledNodes);
      // okColors.Dump(StringBoolTableShow);
    }
    else{
      coloredNodes = new NodeList(n, coloredNodes);//?
      colorTab->Set(n, F::RegMap()->Look(t->head));
    }
  }
  
  for(NodeList *nodes = coalescedNodes; nodes; nodes = nodes->tail)//?
    colorTab->Set(nodes->head, colorTab->Look(GetAlias(nodes->head)));

  LOG("***************************After AssignColors****************************\n");
  ShowStatus();
}

TEMP::TempList *replaceTempList(TEMP::TempList *list, TEMP::Temp *oldd, TEMP::Temp *neww)
{
  LOG("replace t%d -> t%d\n", oldd->Int(), neww->Int());
  for(; list; list = list->tail)
    if(list->head == oldd)
      list->head = neww;
}

AS::InstrList * RewriteProgram(F::Frame *f, AS::InstrList *il)
{
  LOG("RewriteProgram\n");
  static char str[256];
  noSpillTemps = NULL;//?
  for(; spilledNodes; spilledNodes = spilledNodes->tail){
    Node *node = spilledNodes->head;
    TEMP::Temp *spilledtemp = node->NodeInfo();

    f->s_offset -= 8;
    AS::InstrList *last = NULL;
    for(AS::InstrList *instr = il; instr; last = instr, instr = instr->tail){
      TEMP::TempList *tempList_src = NULL;
      TEMP::TempList *tempList_dst = NULL;
      switch (instr->head->kind){
        case AS::Instr::OPER:
          tempList_src = ((AS::OperInstr *)instr->head)->src;
          tempList_dst = ((AS::OperInstr *)instr->head)->dst;
          break;
        case AS::Instr::MOVE:
          tempList_src = ((AS::MoveInstr *)instr->head)->src;
          tempList_dst = ((AS::MoveInstr *)instr->head)->dst;
          break;
      }

      TEMP::Temp *neww;
      if(tempList_src && LIVE::Contain(tempList_src, spilledtemp)){
        neww = TEMP::Temp::NewTemp();
        noSpillTemps = new TEMP::TempList(neww, noSpillTemps);//?
        replaceTempList(tempList_src, spilledtemp, neww);

        sprintf(str, "# Ops!Spilled before\n\tmovq (%s_framesize-%d)(`s0), `d0", f->label->Name().c_str(), -f->s_offset);
		    AS::InstrList *newinstr = new AS::InstrList(new AS::OperInstr(str, new TEMP::TempList(neww, NULL), new TEMP::TempList(F::SP(), NULL), NULL), instr);
        if(last)
          last->tail = newinstr;
        else
          il = newinstr;
      }

      if(tempList_dst && LIVE::Contain(tempList_dst, spilledtemp)){
        // if(!neww){
          neww = TEMP::Temp::NewTemp();
          noSpillTemps = new TEMP::TempList(neww, noSpillTemps);//?
        // }
        replaceTempList(tempList_dst, spilledtemp, neww);

        sprintf(str, "# Ops!Spilled after\n\tmovq `s0, (%s_framesize-%d)(`d0)", f->label->Name().c_str(), -f->s_offset);
		    AS::InstrList *newinstr = new AS::InstrList(new AS::OperInstr(str, new TEMP::TempList(F::SP(), NULL), new TEMP::TempList(neww, NULL), NULL), instr->tail);
        instr->tail = newinstr;
        instr = instr->tail;
      }
    }
  }
  
  return il;
}

TEMP::Map * AssignRegisters(LIVE::LiveGraph g)
{
  LOG("AssignRegisters\n");
  TEMP::Map *res = TEMP::Map::Empty();
  res->Enter(F::SP(), new std::string("%rsp"));

  for(NodeList *nodes = g.graph->Nodes(); nodes; nodes = nodes->tail)
    res->Enter(nodes->head->NodeInfo(), colorTab->Look(nodes->head));
  
  return TEMP::Map::LayerMap(F::RegMap(), res);
}

}  // namespace COL

