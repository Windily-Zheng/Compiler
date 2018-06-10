#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "util.h"
#include "errormsg.h"
#include "table.h"
#include "symbol.h"
#include "types.h"
#include "temp.h"
#include "absyn.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "translate.h"
#include "env.h"
#include "semant.h"
#include "canon.h"
#include "codegen.h"

#define MAXINSTRLEN 100

static Temp_temp munchExp(T_exp e);
static void munchStm(T_stm s);
static Temp_tempList munchArgs(int i, T_expList args);
static void callAux(T_expList args);
static Temp_temp getEax(void);
static Temp_temp getEdx(void);
static Temp_temp getEcx(void);
static Temp_temp getEbx(void);
static Temp_temp getEsi(void);
static Temp_temp getEdi(void);
static Temp_tempList calldefs(void);



static Temp_tempList calldefs(void) {
	static Temp_tempList cd = NULL;
	if (cd == NULL) {
		cd = Temp_TempList(getEax(), Temp_TempList(getEdx(), Temp_TempList(getEbx(),
			Temp_TempList(getEcx(), Temp_TempList(getEsi(), Temp_TempList(getEdi(), NULL))))));
	}
	return cd;
}

static Temp_temp getEax(void) {
	Temp_tempList regs = F_registers();
	return regs->head;
}

static Temp_temp getEdx(void) {
	Temp_tempList regs = F_registers();
	return regs->tail->head;
}

static Temp_temp getEbx(void) {
	Temp_tempList regs = F_registers();
	return regs->tail->tail->head;
}

static Temp_temp getEcx(void) {
	Temp_tempList regs = F_registers();
	return regs->tail->tail->tail->head;
}

static Temp_temp getEsi(void) {
	Temp_tempList regs = F_registers();
	return regs->tail->tail->tail->tail->head;
}

static Temp_temp getEdi(void) {
	Temp_tempList regs = F_registers();
	return regs->tail->tail->tail->tail->tail->head;
}

static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst) {
	if (last != NULL) {
		last->tail = AS_InstrList(inst, NULL);
		last = last->tail;
	}
	else {
		last = iList = AS_InstrList(inst, NULL);
	}
}

static void munchStm(T_stm s) {
	assert(s != NULL);
	switch (s->kind) {
	case T_MOVE: {
		if (s->u.MOVE.dst->kind == T_TEMP
			&& s->u.MOVE.src->kind == T_MEM
			&& s->u.MOVE.src->u.MEM->kind == T_BINOP
			&& s->u.MOVE.src->u.MEM->u.BINOP.op == T_plus
			&& s->u.MOVE.src->u.MEM->u.BINOP.right->kind == T_CONST) {
			Temp_temp r1, r2;
			int con;
			char buf[MAXINSTRLEN];
			r1 = s->u.MOVE.dst->u.TEMP;
			r2 = munchExp(s->u.MOVE.src->u.MEM->u.BINOP.left);
			con = s->u.MOVE.src->u.MEM->u.BINOP.right->u.CONST;
			if (con < 0) {
				con = -con;
				sprintf(buf, "mov `d0, [`s0-%d]\n", con);
			}
			else {
				sprintf(buf, "mov `d0, [`s0+%d]\n", con);
			}
			emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
				Temp_TempList(r2, NULL), NULL));
			return;
		}
		else if (s->u.MOVE.dst->kind == T_TEMP
			&& s->u.MOVE.src->kind == T_MEM
			&& s->u.MOVE.src->u.MEM->kind == T_BINOP
			&& s->u.MOVE.src->u.MEM->u.BINOP.op == T_plus
			&& s->u.MOVE.src->u.MEM->u.BINOP.left->kind == T_CONST) {
			Temp_temp r1, r2;
			int con;
			char buf[MAXINSTRLEN];
			r1 = s->u.MOVE.dst->u.TEMP;
			r2 = munchExp(s->u.MOVE.src->u.MEM->u.BINOP.right);
			con = s->u.MOVE.src->u.MEM->u.BINOP.left->u.CONST;
			if (con < 0) {
				con = -con;
				sprintf(buf, "mov `d0, [`s0-%d]\n", con);
			}
			else {
				sprintf(buf, "mov `d0, [`s0+%d]\n", con);
			}
			emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
				Temp_TempList(r2, NULL), NULL));
			return;
		}
		else if (s->u.MOVE.dst->kind == T_TEMP
			&& s->u.MOVE.src->kind == T_MEM) {
			Temp_temp r1, r2;
			r1 = s->u.MOVE.dst->u.TEMP;
			r2 = munchExp(s->u.MOVE.src->u.MEM);
			emit(AS_Oper("mov `d0, [`s0]\n", Temp_TempList(r1, NULL),
				Temp_TempList(r2, NULL), NULL));
			return;
		}
		else if (s->u.MOVE.dst->kind == T_TEMP
			&& s->u.MOVE.src->kind == T_CONST) {
			Temp_temp r1;
			int con;
			char buf[MAXINSTRLEN];
			r1 = s->u.MOVE.dst->u.TEMP;
			con = s->u.MOVE.src->u.CONST;
			sprintf(buf, "mov `d0, %d\n", con);
			emit(AS_Oper(String(buf), Temp_TempList(r1, NULL), NULL, NULL));
			return;
		}
		else if (s->u.MOVE.dst->kind == T_TEMP) {
			Temp_temp r1, r2;
			r1 = s->u.MOVE.dst->u.TEMP;
			r2 = munchExp(s->u.MOVE.src);
			emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
				Temp_TempList(r2, NULL)));
			return;
		}
		else if (s->u.MOVE.dst->kind == T_MEM
			&& s->u.MOVE.dst->u.MEM->kind == T_BINOP
			&& s->u.MOVE.dst->u.MEM->u.BINOP.op == T_plus
			&& s->u.MOVE.dst->u.MEM->u.BINOP.right->kind == T_CONST) {
			Temp_temp r1, r2;
			char buf[MAXINSTRLEN];
			int con;
			r1 = munchExp(s->u.MOVE.dst->u.MEM->u.BINOP.left);
			r2 = munchExp(s->u.MOVE.src);
			con = s->u.MOVE.dst->u.MEM->u.BINOP.right->u.CONST;
			if (con < 0) {
				con = -con;
				sprintf(buf, "mov [`s0-%d], `s1\n", con);
			}
			else {
				sprintf(buf, "mov [`s0+%d], `s1\n", con);
			}
			emit(AS_Oper(String(buf), NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			return;
		}
		else if (s->u.MOVE.dst->kind == T_MEM
			&& s->u.MOVE.dst->u.MEM->kind == T_BINOP
			&& s->u.MOVE.dst->u.MEM->u.BINOP.op == T_plus
			&& s->u.MOVE.dst->u.MEM->u.BINOP.left->kind == T_CONST) {
			Temp_temp r1, r2;
			char buf[MAXINSTRLEN];
			int con;
			r1 = munchExp(s->u.MOVE.dst->u.MEM->u.BINOP.right);
			r2 = munchExp(s->u.MOVE.src);
			con = s->u.MOVE.dst->u.MEM->u.BINOP.left->u.CONST;
			if (con < 0) {
				con = -con;
				sprintf(buf, "mov [`s0-%d], `s1\n", con);
			}
			else {
				sprintf(buf, "mov [`s0+%d], `s1\n", con);
			}
			emit(AS_Oper(String(buf), NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			return;
		}
		else if (s->u.MOVE.dst->kind == T_MEM) {
			Temp_temp r1, r2;
			r1 = munchExp(s->u.MOVE.dst->u.MEM);
			r2 = munchExp(s->u.MOVE.src);
			emit(AS_Oper("mov [`s0], `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			return;
		}
		else {
			assert(0);
		}
	}
	case T_EXP: {
		munchExp(s->u.EXP);
		return;
	}
	case T_LABEL: {
		char buf[MAXINSTRLEN];
		sprintf(buf, "%s:\n", S_name(s->u.LABEL));
		emit(AS_Label(String(buf), s->u.LABEL));
		return;
	}
	case T_JUMP: {
		char buf[MAXINSTRLEN];
		Temp_label lab;
		assert(s->u.JUMP.exp->kind == T_NAME);
		lab = s->u.JUMP.exp->u.NAME;
		sprintf(buf, "jmp near %s\n", S_name(lab));
		emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(Temp_LabelList(lab, NULL))));
		return;
	}
	case T_CJUMP: {
		char buf[MAXINSTRLEN];
		Temp_temp r1, r2;
		Temp_label lab;
		Temp_labelList labs;
		lab = s->u.CJUMP.true;
		labs = Temp_LabelList(s->u.CJUMP.true, Temp_LabelList(s->u.CJUMP.false, NULL));
		r1 = munchExp(s->u.CJUMP.left);
		r2 = munchExp(s->u.CJUMP.right);
		switch (s->u.CJUMP.op) {
		case T_eq: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "je near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_ne: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jne near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_lt: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jl near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_le: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jle near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_gt: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jg near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_ge: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jge near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_ult: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jb near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_ule: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jbe near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_ugt: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "ja near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		case T_uge: {
			emit(AS_Oper("cmp `s0, `s1\n", NULL, Temp_TempList(r1,
				Temp_TempList(r2, NULL)), NULL));
			sprintf(buf, "jae near %s\n", S_name(lab));
			emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(labs)));
			return;
		}
		default: {
			assert(0);
		}
		}
	}
	default: {
		assert(0);
	}
	}
}


static Temp_temp munchExp(T_exp e) {
	char buf[MAXINSTRLEN];
	assert(e != NULL);
	switch (e->kind) {
	case T_MEM: {
		if (e->u.MEM->kind == T_BINOP
			&& e->u.MEM->u.BINOP.op == T_plus
			&& e->u.MEM->u.BINOP.right->kind == T_CONST) {
			Temp_temp r1, r2;
			int con;
			r1 = Temp_newtemp();
			r2 = munchExp(e->u.MEM->u.BINOP.left);
			con = e->u.MEM->u.BINOP.right->u.CONST;
			if (con < 0) {
				con = -con;
				sprintf(buf, "mov `d0, [`s0-%d]\n", con);
			}
			else {
				sprintf(buf, "mov `d0, [`s0+%d]\n", con);
			}
			emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
				Temp_TempList(r2, NULL), NULL));
			return r1;
		}
		else if (e->u.MEM->kind == T_BINOP
			&& e->u.MEM->u.BINOP.op == T_plus
			&& e->u.MEM->u.BINOP.left->kind == T_CONST) {
			Temp_temp r1, r2;
			int con;
			r1 = Temp_newtemp();
			r2 = munchExp(e->u.MEM->u.BINOP.right);
			con = e->u.MEM->u.BINOP.left->u.CONST;
			if (con < 0) {
				con = -con;
				sprintf(buf, "mov `d0, [`s0-%d]\n", con);
			}
			else {
				sprintf(buf, "mov `d0, [`s0+%d]\n", con);
			}
			emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
				Temp_TempList(r2, NULL), NULL));
			return r1;
		}
		else {
			Temp_temp r1, r2;
			r1 = Temp_newtemp();
			r2 = munchExp(e->u.MEM);
			emit(AS_Oper("mov `d0, [`s0]\n", Temp_TempList(r1, NULL),
				Temp_TempList(r2, NULL), NULL));
			return r1;
		}
	}
	case T_TEMP: {
		return e->u.TEMP;
	}
	case T_CONST: {
		int con;
		Temp_temp r1;
		con = e->u.CONST;
		r1 = Temp_newtemp();
		sprintf(buf, "mov `d0, %d\n", con);
		emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
			NULL, NULL));
		return r1;
	}
	case T_CALL: {
		Temp_tempList l;
		Temp_label lab;
		assert(e->u.CALL.fun->kind == T_NAME);
		l = munchArgs(0, e->u.CALL.args);
		lab = e->u.CALL.fun->u.NAME;
		sprintf(buf, "call %s\n", S_name(lab));
		emit(AS_Oper(String(buf), calldefs(), l, NULL));
		callAux(e->u.CALL.args);
		return F_RV();
	}
	case T_NAME: {
		Temp_temp r1;
		r1 = Temp_newtemp();
		sprintf(buf, "mov `d0,  %s\n", S_name(e->u.NAME));
		emit(AS_Oper(String(buf), Temp_TempList(r1, NULL), NULL, NULL));
		return r1;
	}
	case T_BINOP: {
		switch (e->u.BINOP.op) {
		case T_plus: {
			Temp_temp r1, r2, r3;
			int con;
			if (e->u.BINOP.right->kind == T_CONST&&e->u.BINOP.right->u.CONST >= 0) {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				con = e->u.BINOP.right->u.CONST;
				sprintf(buf, "add `d0, %d\n", con);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
					Temp_TempList(r1, NULL), NULL));
				return r1;
			}
			else if (e->u.BINOP.right->kind == T_CONST&&e->u.BINOP.right->u.CONST<0) {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				con = e->u.BINOP.right->u.CONST;
				sprintf(buf, "sub `d0, %d\n", (-con));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
					Temp_TempList(r1, NULL), NULL));
				return r1;
			}
			else if (e->u.BINOP.left->kind == T_CONST&&e->u.BINOP.left->u.CONST >= 0) {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.right);
				con = e->u.BINOP.left->u.CONST;
				sprintf(buf, "add `d0, %d\n", con);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
					Temp_TempList(r1, NULL), NULL));
				return r1;
			}
			else if (e->u.BINOP.left->kind == T_CONST&&e->u.BINOP.left->u.CONST<0) {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.right);
				con = e->u.BINOP.left->u.CONST;
				sprintf(buf, "sub `d0, %d\n", -con);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
					Temp_TempList(r1, NULL), NULL));
				return r1;
			}
			else {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				r3 = munchExp(e->u.BINOP.right);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper("add `d0, `s1\n", Temp_TempList(r1, NULL),
					Temp_TempList(r1, Temp_TempList(r3, NULL)), NULL));
				return r1;
			}
		}
		case T_minus: {
			Temp_temp r1, r2, r3;
			int con;
			if (e->u.BINOP.right->kind == T_CONST&&e->u.BINOP.right->u.CONST >= 0) {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				con = e->u.BINOP.right->u.CONST;
				sprintf(buf, "sub `d0, %d\n", con);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
					Temp_TempList(r1, NULL), NULL));
				return r1;
			}
			else if (e->u.BINOP.right->kind == T_CONST&&e->u.BINOP.right->u.CONST<0) {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				con = e->u.BINOP.right->u.CONST;
				sprintf(buf, "add `d0, %d\n", -con);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper(String(buf), Temp_TempList(r1, NULL),
					Temp_TempList(r1, NULL), NULL));
				return r1;
			}
			else {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				r3 = munchExp(e->u.BINOP.right);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper("sub `d0, `s1\n", Temp_TempList(r1, NULL),
					Temp_TempList(r1, Temp_TempList(r3, NULL)), NULL));
				return r1;
			}
		case T_mul: {
			Temp_temp r1, r2, r3, tm;
			int con;
			if (e->u.BINOP.right->kind == T_CONST) {
				con = e->u.BINOP.right->u.CONST;
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				tm = Temp_newtemp();
				sprintf(buf, "mov `d0, %d\n", con);
				emit(AS_Oper(String(buf), Temp_TempList(tm, NULL), NULL, NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(getEax(), NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper("imul `s1\n", Temp_TempList(getEax(), Temp_TempList(getEdx(), NULL)),
					Temp_TempList(getEax(), Temp_TempList(tm, NULL)), NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(getEax(), NULL)));
				return r1;
			}
			else if (e->u.BINOP.left->kind == T_CONST) {
				con = e->u.BINOP.left->u.CONST;
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.right);
				tm = Temp_newtemp();
				sprintf(buf, "mov `d0, %d\n", con);
				emit(AS_Oper(String(buf), Temp_TempList(tm, NULL), NULL, NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(getEax(), NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper("imul `s1\n", Temp_TempList(getEax(), Temp_TempList(getEdx(), NULL)),
					Temp_TempList(getEax(), Temp_TempList(tm, NULL)), NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(getEax(), NULL)));
				return r1;
			}
			else {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				r3 = munchExp(e->u.BINOP.right);
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(getEax(), NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper("imul `s1\n", Temp_TempList(getEax(), Temp_TempList(getEdx(), NULL)),
					Temp_TempList(getEax(), Temp_TempList(r3, NULL)), NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(getEax(), NULL)));
				return r1;
			}
		}
		case T_div: {
			Temp_temp r1, r2, r3, tm;
			int con;
			if (e->u.BINOP.right->kind == T_CONST) {
				con = e->u.BINOP.right->u.CONST;
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				tm = Temp_newtemp();
				sprintf(buf, "mov `d0, %d\n", con);
				emit(AS_Oper(String(buf), Temp_TempList(tm, NULL), NULL, NULL));
				emit(AS_Oper("mov `d0, 0\n", Temp_TempList(getEdx(), NULL),
					NULL, NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(getEax(), NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper("idiv `s2\n", Temp_TempList(getEax(), Temp_TempList(getEdx(), NULL)),
					Temp_TempList(getEax(), Temp_TempList(getEdx(), Temp_TempList(tm, NULL))), NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(getEax(), NULL)));
				return r1;
			}
			else {
				r1 = Temp_newtemp();
				r2 = munchExp(e->u.BINOP.left);
				r3 = munchExp(e->u.BINOP.right);
				emit(AS_Oper("mov `d0, 0\n", Temp_TempList(getEdx(), NULL),
					NULL, NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(getEax(), NULL),
					Temp_TempList(r2, NULL)));
				emit(AS_Oper("idiv `s2\n", Temp_TempList(getEax(), Temp_TempList(getEdx(), NULL)),
					Temp_TempList(getEax(), Temp_TempList(getEdx(), Temp_TempList(r3, NULL))),
					NULL));
				emit(AS_Move("mov `d0, `s0\n", Temp_TempList(r1, NULL),
					Temp_TempList(getEax(), NULL)));
				return r1;
			}
		}
					/*we omit and or shift xor for now*/
		default: {
			assert(0);
		}
		}
		}
	default: {
		assert(0);
	}
	}
	}
}


static Temp_tempList munchArgs(int i, T_expList args) {
	Temp_temp r1;
	if (args == NULL) {
		return NULL;
	}
	else {
		r1 = munchExp(args->head);
		munchArgs(i + 1, args->tail);
		emit(AS_Oper("push `s0\n", Temp_TempList(F_SP(), NULL), Temp_TempList(r1, Temp_TempList(F_SP(), NULL)), NULL));
		return NULL;
	}
}

static void callAux(T_expList args) {
	T_expList ptr = args;
	int i = 0;
	char buf[MAXINSTRLEN];
	if (args == NULL) {
		return;
	}
	else {
		while (ptr != NULL) {
			i++;
			ptr = ptr->tail;
		}
		i = i * F_wordSize;
		sprintf(buf, "add `d0, %d\n", i);
		emit(AS_Oper(String(buf), Temp_TempList(F_SP(), NULL),
			Temp_TempList(F_SP(), NULL), NULL));
		return;
	}
}

AS_instrList I_codegen(F_frame f, T_stmList stmList) {
	AS_instrList list; T_stmList sl;
	iList = last = NULL;
	for (sl = stmList; sl != NULL; sl = sl->tail) {
		munchStm(sl->head);
	}
	list = iList;
	return list;
}




