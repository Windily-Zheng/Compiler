%{
#include <string.h>
#include "util.h"
#include "absyn.h"
#include "y.tab.h"
#include "errormsg.h"

int charPos=1;		//position field

int yywrap(void)
{
	charPos=1;
	return 1;
}

/* obtain and adjust the position field */
void adjust(void)
{
	EM_tokPos=charPos;	//obtain the position
	charPos+=yyleng;	//adjust the position
}

const int plen = 100;

char *str = NULL;
int len, mlen;

void str_init() {
	str = checked_malloc(plen);
	mlen = plen;
	str[0] = '\0';
	len = 0;
}

void str_add(char c) {
	if (len + 1 == mlen) {
		str = realloc(str, mlen+plen);
		if (!str) {
			printf("malloc error!\n");
			exit(1);
		}
		mlen += plen;
	}
	str[len++] = c;
	str[len] = '\0';
}

void str_del() {
	free(str);
	str = NULL;
}

%}

%x COMMENT STR VSTR

%%
	int count = 0;	//count the degree of comments
	bool flag;

 /* COMMENT state */
<COMMENT>{
	"/*" {adjust(); count++;}
	"*/" {adjust(); count--; if(!count) BEGIN(0);}
	\n	 {adjust(); EM_newline(); continue;}
	\r	
	. adjust(); 
	<<EOF>> {adjust(); EM_error(EM_tokPos,"EOF in comment"); return 0;}
}

 /* STR state */
<STR>{
	\" {adjust(); yylval.sval = String(str); str_del(); BEGIN(0); if(flag) return STRING;}
	\\[' '\t\n\r]	{adjust(); BEGIN(VSTR);}
	\\n	 {adjust(); str_add('\n');}
	\\t	{adjust(); str_add('\t');}
	\\^[a-zA-Z] adjust();
	\\[0-9]{3}	{adjust(); str_add(atoi(yytext+1));}
	\\\"	{adjust(); str_add('\"');}
	\\\\	{adjust(); str_add('\\');}
	\\[^ntf\"\\' '\t\n\r] {adjust(); flag = FALSE; EM_error(EM_tokPos, "unknown escape sequence: '\\%c'", yytext[1]);}
	\n	 {adjust(); EM_newline(); str_add('\n');}
	\r
	. {adjust(); str_add(yytext[0]);}
	<<EOF>> {adjust(); EM_error(EM_tokPos,"EOF in string"); str_del(); return 0;}
}

 /* VSTR state */
<VSTR>{
	\\ {adjust(); BEGIN(STR);}
	\n	 {adjust(); EM_newline();}
	\r
	.	 adjust(); 
	<<EOF>> {adjust(); EM_error(EM_tokPos,"EOF in string"); return 0;}
}

 /* Beginning of COMMENT state */
"/*" {adjust(); count++; BEGIN(COMMENT);}

 /* Beginning of STR state */
\"	 {adjust(); str_init(); flag = TRUE; BEGIN(STR);}

 /* Blank */
[ \t]*	 {adjust(); continue;}

 /* New Line */
\n	 {adjust(); EM_newline(); continue;}
\r

 /* Punctuation Symbols */
","	 {adjust(); return COMMA;}
":"	 {adjust(); return COLON;}
";"	 {adjust(); return SEMICOLON;}
"("	 {adjust(); return LPAREN;}
")"	 {adjust(); return RPAREN;}
"["	 {adjust(); return LBRACK;}
"]"	 {adjust(); return RBRACK;}
"{"	 {adjust(); return LBRACE;}
"}"	 {adjust(); return RBRACE;}
"."	 {adjust(); return DOT;}
"+"	 {adjust(); return PLUS;}
"-"	 {adjust(); return MINUS;}
"*"	 {adjust(); return TIMES;}
"/"	 {adjust(); return DIVIDE;}
"="	 {adjust(); return EQ;}
"<>" {adjust(); return NEQ;}
"<"	 {adjust(); return LT;}
"<=" {adjust(); return LE;}
">"	 {adjust(); return GT;}
">=" {adjust(); return GE;}
"&"	 {adjust(); return AND;}
"|"	 {adjust(); return OR;}
":=" {adjust(); return ASSIGN;}

 /* Reserved Words */
array	{adjust(); return ARRAY;}
if		{adjust(); return IF;}
then	{adjust(); return THEN;}
else	{adjust(); return ELSE;}
while	{adjust(); return WHILE;}
for		{adjust(); return FOR;}
to		{adjust(); return TO;}
do		{adjust(); return DO;}
let		{adjust(); return LET;}
in		{adjust(); return IN;}
end		{adjust(); return END;}
of		{adjust(); return OF;}
break	{adjust(); return BREAK;}
nil		{adjust(); return NIL;}
function {adjust(); return FUNCTION;}
var		{adjust(); return VAR;}
type  	{adjust(); return TYPE;}
[a-zA-Z]+[a-zA-Z0-9_]*	{adjust(); yylval.sval = String(yytext); return ID;}
[0-9]+	 {adjust(); yylval.ival=atoi(yytext); return INT;}

.	 {adjust(); EM_error(EM_tokPos,"illegal token");}
<<EOF>> return 0;


