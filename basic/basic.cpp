#include "numbers.cpp"
#include "intrinsics.cpp"
#include "string.cpp"
#include "linkedlist.cpp"

#include <Windows.h>
#define ASSERT(Expression) 		do{if(!(Expression)) {*(volatile int *)0 = 0;}}while(0)
#define LOG(text) 				MessageBoxA(NULL, text, "info", 0)
#define FAIL(text)				(LOG(text), 0)

#include "memory.cpp"
#include "io.cpp"
#include "time.cpp"

#if defined(__clang__) || defined(__GNUC__)
#  define alignof	__alignof__
#endif

union opaque64 {
	void* p;
	float64 f;
	int64 i;
};