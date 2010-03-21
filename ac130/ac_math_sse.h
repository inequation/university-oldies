// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

#ifndef AC_MATH_SSE_H
#define AC_MATH_SSE_H

// SSE intrinsics headers
#include <xmmintrin.h>
#include <emmintrin.h>

#ifdef __GNUC__
	#define ALIGNED_16	__attribute__ ((__aligned__ (16)))
#else
	#define ALIGNED_16 	__declspec(align(16))
#endif

/// SSE variant of the 4-element vector.
typedef union {
	ALIGNED_16 float		f[4];	/// Traditional floating point numbers.
	__m128					v;		/// SSE 128-bit data type.
} ac_vec4_t;

/// Convert from __m128 to flat floats.
#define ac_vec_tofloat(a)			_mm_store_ps((a).f, (a).v);
/// Convert from flat floats to __m128.
#define ac_vec_tosse(a)				ac_vec_set((a),							\
										(a).f[0], (a).f[1], (a).f[2] (a).f[3])
/// a = [x, y, z, w]
#define ac_vec_set(a, x, y, z, w)	(a).v = _mm_set_ps((x), (y), (z), (w))
/// a = [b, b, b, b]
#define ac_vec_zero(a, b)			(a).v = _mm_set1_ps((b))
/// c = a + b
#define ac_vec_add(a, b, c)			(c).v = _mm_add_ps((a).v, (b).v)
/// c = a - b
#define ac_vec_sub(a, b, c)			(c).v = _mm_sub_ps((a).v, (b).v)
/// c = a * b (c1 = a1 * b1, c2 = a2 * b2 etc.)
#define ac_vec_mul(a, b, c) 		(c).v = _mm_mul_ps((a).v, (b).v)
/// d = a * b + c
#define ac_vec_ma(a, b, c, d)		(d).v = _mm_add_ps(						\
									_mm_mul_ps((a).v, (b).v), (c).v)

#endif // AC_MATH_SSE_H
