// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

#ifndef AC_MATH_H
#define AC_MATH_H

#include <math.h>

// general math
#define MIN(a, b)			((a) < (b) ? (a) : (b))
#define MAX(a, b)			((a) > (b) ? (a) : (b))

// vector math

// SSE intrinsics headers
#include <xmmintrin.h>

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

// This is to fix certain SSE-related crashes. What happens is that since I'm
// mixing non-SSE and SSE code, it can sometimes happen that the stack is
// aligned to 4 bytes (the legacy way) instead of 16 bytes (what SSE expects).
// This can lead to seemingly random crashes. Adding this attribute causes the
// compiler to add some code to enforce 16-byte alignment.
#define STACK_ALIGN		__attribute__((force_align_arg_pointer))

/// a = [x, y, z, w]
extern inline ac_vec4_t ac_vec_set(float x, float y, float z, float w)
	STACK_ALIGN;

/// a = [b, b, b, b]
extern inline ac_vec4_t ac_vec_setall(float b) STACK_ALIGN;

/// b = -a
extern inline ac_vec4_t ac_vec_negate(ac_vec4_t a) STACK_ALIGN;

/// c = a + b
extern inline ac_vec4_t ac_vec_add(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// c = a - b
extern inline ac_vec4_t ac_vec_sub(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// c = a * b (c1 = a1 * b1, c2 = a2 * b2 etc.)
extern inline ac_vec4_t ac_vec_mul(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// c = [a1 * b, a2 * b, a3 * b, a4 * b]
extern inline ac_vec4_t ac_vec_mulf(ac_vec4_t a, float b) STACK_ALIGN;

/// d = a * b + c
extern inline ac_vec4_t ac_vec_ma(ac_vec4_t a, ac_vec4_t b, ac_vec4_t c)
	STACK_ALIGN;

/// Dot product of a and b.
extern inline float ac_vec_dot(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// Cross product of a and b.
extern inline ac_vec4_t ac_vec_cross(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// Length of vector a.
extern inline float ac_vec_length(ac_vec4_t a) STACK_ALIGN;

/// Vector normalization.
extern inline ac_vec4_t ac_vec_normalize(ac_vec4_t a) STACK_ALIGN;

/// Vector decomposition into a unit length direction vector and length scalar.
/// \param v	vector to decompose
/// \param a	pointer to vector to put the direction vector into
extern inline float ac_vec_decompose(ac_vec4_t b, ac_vec4_t *a) STACK_ALIGN;

/// Write from __m128 (a) to flat floats (b).
extern inline void ac_vec_tofloat(ac_vec4_t a, float b[4]) STACK_ALIGN;

/// Write from flat floats (a) to __m128 (b).
extern inline ac_vec4_t ac_vec_tosse(float *f) STACK_ALIGN;

#endif // AC_MATH_H
