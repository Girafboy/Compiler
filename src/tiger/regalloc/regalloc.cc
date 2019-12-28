#include "tiger/regalloc/regalloc.h"

namespace RA
{
static StackVarList *varlist = new StackVarList(NULL, 0);
TEMP::Map *regmap = F::RegMap();

Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
	// TODO: Put your codes here (lab6).
	TEMP::Map *destmap = TEMP::Map::Empty();
	AS::InstrList *cur = il->tail;
	while (cur){
		mapMapping(il, cur, f, destmap);
		cur = cur->tail;
	}

	TEMP::Map *resmap = TEMP::Map::LayerMap(regmap, destmap);
	return Result(resmap, il);
}

AS::InstrList *StrongSplice(AS::InstrList *il, AS::Instr *i)
{
	if (il)
		il = AS::InstrList::Splice(il, new AS::InstrList(i, NULL));
	else
		il = new AS::InstrList(i, NULL);
	return il;
}

AS::InstrList *fprefetch(TEMP::TempList *templist, F::Frame *frame, TEMP::Map *destmap){
	static char str[256];

	int count = 0;
	AS::InstrList *prefetch = NULL;

	for(; templist && templist->head; templist = templist->tail){
		TEMP::Temp *temp = templist->head;
		if (!regmap->Look(temp)){
			int offset = varlist->get(temp);
			if (!offset)
				offset = varlist->set(temp, frame);
			else{
				TEMP::Temp *newtemp = TEMP::Temp::NewTemp();
				templist->head = newtemp;
				offset = varlist->set(newtemp, frame, offset);
			}
			if (!count){
				sprintf(str, "movq -%d(%%rbp), %%r10", offset);
				destmap->Enter(templist->head, new std::string("%r10"));
			}else if (count == 1){
				sprintf(str, "movq -%d(%%rbp), %%r11", offset);
				destmap->Enter(templist->head, new std::string("%r11"));
			}
			prefetch = StrongSplice(prefetch, new AS::MoveInstr(str, NULL, NULL));
			count++;
		}
	}
	return prefetch;
}

AS::InstrList *frestore(TEMP::TempList *templist, F::Frame *frame, TEMP::Map *destmap){
	static char str[256];

	int count = 0;
	AS::InstrList *restore = NULL;

	for(; templist && templist->head; templist = templist->tail){
		TEMP::Temp *temp = templist->head;
		if (regmap->Look(temp))
			continue;

		int offset = varlist->get(temp);
		if (!offset)
			offset = varlist->set(temp, frame);
		else{
			TEMP::Temp *newtemp = TEMP::Temp::NewTemp();
			templist->head = newtemp;
			offset = varlist->set(newtemp, frame, offset);
		}
		if (!count){
			sprintf(str, "movq %%r12, -%d(%%rbp)", offset);
			destmap->Enter(templist->head, new std::string("%r12"));
		}
		else if (count == 1){
			sprintf(str, "movq %%r13, -%d(%%rbp)", offset);
			destmap->Enter(templist->head, new std::string("%r13"));
		}
		restore = StrongSplice(restore, new AS::MoveInstr(str, NULL, NULL));
		count++;
	}
	return restore;
}

AS::InstrList *mapMapping(AS::InstrList *il, AS::InstrList *cur, F::Frame *frame, TEMP::Map *destmap)
{
	TEMP::TempList *tempList_src;
	TEMP::TempList *tempList_dst;
	switch (cur->head->kind){
		case AS::Instr::OPER:
			tempList_src = ((AS::OperInstr *)cur->head)->src;
			tempList_dst = ((AS::OperInstr *)cur->head)->dst;
			break;
		case AS::Instr::MOVE:
			tempList_src = ((AS::MoveInstr *)cur->head)->src;
			tempList_dst = ((AS::MoveInstr *)cur->head)->dst;
			break;
	}

	while (il && il->tail != cur)
		il = il->tail;
	AS::InstrList *prefetch = fprefetch(tempList_src, frame, destmap);
	if (prefetch){
		il->tail = prefetch;
		AS::InstrList::Splice(prefetch, cur);
	}

	AS::InstrList *restore = frestore(tempList_dst, frame, destmap);
	if (restore){
		AS::InstrList::Splice(restore, cur->tail);
		cur->tail = restore;
	}
}

int StackVarList::get(TEMP::Temp *t)
{
	StackVarList *cur = this->next;
	while (cur && cur->tmp != t)
		cur = cur->next;
	if (cur)
		return cur->offset;
	return 0;
}

int StackVarList::set(TEMP::Temp *t, F::Frame *f, int off)
{
	if (!off){
		f->s_offset -= F::wordsize;
		off = -f->s_offset;
	}

	StackVarList *cur = this;
	while (cur->next)
		cur = cur->next;
	cur->next = new StackVarList(t, off);
	return off;
}

} // namespace RA
