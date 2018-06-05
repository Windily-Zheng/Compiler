/*
 * absyn.h - Abstract Syntax Header
 *
 * All types and functions declared in this header file begin with "A_"
 * Linked list types end with "..list"
 */

#ifndef ABSYN_H
#define ABSYN_H

#include "util.h"
#include "symbol.h"

/* Type Definitions */

typedef int A_pos;		//position

typedef struct A_var_ *A_var;	//variable
typedef struct A_exp_ *A_exp;	//expression
typedef struct A_dec_ *A_dec;	//declaration
typedef struct A_ty_ *A_ty;		//type

typedef struct A_decList_ *A_decList;	//declaration list
typedef struct A_expList_ *A_expList;	//expression list
typedef struct A_field_ *A_field;		//field
typedef struct A_fieldList_ *A_fieldList;	//field list
typedef struct A_fundec_ *A_fundec;		//function declaration
typedef struct A_fundecList_ *A_fundecList;	//function declaration list
typedef struct A_namety_ *A_namety;		//type declaration
typedef struct A_nametyList_ *A_nametyList;	//type declaration list
typedef struct A_efield_ *A_efield;		//field expression
typedef struct A_efieldList_ *A_efieldList;	//field expression list

typedef enum {A_plusOp, A_minusOp, A_timesOp, A_divideOp,
	     A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp} A_oper;	//operators

struct A_var_	/* variable */
       {enum {A_simpleVar, A_fieldVar, A_subscriptVar} kind;	//kind of variable
        A_pos pos;						//position
	union {S_symbol simple;				//simple variable
	       struct {A_var var;
		       S_symbol sym;} field;	//field variable
	       struct {A_var var;
		       A_exp exp;} subscript;	//subscript variable
	     } u;
      };

struct A_exp_	/* expression */
      {enum {A_varExp, A_nilExp, A_intExp, A_stringExp, A_callExp,
	       A_opExp, A_recordExp, A_seqExp, A_assignExp, A_ifExp,
	       A_whileExp, A_forExp, A_breakExp, A_letExp, A_arrayExp} kind; //kind of expression
       A_pos pos;	//position
       union {A_var var;
	      // nil; - needs only the pos
	      int intt;		//int value
	      string stringg;	//string value
	      struct {S_symbol func; A_expList args;} call;	//function call
	      struct {A_oper oper; A_exp left; A_exp right;} op;	//operation expression
	      struct {S_symbol typ; A_efieldList fields;} record;	//record
	      A_expList seq;	//sequence
	      struct {A_var var; A_exp exp;} assign;	//assignment expression
	      struct {A_exp test, then, elsee;} iff; 	//if expression(elsee is optional)
	      struct {A_exp test, body;} whilee;		//while expression
	      struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr;	//for expression
	      // break; - needs only the pos
	      struct {A_decList decs; A_exp body;} let;	//let expression
	      struct {S_symbol typ; A_exp size, init;} array;	//array expression
	    } u;
     };

struct A_dec_ 	/* declaration */
    {enum {A_functionDec, A_varDec, A_typeDec} kind;	//kind of declaration
     A_pos pos;		//position
     union {A_fundecList function;		//function
     	/* escape field is initialized to true,
	       but may change after the initial declaration */
	    struct {S_symbol var; S_symbol typ; A_exp init; bool escape;} var;	//variable
	    A_nametyList type;		//type
	  } u;
   };

struct A_ty_ 	/* type */
	{enum {A_nameTy, A_recordTy, A_arrayTy} kind;	//kind of type
	 A_pos pos;		//position
	 union {S_symbol name;	//type name(type-id)
		 A_fieldList record;	//record
		 S_symbol array;	//array
		} u;
	};

/* Linked lists and nodes of lists */

struct A_field_ {S_symbol name, typ; A_pos pos; bool escape;};	//field
struct A_fieldList_ {A_field head; A_fieldList tail;};			//field list
struct A_expList_ {A_exp head; A_expList tail;};				//expression list
struct A_fundec_ {A_pos pos;									//function declaration
                 S_symbol name; A_fieldList params; 
		 S_symbol result; A_exp body;};

struct A_fundecList_ {A_fundec head; A_fundecList tail;};		//function declaration list
struct A_decList_ {A_dec head; A_decList tail;};				//declaration list
struct A_namety_ {S_symbol name; A_ty ty;};						//type name
struct A_nametyList_ {A_namety head; A_nametyList tail;};		//type name list
struct A_efield_ {S_symbol name; A_exp exp;};					//field expression
struct A_efieldList_ {A_efield head; A_efieldList tail;};		//field expression list


/* Function Prototypes */
A_var A_SimpleVar(A_pos pos, S_symbol sym);						//simple variable
A_var A_FieldVar(A_pos pos, A_var var, S_symbol sym);			//field variable
A_var A_SubscriptVar(A_pos pos, A_var var, A_exp exp);			//subscript variable
A_exp A_VarExp(A_pos pos, A_var var);							//variable expression
A_exp A_NilExp(A_pos pos);										//nil expression(only needs pos)
A_exp A_IntExp(A_pos pos, int i);								//int expression
A_exp A_StringExp(A_pos pos, string s);							//string expression
A_exp A_CallExp(A_pos pos, S_symbol func, A_expList args);		//function call
A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right);	//operation expression
A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields);//record expression
A_exp A_SeqExp(A_pos pos, A_expList seq);						//sequence expression
A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp);				//assignment expression
A_exp A_IfExp(A_pos pos, A_exp test, A_exp then, A_exp elsee);	//if expression(else is optional)
A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body);			//while expression
A_exp A_ForExp(A_pos pos, S_symbol var, A_exp lo, A_exp hi, A_exp body);	//for expression
A_exp A_BreakExp(A_pos pos);									//break expression(only needs pos)
A_exp A_LetExp(A_pos pos, A_decList decs, A_exp body);			//let expression
A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init);	//array expression
A_dec A_FunctionDec(A_pos pos, A_fundecList function);				//function declaration
A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init);	//variable declaration
A_dec A_TypeDec(A_pos pos, A_nametyList type);						//type declaration
A_ty A_NameTy(A_pos pos, S_symbol name);							//type name
A_ty A_RecordTy(A_pos pos, A_fieldList record);						//record
A_ty A_ArrayTy(A_pos pos, S_symbol array);							//array
A_field A_Field(A_pos pos, S_symbol name, S_symbol typ);			//field
A_fieldList A_FieldList(A_field head, A_fieldList tail);			//field list
A_expList A_ExpList(A_exp head, A_expList tail);					//expression list
A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result,	//function declaration
		  A_exp body);
A_fundecList A_FundecList(A_fundec head, A_fundecList tail);		//function declaration list
A_decList A_DecList(A_dec head, A_decList tail);					//declaration list
A_namety A_Namety(S_symbol name, A_ty ty);							//type name
A_nametyList A_NametyList(A_namety head, A_nametyList tail);		//type name list
A_efield A_Efield(S_symbol name, A_exp exp);						//field expression
A_efieldList A_EfieldList(A_efield head, A_efieldList tail);		//field expression list

#endif
