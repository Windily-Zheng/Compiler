#pragma once
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"

/*
* canon.h - convert the tree into basic blocks and traces
*
*/
typedef struct C_stmListList_ *C_stmListList;
struct C_block { C_stmListList stmLists; Temp_label label; };
struct C_stmListList_ { T_stmList head; C_stmListList tail; };

/* delete ESEQ and pop CALL onto the top */
T_stmList C_linearize(T_stm stm);

/* produce basic block list and give every bloc a LABEL */
struct C_block C_basicBlocks(T_stmList stmList);

/*
* Every CJUMP(_,t,f) is immediately followed by LABEL f.
* JUMP(NAME l) LABEL l: delete the JUMP
*/
T_stmList C_traceSchedule(struct C_block b);

