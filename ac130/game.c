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

void ac_game_frame(float frameTime, ac_input_t *input) {
	static ac_viewpoint_t vp = {
		{{0, 0, 0, 0}},
		{0, 0},
		0
	};
	vp.angles[0] += ((float)input->deltaX) * 0.01;
	vp.angles[1] += ((float)input->deltaY) * 0.01;
	if (vp.angles[0] > M_PI)
		vp.angles[0] -= M_PI * 2;
	else if (vp.angles[0] < -M_PI)
		vp.angles[0] += M_PI * 2;
	if (vp.angles[1] > M_PI)
		vp.angles[1] -= M_PI * 2;
	else if (vp.angles[1] < -M_PI)
		vp.angles[1] += M_PI * 2;
	ac_vec_set(vp.origin, 0.f, 300.f, 0.f, 0.f);
	vp.fov = M_PI * 0.15;
	ac_renderer_start_scene(&vp);
	ac_renderer_finish_3D();
	ac_renderer_composite(false);
	if (input->deltaX || input->deltaY)
		printf("%.1f %.1f\n", vp.angles[0] / M_PI * 180.f, vp.angles[1] / M_PI * 180.f);
}
