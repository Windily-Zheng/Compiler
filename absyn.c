/*
 * absyn.c - Abstract Syntax Functions. Most functions create an instance of an
 *           abstract syntax rule.
 */

#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */

/* Simple Variable */
A_var A_SimpleVar(A_pos pos, S_symbol sym)
{A_var p = checked_malloc(sizeof(*p));
 p->kind=A_simpleVar;	//kind of variable is simple variable
 p->pos=pos;			//position
 p->u.simple=sym;		//symbol of simple variable
 return p;
}

/* Field Variable */
A_var A_FieldVar(A_pos pos, A_var var, S_symbol sym)
{A_var p = checked_malloc(sizeof(*p));
 p->kind=A_fieldVar;	//kind of variable is field variable
 p->pos=pos;			//position
 p->u.field.var=var;	//variable(lvalue)
 p->u.field.sym=sym;	//symbol of field variable
 return p;
}

/* Subscript Variable */
A_var A_SubscriptVar(A_pos pos, A_var var, A_exp exp)
{A_var p = checked_malloc(sizeof(*p));
 p->kind=A_subscriptVar;	//kind of variable is subscript variable
 p->pos=pos;				//position
 p->u.subscript.var=var;	//array variable
 p->u.subscript.exp=exp;	//expression of array index
 return p;
}

/* Variable Expression */
A_exp A_VarExp(A_pos pos, A_var var)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_varExp;			//kind of expression is variable expression
 p->pos=pos;
 p->u.var=var;				//the variable
 return p;
}

/* Nil Expression */
A_exp A_NilExp(A_pos pos)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_nilExp;			//kind of expression is nil expression
 p->pos=pos;				//nil expression only needs pos
 return p;
}

/* Int Expression */
A_exp A_IntExp(A_pos pos, int i)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_intExp;			//kind of expression is int expression
 p->pos=pos;
 p->u.intt=i;				//the int value
 return p;
}

/* String Expression */
A_exp A_StringExp(A_pos pos, string s)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_stringExp;		//kind of expression is string expression
 p->pos=pos;				
 p->u.stringg=s;			//the string value
 return p;
}

/* Call Expression */
A_exp A_CallExp(A_pos pos, S_symbol func, A_expList args)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_callExp;			//kind of expression is function call expression
 p->pos=pos;
 p->u.call.func=func;		//the called function
 p->u.call.args=args;		//the arguments of the function
 return p;
}

/* Operation Expression */
A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_opExp;			//kind of expression is operation expression
 p->pos=pos;
 p->u.op.oper=oper;			//the operator
 p->u.op.left=left;			//the left operand
 p->u.op.right=right;		//the right operand
 return p;
}

/* Record Expression */
A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_recordExp;		//kind of expression is record expression
 p->pos=pos;
 p->u.record.typ=typ;		//the record type
 p->u.record.fields=fields;	//the record fields
 return p;
}

/* Sequence Expression */
A_exp A_SeqExp(A_pos pos, A_expList seq)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_seqExp;			//kind of expression is sequence expression
 p->pos=pos;
 p->u.seq=seq;				//the sequence
 return p;
}

/* Assignment Expression */
A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_assignExp;		//kind of expression is assignment expression
 p->pos=pos;
 p->u.assign.var=var;		//the assigned variable
 p->u.assign.exp=exp;		//the expression to assign
 return p;
}

/* If Expression */
A_exp A_IfExp(A_pos pos, A_exp test, A_exp then, A_exp elsee)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_ifExp;			//kind of expression is if expression
 p->pos=pos;
 p->u.iff.test=test;		//the if condition to test
 p->u.iff.then=then;		//then statement
 p->u.iff.elsee=elsee;		//elsee statement(optional)
 return p;
}

/* While Expression */
A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_whileExp;		//kind of expression is while expression
 p->pos=pos;
 p->u.whilee.test=test;		//the condition to test
 p->u.whilee.body=body;		//the body of while-loop
 return p;
}

/* For Expression */
A_exp A_ForExp(A_pos pos, S_symbol var, A_exp lo, A_exp hi, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_forExp;			//kind of expression is for expression
 p->pos=pos;
 p->u.forr.var=var;			//the for-loop variable
 p->u.forr.lo=lo;			//the initial value of for-loop variable
 p->u.forr.hi=hi;			//the ending value of for-loop variable
 p->u.forr.body=body;		//the body of for-loop
 p->u.forr.escape=TRUE;		//the escape field is always initialized to true
 return p;
}

/* Break Expression */
A_exp A_BreakExp(A_pos pos)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_breakExp;		//kind of expression is break expression
 p->pos=pos;				//break expression only needs pos
 return p;
}

/* Let Expression */
A_exp A_LetExp(A_pos pos, A_decList decs, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_letExp;			//kind of expression is let expression
 p->pos=pos;
 p->u.let.decs=decs;		//the declarations
 p->u.let.body=body;		//the body
 return p;
}

/* Array Expression */
A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_arrayExp;		//kind of expression is array expression
 p->pos=pos;
 p->u.array.typ=typ;		//the type of array
 p->u.array.size=size;		//the size of array
 p->u.array.init=init;		//the initial value of array
 return p;
}

/* Function Declaration */
A_dec A_FunctionDec(A_pos pos, A_fundecList function)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_functionDec;		//kind of declaration is function declaration
 p->pos=pos;
 p->u.function=function;	//the function to declare
 return p;
}

/* Variable Declaration */
A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_varDec;			//kind of declaration is variable declaration
 p->pos=pos;
 p->u.var.var=var;			//the variable to declare
 p->u.var.typ=typ;			//the type of the variable(optional)
 p->u.var.init=init;		//the initial value of the variable
 p->u.var.escape=TRUE;		//the escape field is always initialized to true
 return p;
}

/* Type Declaration */
A_dec A_TypeDec(A_pos pos, A_nametyList type)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_typeDec;			//kind of declaration is type declaration
 p->pos=pos;
 p->u.type=type;			//the type to declare
 return p;
}

/* Type Name */
A_ty A_NameTy(A_pos pos, S_symbol name)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_nameTy;			//kind is type name
 p->pos=pos;
 p->u.name=name;			//the type name(type-id)
 return p;
}

/* Record Type */
A_ty A_RecordTy(A_pos pos, A_fieldList record)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_recordTy;		//kind is record type
 p->pos=pos;
 p->u.record=record;		//the field lists of record
 return p;
}

/* Array Type */
A_ty A_ArrayTy(A_pos pos, S_symbol array)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_arrayTy;			//kind is array type
 p->pos=pos;
 p->u.array=array;			//the type of array
 return p;
}

/* Field */
A_field A_Field(A_pos pos, S_symbol name, S_symbol typ)
{A_field p = checked_malloc(sizeof(*p));
 p->pos=pos;
 p->name=name;				//field name
 p->typ=typ;				//field type
 p->escape=TRUE;			//the escape field is always initialized to true
 return p;
}

/* Field List */
A_fieldList A_FieldList(A_field head, A_fieldList tail)
{A_fieldList p = checked_malloc(sizeof(*p));
 p->head=head;				//head of field list
 p->tail=tail;				//tail of field list
 return p;
}

/* Expression List */
A_expList A_ExpList(A_exp head, A_expList tail)
{A_expList p = checked_malloc(sizeof(*p));
 p->head=head;				//head of expression list
 p->tail=tail;				//tail of expression list
 return p;
}

/* Function Declaration */
A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result,
		  A_exp body)
{A_fundec p = checked_malloc(sizeof(*p));
 p->pos=pos;				
 p->name=name;				//function name
 p->params=params;			//function parameters
 p->result=result;			//return result
 p->body=body;				//function body
 return p;
}

/* Function Declaration List */
A_fundecList A_FundecList(A_fundec head, A_fundecList tail)
{A_fundecList p = checked_malloc(sizeof(*p));
 p->head=head;				//head of function declaration list
 p->tail=tail;				//tail of function declaration list
 return p;
}

/* Declaration List */
A_decList A_DecList(A_dec head, A_decList tail)
{A_decList p = checked_malloc(sizeof(*p));
 p->head=head;				//head of declaration list
 p->tail=tail;				//head of declaration list
 return p;
}

/* Type Declaration */
A_namety A_Namety(S_symbol name, A_ty ty)
{A_namety p = checked_malloc(sizeof(*p));
 p->name=name;				//type-id
 p->ty=ty;					//type
 return p;
}

/* Type Declaration List */
A_nametyList A_NametyList(A_namety head, A_nametyList tail)
{A_nametyList p = checked_malloc(sizeof(*p));
 p->head=head;				//head of type declaration list
 p->tail=tail;				//tail of type declaration list
 return p;
}

/* Field Expression */
A_efield A_Efield(S_symbol name, A_exp exp)
{A_efield p = checked_malloc(sizeof(*p));
 p->name=name;				//field name
 p->exp=exp;				//field expression
 return p;
}

/* Field Expression List */
A_efieldList A_EfieldList(A_efield head, A_efieldList tail)
{A_efieldList p = checked_malloc(sizeof(*p));
 p->head=head;				//head of expression list
 p->tail=tail;				//tail of expression list
 return p;
}

