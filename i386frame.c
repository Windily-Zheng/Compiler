#include "util.h"
#include "temp.h"
#include "frame.h"
#include "assem.h"

#define DB 0

struct F_frame_ {
	F_accessList formals, locals;
	int local_count;
	F_access lastEscape;
	Temp_label label;
};

struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};

const int F_wordSize = 4;		//i386: 32bit
static const int F_keep = 4;	//number of parameters kept in regs;

Temp_map F_tempMap = NULL;
Temp_map F_TempMap()
{
	if (F_tempMap)
	{
		return  F_tempMap;
	}
	else
	{
		F_tempMap = Temp_empty();
		return  F_tempMap;
	}
}

static F_access InFrame(int offset);
static F_access InReg(Temp_temp reg);

F_accessList F_AccessList(F_access head, F_accessList tail) {
	F_accessList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l;
}

Temp_temp F_FP() {
	static Temp_temp t = NULL;
	if (!t)
		t = Temp_newtemp();
	return t;
}

Temp_temp F_RV() {
	static Temp_temp t = NULL;
	if (!t)
		t = Temp_newtemp();
	return t;
}


Temp_tempList F_registers(void) {
	static Temp_tempList regs = NULL;
	if (regs == NULL) {
		Temp_map regmap = Temp_empty();
		string p[8];
		int i;
		p[0] = "eax"; p[1] = "edx"; p[2] = "ebx"; p[3] = "ecx"; p[4] = "esi"; p[5] = "edi"; p[6] = "ebp"; p[7] = "esp";
		for (i = 1; i <= 8; i++) {
			regs = Temp_TempList(Temp_newtemp(), regs);
			Temp_enter(regmap, regs->head, p[8 - i]);
		}
		F_tempMap = Temp_layerMap(regmap, Temp_name());
}
	return regs;
}

AS_instrList F_useSpill(F_frame f, F_access spill, Temp_temp temp) {
	AS_instr in;
	AS_instrList ins;
	char buf[100];
	int con;

	assert(spill->kind == inFrame);
	con = spill->u.offset;
	if (con < 0) {
		con = -con;
		sprintf(buf, "mov `d0, [`s0-%d]\n", con);
	}
	else {
		sprintf(buf, "mov `d0, [`s0+%d]\n", con);
	}
	in = AS_Oper(String(buf), Temp_TempList(temp, NULL), Temp_TempList(F_FP(), NULL), NULL);
	ins = AS_InstrList(in, NULL);
	return ins;
}

AS_instrList F_defineSpill(F_frame f, F_access spill, Temp_temp temp) {
	AS_instr in;
	AS_instrList ins;
	char buf[100];
	int con;

	assert(spill->kind == inFrame);
	con = spill->u.offset;
	if (con < 0) {
		con = -con;
		sprintf(buf, "mov [`s0-%d], `s1\n", con);
	}
	else {
		sprintf(buf, "mov [`s0+%d], `s1\n", con);
	}
	in = AS_Oper(String(buf), NULL, Temp_TempList(F_FP(), Temp_TempList(temp, NULL)), NULL);
	ins = AS_InstrList(in, NULL);
	return ins;
}


int F_getOffset(F_frame f) {
	F_access acc = f->lastEscape;

	if (acc == NULL) {
		return 0;
	}
	else {
		assert(acc->kind == inFrame);
		return acc->u.offset;
	}
}

static Temp_temp F_eax() {
	Temp_tempList regs = F_registers();

	return regs->head;
}

static Temp_temp F_edx() {
	Temp_tempList regs = F_registers();

	return regs->tail->head;
}

static Temp_temp F_ebx() {
	Temp_tempList regs = F_registers();

	return regs->tail->tail->head;
}

static Temp_temp F_ecx() {
	Temp_tempList regs = F_registers();

	return regs->tail->tail->tail->head;
}

static Temp_temp F_esi() {
	Temp_tempList regs = F_registers();

	return regs->tail->tail->tail->tail->head;
}

static Temp_temp F_edi() {
	Temp_tempList regs = F_registers();

	return regs->tail->tail->tail->tail->tail->head;
}

static Temp_temp F_ebp() {
	Temp_tempList regs = F_registers();

	return regs->tail->tail->tail->tail->tail->tail->head;
}

static Temp_temp F_esp() {
	Temp_tempList regs = F_registers();

	return regs->tail->tail->tail->tail->tail->tail->tail->head;
}



Temp_temp F_SP(void) {
	Temp_tempList regs = F_registers(), ptr;
	ptr = regs;
	while (ptr->tail != NULL) {
		ptr = ptr->tail;
	}
	return ptr->head;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
#if DB
	printf("LB: %s\n", Temp_labelstring(name));
#endif
	F_frame fr = checked_malloc(sizeof(*fr));
	fr->label = name;		
	fr->formals = NULL;
	fr->locals = NULL;
	fr->local_count = 0;

	F_accessList head = NULL, tail = NULL;
	int rn = 0, fn = 0;
	U_boolList ptr;
	for (ptr = formals; ptr; ptr = ptr->tail) {
		F_access ac = NULL;
		if (rn < F_keep && !(ptr->head)) {
			ac = InReg(Temp_newtemp());	
			rn++;
		} else {
			fn++;
			ac = InFrame((fn+1)*F_wordSize);	//one word for return address
		}

		if (head) {
			tail->tail = F_AccessList(ac, NULL);
			tail = tail->tail;
		} else {
			head = F_AccessList(ac, NULL);
			tail = head;
		}
	}
	fr->formals = head;

	return fr;
}

Temp_label F_name(F_frame f) {
	return f->label;
}

F_accessList F_formals(F_frame f) {
	return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
	f->local_count++;
	if (escape) 
		return InFrame(-F_wordSize * (f->local_count));
	else 
		return InReg(Temp_newtemp());
}

T_exp F_Exp(F_access acc, T_exp framePtr) {
	if (acc->kind == inReg)
		return T_Temp(acc->u.reg);
	else
		return T_Mem(T_Binop(T_plus, T_Const(acc->u.offset), framePtr));
}

T_exp F_externalCall(string s, T_expList args) {
	return T_Call(T_Name(Temp_namedlabel(s)), args);
	//todo
}

static F_access InFrame(int offset) {
	F_access ac = checked_malloc(sizeof(*ac));
	ac->kind = inFrame;
	ac->u.offset = offset;
#if DB
	printf("\t\tinFrame: %d\n", offset);
#endif
	return ac;
}

static F_access InReg(Temp_temp reg) {
	F_access ac = checked_malloc(sizeof(*ac));
	ac->kind = inReg;
	ac->u.reg = reg;
#if DB
	printf("\t\tinReg\n");
#endif
	return ac;
}


/************** fragment ******************/
F_frag F_StringFrag(Temp_label label, string str) {
	F_frag frag = checked_malloc(sizeof(*frag));
	frag->kind = F_stringFrag;
	frag->u.stringg.label = label;
	frag->u.stringg.str = str;
	return frag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
	F_frag frag = checked_malloc(sizeof(*frag));
	frag->kind = F_procFrag;
	frag->u.proc.body = body;
	frag->u.proc.frame = frame;
	return frag;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
	F_fragList fl = checked_malloc(sizeof(*fl));
	fl->head = head;
	fl->tail = tail;
	return fl;
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
	// todo:
	return stm;
}


#include "tree.h"
#include "printtree.h"
void F_printFragList(F_fragList fl) {
	for (; fl; fl = fl->tail) {
		F_frag f = fl->head;
		if (f->kind == F_stringFrag)
			printf("string: %s\n", f->u.stringg.str);
		else
			printStmList(stdout, T_StmList(f->u.proc.body, NULL));
	}
}

