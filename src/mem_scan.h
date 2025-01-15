#pragma once

#include <cstdlib>
#include <windows.h>
#include <TlHelp32.h>
#include "win_internals.h"

#ifdef __cplusplus
extern "C"
{
#endif

char* FindPattern(char* pattern, char* mask, char* begin, intptr_t size);

char* FindPatternBasic(char* pattern, char* mask, char* begin, intptr_t size);

char* FindPatternIn(char* pattern, char* mask, char* moduleName);

#ifdef __cplusplus
}
#endif
