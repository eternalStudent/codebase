#if defined(__GNUC__)
#  define COMPILER_GCC 1
#elif defined(__clang__)
#  define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#  define COMPILER_MSVC 1
#  include <intrin.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#  define _OS_WINDOWS 1
#elif defined(__ANDROID__)
#  define _OS_ANDROID 1
#  define _OS_UNIX 1
#  define _MATH_ALREADY_DEFINED_ 1
#  define _NO_ROTATE_ 1
#elif defined(__linux__)
#  define _OS_LINUX 1
#  define _OS_UNIX 1
#endif

#include <memory.h>
#include <immintrin.h>

#include "numbers.cpp"
#include "bits.cpp"
#include "linkedlist.cpp"
#include "string.cpp"

#if defined(_OS_WINDOWS)
#  include <Windows.h>
#elif defined(_OS_ANDROID)
#  include <unistd.h>
#  include <jni.h>
#  include <android/log.h>
#  include <android/asset_manager.h>
#  include <android_native_app_glue.h>
#elif defined(_OS_LINUX)
#  include <unistd.h>
#endif

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#if defined(DEBUG)
#  define ASSERT(expression) 	do{if(!(expression)) {*(int32*)0 = 0;}}while(0)
#else
#  define ASSERT(expression)
#endif

#if !defined(COMPILER_MSVC)
#  define alignof	__alignof__
#endif

#define HAS_FLAGS(all, flags) (((all) & (flags)) == (flags))

#ifndef LOG
#  define LOG(...) 			do{}while(0)
#  define FAIL(...)			0
#else
#  define FAIL(...)			(LOG(__VA_ARGS__), 0)
#endif

#include "memory.cpp"
#include "io.cpp"
#include "time.cpp"
#include "binaryreader.cpp"

union opaque64 {
	void* p;
	float64 f;
	int64 i;
};