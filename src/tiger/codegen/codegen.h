#ifndef TIGER_CODEGEN_CODEGEN_H_
#define TIGER_CODEGEN_CODEGEN_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/translate/tree.h"

namespace CG {

static AS::InstrList *instr_list = NULL;

static void saveCalleeRegs();
static void restoreCalleeRegs();
static void munchStm(T::Stm *stm);
static TEMP::Temp *munchExp(T::Exp *exp);
static void munchArgs(T::ExpList *list);

static void emit(AS::Instr *instr);

AS::InstrList* Codegen(F::Frame* f, T::StmList* stmList);
}
#endif