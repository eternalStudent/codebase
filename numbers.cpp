typedef unsigned char byte;

#include <stdint.h>

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#define MAX_INT16 0x7fff
#define MAX_INT32 0x7fffffff
#define MAX_INT64 0x7fffffffffffffff

#define MIN_INT16 0x8000
#define MIN_INT32 0x80000000
#define MIN_INT64 0x8000000000000000

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;
typedef double float64;

#define MIN(x,y) (x)<(y)?(x):(y)
#define MAX(x,y) (x)<(y)?(y):(x)
#define ABS(x)   (x)<0? -(x):(x)

inline float32 INF32() {
    union {uint32 u; float32 f;} data;
    data.u = 0x7f800000;
    return data.f;
}

inline float32 PI() {
    union {uint32 u; float32 f;} data;
    data.u = 0x40490FDB;
    return data.f;
}

inline float32 DEG_TO_RAD() {
    union {uint32 u; float32 f;} data;
    data.u = 0x3C8EFA35;
    return data.f;
}

// =2^exp
inline float32 LoadExp(int32 exp) {
    uint32 exp_part = (uint32)(exp+127);
    union {uint32 u; float32 f;} data;
    data.u = exp_part << 23;
    return data.f;
}

#include <math.h>