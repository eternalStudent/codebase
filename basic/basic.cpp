#include "numbers.cpp"
#include "intrinsics.cpp"
#include "linkedlist.cpp"
#include "string.cpp"

#if defined(_WIN32)
#  include <Windows.h>
#endif

#if defined(__gnu_linux__)
#  include <unistd.h>
#endif

#define ASSERT(Expression) 		do{if(!(Expression)) {*(volatile int *)0 = 0;}}while(0)
#ifndef LOG
#  define LOG(text) 			do{}while(0)
#  define FAIL(text)			0
#else
#  define FAIL(text)			(LOG(text), 0)
#endif

#include "memory.cpp"
#include "io.cpp"
#include "time.cpp"
#include "binary_reader.cpp"
#include "array.cpp"

#if defined(__clang__) || defined(__GNUC__)
#  define alignof	__alignof__
#endif

union opaque64 {
	void* p;
	float64 f;
	int64 i;
};