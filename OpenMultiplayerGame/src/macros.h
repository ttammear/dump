#pragma once

#define FLAGSET(var, flag) ((var & flag) != 0)
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#define SETFLAG(var, flag) var |= flag
#define CLEARFLAG(var, flag) var &= ~flag

