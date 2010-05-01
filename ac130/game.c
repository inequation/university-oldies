// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Game logic module

#include "ac130.h"

int			g_num_trees;
ac_tree_t	*g_trees;

int			g_num_bldgs;
ac_bldg_t	*g_bldgs;

bool ac_game_init(void) {
	// set new terrain heightmap
	ac_gen_terrain(0xDEADBEEF);
	ac_renderer_set_heightmap();

	// generate proplists
	g_trees = malloc(sizeof(*g_trees) * MAX_NUM_TREES);
	g_bldgs = malloc(sizeof(*g_bldgs) * MAX_NUM_TREES);
	ac_gen_proplists(&g_num_trees, g_trees, &g_num_bldgs, g_bldgs);
	return true;
}

void ac_game_shutdown(void) {
	free(g_trees);
	free(g_bldgs);
}

#define FLOATING_RADIUS		200.f
#define TIME_SCALE			0.04
#define MOUSE_SCALE			0.001
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
	if (vp.angles[1] > M_PI * -0.165)
		vp.angles[1] = M_PI * -0.165;
	else if (vp.angles[1] < M_PI * -0.45)
		vp.angles[1] = M_PI * -0.45;
#else
	if (vp.angles[1] > M_PI * 0.5)
		vp.angles[1] = M_PI * 0.5;
	else if (vp.angles[1] < M_PI * -0.5)
		vp.angles[1] = M_PI * -0.5;
#endif

	vp.origin = ac_vec_set(0.f, 200.f, 0.f, 0.f);
	tmp = ac_vec_set(cosf(plane_angle) * FLOATING_RADIUS,
				0.f,
				sinf(plane_angle) * FLOATING_RADIUS,
				0.f);
	vp.origin = ac_vec_add(tmp, vp.origin);
	vp.fov = M_PI * 0.06;
	ac_renderer_start_scene(&vp);
	ac_renderer_finish_3D();
	ac_renderer_composite(false);
}
