#pragma once

/* generate i386 code */
#include "assem.h"
#include "frame.h"

AS_instrList I_codegen(F_frame, T_stmList);