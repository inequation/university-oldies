// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Math module; actually, just a place where the temporary vector can be safely
// compiled into the program

#include "ac_math.h"

inline ac_vec4_t ac_vec_set(float x, float y, float z, float w) {
	return (ac_vec4_t)_mm_set_ps(w, z, y, x);
}

inline ac_vec4_t ac_vec_setall(float b) {
	return (ac_vec4_t)_mm_set1_ps(b);
}

inline ac_vec4_t ac_vec_add(ac_vec4_t a, ac_vec4_t b) {
	return (ac_vec4_t)_mm_add_ps(a.sse, b.sse);
}

inline ac_vec4_t ac_vec_sub(ac_vec4_t a, ac_vec4_t b) {
	return (ac_vec4_t)_mm_sub_ps(a.sse, b.sse);
}

inline ac_vec4_t ac_vec_mul(ac_vec4_t a, ac_vec4_t b) {
	return (ac_vec4_t)_mm_mul_ps(a.sse, b.sse);
}

inline ac_vec4_t ac_vec_mulf(ac_vec4_t a, float b) {
	ac_vec4_t tmp;
	tmp = ac_vec_setall(b);
	return (ac_vec4_t)_mm_mul_ps(a.sse, tmp.sse);
}

inline ac_vec4_t ac_vec_ma(ac_vec4_t a, ac_vec4_t b, ac_vec4_t c) {
	return (ac_vec4_t)_mm_add_ps(_mm_mul_ps(a.sse, b.sse), c.sse);
}

inline float ac_vec_dot(ac_vec4_t a, ac_vec4_t b) {
	ac_vec4_t tmp = ac_vec_mul(a, b);
	return tmp.f[0] + tmp.f[1] + tmp.f[2] + tmp.f[3];
}

inline ac_vec4_t ac_vec_cross(ac_vec4_t a, ac_vec4_t b) {
	return ac_vec_set(a.f[1] * b.f[2] - a.f[2] * b.f[1],
					a.f[2] * b.f[0] - a.f[0] * b.f[2],
					a.f[0] * b.f[1] - a.f[1] * b.f[0],
					0.f);
}

inline float ac_vec_length(ac_vec4_t a) {
	return sqrtf(ac_vec_dot(a, a));
}

inline ac_vec4_t ac_vec_normalize(ac_vec4_t a) {
	float invl = 1.f / ac_vec_length(a);
	return ac_vec_mulf(a, invl);
}

inline void ac_vec_tofloat(ac_vec4_t a, float b[4]) {
	_mm_store_ps(b, a.sse);
}

inline ac_vec4_t ac_vec_tosse(float *f) {
	return ac_vec_set(f[0], f[1], f[2], f[3]);
}
