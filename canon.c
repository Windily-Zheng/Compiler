/*
* canon.c - Functions to convert the IR trees into basic blocks and traces.
*
*/
#include <stdio.h>

#include "canon.h"

/* 
* linearize procedure 
*/


/* A expression list*/
typedef struct expRefList_ *expRefList;
struct expRefList_ { T_exp *head; expRefList tail; };
static T_stm do_stm(T_stm stm);
static struct stmExp do_exp(T_exp exp);

static C_stmListList Blocks(T_stmList stms, Temp_label done);
static T_stmList getNext(void);


//construction
static expRefList ExpRefList(T_exp *head, expRefList tail)
{
	expRefList p = (expRefList)checked_malloc(sizeof *p);
	p->head = head;
	p->tail = tail;
	return p;
}

/* judge if a stm is a const or a exp, AKA nop stm */
static bool isNop(T_stm x)
{
	return x->kind == T_EXP && x->u.EXP->kind == T_CONST;
}

/* 
* judge if a stm is exchangable with a exp 
* assume that nop stm can exchange with any exps
* and const or name exp can exchange with any stms
*/
static bool commute(T_stm x, T_exp y)
{
	if (isNop(x))		//the stm is a non-stm
		return TRUE;

	if (y->kind == T_NAME || y->kind == T_CONST) //the exp is a const or a name
		return TRUE;

	return FALSE;
}

/* get a stm equivalent to SEQ(x, y) */
static T_stm seq(T_stm x, T_stm y)
{
	if (isNop(x)) return y;
	if (isNop(y)) return x;
	return T_Seq(x, y);
}


struct stmExp { T_stm s; T_exp e; };	//e contains no ESEQ

static struct stmExp StmExp(T_stm stm, T_exp exp) {
	struct stmExp x;
	x.s = stm;
	x.e = exp;
	return x;
}


/* get a exp list, extract ESEQs and merge the results */
static T_stm reorder(expRefList rlist) {
	if (!rlist)
		return T_Exp(T_Const(0)); // nop, get a CONST 0
	else if ((*rlist->head)->kind == T_CALL) {	//A CALL      
		Temp_temp t = Temp_newtemp();
		*rlist->head = T_Eseq(T_Move(T_Temp(t), *rlist->head), T_Temp(t)); //ESEQ(MOVE(TEMP t, CALL(fun, args)), Temp t)
		return reorder(rlist);
	}
	else
	{
		struct stmExp hd = do_exp(*rlist->head);	//extract EXEQ from the head, get a (stm, exp) ,exp with no ESEQ
		T_stm s = reorder(rlist->tail);	//do the same thing to the tail, at last we get a stm
		if (commute(s, hd.e)) {			//if exchangable
			*rlist->head = hd.e;	//exchange
			return seq(hd.s, s);	//merge results
		}
		else
		{
			Temp_temp t = Temp_newtemp();
			*rlist->head = T_Temp(t);
			return seq(hd.s, seq(T_Move(T_Temp(t), hd.e), s));	//SEQ(s, SEQ(MOVE(TEMP t, e), s))
		}
	}
}

static expRefList get_call_rlist(T_exp exp)
{
	expRefList rlist, curr;
	T_expList args = exp->u.CALL.args;
	curr = rlist = ExpRefList(&exp->u.CALL.fun, NULL);
	for (; args; args = args->tail) {
		curr = curr->tail = ExpRefList(&args->head, NULL);
	}
	return rlist;
}



/* change an exp to an (stm, exp), where exp contains no ESEQ*/
static struct stmExp do_exp(T_exp exp)
{
	switch (exp->kind) {
	case T_BINOP:
		return StmExp(reorder(ExpRefList(&exp->u.BINOP.left,
			ExpRefList(&exp->u.BINOP.right, NULL))),
			exp);
	case T_MEM:
		return StmExp(reorder(ExpRefList(&exp->u.MEM, NULL)), exp);
	case T_ESEQ:
	{struct stmExp x = do_exp(exp->u.ESEQ.exp);
	return StmExp(seq(do_stm(exp->u.ESEQ.stm), x.s), x.e);
	}
	case T_CALL:
		return StmExp(reorder(get_call_rlist(exp)), exp);
	default:
		return StmExp(reorder(NULL), exp);
	}
}

/* extract all ESEQs in a stm*/
static T_stm do_stm(T_stm stm)
{
	switch (stm->kind) {
	case T_SEQ:
		return seq(do_stm(stm->u.SEQ.left), do_stm(stm->u.SEQ.right));
	case T_JUMP:
		return seq(reorder(ExpRefList(&stm->u.JUMP.exp, NULL)), stm);
	case T_CJUMP:
		return seq(reorder(ExpRefList(&stm->u.CJUMP.left,
			ExpRefList(&stm->u.CJUMP.right, NULL))), stm);
	case T_MOVE:
		if (stm->u.MOVE.dst->kind == T_TEMP && stm->u.MOVE.src->kind == T_CALL)
			return seq(reorder(get_call_rlist(stm->u.MOVE.src)), stm);
		else if (stm->u.MOVE.dst->kind == T_TEMP)
			return seq(reorder(ExpRefList(&stm->u.MOVE.src, NULL)), stm);
		else if (stm->u.MOVE.dst->kind == T_MEM)
			return seq(reorder(ExpRefList(&stm->u.MOVE.dst->u.MEM,
				ExpRefList(&stm->u.MOVE.src, NULL))), stm);
		else if (stm->u.MOVE.dst->kind == T_ESEQ) {
			T_stm s = stm->u.MOVE.dst->u.ESEQ.stm;
			stm->u.MOVE.dst = stm->u.MOVE.dst->u.ESEQ.exp;
			return do_stm(T_Seq(s, stm));
		}
		assert(0);
	case T_EXP:
		if (stm->u.EXP->kind == T_CALL)
			return seq(reorder(get_call_rlist(stm->u.EXP)), stm);
		else return seq(reorder(ExpRefList(&stm->u.EXP, NULL)), stm);
	default:
		return stm;
	}
}

/* delete ESEQ and pop CALL onto the top */
static T_stmList linear(T_stm stm, T_stmList right)
{
	if (stm->kind == T_SEQ)
		return linear(stm->u.SEQ.left,
			linear(stm->u.SEQ.right,
				right));
	else return T_StmList(stm, right);
}

T_stmList C_linearize(T_stm stm)
{
	return linear(do_stm(stm), NULL);
}

/* basic blocks procedure */

//construction
static C_stmListList StmListList(T_stmList head, C_stmListList tail)
{
	C_stmListList p = (C_stmListList)checked_malloc(sizeof *p);
	p->head = head; p->tail = tail;
	return p;
}

/* search to the end of basic block, so we get next block 
* when we have found a JUMP or CJUMP, we know we get to the end of a block
* we put the next stmList to the next position in the stmListList
*/
static C_stmListList next(T_stmList prevstms, T_stmList stms, Temp_label lb)
{
	if (!stms)
		return next(prevstms,
			T_StmList(
				T_Jump(
					T_Name(lb), 
					Temp_LabelList(lb, NULL)),
				NULL),	
			lb);

	if (stms->head->kind == T_JUMP || stms->head->kind == T_CJUMP) {	//the end
		C_stmListList stmLists;
		prevstms->tail = stms;
		stmLists = Blocks(stms->tail, lb);		//attach the label
		stms->tail = NULL;						//there is no next stm
		return stmLists;
	}
	else if (stms->head->kind == T_LABEL) {		//the beginning
		Temp_label lab = stms->head->u.LABEL;
		return next(prevstms, 
			T_StmList(
				T_Jump(
					T_Name(lab), 
					Temp_LabelList(lab, NULL)),
				stms), 
			lb);
	}
	else {
		prevstms->tail = stms;
		return next(stms, stms->tail, lb);
	}
}

/* make the beginning of a basic block a LABEL */
static C_stmListList Blocks(T_stmList stms, Temp_label lb)
{
	if (!stms) {
		return NULL;
	}
	if (stms->head->kind == T_LABEL) //if the head is already a LABEL
	{
		return StmListList(stms, next(stms, stms->tail, lb)); //we simply put to the StmListList, and get the next
	}

	return Blocks(
		T_StmList(
			T_Label(Temp_newlabel()),	//get a new LABEL
			stms),
		lb);	
}

/* produce basic block list and give every bloc a LABEL */
struct C_block C_basicBlocks(T_stmList stmList)
{
	struct C_block b;

	b.label = Temp_newlabel();
	b.stmLists = Blocks(stmList, b.label);

	return b;
}


/* trace schedule procedure */

static S_table block_env;
static struct C_block global_block;

static T_stmList getLast(T_stmList list)
{
	T_stmList last = list;
	while (last->tail->tail) 
		last = last->tail;
	return last;
}

static void trace(T_stmList list)
{
	T_stmList last = getLast(list);
	T_stm lab = list->head;
	T_stm s = last->tail->head;

	S_enter(block_env, lab->u.LABEL, NULL);

	if (s->kind == T_JUMP)				//check the branch target JUMP
	{
		T_stmList target = (T_stmList)S_look(block_env, s->u.JUMP.jumps->head);
		if (!s->u.JUMP.jumps->tail && target)
		{
			last->tail = target;
			trace(target);
		}
		else last->tail->tail = getNext(); 
	}
		
	else if (s->kind == T_CJUMP)		//check the branch target of CJUMP, we keep the false one
	{
		T_stmList trueTarget = (T_stmList)S_look(block_env, s->u.CJUMP.true);
		T_stmList falseTarget = (T_stmList)S_look(block_env, s->u.CJUMP.false);
		if (falseTarget) 
		{
			last->tail->tail = falseTarget;	//keep it
			trace(falseTarget);
		}
		else if (trueTarget)			//exchange the true one and the false one
		{			
			last->tail->head = T_Cjump(T_notRel(s->u.CJUMP.op), s->u.CJUMP.left,
				s->u.CJUMP.right, s->u.CJUMP.false,
				s->u.CJUMP.true);
			last->tail->tail = trueTarget;
			trace(trueTarget);
		}
		else {							//CJUMP(cond, a, b, lt, lf') LABEL lf' JUMP(NAME lf)
			Temp_label false = Temp_newlabel();		//get a new LABEL
			last->tail->head = T_Cjump(s->u.CJUMP.op, s->u.CJUMP.left,
				s->u.CJUMP.right, s->u.CJUMP.true, false);
			last->tail->tail = T_StmList(T_Label(false), getNext());
		}
	}
	else assert(0);
}

/* 
* get the next block from the list of stmLists, using only those that have
* not been traced yet 
*/
static T_stmList getNext()
{
	if (!global_block.stmLists)
		return T_StmList(T_Label(global_block.label), NULL);
	else {
		T_stmList s = global_block.stmLists->head;
		if (S_look(block_env, s->head->u.LABEL)) {/* label exists in the table */
			trace(s);
			return s;
		}
		else {
			global_block.stmLists = global_block.stmLists->tail;
			return getNext();
		}
	}
}
/* 
* Every CJUMP(_,t,f) is immediately followed by LABEL f.
* JUMP(NAME l) LABEL l: delete the JUMP
*/
T_stmList C_traceSchedule(struct C_block b)
{
	C_stmListList sList;
	block_env = S_empty();
	global_block = b;

	for (sList = global_block.stmLists; sList; sList = sList->tail) 
	{
		S_enter(block_env, sList->head->head->u.LABEL, sList->head);
	}

	return getNext();
}

