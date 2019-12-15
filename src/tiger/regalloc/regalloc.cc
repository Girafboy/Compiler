#include "tiger/regalloc/regalloc.h"

namespace RA
{

Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
	// TODO: Put your codes here (lab6).
	TEMP::Map *regmap = TEMP::Map::Empty();
	regmap->Enter(F::SP(), new std::string("%rsp"));
	regmap->Enter(F::RAX(), new std::string("%rax"));
	regmap->Enter(F::RBP(), new std::string("%rbp"));
	regmap->Enter(F::RDI(), new std::string("%rdi"));
	regmap->Enter(F::RSI(), new std::string("%rsi"));
	regmap->Enter(F::RDX(), new std::string("%rdx"));
	regmap->Enter(F::RCX(), new std::string("%rcx"));
	regmap->Enter(F::R8(), new std::string("%r8"));
	regmap->Enter(F::R9(), new std::string("%r9"));
	regmap->Enter(F::R10(), new std::string("%r10"));
	regmap->Enter(F::R11(), new std::string("%r11"));
	regmap->Enter(F::R12(), new std::string("%r12"));
	regmap->Enter(F::R13(), new std::string("%r13"));
	regmap->Enter(F::R14(), new std::string("%r14"));
	regmap->Enter(F::R15(), new std::string("%r15"));
	TEMP::Map *destmap = TEMP::Map::Empty();
	AS::InstrList *root = il;
	StackVarList *varlist = NULL;
	AS::InstrList *cur = root;
	while (cur){
		mapMapping(f, &root, cur, varlist, destmap, regmap);
		cur = cur->tail;
	}

	TEMP::Map *resmap = TEMP::Map::LayerMap(regmap, destmap);
	return Result(resmap, root);
}

AS::InstrList *StrongSplice(AS::InstrList *il, AS::Instr *i)
{
	if (il)
		il = AS::InstrList::Splice(il, new AS::InstrList(i, NULL));
	else
		il = new AS::InstrList(i, NULL);
	return il;
}

AS::InstrList *mapMapping(F::Frame *f, AS::InstrList **root, AS::InstrList *il, StackVarList *&varlist, TEMP::Map *destmap, TEMP::Map *regmap)
{
	static char str[256];
	AS::InstrList *tail = il->tail;
	TEMP::TempList *tempList_src;
	TEMP::TempList *tempList_dst;
	AS::InstrList *prefetch = NULL;
	AS::InstrList *restore = NULL;
	TEMP::Temp *temp = NULL;
	TEMP::Temp *newtemp = NULL;

	int count = 0;
	int offset = 0;
	switch (il->head->kind)
	{
	case AS::Instr::OPER:
	{
		AS::OperInstr *operIns = (AS::OperInstr *)il->head;
		tempList_src = operIns->src;
		tempList_dst = operIns->dst;
		break;
	}
	case AS::Instr::MOVE:
	{
		AS::MoveInstr *moveIns = (AS::MoveInstr *)il->head;
		tempList_src = moveIns->src;
		tempList_dst = moveIns->dst;
		break;
	}
	default:
		return NULL;
	}

	TEMP::TempList *tempList = tempList_src;
	while (tempList && tempList->head){
		temp = tempList->head;
		if (!regmap->Look(temp)){
			offset = StackVarList::get(varlist, tempList->head);
			if (!offset)
				offset = StackVarList::set(varlist, tempList->head, f);
			else{
				newtemp = TEMP::Temp::NewTemp();
				tempList->head = newtemp;
				offset = StackVarList::set(varlist, newtemp, f, offset);
			}
			if (!count){
				sprintf(str, "\tmovq -%d(%%rbp), %%r10\n", offset);
				destmap->Enter(tempList->head, new std::string("%r10"));
			}else if (count == 1){
				sprintf(str, "\tmovq -%d(%%rbp), %%r11\n", offset);
				destmap->Enter(tempList->head, new std::string("%r11"));
			}
			prefetch = StrongSplice(prefetch, new AS::MoveInstr(str, NULL, NULL));
			count++;
		}

		tempList = tempList->tail;
	}

	tempList = tempList_dst;
	count = 0;
	while (tempList && tempList->head){
		temp = tempList->head;
		if (!regmap->Look(temp)){
			offset = StackVarList::get(varlist, tempList->head);
			if (!offset)
				offset = StackVarList::set(varlist, tempList->head, f);
			else{
				newtemp = TEMP::Temp::NewTemp();
				tempList->head = newtemp;
				offset = StackVarList::set(varlist, newtemp, f, offset);
			}
			if (!count){
				sprintf(str, "\tmovq %%r12, -%d(%%rbp)\n", offset);
				destmap->Enter(tempList->head, new std::string("%r12"));
			}
			else if (count == 1){
				sprintf(str, "\tmovq %%r13, -%d(%%rbp)\n", offset);
				destmap->Enter(tempList->head, new std::string("%r13"));
			}
			restore = StrongSplice(restore, new AS::MoveInstr(str, NULL, NULL));
			count++;
		}
		tempList = tempList->tail;
	}

	AS::InstrList *last_il;
	if (*root == il)
		last_il = NULL;
	else{
		last_il = *root;
		while (last_il && last_il->tail != il)
			last_il = last_il->tail;
	}

	if (last_il){
		if (prefetch){
			last_il->tail = prefetch;
			AS::InstrList::Splice(prefetch, il);
		}
		if (restore){
			il->tail = restore;
			AS::InstrList::Splice(restore, tail);
		}
	}
}

int StackVarList::get(StackVarList *root, TEMP::Temp *t)
{
	StackVarList *cur = root;
	while (cur && cur->tmp != t)
		cur = cur->next;
	if (cur){
		assert(cur->tmp == t);
		return cur->offset;
	}
	else
		return 0;
}

int StackVarList::set(StackVarList *&root, TEMP::Temp *t, F::Frame *f, int off)
{
	if (!off){
		f->s_offset -= F::wordsize;
		off = -f->s_offset;
	}

	StackVarList *cur = root;
	if (!cur){
		root = new StackVarList(t, off);
		return off;
	}
	while (cur->next)
		cur = cur->next;
	cur->next = new StackVarList(t, off);
	return off;
}

} // namespace RA
