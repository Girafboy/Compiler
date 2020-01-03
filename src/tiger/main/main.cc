#include <cstdio>
#include <fstream>
#include <string>

#include "tiger/absyn/absyn.h"
#include "tiger/canon/canon.h"
#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/escape/escape.h"
#include "tiger/frame/frame.h"
#include "tiger/parse/parser.h"
#include "tiger/regalloc/regalloc.h"
#include "tiger/translate/tree.h"
#include "tiger/regalloc/color.h"

// #define DEBUG

FILE *debug_log;

extern EM::ErrorMsg errormsg;

A::Exp* absyn_root;
std::ifstream infile;

namespace {

TEMP::Map* temp_map;

void do_proc(FILE* out, F::ProcFrag* procFrag) {
  temp_map = TEMP::Map::Empty();
  // Init temp_map
  // fprintf(debug_log, "doProc for function %s:\n", procFrag->frame->label->Name().c_str());
  // (new T::StmList(procFrag->body, NULL))->Print(debug_log);
  // fprintf(debug_log, "-------====IR tree=====-----\n");

  T::StmList* stmList = C::Linearize(procFrag->body);
  // stmList->Print(debug_log);
  // fprintf(debug_log, "-------====Linearlized=====-----\n");  /* 8 */
  struct C::Block blo = C::BasicBlocks(stmList);
  // C::StmListList* stmLists = blo.stmLists;
  // for (; stmLists; stmLists = stmLists->tail) {
  //  	stmLists->head->Print(debug_log);
  // 	fprintf(debug_log, "------====Basic block=====-------\n");
  // }
  stmList = C::TraceSchedule(blo);
  // stmList->Print(debug_log);
  // fprintf(debug_log, "-------====trace=====-----\n");

  // lab5&lab6: code generation
  AS::InstrList* iList = CG::Codegen(procFrag->frame, stmList); /* 9 */
  // iList->Print(debug_log, TEMP::Map::LayerMap(F::RegMap(), TEMP::Map::Name()));
  // fprintf(debug_log, "----======before RA=======-----\n");

  // lab6: register allocation
  RA::Result allocation = RA::RegAlloc(procFrag->frame, iList); /* 11 */
  // iList->Print(debug_log, allocation.coloring);
  // allocation.coloring->DumpMap(debug_log);
  // fprintf(debug_log, "----======after RA=======-----\n");

  AS::Proc* proc = F::F_procEntryExit3(procFrag->frame, allocation.il);

  std::string procName = procFrag->frame->label->Name();
  fprintf(out, ".globl %s\n", procName.c_str());
  fprintf(out, ".type %s, @function\n", procName.c_str());
  // prologue
  fprintf(out, "%s", proc->prolog.c_str());
  // body
  proc->body->Print(out,
                    TEMP::Map::LayerMap(temp_map, allocation.coloring));
  // epilog
  fprintf(out, "%s", proc->epilog.c_str());
  fprintf(out, ".size %s, .-%s\n", procName.c_str(), procName.c_str());
}

void do_str(FILE* out, F::StringFrag* strFrag) {
  fprintf(out, "%s:\n", strFrag->label->Name().c_str());
  int length = strFrag->str.size();
  // it may contains zeros in the middle of string. To keep this work, we need
  // to print all the charactors instead of using fprintf(str)
  fprintf(out, ".long %d\n", length);
  fprintf(out, ".string \"");
  for (int i = 0; i < length; i++) {
    if (strFrag->str[i] == '\n') {
      fprintf(out, "\\n");
    } else if (strFrag->str[i] == '\t') {
      fprintf(out, "\\t");
    } else if (strFrag->str[i] == '\"') {
      fprintf(out, "\\\"");
    } else {
      fprintf(out, "%c", strFrag->str[i]);
    }
  }
  fprintf(out, "\"\n");
}

}  // namespace

int main(int argc, char** argv) {
  F::FragList* frags = nullptr;
  char outfile[100];
  FILE* out = stdout;
  if (argc < 2) {
    fprintf(stderr, "usage: tiger-compiler file.tig\n");
    exit(1);
  }

  errormsg.Reset(argv[1], infile);
  Parser parser(infile, std::cerr);
  parser.parse();

  if (!absyn_root) return 1;


  // Lab 6: escape analysis
  // If you have implemented escape analysis, uncomment this
  ESC::FindEscape(absyn_root); /* set varDec's escape field */

  // Lab5: translate IR tree
  frags = TR::TranslateProgram(absyn_root);
  if (errormsg.anyErrors) return 1; /* don't continue */

  /* convert the filename */
  sprintf(outfile, "%s.s", argv[1]);
  out = fopen(outfile, "w");
  // debug_log = fopen("main.log", "a+");
  // fprintf(debug_log, "testcase %s %s\n", argv[0], argv[1]);
  fprintf(out, ".text\n");
  for (F::FragList* fragList = frags; fragList; fragList = fragList->tail)
    if (fragList->head->kind == F::Frag::Kind::PROC) {
      do_proc(out, static_cast<F::ProcFrag*>(fragList->head));
    }

  fprintf(out, ".section .rodata\n");
  for (F::FragList* fragList = frags; fragList; fragList = fragList->tail)
    if (fragList->head->kind == F::Frag::Kind::STRING) {
      do_str(out, static_cast<F::StringFrag*>(fragList->head));
    }

  fclose(out);
  // fclose(debug_log);
  return 0;
}