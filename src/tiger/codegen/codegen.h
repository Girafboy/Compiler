#ifndef TIGER_CODEGEN_CODEGEN_H_
#define TIGER_CODEGEN_CODEGEN_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/translate/tree.h"

namespace CG {

static AS::InstrList *instr_list = NULL;
static AS::InstrList *instr_tail = NULL;

static void saveCalleeRegs();
static void restoreCalleeRegs();
static void matchStm(T::Stm *stm);
static TEMP::Temp *matchExp(T::Exp *exp);
static void matchArgs(T::ExpList *list);

static void appendInstr(AS::Instr *instr);

AS::InstrList* Codegen(F::Frame* f, T::StmList* stmList);
}
#endif