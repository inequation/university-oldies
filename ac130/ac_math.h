// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

#ifndef AC_MATH_H
#define AC_MATH_H

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

/// Vector for intermediate calculations used in some of the macros.
extern ac_vec4_t			__ac_tmpvec;

/// Write from __m128 (a) to flat floats (b).
#define ac_vec_tofloat(a, b)		_mm_store_ps((b), (a).sse);
/// Write from flat floats (a) to __m128 (b).
#define ac_vec_tosse(a, b)			ac_vec_set((b),							\
										(a)[0], (a)[1], (a)[2], (a)[3])
/// a = [x, y, z, w]
#define ac_vec_set(a, x, y, z, w)	(a).sse = _mm_set_ps((w), (z), (y), (x))
/// a = [b, b, b, b]
#define ac_vec_setall(a, b)			(a).sse = _mm_set1_ps((b))
/// c = a + b
#define ac_vec_add(a, b, c)			(c).sse = _mm_add_ps((a).sse, (b).sse)
/// c = a - b
#define ac_vec_sub(a, b, c)			(c).sse = _mm_sub_ps((a).sse, (b).sse)
/// c = a * b (c1 = a1 * b1, c2 = a2 * b2 etc.)
#define ac_vec_mul(a, b, c) 		(c).sse = _mm_mul_ps((a).sse, (b).sse)
/// d = a * b + c
#define ac_vec_ma(a, b, c, d)		(d).sse = _mm_add_ps(					\
									_mm_mul_ps((a).sse, (b).sse), (c).sse)
/// Dot product of a and b
#define ac_vec_dot(a, b)			(										\
										ac_vec_mul((a), (b), __ac_tmpvec),	\
										__ac_tmpvec.f[0]					\
										+ __ac_tmpvec.f[1]					\
										+ __ac_tmpvec.f[2]					\
										+ __ac_tmpvec.f[3]					\
									)
/// Length of vector a
#define ac_vec_length(a)			sqrtf(ac_vec_dot((a), (a)))

#endif // AC_MATH_H
