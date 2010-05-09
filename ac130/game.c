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
	WP_M102,
	/// M61 tracer round; not really a separate weapon, but we need to
	/// distinguish between normal and tracer rounds
	WP_M61_TRACER
} weap_t;

typedef struct {
	weap_t	weap;
	ac_vec4_t	pos;
	ac_vec4_t	vel;
} projectile_t;

typedef struct {
	ac_vec4_t	pos;
	ac_vec4_t	vel;
	float		scale;
	float		life;
	float		alpha;
	float		angle;
	weap_t		weap;
} particle_t;

/// Real rate of fire: 6000 rounds per minute
#define WEAP_FIREDELAY_M61	0.01
/// Real rate of fire: 120 rounds per minute
#define WEAP_FIREDELAY_L60	0.5
/// Real rate of fire: 10 rounds per minute
#define WEAP_FIREDELAY_M102	6
/// Real muzzle velocity: 1050m/s
#define WEAP_MUZZVEL_M61	350//1050
/// Real muzzle velocity: 881m/s
#define WEAP_MUZZVEL_L60	300//881
/// Real muzzle velocity: 494m/s
#define WEAP_MUZZVEL_M102	160//494

#define MAX_PROJECTILES		512
projectile_t	g_projs[MAX_PROJECTILES];
#define MAX_PARTICLES		1024
particle_t		g_particles[MAX_PARTICLES];

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

#define SHAKE_TIME			0.4
float			g_shake_time = 0;

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
	memset(g_particles, 0, sizeof(g_particles));
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

int ac_game_particle_cmp(const void *p1, const void *p2) {
	float diff;
	// push inactive particles towards the end of the array
	if (((particle_t *)p1)->weap == WP_NONE)
		return 1;
	if (((particle_t *)p2)->weap == WP_NONE)
		return -1;
	// sort the particles back-to-front
	// cast the particle positions onto the view axis
	diff = ac_vec_dot(((particle_t *)p1)->pos, g_forward)
		- ac_vec_dot(((particle_t *)p2)->pos, g_forward);
	if (diff < 0.f)	// p1 is closer than p2, draw p2 first
		return 1;
	if (diff > 0.f)	// p2 is closer than p1, draw p1 first
		return -1;
	return 0;	// p1 and p2 are equally distant from the viewpoint
}

#define USE_QSORT
void ac_game_advance_particles(float t) {
	int i;
	ac_vec4_t grav = ac_vec_mul(g_gravity, g_frameTimeVec);
	ac_vec4_t tmp;
	float f, g;
	particle_t *p;

	// we need the proper Z-order, so sort the particle array first
#ifdef USE_QSORT
	qsort(g_particles, sizeof(g_particles) / sizeof(g_particles[0]),
		sizeof(g_particles[0]), ac_game_particle_cmp);
#endif

	for (i = 0, p = g_particles;
		i < sizeof(g_particles) / sizeof(g_particles[0]); i++, p++) {
		// the array is sorted, so break upon the first occurence of an unused
		// particle
		if (p->weap == WP_NONE)
#ifdef USE_QSORT
			break;
#else
			continue;
#endif
		p->life -= t;
		if (p->life < 0.f) {
			p->weap = WP_NONE;
			continue;
		}
		// find the new position
		p->pos = ac_vec_add(p->pos, ac_vec_mul(p->vel, g_frameTimeVec));
		f = ac_vec_decompose(p->vel, &tmp);
		switch (p->weap) {
			case WP_M61:
			case WP_M61_TRACER:
				f *= f * -0.25;
				g = 0.5;
				// fade the alpha away during the last second
				if (p->life < 1.f)
					p->alpha = p->life;
				break;
			case WP_L60:
				f *= f * -0.8;
				g = 0.025;
				// fade the alpha away during the last 2 seconds
				if (p->life < 2.f)
					p->alpha = p->life * 0.5;
				break;
			case WP_M102:
				f *= f * -0.6;
				g = 0.02;
				// fade the alpha away during the last 4 seconds
				if (p->life < 4.f)
					p->alpha = p->life * 0.25;
				break;
			default:
				break;
		}
		// slow the smoke down
		tmp = ac_vec_mulf(tmp, f);
		tmp = ac_vec_mul(tmp, g_frameTimeVec);
		p->vel = ac_vec_add(p->vel, tmp);
		// add reduced gravity
		tmp = ac_vec_mulf(grav, g);
		p->vel = ac_vec_add(p->vel, tmp);
		// draw the particle
		ac_renderer_draw_fx(p->pos, p->scale, p->alpha, p->angle);
	}
}

void ac_game_explode(ac_vec4_t pos, weap_t w) {
	int i, j;
	particle_t *p;
	ac_vec4_t dir;

	switch (w) {
		case WP_M61:
		case WP_M61_TRACER:
			for (i = 0, j = 0, p = g_particles;
				i < sizeof(g_particles) / sizeof(g_particles[0]) && j < 4;
				i++, p++) {
				if (p->weap != WP_NONE)
					continue;
				p->weap = w;
				p->pos = pos;
				p->alpha = 1.f;
				p->scale = 0.4;
				p->life = 1.5 + 0.001 * (rand() % 101);
				p->angle = 0.01 * (rand() % 628);
				dir = ac_vec_set(-20 + rand() % 41, 100, -20 + rand() % 41, 0);
				dir = ac_vec_normalize(dir);
				p->vel = ac_vec_mulf(dir, 10.f);
				j++;
			}
			break;
		case WP_L60:
			pos = ac_vec_add(pos, ac_vec_set(0, 3, 0, 0));
			for (i = 0, j = 0, p = g_particles;
				i < sizeof(g_particles) / sizeof(g_particles[0]) && j < 12;
				i++, p++) {
				if (p->weap != WP_NONE)
					continue;
				p->weap = w;
				p->pos = pos;
				p->alpha = 1.f;
				p->scale = 3.5 + 0.001 * (rand() % 1001);
				p->life = 5.75 + 0.005 * (rand() % 101);
				p->angle = 0.01 * (rand() % 628);
				if (j < 6)
					dir = ac_vec_set(
						-30 + rand() % 61,
						90,
						-30 + rand() % 61,
						0);
				else
					dir = ac_vec_set(
						-50 + rand() % 101,
						rand() % 40,
						-50 + rand() % 101,
						0);
				dir = ac_vec_normalize(dir);
				p->vel = ac_vec_mulf(dir, 40.f);
				j++;
			}
			break;
		case WP_M102:
			pos = ac_vec_add(pos, ac_vec_set(0, 6, 0, 0));
			for (i = 0, j = 0, p = g_particles;
				i < sizeof(g_particles) / sizeof(g_particles[0]) && j < 18;
				i++, p++) {
				if (p->weap != WP_NONE)
					continue;
				p->weap = w;
				p->pos = pos;
				p->alpha = 1.f;
				p->scale = (j % 2 == 0 ? 9.5 : 6.5) + 0.001 * (rand() % 1001);
				p->life = 8;
				p->angle = 0.01 * (rand() % 628);
				if (j < 9)
					dir = ac_vec_set(
						-30 + rand() % 61,
						90,
						-30 + rand() % 61,
						0);
				else
					dir = ac_vec_set(
						-50 + rand() % 101,
						rand() % 40,
						-50 + rand() % 101,
						0);
				dir = ac_vec_normalize(dir);
				p->vel = ac_vec_mulf(dir, 140.f);
				j++;
			}
			break;
		default:
			break;
	}
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
			//printf("OUT! %d\n", (int)g_projs[i].weap);
			g_projs[i].weap = WP_NONE;
			continue;
		}
		// terrain collision detection
		h = ac_game_sample_height(npos.f[0], npos.f[2]);
		if (npos.f[1] < h) {
			// find the impact point
			ip = ac_game_collide(ac_vec_add(ofs, g_projs[i].pos), npos);
			//printf("HIT! %f %f %f\n", ip.f[0], ip.f[1], ip.f[2]);
			ac_game_explode(ac_vec_sub(ip, ofs), g_projs[i].weap);
			g_projs[i].weap = WP_NONE;
		}
		g_projs[i].pos = ac_vec_sub(npos, ofs);
		// add gravity
		g_projs[i].vel = ac_vec_add(g_projs[i].vel, grav);
		// draw tracers
		switch (g_projs[i].weap) {
			case WP_M61_TRACER:
				ac_renderer_draw_tracer(g_projs[i].pos,
					ac_vec_normalize(g_projs[i].vel), 3.f);
				break;
			case WP_L60:
				ac_renderer_draw_tracer(g_projs[i].pos,
					ac_vec_normalize(g_projs[i].vel), 5.f);
				break;
			default:
				break;
		}
	}
}

void ac_game_fire_weapon(weap_t w, float t) {
	int i;
	static int m61 = 0;
	//printf("FIRE! %d\n", (int)w);
	// find a free projectile slot
	for (i = 0; i < sizeof(g_projs) / sizeof(g_projs[0]); i++) {
		if (g_projs[i].weap == WP_NONE) {
			g_projs[i].weap = w;
			g_projs[i].pos = g_viewpoint.origin;
			//g_projs[i].pos.f[1] += 0.5;
			switch (w) {
				case WP_M61:
					// tracer round every 5 rounds
					if (++m61 % 5 == 0)
						g_projs[i].weap = WP_M61_TRACER;
					g_projs[i].vel = ac_vec_mulf(g_forward, WEAP_MUZZVEL_M61);
					break;
				case WP_L60:
					g_projs[i].vel = ac_vec_mulf(g_forward, WEAP_MUZZVEL_L60);
					break;
				case WP_M102:
					g_projs[i].vel = ac_vec_mulf(g_forward, WEAP_MUZZVEL_M102);
					g_shake_time = t;
					break;
				// shut up compiler
				case WP_NONE:
				case WP_M61_TRACER:
					break;
			}
			return;
		}
	}
}

void ac_game_weapons_think(ac_input_t *in, float t, float ft) {
	// times since last shots
	static float m61 = WEAP_FIREDELAY_M61;
	static float l60 = WEAP_FIREDELAY_L60;
	static float m102 = WEAP_FIREDELAY_M102;
	static weap_t weap = WP_M61;
	static bool rpressed = false;

	m61 += ft;
	l60 += ft;
	m102 += ft;

	if (in->flags & INPUT_MOUSE_LEFT) {
		// fire if the gun's cooled down already
		switch (weap) {
			case WP_M61:
				// this gun needs special handling - it fires at 6000 rpm, so
				// make sure we get all the rounds out even if the FPS stutters
				while (ft >= WEAP_FIREDELAY_M61) {
					ac_game_fire_weapon(weap, t);
					ft -= WEAP_FIREDELAY_M61;
				}
				m61 += ft;
				if (m61 >= WEAP_FIREDELAY_M61) {
					m61 = 0;
					ac_game_fire_weapon(weap, t);
				}
				break;
			case WP_L60:
				if (l60 >= WEAP_FIREDELAY_L60) {
					l60 = 0;
					ac_game_fire_weapon(weap, t);
				}
				break;
			case WP_M102:
				if (m102 >= WEAP_FIREDELAY_M102) {
					m102 = 0;
					ac_game_fire_weapon(weap, t);
				}
				break;
			default:
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
	float fy, fp;
	ac_vec4_t tmp;
	ac_viewpoint_t vp;

	g_frameTimeVec = ac_vec_setall(frameTime);

	// handle viewpoint movement
	g_viewpoint.angles[0] -= ((float)input->deltaX) * MOUSE_SCALE
		+ frameTime * TIME_SCALE;	// keep the cam in sync with the plane
	g_viewpoint.angles[1] -= ((float)input->deltaY) * MOUSE_SCALE;
#if 1
	if (g_viewpoint.angles[0] < -plane_angle + M_PI * 0.2)
		g_viewpoint.angles[0] = -plane_angle + M_PI * 0.2;
	else if (g_viewpoint.angles[0] > -plane_angle + M_PI * 0.8)
		g_viewpoint.angles[0] = -plane_angle + M_PI* 0.8;
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

	g_viewpoint.origin = ac_vec_set(0.f, 250.f, 0.f, 0.f);
	tmp = ac_vec_set(cosf(plane_angle) * FLOATING_RADIUS,
				0.f,
				sinf(plane_angle) * FLOATING_RADIUS,
				0.f);
	g_viewpoint.origin = ac_vec_add(tmp, g_viewpoint.origin);
	g_viewpoint.fov = M_PI * 0.04;

	// calculate firing axis
	// apply bullet spread (~0,45 of a degree)
	fy = g_viewpoint.angles[0] - 0.004 + 0.001 * (rand() % 9);
	fp = g_viewpoint.angles[1] - 0.004 + 0.001 * (rand() % 9);
	g_forward = ac_vec_set(
		-cosf(fp) * sinf(fy), sinf(fp), -cosf(fp) * cosf(fy), 0);

	// operate the weapons
	ac_game_weapons_think(input, time, frameTime);

	// generate another viewpoint (for gun shakes etc.)
	if (time - g_shake_time <= SHAKE_TIME) {
		float lerp = expf(-4 * (time - g_shake_time) / SHAKE_TIME);
		memcpy(&vp, &g_viewpoint, sizeof(vp));
		vp.angles[0] += (-0.0175 + 0.000035 * (rand() % 1001)) * lerp;
		vp.angles[1] += (-0.0175 + 0.000035 * (rand() % 1001)) * lerp;
		ac_renderer_start_scene(&vp);
		//ac_renderer_start_scene(&g_viewpoint);
	} else
		ac_renderer_start_scene(&g_viewpoint);

	// advance the non-player elements of the world
	ac_game_advance_projectiles();
	ac_renderer_start_fx();
	ac_game_advance_particles(frameTime);
	ac_renderer_finish_fx();

	ac_renderer_finish_3D();
	ac_renderer_composite(false);
}
