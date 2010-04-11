// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

#ifndef AC_MATH_H
#define AC_MATH_H

#include <math.h>
// SSE intrinsics headers
#include <xmmintrin.h>
#include <emmintrin.h>

#ifdef __GNUC__
	#define ALIGNED_16	__attribute__ ((__aligned__ (16)))
#else
	#define ALIGNED_16 	__declspec(align(16))
#endif

/// 4-element vector union.
typedef union {
	ALIGNED_16 float		f[4];	/// Traditional floating point numbers.
	__m128					sse;	/// SSE 128-bit data type.
} ac_vec4_t;

/// a = [x, y, z, w]
extern inline ac_vec4_t ac_vec_set(float x, float y, float z, float w);

/// a = [b, b, b, b]
extern inline ac_vec4_t ac_vec_setall(float b);

/// c = a + b
extern inline ac_vec4_t ac_vec_add(ac_vec4_t a, ac_vec4_t b);

/// c = a - b
extern inline ac_vec4_t ac_vec_sub(ac_vec4_t a, ac_vec4_t b);

/// c = a * b (c1 = a1 * b1, c2 = a2 * b2 etc.)
extern inline ac_vec4_t ac_vec_mul(ac_vec4_t a, ac_vec4_t b);

/// c = [a1 * b, a2 * b, a3 * b, a4 * b]
extern inline ac_vec4_t ac_vec_mulf(ac_vec4_t a, float b);

/// d = a * b + c
extern inline ac_vec4_t ac_vec_ma(ac_vec4_t a, ac_vec4_t b, ac_vec4_t c);

/// Dot product of a and b
extern inline float ac_vec_dot(ac_vec4_t a, ac_vec4_t b);

/// Cross product of a and b
extern inline ac_vec4_t ac_vec_cross(ac_vec4_t a, ac_vec4_t b);

/// Length of vector a
extern inline float ac_vec_length(ac_vec4_t a);

/// Vector normalization
extern inline ac_vec4_t ac_vec_normalize(ac_vec4_t a);

/// Write from __m128 (a) to flat floats (b).
extern inline void ac_vec_tofloat(ac_vec4_t a, float b[4]);

/// Write from flat floats (a) to __m128 (b).
extern inline ac_vec4_t ac_vec_tosse(float *f);

#endif // AC_MATH_H
