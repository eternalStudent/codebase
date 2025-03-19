#if defined(__GNUC__)
#  define COMPILER_GCC 1
#elif defined(__clang__)
#  define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#  define COMPILER_MSVC 1
#endif

#if defined(_WIN32) || defined(_WIN64)
#  define OS_WINDOWS 1
#elif defined(__ANDROID__)
#  define OS_ANDROID 1
#  define OS_UNIX 1
#  define _MATH_ALREADY_DEFINED_ 1
#  define _ABS_ALREADY_DEFINED_ 1
#  define _NO_ROTATE_ 1
#elif defined(__linux__)
#  define OS_LINUX 1
#  define OS_UNIX 1
#  define _ABS_ALREADY_DEFINED_ 1
#  define _NO_ROTATE_ 1
#endif

#if defined(OS_WINDOWS)
#  define NOMINMAX
#  include <windows.h>
#  include <tmmintrin.h>
#  include <wmmintrin.h>
#  include <intrin.h>
#elif defined(OS_ANDROID)
#  include <unistd.h>
#  include <jni.h>
#  include <android/log.h>
#  include <android/asset_manager.h>
#  include <android_native_app_glue.h>
#elif defined(OS_LINUX)
#  include <unistd.h>
#endif

#include <memory.h>
#include <immintrin.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define GLUE2(x, y) x##y
#define GLUE(x, y) GLUE2(x, y)

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
# define __debugbreak() __builtin_trap()
#endif

#if defined(DEBUG)
#  define ASSERT(expression) 	do{if(!(expression)) {__debugbreak();}}while(0)
#else
#  define ASSERT(expression)	(void)(expression)
#endif

#if defined(OS_WINDOWS)
#  define ASSERT_HR(hr) ASSERT(SUCCEEDED(hr))
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#  define alignof	__alignof__
#endif

#define HAS_FLAGS(all, flags) (((all) & (flags)) == (flags))

// TODO: do something better here
#ifndef LOG
#  define LOG(...) 			do{}while(0)
#  define FAIL(...)			0
#else
#  define FAIL(...)			(LOG(__VA_ARGS__), 0)
#endif

#include "numbers.cpp"
#include "bits.cpp"
#include "linkedlist.cpp"
#include "string.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "time.cpp"
#include "binaryreader.cpp"

union opaque64 {
	void* p;
	float64 f;
	int64 i;
};