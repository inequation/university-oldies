// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Game logic module

#include "ac130.h"

typedef enum {
	WP_NONE,
	/// M61 Vulcan minigun bullet
	WP_M61,
	/// L/60 Bofors cannon round
	WP_L60,
	/// M102 howitzer round
	WP_M102
} ac_weap_t;

typedef struct {
	ac_weap_t	weap;
	ac_vec4_t	pos;
	ac_vec4_t	vel;
} ac_projectile_t;

/// Real rate of fire: 6000 rounds per minute
#define WEAP_FIREDELAY_M61	0.01
/// Real rate of fire: 120 rounds per minute
#define WEAP_FIREDELAY_L60	0.5
/// Real rate of fire: 10 rounds per minute
#define WEAP_FIREDELAY_M102	6
/// Real muzzle velocity: 1050m/s
#define WEAP_MUZZVEL_M61	1050
/// Real muzzle velocity: 881m/s
#define WEAP_MUZZVEL_L60	881
/// Real muzzle velocity: 494m/s
#define WEAP_MUZZVEL_M102	494

#define MAX_PROJECTILES		1024
ac_projectile_t	g_projs[MAX_PROJECTILES];
int				g_num_projs;

int				g_num_trees;
ac_tree_t		*g_trees;

int				g_num_bldgs;
ac_bldg_t		*g_bldgs;

// some helper vectors for physics
ac_vec4_t		g_gravity;
ac_vec4_t		g_frameTimeVec;

ac_vec4_t		g_forward;

ac_viewpoint_t	g_viewpoint = {
	{{0, 0, 0, 0}},
	{0, 0},
	0
};

bool ac_game_init(void) {
	// set new terrain heightmap
	ac_gen_terrain(0xDEADBEEF);
	ac_renderer_set_heightmap();

	// generate proplists
	g_trees = malloc(sizeof(*g_trees) * MAX_NUM_TREES);
	g_bldgs = malloc(sizeof(*g_bldgs) * MAX_NUM_TREES);
	ac_gen_proplists(&g_num_trees, g_trees, &g_num_bldgs, g_bldgs);

	g_gravity = ac_vec_set(0, -9.81, 0, 0);

	memset(g_projs, 0, sizeof(g_projs));
	return true;
}

void ac_game_shutdown(void) {
	free(g_trees);
	free(g_bldgs);
}

float ac_game_sample_height(float x, float y) {
	// bilinear filtering
	float xi, yi, xfrac, yfrac;
	float fR1, fR2;

	xfrac = modff(x, &xi);
	yfrac = modff(y, &yi);

	// bilinear filtering
	fR1 = (1.f - xfrac) * g_heightmap[(int)yi * HEIGHTMAP_SIZE + (int)xi]
		+ xfrac * g_heightmap[(int)yi * HEIGHTMAP_SIZE + (int)xi + 1];
	fR2 = (1.f - xfrac) * g_heightmap[((int)yi + 1) * HEIGHTMAP_SIZE + (int)xi]
		+ xfrac * g_heightmap[((int)yi + 1) * HEIGHTMAP_SIZE + (int)xi + 1];
	return ((1.f - yfrac) * fR1 + yfrac * fR2) * HEIGHT_SCALE;
}

ac_vec4_t ac_game_collide(ac_vec4_t p1, ac_vec4_t p2) {
	ac_vec4_t half = ac_vec_setall(0.5);
	ac_vec4_t v = ac_vec_sub(p2, p1);
	ac_vec4_t p;
	float h;
	int i;
	// bisect for at most 4 steps or until a solution within 10cm is found
	for (i = 0; i < 4; i++) {
		v = ac_vec_mul(v, half);
		p = ac_vec_add(p1, v);
		h = ac_game_sample_height(p.f[0], p.f[2]);
		if (fabs(p.f[1] - h) < 0.1)
			break;
		if (p.f[1] < h)
			p2 = ac_vec_add(p1, v);
		else
			p1 = ac_vec_add(p1, v);
	}
	p.f[1] = h;
	return p;
}

void ac_game_advance_projectiles(void) {
	int i;
	ac_vec4_t grav = ac_vec_mul(g_gravity, g_frameTimeVec);
	ac_vec4_t npos, ip;
	ac_vec4_t ofs = ac_vec_set(HEIGHTMAP_SIZE / 2, 0, HEIGHTMAP_SIZE / 2, 0);
	float h;

	for (i = 0; i < sizeof(g_projs) / sizeof(g_projs[0]); i++) {
		if (g_projs[i].weap == WP_NONE)
			continue;
		// find the new position
		npos = ac_vec_mul(g_projs[i].vel, g_frameTimeVec);
		npos = ac_vec_add(g_projs[i].pos, npos);
		npos = ac_vec_add(ofs, npos);
		// see if we haven't gone off the map
		if (npos.f[0] < 0 || npos.f[2] < 0
			|| npos.f[0] > HEIGHTMAP_SIZE - 1
			|| npos.f[2] > HEIGHTMAP_SIZE - 1) {
			printf("OUT! %d\n", (int)g_projs[i].weap);
			g_projs[i].weap = WP_NONE;
			continue;
		}
		// terrain collision detection
		h = ac_game_sample_height(npos.f[0], npos.f[2]);
		if (npos.f[1] < h) {
			// find the impact point
			ip = ac_game_collide(ac_vec_add(ofs, g_projs[i].pos), npos);
			printf("HIT! %f %f %f\n", ip.f[0], ip.f[1], ip.f[2]);
			g_projs[i].weap = WP_NONE;
		}
		g_projs[i].pos = ac_vec_sub(npos, ofs);
		// add gravity
		g_projs[i].vel = ac_vec_add(g_projs[i].vel, grav);
		switch (g_projs[i].weap) {
			case WP_M61:
				break;
			case WP_L60:
				break;
			case WP_M102:
				// no tracers for the howitzer
				break;
			default:
				break;
		}
	}
}

void ac_game_fire_weapon(ac_weap_t w) {
	int i;
	printf("FIRE! %d\n", (int)w);
	// find a free projectile slot
	for (i = 0; i < sizeof(g_projs) / sizeof(g_projs[0]); i++) {
		if (g_projs[i].weap == WP_NONE) {
			g_projs[i].weap = w;
			g_projs[i].pos = g_viewpoint.origin;
			switch (w) {
				case WP_M61:
					g_projs[i].vel = ac_vec_mulf(g_forward, WEAP_MUZZVEL_M61);
					break;
				case WP_L60:
					g_projs[i].vel = ac_vec_mulf(g_forward, WEAP_MUZZVEL_L60);
					break;
				case WP_M102:
					g_projs[i].vel = ac_vec_mulf(g_forward, WEAP_MUZZVEL_M102);
					break;
				// shut up compiler
				case WP_NONE:
					break;
			}
			return;
		}
	}
}

void ac_game_weapons_think(ac_input_t *in, float t) {
	// times since last shots
	static float m61 = WEAP_FIREDELAY_M61;
	static float l60 = WEAP_FIREDELAY_L60;
	static float m102 = WEAP_FIREDELAY_M102;
	static ac_weap_t weap = WP_M61;
	static bool rpressed = false;

	m61 += t;
	l60 += t;
	m102 += t;

	if (in->flags & INPUT_MOUSE_LEFT) {
		// fire if the gun's cooled down already
		switch (weap) {
			case WP_M61:
				if (m61 >= WEAP_FIREDELAY_M61) {
					m61 = 0;
					ac_game_fire_weapon(weap);
				}
				break;
			case WP_L60:
				if (l60 >= WEAP_FIREDELAY_L60) {
					l60 = 0;
					ac_game_fire_weapon(weap);
				}
				break;
			case WP_M102:
				if (m102 >= WEAP_FIREDELAY_M102) {
					m102 = 0;
					ac_game_fire_weapon(weap);
				}
				break;
			// shut up compiler
			case WP_NONE:
				break;
		}
		rpressed = false;
	} else if (in->flags & INPUT_MOUSE_RIGHT && !rpressed) {
		// cycle the weapons
		rpressed = true;
		weap++;
		if (weap > WP_M102)
			weap = WP_M61;
	} else if (!(in->flags & INPUT_MOUSE_RIGHT))
		rpressed = false;
}

#define FLOATING_RADIUS		200.f
#define TIME_SCALE			-0.04
#define MOUSE_SCALE			0.001
void ac_game_frame(int ticks, float frameTime, ac_input_t *input) {
	float time = (float)ticks * 0.001;
	float plane_angle = time * TIME_SCALE;
	ac_vec4_t tmp;

	g_frameTimeVec = ac_vec_setall(frameTime);

	// advance the non-player elements of the world
	ac_game_advance_projectiles();

	// handle viewpoint movement
	g_viewpoint.angles[0] -= ((float)input->deltaX) * MOUSE_SCALE
		+ frameTime * TIME_SCALE;	// keep the cam in sync with the plane
	g_viewpoint.angles[1] -= ((float)input->deltaY) * MOUSE_SCALE;
#if 1
	if (g_viewpoint.angles[0] < -plane_angle + M_PI * 0.2)
		g_viewpoint.angles[0] = -plane_angle + M_PI * 0.2;
	else if (g_viewpoint.angles[0] > -plane_angle + M_PI * 0.8)
		g_viewpoint.angles[0] = -plane_angle + M_PI * 0.8;
	if (g_viewpoint.angles[1] > M_PI * -0.165)
		g_viewpoint.angles[1] = M_PI * -0.165;
	else if (g_viewpoint.angles[1] < M_PI * -0.45)
		g_viewpoint.angles[1] = M_PI * -0.45;
#else
	if (g_viewpoint.angles[1] > M_PI * 0.5)
		g_viewpoint.angles[1] = M_PI * 0.5;
	else if (g_viewpoint.angles[1] < M_PI * -0.5)
		g_viewpoint.angles[1] = M_PI * -0.5;
#endif

	g_viewpoint.origin = ac_vec_set(0.f, 200.f, 0.f, 0.f);
	tmp = ac_vec_set(cosf(plane_angle) * FLOATING_RADIUS,
				0.f,
				sinf(plane_angle) * FLOATING_RADIUS,
				0.f);
	g_viewpoint.origin = ac_vec_add(tmp, g_viewpoint.origin);
	g_viewpoint.fov = M_PI * 0.06;

	// calculate firing axis
	g_forward = ac_vec_set(
		-cosf(g_viewpoint.angles[1]) * sinf(g_viewpoint.angles[0]),
		sinf(g_viewpoint.angles[1]),
		-cosf(g_viewpoint.angles[1]) * cosf(g_viewpoint.angles[0]),
		0);

	// operate the weapons
	ac_game_weapons_think(input, frameTime);

	ac_renderer_start_scene(&g_viewpoint);
	ac_renderer_finish_3D();
	ac_renderer_composite(false);
}
