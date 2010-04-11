// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Game logic module

#include "ac130.h"

const uchar	*g_hmap;

bool ac_game_init(void) {
	// set new terrain heightmap
	g_hmap = ac_gen_terrain(0xDEADBEEF);
	ac_renderer_set_heightmap((uchar *)g_hmap);
	return true;
}

void ac_game_shutdown(void) {
}

#define FLOATING_RADIUS		200.f
#define TIME_SCALE			0.09
#define MOUSE_SCALE			0.0015
void ac_game_frame(int ticks, float frameTime, ac_input_t *input) {
	static ac_viewpoint_t vp = {
		{{0, 0, 0, 0}},
		{0, 0},
		0
	};
	float time = (float)ticks * 0.001;
	float plane_angle = time * TIME_SCALE;
	ac_vec4_t tmp;

	vp.angles[0] -= ((float)input->deltaX) * MOUSE_SCALE
		+ frameTime * TIME_SCALE;	// keep the cam in sync with the plane
	vp.angles[1] -= ((float)input->deltaY) * MOUSE_SCALE;
#if 1
	if (vp.angles[0] < -plane_angle + M_PI * 0.2)
		vp.angles[0] = -plane_angle + M_PI * 0.2;
	else if (vp.angles[0] > -plane_angle + M_PI * 0.8)
		vp.angles[0] = -plane_angle + M_PI * 0.8;
	if (vp.angles[1] > M_PI * -0.125)
		vp.angles[1] = M_PI * -0.125;
	else if (vp.angles[1] < M_PI * -0.45)
		vp.angles[1] = M_PI * -0.45;
#else
	if (vp.angles[1] > M_PI * 0.5)
		vp.angles[1] = M_PI * 0.5;
	else if (vp.angles[1] < M_PI * -0.5)
		vp.angles[1] = M_PI * -0.5;
#endif

	vp.origin = ac_vec_set(0.f, 100.f, 0.f, 0.f);
	tmp = ac_vec_set(cosf(plane_angle) * FLOATING_RADIUS,
				0.f,
				sinf(plane_angle) * FLOATING_RADIUS,
				0.f);
	vp.origin = ac_vec_add(tmp, vp.origin);
	vp.fov = 45.f / 180.f * M_PI;
	ac_renderer_start_scene(&vp);
	ac_renderer_finish_3D();
	ac_renderer_composite(false);
}
