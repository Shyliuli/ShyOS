#ifndef SHY_TYPE_H
#define SHY_TYPE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define var auto
#elif defined(__GNUC__) || defined(__clang__)
#define var __auto_type
#else
#error "var requires C23 auto or GNU __auto_type"
#endif

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef intptr_t isize;
typedef uintptr_t usize;

typedef float f32;
typedef double f64;

#endif
