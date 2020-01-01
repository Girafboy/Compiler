#include "tiger/regalloc/regalloc.h"

namespace RA
{
static TAB::Table<TEMP::Temp, int> *tempmap = new TAB::Table<TEMP::Temp, int>();

Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
	// TODO: Put your codes here (lab6).
	TEMP::Map *destmap = TEMP::Map::Empty();
	AS::InstrList *cur = il->tail;
	while (cur){
		mapMapping(il, cur, f, destmap);
		cur = cur->tail;
	}

	TEMP::Map *resmap = TEMP::Map::LayerMap(F::RegMap(), destmap);
	return Result(resmap, il);
}

AS::InstrList *fprefetch(TEMP::TempList *templist, F::Frame *frame, TEMP::Map *destmap){
	static char str[256];

	int count = 0;
	AS::InstrList *prefetch = NULL;

	for(; templist && templist->head; templist = templist->tail){
		if (F::RegMap()->Look(templist->head))
			continue;
			
		int *offset = tempmap->Look(templist->head);
		if (!offset){
			frame->s_offset -= F::wordsize;
			offset = new int(frame->s_offset);
		}else
			templist->head = TEMP::Temp::NewTemp();
		tempmap->Enter(templist->head, offset);

		sprintf(str, "movq (%s_framesize-%d)(`s0), `d0", frame->label->Name().c_str(), -*offset);
		TEMP::Temp *r = NULL;
		if(count == 0){
			r = F::R10();
			destmap->Enter(templist->head, new std::string("%r10"));
		}	
		if(count == 1){
			r = F::R11();
			destmap->Enter(templist->head, new std::string("%r11"));
		}	
		prefetch = new AS::InstrList(new AS::OperInstr(str, new TEMP::TempList(r, NULL), new TEMP::TempList(F::SP(), NULL), NULL), prefetch);
		count++;
	}
	return prefetch;
}

AS::InstrList *frestore(TEMP::TempList *templist, F::Frame *frame, TEMP::Map *destmap){
	static char str[256];

	int count = 0;
	AS::InstrList *restore = NULL;

	for(; templist && templist->head; templist = templist->tail){
		if (F::RegMap()->Look(templist->head))
			continue;

		int *offset = tempmap->Look(templist->head);
		if (!offset){
			frame->s_offset -= F::wordsize;
			offset = new int(frame->s_offset);
		}else
			templist->head = TEMP::Temp::NewTemp();
		tempmap->Enter(templist->head, offset);

		sprintf(str, "movq `s0, (%s_framesize-%d)(`d0)", frame->label->Name().c_str(), -*offset);
		TEMP::Temp *r = NULL;
		if(count == 0){
			r = F::R12();
			destmap->Enter(templist->head, new std::string("%r12"));
		}	
		if(count == 1){
			r = F::R13();
			destmap->Enter(templist->head, new std::string("%r13"));
		}	
		restore = new AS::InstrList(new AS::OperInstr(str, new TEMP::TempList(F::SP(), NULL), new TEMP::TempList(r, NULL), NULL), restore);
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

} // namespace RA
