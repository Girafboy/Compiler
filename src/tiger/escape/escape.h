#ifndef TIGER_ESCAPE_ESCAPE_H_
#define TIGER_ESCAPE_ESCAPE_H_

#include "tiger/absyn/absyn.h"

namespace ESC {

class EscapeEntry {
 public:
  int depth;
  bool* escape;

  EscapeEntry(int depth, bool* escape) : depth(depth), escape(escape) {}
};

void FindEscape(A::Exp* exp);

static void traverseExp(S::Table<EscapeEntry> *env, int depth, A::Exp *e);
static void traverseDec(S::Table<EscapeEntry> * env, int depth, A::Dec *d);
static void traverseVar(S::Table<EscapeEntry> * env, int depth, A::Var *v);

}  // namespace ESC

#endif