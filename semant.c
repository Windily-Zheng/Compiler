#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "types.h"
#include "absyn.h"
#include "translate.h"
#include "env.h"
#include "semant.h"

#define DB 1


struct varRecord{
	
	int kind;
	S_symbol symbol;
	int modified;
	union {
		int int_value;
		string str_value;
	}u;
};

struct varRecord var_arr[20];

int var_num = 0;
int find = -1;

struct expty {
	Tr_exp exp;
	Ty_ty ty;
};

struct expty expTy(Tr_exp exp, Ty_ty ty) {
	struct expty e;
	e.exp = exp;
	e.ty = ty;
	return e;
}

static struct expty transVar(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_var v);
static struct expty transExp(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_exp a);
static Tr_exp transDec(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_dec d);
static Ty_ty transTy(S_table tenv, A_ty a);


static Ty_fieldList transFieldList(S_table tenv, A_fieldList a); 
static Ty_tyList transTyList(S_table tenv, A_fieldList a);

static Ty_ty actual_ty(Ty_ty t); 
static bool cmp_ty(Ty_ty a, Ty_ty b); 

static U_boolList makeFormalList(A_fieldList params);


//there is no unique items in set
struct set_ {
	S_symbol s[1000];
	int pos;
};
typedef struct set_ *set;

static set set_init();
static void set_reset(set s);
static bool set_push(set s, S_symbol x);

static set s;

//layer of loop
static int loop;


F_fragList SEM_transProg(A_exp exp) {
	assert(exp);
	S_table tenv = E_base_tenv();
	S_table venv = E_base_venv();
	loop = 0;
	s = set_init();
	transExp(Tr_outermost(), NULL, venv, tenv, exp);
#if DB
	F_printFragList(Tr_getResult());
#endif
	return Tr_getResult();
}

struct expty transVar(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_var v) {
	assert(level);
	int i;
	switch (v->kind) {
		case A_simpleVar: {
		
			
			for (i = 0; i < var_num; i++)
			{
				if ( var_arr[i].symbol == v->u.simple && var_arr[i].modified == 0)
				{
					find = i;

					if (var_arr[i].kind == 1)
					{
						return expTy(Tr_intExp(var_arr[i].u.int_value), Ty_Int());
					}
						
					else
						return expTy(Tr_intExp(var_arr[i].u.int_value), Ty_String());
				}
			}

			E_enventry e = S_look(venv, v->u.simple);
			if (e && e->kind == E_varEntry) 
				return expTy(Tr_simpleVar(e->u.var.access, level), actual_ty(e->u.var.ty));
			else {
				EM_error(v->pos, "undefined variable '%s'", S_name(v->u.simple));
				return expTy(Tr_null(), Ty_Int());
			}
		}
		case A_fieldVar: {
			struct expty exp = transVar(level, breakk, venv, tenv, v->u.field.var); 
			Ty_ty t = exp.ty;
			if (t->kind != Ty_record) {
				EM_error(v->pos, "variable not record");
			} else {
				Ty_fieldList fl = t->u.record;
				int i;
				for (fl = t->u.record, i = 0; fl; fl = fl->tail, i++)
					if (fl->head->name == v->u.field.sym) 
						return expTy(Tr_fieldVar(exp.exp, i), actual_ty(fl->head->ty));
				EM_error(v->pos, "'%s' was not a member", S_name(v->u.field.sym));
			}
			return expTy(Tr_null(), Ty_Int());
		}
		case A_subscriptVar: {
			struct expty var = transVar(level, breakk, venv, tenv, v->u.subscript.var); 
			if (var.ty->kind != Ty_array) 
				EM_error(v->pos, "variable not a array");
			else {
				struct expty exp = transExp(level, breakk, venv, tenv, v->u.subscript.exp); 
				if (exp.ty->kind != Ty_int) 
					EM_error(v->pos, "Subscript was not an integer");
				else 
					return expTy(Tr_subscriptVar(var.exp, Tr_null()), actual_ty(var.ty->u.array));
			}
			return expTy(Tr_null(), actual_ty(var.ty->u.array));
		}
	}
	assert(0);
}

struct expty transExp(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_exp a) {
	assert(level);
	switch (a->kind) {
		case A_varExp: 
			return transVar(level, breakk, venv, tenv, a->u.var);	
		case A_nilExp: 
			return expTy(Tr_nilExp(), Ty_Nil());	
		case A_intExp: 
			return expTy(Tr_intExp(a->u.intt), Ty_Int());	
		case A_stringExp: 
			return expTy(Tr_stringExp(a->u.stringg), Ty_String());	
		case A_callExp: {
			E_enventry fun = S_look(venv, a->u.call.func);
			if (!fun) {
				EM_error(a->pos, "undeclared function '%s'", S_name(a->u.call.func));
				return expTy(Tr_null(), Ty_Int());
			} else if (fun->kind == E_varEntry) {
				EM_error(a->pos, "'%s' was a variable", S_name(a->u.call.func));
				return expTy(Tr_null(), fun->u.var.ty);
			}

			Ty_tyList tl = fun->u.fun.formals;
			A_expList el = a->u.call.args;
			Tr_expList h = NULL, t = NULL;
			for (; tl && el; tl = tl->tail, el = el->tail) {
				struct expty exp = transExp(level, breakk, venv, tenv, el->head);
				if (!cmp_ty(tl->head, exp.ty)) {
					EM_error(a->pos, "argument type mismatch");
					return expTy(Tr_null(), fun->u.var.ty);
				}
				if (h) {
					t->tail = Tr_ExpList(exp.exp, NULL);
					t = t->tail;
				} else {
					h = Tr_ExpList(exp.exp, NULL);
					t = h;
				}
			}
			if (tl) 
				EM_error(a->pos, "too few arguments");
			else if (el)
				EM_error(a->pos, "too many arguments");
			else
				return expTy(Tr_callExp(fun->u.fun.label, h, fun->u.fun.level, level), fun->u.fun.result);
			return expTy(Tr_null(), fun->u.fun.result);
		}
		case A_opExp: {
			A_oper oper = a->u.op.oper;
			struct expty left = transExp(level, breakk, venv, tenv, a->u.op.left);
			struct expty right = transExp(level, breakk, venv, tenv, a->u.op.right);
			switch (oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp: {
					if (left.ty->kind != Ty_int) 
						EM_error(a->u.op.left->pos, "integer required");
					else if (right.ty->kind != Ty_int) 
						EM_error(a->u.op.right->pos, "integer required");
					else
						return expTy(Tr_arithExp(a->u.op.oper, left.exp, right.exp), Ty_Int());
					return expTy(Tr_null(), Ty_Int());
				} 
				case A_eqOp:
				case A_neqOp: {
					Tr_exp exp = Tr_null();
					if (!cmp_ty(left.ty, right.ty))
						EM_error(a->u.op.left->pos, "comparison type mismatch");
					else if (left.ty->kind == Ty_int)
						exp = Tr_relExp(a->u.op.oper, left.exp, right.exp);
					else if (left.ty->kind == Ty_string)
						exp = Tr_eqStrExp(a->u.op.oper, left.exp, right.exp);
					else if (left.ty->kind == Ty_array)
						exp = Tr_eqRefExp(a->u.op.oper, left.exp, right.exp);
					else if (left.ty->kind == Ty_record)
						exp = Tr_eqRefExp(a->u.op.oper, left.exp, right.exp);
					else
						EM_error(a->u.op.left->pos, "unexpected type in comparsion");

					return expTy(exp, Ty_Int());
				}
				case A_ltOp:
				case A_leOp:
				case A_gtOp:
				case A_geOp: {
					if (left.ty->kind != Ty_int) 
						EM_error(a->u.op.left->pos, "integer required");
					else if (right.ty->kind != Ty_int) 
						EM_error(a->u.op.right->pos, "integer required");
					else
						return expTy(Tr_relExp(a->u.op.oper, left.exp, right.exp), Ty_Int());
					return expTy(Tr_null(), Ty_Int());
				}
			} 
		}
		case A_recordExp: {
			Ty_ty t = actual_ty(S_look(tenv, a->u.record.typ));
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.record.typ));
				return expTy(Tr_null(), Ty_Int());
			} else if (t->kind != Ty_record) {
				EM_error(a->pos, "'%s' was not a record type", S_name(a->u.record.typ));
				return expTy(Tr_null(), Ty_Record(NULL));
			}

			Ty_fieldList ti = t->u.record;
			A_efieldList ei = a->u.record.fields;
			Tr_expList el = NULL;
			int num = 0;
			for (; ti && ei; ti = ti->tail, ei = ei->tail) {
				if (ti->head->name != ei->head->name) {
					EM_error(a->pos, "need member '%s' but '%s'", S_name(ti->head->name), S_name(ei->head->name));
					continue;
				}
				struct expty exp = transExp(level, breakk, venv, tenv, ei->head->exp);
				el = Tr_ExpList(exp.exp, el);
				num++;
				if (!cmp_ty(ti->head->ty, exp.ty))
					EM_error(a->pos, "member '%s' type mismatch", S_name(ti->head->name));
			}

			if (ti) 
				EM_error(a->pos, "too few initializers for '%s'", S_name(a->u.record.typ));
			else if (ei)
				EM_error(a->pos, "too many initializers for '%s'", S_name(a->u.record.typ));

			if (num)
				return expTy(Tr_recordExp(num, el), t);
			else
				return expTy(Tr_null(), Ty_Record(NULL));
		}
		case A_seqExp: {
			if (a->u.seq == NULL)
				return expTy(Tr_null(), Ty_Void());
			A_expList al;
			struct expty et;
			Tr_expList tl = NULL;
			for (al = a->u.seq; al; al =al->tail) {
				et = transExp(level, breakk, venv, tenv,al->head);
				tl = Tr_ExpList(et.exp,tl);	
			}
			return expTy(Tr_seqExp(tl), et.ty);
		}
		case A_assignExp: {
			struct expty var = transVar(level, breakk, venv, tenv, a->u.assign.var);
			if (find != -1)
			{
				var_arr[find].modified = 1;
				find = -1;
				var = transVar(level, breakk, venv, tenv, a->u.assign.var);
			}
			struct expty exp = transExp(level, breakk, venv, tenv, a->u.assign.exp);	
			if (!cmp_ty(var.ty, exp.ty))
				EM_error(a->pos, "assignment type mismatch");
			return expTy(Tr_assignExp(var.exp, exp.exp), Ty_Void());
		}
		case A_ifExp: {
			struct expty t = transExp(level, breakk, venv, tenv, a->u.iff.test);	
			struct expty h = transExp(level, breakk, venv, tenv, a->u.iff.then);	
			if (t.ty->kind != Ty_int)
				EM_error(a->pos, "test exp was not an integer");
			if (a->u.iff.elsee) {
				struct expty e = transExp(level, breakk, venv, tenv, a->u.iff.elsee);
				if (!cmp_ty(h.ty, e.ty) && !(h.ty->kind == Ty_nil && e.ty->kind == Ty_nil))
					EM_error(a->pos, "then-else had different types");
				return expTy(Tr_ifExp(t.exp, h.exp, e.exp), h.ty);
			} else {
				if (h.ty->kind != Ty_void)
					EM_error(a->pos, "if-then shouldn't return a value");
				return expTy(Tr_ifExp(t.exp, h.exp, NULL), Ty_Void());
			}
		}
		case A_whileExp: {
			struct expty t = transExp(level, breakk, venv, tenv, a->u.whilee.test);	
			Tr_exp done = Tr_doneExp();
			loop++;
			struct expty b = transExp(level, done, venv, tenv, a->u.whilee.body);	
			loop--;
			if (t.ty->kind != Ty_int)
				EM_error(a->pos, "while-exp was not an integer");
			if (b.ty->kind != Ty_void)
				EM_error(a->pos, "do-exp shouldn't return a value");
			return expTy(Tr_whileExp(t.exp, b.exp, done), Ty_Void());
		}
		case A_forExp: {
			struct expty loet = transExp(level, breakk, venv, tenv, a->u.forr.lo);
			struct expty hiet = transExp(level, breakk, venv, tenv, a->u.forr.hi);
			Tr_exp done = Tr_doneExp();
			if (loet.ty->kind != Ty_int)
				EM_error(a->pos, "low bound was not an integer");
			if (hiet.ty->kind != Ty_int)
				EM_error(a->pos, "high bound was not an integer");
			
			S_beginScope(venv);
			loop++;
			Tr_access iac = Tr_allocLocal(level, a->u.forr.escape);
			S_enter(venv, a->u.forr.var, E_VarEntry(iac, Ty_Int()));
			struct expty bodyet = transExp(level, done, venv, tenv, a->u.forr.body);
			S_endScope(venv);
			loop--;
			if (bodyet.ty->kind != Ty_void)
				EM_error(a->pos, "body exp shouldn't return a value");
			Tr_exp forexp = Tr_forExp(level, iac, loet.exp, hiet.exp, bodyet.exp, done);
			return expTy(forexp, Ty_Void());
		}
		case A_breakExp: {
			if (!loop) {
				EM_error(a->pos, "break statement not within loop");
				return expTy(Tr_null(), Ty_Void());
			} else
				return expTy(Tr_breakExp(breakk), Ty_Void());
		}
		case A_letExp: {
			struct expty exp;
			A_decList lt;
			S_beginScope(venv);
			S_beginScope(tenv);
			Tr_exp te;
			Tr_expList el = NULL;
			for (lt = a->u.let.decs; lt; lt = lt->tail) {
				te = transDec(level, breakk, venv, tenv, lt->head);
				el = Tr_ExpList(te, el);
			}
			exp = transExp(level, breakk, venv, tenv, a->u.let.body);
			el = Tr_ExpList(exp.exp, el);
			S_endScope(venv);
			S_endScope(tenv);
			return expTy(Tr_seqExp(el), exp.ty);
		}
		case A_arrayExp: {
			Ty_ty t = actual_ty(S_look(tenv, a->u.array.typ));
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.array.typ));
				return expTy(Tr_null(), Ty_Int());
			} else if (t->kind != Ty_array) {
				EM_error(a->pos, "'%s' was not a array type", S_name(a->u.array.typ));
				return expTy(Tr_null(), Ty_Int());
			}
			struct expty z = transExp(level, breakk, venv, tenv, a->u.array.size);	
			struct expty i = transExp(level, breakk, venv, tenv, a->u.array.init);	
			if (z.ty->kind != Ty_int)
				EM_error(a->pos, "array size was not an integer value");
			if (!cmp_ty(i.ty, t->u.array))
				EM_error(a->pos, "array init type mismatch");
			return expTy(Tr_arrayExp(z.exp, i.exp), t);
		}
	}
	assert(0);
}

Tr_exp transDec(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_dec d) {
	assert(level);
	int j, i = var_num;
	switch (d->kind) {
		case A_varDec: {
			struct expty e = transExp(level, breakk, venv, tenv, d->u.var.init);
			Tr_access ac = Tr_allocLocal(level, d->u.var.escape);
			if (d->u.var.typ) {
				Ty_ty t = S_look(tenv, d->u.var.typ);
				if (!t)
					EM_error(d->pos, "undefined type '%s'", S_name(d->u.var.typ));
				else {
					if (!cmp_ty(t, e.ty)) 
						EM_error(d->pos, "var init type mismatch");
#if DB
					if (d->u.var.init->kind == A_intExp && var_num < 20)
					{
						for (j = 0; j < var_num; j++)
							if (var_arr[j].symbol == d->u.var.var)
							{
								i = j;
								break;
							}
						if (i == var_num)
							var_num++;
						var_arr[i].modified = 0;
						var_arr[i].kind = 1;
						var_arr[i].symbol = d->u.var.var;
						var_arr[i].u.int_value = d->u.var.init->u.intt;
						printf("var: %s %d\n", S_name(d->u.var.var), d->u.var.init->u.intt);
					}			
					else
						if (d->u.var.init->kind == A_stringExp && var_num < 20)
						{
							for (j = 0; j < var_num; j++)
								if (var_arr[j].symbol == d->u.var.var)
								{
									i = j;
									break;
								}
							if (i == var_num)
								var_num++;
							var_arr[i].modified = 0;
							var_arr[i].kind = 2;
							var_arr[i].symbol = d->u.var.var;
							var_arr[i].u.str_value = d->u.var.init->u.stringg;
							printf("var: %s \"%s\"\n", S_name(d->u.var.var), d->u.var.init->u.stringg);
					    }
					else
						printf("var: %s escape: %d\n", S_name(d->u.var.var), d->u.var.escape);
#endif
					S_enter(venv, d->u.var.var, E_VarEntry(ac, t));
					return Tr_assignExp(Tr_simpleVar(ac, level), e.exp);
				}
			}
			if (e.ty == Ty_Void())
				EM_error(d->pos, "initialize with no value");
			else if (e.ty == Ty_Nil())
				EM_error(d->pos, "'%s' is not a record", S_name(d->u.var.var));
#if DB
			if (d->u.var.init->kind == A_intExp && var_num < 20)
			{
				for (j = 0; j < var_num; j++)
					if (var_arr[j].symbol == d->u.var.var)
					{
						i = j;
						break;
					}
				if (i == var_num)
					var_num++;
				var_arr[i].modified = 0;
				var_arr[i].kind = 1;
				var_arr[i].symbol = d->u.var.var;
				var_arr[i].u.int_value = d->u.var.init->u.intt;
				printf("var: %s %d\n", S_name(d->u.var.var), d->u.var.init->u.intt);
			}
			else
				if (d->u.var.init->kind == A_stringExp && var_num < 20)
				{
					for (j = 0; j < var_num; j++)
						if (var_arr[j].symbol == d->u.var.var)
						{
							i = j;
							break;
						}
					if (i == var_num)
						var_num++;
					var_arr[i].modified = 0;
					var_arr[i].kind = 2;
					var_arr[i].symbol = d->u.var.var;
					var_arr[i].u.str_value = d->u.var.init->u.stringg;
					printf("var: %s \"%s\"\n", S_name(d->u.var.var), d->u.var.init->u.stringg);
				}
				else
					printf("var: %s escape: %d\n", S_name(d->u.var.var), d->u.var.escape);
#endif
			S_enter(venv, d->u.var.var, E_VarEntry(ac, e.ty));
			return Tr_assignExp(Tr_simpleVar(ac, level), e.exp);
		}

		case A_typeDec: {
			A_nametyList l;
			set_reset(s);
			for (l = d->u.type; l; l = l->tail) {
				if (!set_push(s, l->head->name)) {
					EM_error(d->pos, "redefinition of '%s'", S_name(l->head->name));
					continue;
				}
				Ty_ty t = Ty_Name(l->head->name, NULL);
				S_enter(tenv, l->head->name, t);
			}

			set_reset(s);
			for (l = d->u.type; l; l = l->tail) {
				if (!set_push(s, l->head->name)) 
					continue;
				Ty_ty t = S_look(tenv, l->head->name);
				t->u.name.ty = transTy(tenv, l->head->ty);
			}

			//check recursive definition
			for (l = d->u.type; l; l = l->tail) {
				Ty_ty t = S_look(tenv, l->head->name);
				set_reset(s);
				t = t->u.name.ty;
				while (t && t->kind == Ty_name) {
					if (!set_push(s, t->u.name.sym)) {
						EM_error(d->pos, "illegal recursive definition '%s'", S_name(t->u.name.sym));
						t->u.name.ty = Ty_Int();
						break;
					}
					t = t->u.name.ty;	
					t = t->u.name.ty;	
				}
			}
			return Tr_null();
		}
		case A_functionDec: {
			A_fundecList fl;
			set fs = set_init();
			set_reset(fs);
			for (fl = d->u.function; fl; fl = fl->tail) {
				A_fundec fun = fl->head;
				if (!set_push(fs, fun->name)) {
					EM_error(fun->pos, "redefinition of function '%s'", S_name(fun->name));
					continue;
				}
				Ty_ty re = NULL;
				if (fun->result) {
					re = S_look(tenv, fun->result);
					if (!re) {
						EM_error(d->pos, "function result: undefined type '%s'", S_name(fun->result));
						re = Ty_Int();
					}
				} else
					re = Ty_Void();
				set_reset(s);
				Ty_tyList pa = transTyList(tenv, fun->params);
				Temp_label la = Temp_newlabel();
				Tr_level le = Tr_newLevel(level, la, makeFormalList(fun->params));
				S_enter(venv, fun->name, E_FunEntry(le, la, pa, re));
			}

			set_reset(fs);
			for (fl = d->u.function; fl; fl = fl->tail) {
				A_fundec fun = fl->head;
				if (!set_push(fs, fun->name))
					continue;
				E_enventry ent = S_look(venv, fun->name);
				S_beginScope(venv);
				A_fieldList el = fun->params;
				Tr_accessList al = Tr_formals(ent->u.fun.level);
				set_reset(s);
				for (; el; el = el->tail) {
					if (!set_push(s, el->head->name))
						continue;
					Ty_ty t = S_look(tenv, el->head->typ);
					if (!t)
						t = Ty_Int();
					assert(al);
					S_enter(venv, el->head->name, E_VarEntry(al->head, t));
					al = al->tail;
				}
				struct expty exp = transExp(ent->u.fun.level, breakk, venv, tenv, fun->body);
				Tr_procEntryExit(ent->u.fun.level, exp.exp, al);

				if (ent->u.fun.result->kind == Ty_void && exp.ty->kind != Ty_void)
					EM_error(fun->pos, "procedure '%s' should't return a value", S_name(fun->name));
				else if (!cmp_ty(ent->u.fun.result, exp.ty))
					EM_error(fun->pos, "body result type mismatch");
				S_endScope(venv);
			}
			return Tr_null();
		}
	}//end of switch()
	assert(0);
}

Ty_ty transTy(S_table tenv, A_ty a) {
	switch (a->kind) {
		case A_nameTy: {
			Ty_ty t = S_look(tenv, a->u.name);
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.name));
				return Ty_Int();
			} else
				return Ty_Name(a->u.name, t);
		}
		case A_recordTy:
			set_reset(s);
			return Ty_Record(transFieldList(tenv, a->u.record));
		case A_arrayTy: {
			Ty_ty t = S_look(tenv, a->u.array);
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.array));
				return Ty_Array(Ty_Int());
			} else
				return Ty_Array(S_look(tenv, a->u.array));
		}
	}
	assert(0);
}



Ty_fieldList transFieldList(S_table tenv, A_fieldList a) {
	if (!a)
		return NULL;
	if (!set_push(s, a->head->name)) {
		EM_error(a->head->pos, "redeclaration of '%s'", S_name(a->head->name));
		return transFieldList(tenv, a->tail);
	}
	Ty_ty t = S_look(tenv, a->head->typ);
	if (!t) {
		EM_error(a->head->pos, "undefined type '%s'", S_name(a->head->typ));
		t = Ty_Int();
	}
	return Ty_FieldList(Ty_Field(a->head->name, t), transFieldList(tenv, a->tail));
}

Ty_tyList transTyList(S_table tenv, A_fieldList a) {
	if (!a)
		return NULL;
	if (!set_push(s, a->head->name)) {
		EM_error(a->head->pos, "redeclaration of '%s'", S_name(a->head->name));
		return transTyList(tenv, a->tail);
	}
	Ty_ty t = S_look(tenv, a->head->typ);
	if (!t) {
		EM_error(a->head->pos, "undefined type '%s'", S_name(a->head->typ));
		t = Ty_Int();
	}
	return Ty_TyList(t, transTyList(tenv, a->tail));
}

U_boolList makeFormalList(A_fieldList params) {
	U_boolList head = NULL, tail = NULL;
	while (params) {
		if (head) {
			tail->tail = U_BoolList(params->head->escape, NULL);
			tail = tail->tail;
		} else {
			head = U_BoolList(params->head->escape, NULL);
			tail = head;
		}
		params = params->tail;
	}
	return head;
}


//same type use same Ty_ty
bool cmp_ty(Ty_ty a, Ty_ty b) {
	assert(a&&b);
	a = actual_ty(a);
	b = actual_ty(b);
	if (a == b && a->kind != Ty_nil)
		return TRUE;
	else {
		if (a->kind == Ty_record && b->kind == Ty_nil || a->kind == Ty_nil && b->kind == Ty_record)
			return TRUE;
		else 
			return FALSE;
	}
}

//skip Ty_name
Ty_ty actual_ty(Ty_ty t) {
	if (!t)
		return NULL;

	while (t && t->kind == Ty_name)
		t = t->u.name.ty;
	return t;
}


set set_init() {
	set t = checked_malloc(sizeof(*t));
	t->pos = 0;
	return t;
}

void set_reset(set s) {
	s->pos = 0;
}

bool set_push(set s, S_symbol x) {
	int i;
	for (i = 0; i < s->pos; i++)
		if (s->s[i] == x)
			return FALSE;
	s->s[s->pos++] = x;
	return TRUE;
}



