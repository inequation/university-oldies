// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

#ifndef AC_MATH_H
#define AC_MATH_H

// comment this define out to disable SSE2
#define USE_SSE

#ifdef USE_SSE
	#include "ac_math_sse.h"
#else
	// traditional CPU floating point math
	#include "ac_math_float.h"
#endif // USE_SSE

/// Standard 3x3 (or 3x4, actually) matrix.
typedef ac_vec4_t			ac_mat3_t[3];

#endif // AC_MATH_H
