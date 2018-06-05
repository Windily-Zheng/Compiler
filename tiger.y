%{
#include <stdio.h>
#include "util.h"
#include "symbol.h" 
#include "errormsg.h"
#include "absyn.h"

#define YYDEBUG 1

int yylex(void); /* function prototype */

void yyerror(char *s)
{
	EM_error(EM_tokPos, "%s", s);
}

A_exp absyn_root;	//the root of abstract syntax tree
%}


%union {
	int pos;
	int ival;		//int variable
	string sval;	//string variable
	A_exp exp;		//expression
	A_var var;		//variable
	A_expList expList;		//expression list
	A_efieldList efieldList;	//field expression list
	A_decList decList;		//declaration list
	A_dec dec;				//declaration
	A_ty ty;				//type
	A_nametyList nametyList;	//type name list
	A_fundecList fundecList;	//function declaration list
	A_namety namety;			//type name
	A_fundec fundec;			//function declaration
	A_fieldList fieldList;		//field list
	A_field field;				//field
}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%left SEMICOLON
%right THEN ELSE DOT DO OF
%right ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS


%type <exp> program exp funcall
%type <var> lvalue
%type <expList> expseq paraseq
%type <efieldList> asseq
%type <decList> decs
%type <dec>	dec vardec
%type <nametyList> tydecs
%type <fundecList> fundecs
%type <namety> tydec
%type <fundec> fundec
%type <ty> ty
%type <fieldList> tyfields tyfield

%start program

%%

program	: exp    {absyn_root=$1;}

exp : lvalue			{$$ = A_VarExp(EM_tokPos, $1);}		//variable expression
	| lvalue ASSIGN exp {$$ = A_AssignExp(EM_tokPos, $1, $3);}	//assignment expression
	| INT				{$$ = A_IntExp(EM_tokPos, $1);}		//int expression
	| STRING			{$$ = A_StringExp(EM_tokPos, $1);}	//string expression
	| NIL				{$$ = A_NilExp(EM_tokPos);}			//nil expression

	| LPAREN RPAREN			{$$ = A_SeqExp(EM_tokPos, NULL);}	//sequence expression
	| LPAREN expseq RPAREN	{$$ = A_SeqExp(EM_tokPos, $2);}		//sequence expression

	| MINUS exp %prec UMINUS {$$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}	//negative expression
	| exp PLUS exp		{$$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);}	//'+' expression
	| exp MINUS exp		{$$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);}	//'-' expression
	| exp TIMES exp		{$$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);}	//'*' expression
	| exp DIVIDE exp	{$$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3);}	//'/' expression

	| exp EQ exp	{$$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);}		//'='  expression
	| exp NEQ exp	{$$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);}		//'!=' expression
	| exp LT exp	{$$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);}		//'<'  expression
	| exp LE exp	{$$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);}		//'<=' expression
	| exp GT exp	{$$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);}		//'>'  expression
	| exp GE exp	{$$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);}		//'>=' expression

	| exp AND exp	{$$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0));}	//'&' expression
	| exp OR exp	{$$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos, 1), $3);}	//'|' expression

	| funcall	{$$ = $1;}		//function call

	| ID LBRACK exp RBRACK OF exp	{$$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);}		//array expression
	| ID LBRACE RBRACE				{$$ = A_RecordExp(EM_tokPos, S_Symbol($1), NULL);}		//record expression
	| ID LBRACE asseq RBRACE		{$$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);}		//record expression

	| IF exp THEN exp					{$$ = A_IfExp(EM_tokPos, $2, $4, NULL);}			//if-then expression
	| IF exp THEN exp ELSE exp			{$$ = A_IfExp(EM_tokPos, $2, $4, $6);}				//if-then-else expression
	| WHILE exp DO exp					{$$ = A_WhileExp(EM_tokPos, $2, $4);}				//while-do expression
	| FOR ID ASSIGN exp TO exp DO exp	{$$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);}	//for expression
	| BREAK								{$$ = A_BreakExp(EM_tokPos);}						//break expression

	| LET decs IN END			{$$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, NULL));}	//let expression
	| LET decs IN expseq END	{$$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));}	//let expression

	| LPAREN error RPAREN	{$$ = A_SeqExp(EM_tokPos, NULL);}	//error
	| error SEMICOLON exp	{$$ = $3;}							//error
	;


lvalue	: ID						{$$ = A_SimpleVar(EM_tokPos,S_Symbol($1));}			//simple variable
		| lvalue DOT ID				{$$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));}		//field variable
		| lvalue LBRACK exp RBRACK	{$$ = A_SubscriptVar(EM_tokPos, $1, $3);}			//subscript variable
		| ID LBRACK exp RBRACK		{$$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos, S_Symbol($1)), $3);}	//subscript variable
		;   


expseq	: exp					{$$ = A_ExpList($1, NULL);}		//expression
		| exp SEMICOLON expseq	{$$ = A_ExpList($1, $3);}		//expression sequence
		;


funcall	: ID LPAREN RPAREN			{$$ = A_CallExp(EM_tokPos, S_Symbol($1), NULL);}	//function call
		| ID LPAREN paraseq RPAREN	{$$ = A_CallExp(EM_tokPos, S_Symbol($1), $3);}		//function call with parameters sequence
		;

paraseq : exp				{$$ = A_ExpList($1, NULL);}		//parameter expression
		| exp COMMA paraseq	{$$ = A_ExpList($1, $3);}		//parameters sequence
		;

asseq	: ID EQ exp				{$$ = A_EfieldList(A_Efield(S_Symbol($1), $3), NULL);}	//field expression list
		| ID EQ exp COMMA asseq	{$$ = A_EfieldList(A_Efield(S_Symbol($1), $3), $5);}	//field expression list
		;


decs	: dec decs	{$$ = A_DecList($1, $2);}		//declaration list
		|			{$$ = NULL;}					//empty string
		;

dec	: tydecs	{$$ = A_TypeDec(EM_tokPos, $1);}	//type declarations
	| vardec	{$$ = $1;}							//variable declaration
	| fundecs	{$$ = A_FunctionDec(EM_tokPos, $1);}	//function declarations
	;

tydecs	: tydec			{$$ = A_NametyList($1, NULL);}	//type declaration
		| tydec tydecs	{$$ = A_NametyList($1, $2);}	//type declarations
		;

tydec	: TYPE ID EQ ty	{$$ = A_Namety(S_Symbol($2), $4);}	//type declaration
		;

ty	: ID						{$$ = A_NameTy(EM_tokPos, S_Symbol($1));}	//type-id
	| LBRACE tyfields RBRACE	{$$ = A_RecordTy(EM_tokPos, $2);}			//record type
	| ARRAY OF ID				{$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}	//array type
	; 

tyfields: tyfield	{$$ = $1;}		//fields
		|			{$$ = NULL;}	//empty string
		;

tyfield	: ID COLON ID					{$$ = A_FieldList(A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3)), NULL);}	//field
		| ID COLON ID COMMA tyfield		{$$ = A_FieldList(A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3)), $5);}		//fields
		;

vardec	: VAR ID ASSIGN exp				{$$ = A_VarDec(EM_tokPos, S_Symbol($2), NULL, $4);}		//variable declaration
		| VAR ID COLON ID ASSIGN exp	{$$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6);}	//variable declaration
		;

fundecs	: fundec			{$$ = A_FundecList($1, NULL);}		//function declaration
		| fundec fundecs	{$$ = A_FundecList($1, $2);}		//function declarations
		;

fundec	: FUNCTION ID LPAREN tyfields RPAREN EQ exp				{$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL, $7);}		//function declaration
		| FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp	{$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);}	//function declaration
		;


	
