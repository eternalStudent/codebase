#include "numbers.cpp"
#include "intrinsics.cpp"

#include <Windows.h>
#define Assert(Expression) 		if(!(Expression)) {*(volatile int *)0 = 0;}
#define Log(text) 				MessageBoxA(NULL, text, "info", 0)
#define Fail(text)				(Log(text), 0)

#include "arena.cpp"
#include "memory.cpp"
#include "string.cpp"
#include "io.cpp"