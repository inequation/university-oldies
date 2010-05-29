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

bool			g_paused = true;

// some helper vectors for physics and other mechanics
float			g_time;
float			g_frameTime;
ac_vec4_t		g_gravity;
ac_vec4_t		g_frameTimeVec;

ac_vec4_t		g_forward;

ac_viewpoint_t	g_viewpoint = {
	{{0, 0, 0, 0}},
	{0, 0},
	0
};

weap_t			g_weapon = WP_M61;

#define SHAKE_TIME			0.45
float			g_shake_time = -SHAKE_TIME;

#define NEGATIVE_TIME		0.25
float			g_neg_time = 0.f;

#define EXPLOSION_TIME		2.0
float			g_expl_time = -EXPLOSION_TIME;

bool ac_game_init(void) {
	// set new terrain heightmap
	ac_gen_terrain(0xDEADBEEF);
	ac_renderer_set_heightmap();

	// generate proplists
	g_trees = malloc(sizeof(*g_trees) * MAX_NUM_TREES);
	g_bldgs = malloc(sizeof(*g_bldgs) * MAX_NUM_TREES);
	ac_gen_proplists(&g_num_trees, g_trees, &g_num_bldgs, g_bldgs);

	// final tick before game is ready
	ac_game_loading_tick();

	g_gravity = ac_vec_set(0, -9.81, 0, 0);

	memset(g_projs, 0, sizeof(g_projs));
	memset(g_particles, 0, sizeof(g_particles));

	g_viewpoint.angles[0] = M_PI * 0.5;
	g_viewpoint.angles[1] = M_PI * -0.17;
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
	// also find the nearest and farthest particles
	if (diff < 0.f)	// p1 is closer than p2, draw p2 first
		return 1;
	if (diff > 0.f)	// p2 is closer than p1, draw p1 first
		return -1;
	return 0;	// p1 and p2 are equally distant from the viewpoint
}

#define USE_QSORT
void ac_game_advance_particles(void) {
	int i;
	ac_vec4_t grav = ac_vec_mul(g_gravity, g_frameTimeVec);
	ac_vec4_t tmp;
	float f, g = 1.f;
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
		p->life -= g_frameTime;
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
				f = f * f * -0.25 * g_frameTime;
				g = 0.5;
				// fade the alpha away during the last second
				if (p->life < 1.f)
					p->alpha = p->life;
				break;
			case WP_L60:
				f = f * f * -0.8 * g_frameTime;
				g = 0.025;
				// fade the alpha away during the last 2 seconds
				if (p->life < 2.f)
					p->alpha = p->life * 0.5;
				break;
			case WP_M102:
				f = f * f * -0.6 * g_frameTime;
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
				dir = ac_vec_set(
					-2000 + (rand() % 4001),
					10000,
					-2000 + (rand() % 4001),
					0);
				dir = ac_vec_normalize(dir);
				p->vel = ac_vec_mulf(dir, 10.f);
				j++;
			}
			break;
		case WP_L60:
			pos = ac_vec_add(pos, ac_vec_set(0, 4, 0, 0));
			for (i = 0, j = 0, p = g_particles;
				i < sizeof(g_particles) / sizeof(g_particles[0]) && j < 24;
				i++, p++) {
				if (p->weap != WP_NONE)
					continue;
				p->weap = w;
				p->pos = pos;
				p->alpha = 1.f;
				p->scale = 3.5 + 0.001 * (rand() % 1001);
				p->life = 5.75 + 0.005 * (rand() % 101);
				p->angle = 0.01 * (rand() % 628);
				if (j < 12)
					dir = ac_vec_set(
						-30000 + (rand() % 60001),
						90000,
						-30000 + (rand() % 60001),
						0);
				else
					dir = ac_vec_set(
						-50000 + (rand() % 100001),
						rand() % 4000,
						-50000 + (rand() % 100001),
						0);
				dir = ac_vec_normalize(dir);
				p->vel = ac_vec_mulf(dir, 10 + (rand() % 16));
				if (j % 6 == 0)
					p->vel = ac_vec_mulf(p->vel, 0.2);
				else if (j % 6 == 1)
					p->vel = ac_vec_mulf(p->vel, 0.4);
				j++;
			}
			break;
		case WP_M102:
			g_expl_time = g_time;
			pos = ac_vec_add(pos, ac_vec_set(0, 6, 0, 0));
			for (i = 0, j = 0, p = g_particles;
				i < sizeof(g_particles) / sizeof(g_particles[0]) && j < 36;
				i++, p++) {
				if (p->weap != WP_NONE)
					continue;
				p->weap = w;
				p->pos = pos;
				p->alpha = 1.f;
				p->scale = (j % 2 == 0 ? 9.5 : 6.5) + 0.001 * (rand() % 1001);
				p->life = 8.75 + 0.005 * (rand() % 101);
				p->angle = 0.01 * (rand() % 628);
				if (j < 18)
					dir = ac_vec_set(
						-30000 + (rand() % 60001),
						90000,
						-30000 + (rand() % 60001),
						0);
				else
					dir = ac_vec_set(
						-50000 + (rand() % 100001),
						rand() % 40000,
						-50000 + (rand() % 100001),
						0);
				dir = ac_vec_normalize(dir);
				p->vel = ac_vec_mulf(dir, 130 + (rand() % 21));
				if (j % 3 == 0)
					p->vel = ac_vec_mulf(p->vel, 0.35);
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
		// draw tracers
		switch (g_projs[i].weap) {
			case WP_M61_TRACER:
				ac_renderer_draw_tracer(g_projs[i].pos,
					ac_vec_normalize(g_projs[i].vel), 3.f);
			case WP_M61:	// intentional fall-through!
				// add full gravity
				g_projs[i].vel = ac_vec_add(g_projs[i].vel, grav);
				break;
			case WP_L60:
				ac_renderer_draw_tracer(g_projs[i].pos,
					ac_vec_normalize(g_projs[i].vel), 5.f);
				// add reduced gravity
				g_projs[i].vel = ac_vec_add(g_projs[i].vel,
					ac_vec_mulf(grav, 0.5));
				break;
			case WP_M102:
				// add reduced gravity
				g_projs[i].vel = ac_vec_add(g_projs[i].vel,
					ac_vec_mulf(grav, 0.3));
				break;
			default:	// shut up compiler
				break;
		}
	}
}

void ac_game_fire_weapon(weap_t w) {
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
					g_shake_time = g_time;
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

void ac_game_player_think(ac_input_t *in) {
	// times since last shots
	static float m61 = WEAP_FIREDELAY_M61;
	static float l60 = WEAP_FIREDELAY_L60;
	static float m102 = WEAP_FIREDELAY_M102;
	static bool rpressed = false, npressed = false, pausepressed = false;

	if (g_paused) {
		if (in->flags & INPUT_PAUSE && !pausepressed) {
			g_paused = false;
			pausepressed = true;
		} else if (!(in->flags & INPUT_PAUSE))
			pausepressed = false;
	} else {
		m61 += g_frameTime;
		l60 += g_frameTime;
		m102 += g_frameTime;

		// mouse button handling
		if (in->flags & INPUT_MOUSE_LEFT) {
			// fire if the gun's cooled down already
			switch (g_weapon) {
				case WP_M61:
					// this gun needs special handling - it fires at 6000 rpm, so
					// make sure we get all the rounds out even if the FPS stutters
					{
						float ft = g_frameTime;
						while (ft >= WEAP_FIREDELAY_M61) {
							ac_game_fire_weapon(g_weapon);
							ft -= WEAP_FIREDELAY_M61;
						}
						m61 += ft;
						if (m61 >= WEAP_FIREDELAY_M61) {
							m61 = 0;
							ac_game_fire_weapon(g_weapon);
						}
					}
					break;
				case WP_L60:
					if (l60 >= WEAP_FIREDELAY_L60) {
						l60 = 0;
						ac_game_fire_weapon(g_weapon);
					}
					break;
				case WP_M102:
					if (m102 >= WEAP_FIREDELAY_M102) {
						m102 = 0;
						ac_game_fire_weapon(g_weapon);
					}
					break;
				default:
					break;
			}
			rpressed = false;
		} else if (in->flags & INPUT_MOUSE_RIGHT && !rpressed) {
			// cycle the weapons
			rpressed = true;
			g_weapon++;
			if (g_weapon > WP_M102)
				g_weapon = WP_M61;
		} else if (!(in->flags & INPUT_MOUSE_RIGHT))
			rpressed = false;

		// whot/bhot switch
		if (in->flags & INPUT_NEGATIVE && !npressed) {
			// negative time means positive->negative transition
			if (g_neg_time >= 0)
				g_neg_time = -g_time;
			else
				g_neg_time = g_time;
			npressed = true;
		} else if (!(in->flags & INPUT_NEGATIVE))
			npressed = false;

		// keyboard weapon switching
		if (in->flags & INPUT_1)
			g_weapon = WP_M61;
		else if (in->flags & INPUT_2)
			g_weapon = WP_L60;
		else if (in->flags & INPUT_3)
			g_weapon = WP_M102;

		// pausing
		if (in->flags & INPUT_PAUSE && !pausepressed) {
			g_paused = true;
			pausepressed = true;
		} else if (!(in->flags & INPUT_PAUSE))
			pausepressed = false;
	}
}

static const float g_reticle_M102[][2] = {
	// horizontal cross
	{0.4, 0.5},		{0.47, 0.5},
	{0.53, 0.5},	{0.6, 0.5},

	// vertical cross
	{0.5, 0.4},		{0.5, 0.47},
	{0.5, 0.53},	{0.5, 0.6},

	// the box
	{0.47, 0.47},	{0.53, 0.47},
	{0.47, 0.53},	{0.53, 0.53},
	{0.47, 0.47},	{0.47, 0.53},
	{0.53, 0.47},	{0.53, 0.53},

	// top left corner
	{0.25, 0.25},	{0.25, 0.29},
	{0.25, 0.25},	{0.285, 0.25},

	// top right corner
	{0.75, 0.25},	{0.75, 0.29},
	{0.75, 0.25},	{0.715, 0.25},

	// bottom left corner
	{0.25, 0.75},	{0.25, 0.71},
	{0.25, 0.75},	{0.285, 0.75},

	// bottom right corner
	{0.75, 0.75},	{0.75, 0.71},
	{0.75, 0.75},	{0.715, 0.75}
};

static const float g_reticle_L60[][2] = {
	// horizontal cross
	{0.25, 0.5},	{0.475, 0.5},
	{0.525, 0.5},	{0.75, 0.5},

	// vertical cross
	{0.5, 0.25},	{0.5, 0.47},
	{0.5, 0.53},	{0.5, 0.75},

	// horizontal distance bars
	{0.25, 0.485},	{0.25, 0.515},
	{0.325, 0.49},	{0.325, 0.51},
	{0.4, 0.49},	{0.4, 0.51},
	{0.6, 0.49},	{0.6, 0.51},
	{0.675, 0.49},	{0.675, 0.51},
	{0.75, 0.485},	{0.75, 0.515},

	// vertical distance bars
	{0.485, 0.25},	{0.515, 0.25},
	{0.49, 0.325},	{0.51, 0.325},
	{0.49, 0.4},	{0.51, 0.4},
	{0.49, 0.6},	{0.51, 0.6},
	{0.49, 0.675},	{0.51, 0.675},
	{0.485, 0.75},	{0.515, 0.75}
};

static const float g_reticle_M61[][2] = {
	// horizontal cross
	{0.38, 0.5},	{0.49, 0.5},
	{0.51, 0.5},	{0.62, 0.5},

	// vertical cross
	{0.5, 0.51},	{0.5, 0.62},

	// top left corner
	{0.33, 0.33},	{0.33, 0.37},
	{0.33, 0.33},	{0.37, 0.33},

	// top right corner
	{0.67, 0.33},	{0.67, 0.37},
	{0.67, 0.33},	{0.63, 0.33},

	// bottom left corner
	{0.33, 0.67},	{0.33, 0.63},
	{0.33, 0.67},	{0.37, 0.67},

	// bottom right corner
	{0.67, 0.67},	{0.67, 0.63},
	{0.67, 0.67},	{0.63, 0.67}
};

void ac_game_drawHUD(float neg) {
	char buf[64];
	ac_vec4_t p;

	// static elements of the HUD
	// different weapons have different reticles
	switch (g_weapon) {
		case WP_M61:
			ac_renderer_draw_lines((float (*)[2])g_reticle_M61,
				sizeof(g_reticle_M61) / sizeof(g_reticle_M61[0]), 3.f);
			ac_renderer_draw_string("25mm", 0, 0.9, 0.6);
			break;
		case WP_L60:
			ac_renderer_draw_lines((float (*)[2])g_reticle_L60,
				sizeof(g_reticle_L60) / sizeof(g_reticle_L60[0]), 3.f);
			ac_renderer_draw_string("40mm", 0, 0.9, 0.6);
			break;
		case WP_M102:
			ac_renderer_draw_lines((float (*)[2])g_reticle_M102,
				sizeof(g_reticle_M102) / sizeof(g_reticle_M102[0]), 3.f);
			ac_renderer_draw_string("105mm", 0, 0.9, 0.6);
			break;
		default:	// shut up compiler
			break;
	}
	ac_renderer_draw_string("\n"
		"0    A-G   MAN NARO\n"
		"RAY\n"
		"FF 30\n"
		"LIR\n"
		"\n"
		"BORE", 0, 0, 0.6);

	// dynamic elements
	// find the distance to the point we're looking at
	p = ac_vec_ma(g_forward, ac_vec_setall(800), g_viewpoint.origin);
	p = ac_game_collide(g_viewpoint.origin, p);
	p = ac_vec_sub(p, g_viewpoint.origin);
	sprintf(buf, "T\n"
		"G\n"
		"T\n"
		"\n"
		"Z\n"
		"Q\n"
		"\n"
		"F\n"
		"S\n"
		"T\n"
		"%s N\n"
		"SCORE %08d  TARG DIST %4.0f",
		neg > 0.5 ? "BHOT" : "WHOT", 0, ac_vec_length(p));
	ac_renderer_draw_string(buf, -1, 0, 0.6);
}

void ac_game_draw_instructions(void) {
	ac_renderer_draw_string("CONTROLS:\n", 0.02, 0.02, 1.f);
	ac_renderer_draw_string("Mouse:              Aim\n"
		"Left mouse button:  Fire\n"
		"Right mouse button: Toggle weapons\n"
		"1, 2, 3:            Weapon selection\n"
		"F:                  Toggle white hot/black\n"
		"                    hot vision\n"
		"P:                  Pause game\n"
		"Esc:                Quit game", 0.02, 0.1, 0.55);
	ac_renderer_draw_string("OBJECTIVE:\n", 0.02, 0.48, 1.f);
	ac_renderer_draw_string("Somewhere in the world, war rages. You are\n"
		"a TV operator aboard an AC-130 gunship.\n"
		"Your mission is to provide fire support\n"
		"for friendly troops on the ground. Beware\n"
		"of friendly fire! *Do not* fire on troops\n"
		"marked by a flashing IR strobe!", 0.02, 0.56, 0.55);
}

#define TOTAL_TICKS	6410.f
void ac_game_loading_tick() {
	static char buf[32];
	static float pts[][2] = {
		{0.25, 0.97}, {0.25, 0.97}
	};
	static int counter = 0;
	counter++;
	ac_renderer_start_scene(0, NULL);
	ac_renderer_finish_fx();
	ac_renderer_finish_3D();

	// draw the instructions
	ac_game_draw_instructions();
	// draw the progress bar
	pts[1][0] = 0.25 + 0.5 * (float)counter / TOTAL_TICKS;
	ac_renderer_draw_lines(pts, 2, 12.f);
	// draw percentage
	sprintf(buf, "LOADING - %.0f%%", (float)counter / TOTAL_TICKS * 100.f);
	ac_renderer_draw_string(buf, 0.25, 0.87, 1.0);
	ac_renderer_draw_string("(C) 2010, Leszek Godlewski - www.inequation.org",
		-0.995, 0.005, 0.3);

	ac_renderer_finish_2D();
	ac_renderer_composite(0.f, 0.f);
}

#define FLOATING_RADIUS		200.f
#define TIME_SCALE			-0.04
#define MOUSE_SCALE			0.001
void ac_game_viewpoint_think(ac_input_t *input) {
	float plane_angle = g_time * TIME_SCALE;
	float fy, fp;
	ac_vec4_t tmp;

	// zoom based on weapon selection
	switch (g_weapon) {
		case WP_M61:
			g_viewpoint.fov = M_PI * 0.02;
			fy = 0.5;
			break;
		case WP_L60:
			g_viewpoint.fov = M_PI * 0.04;
			fy = 1.f;
			break;
		case WP_M102:
			g_viewpoint.fov = M_PI * 0.08;
			fy = 1.5f;
			break;
		default:	// shut up compiler
			break;
	}

	// handle viewpoint movement
	if (!g_paused) {
		g_viewpoint.angles[0] -= ((float)input->deltaX) * MOUSE_SCALE * fy
			+ g_frameTime * TIME_SCALE;	// keep the cam in sync with the plane
		g_viewpoint.angles[1] -= ((float)input->deltaY) * MOUSE_SCALE * fy;
	}
#if 1
	if (g_viewpoint.angles[0] < -plane_angle + M_PI * 0.2)
		g_viewpoint.angles[0] = -plane_angle + M_PI * 0.2;
	else if (g_viewpoint.angles[0] > -plane_angle + M_PI * 0.8)
		g_viewpoint.angles[0] = -plane_angle + M_PI* 0.8;
	if (g_viewpoint.angles[1] > M_PI * -0.17)
		g_viewpoint.angles[1] = M_PI * -0.17;
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

	// calculate firing axis
	// apply bullet spread (~0,45 of a degree)
	fy = g_viewpoint.angles[0] - 0.004 + 0.001 * (rand() % 9);
	fp = g_viewpoint.angles[1] - 0.004 + 0.001 * (rand() % 9);
	g_forward = ac_vec_set(
		-cosf(fp) * sinf(fy), sinf(fp), -cosf(fp) * cosf(fy), 0);
}

void ac_game_frame(int ticks, float frameTime, ac_input_t *input) {
	static int gameTicks = 0;
	static int lastTicks = 0;
	static float neg = 0.f;
	float expld;
	ac_viewpoint_t vp;

	// don't advance the clocks if paused
	if (!g_paused) {
		gameTicks += ticks - lastTicks;
		g_frameTime = frameTime;
	} else
		g_frameTime = 0.f;
	lastTicks = ticks;
	g_frameTimeVec = ac_vec_setall(g_frameTime);
	g_time = (float)gameTicks * 0.001;

	// handle effects - inversion and contrast enhancement
	// negative time means positive->negative transition
	if (g_neg_time < 0 && g_time + g_neg_time <= NEGATIVE_TIME) {
		neg = (g_time + g_neg_time) / NEGATIVE_TIME;
		if (neg > 1.f)
			neg = 1.f;
	} else if (g_neg_time > 0 && g_time - g_neg_time <= NEGATIVE_TIME) {
		neg = 1.f - (g_time - g_neg_time) / NEGATIVE_TIME;
		if (neg < 0.f)
			neg = 0.f;
	}
	if (g_time - g_expl_time <= EXPLOSION_TIME) {
		expld = EXPLOSION_TIME
			* (1.f - (g_time - g_expl_time) / EXPLOSION_TIME);
		if (expld > 1.f)
			expld = 1.f;
	} else
		expld = 0.f;

	// advance the viewpoint
	ac_game_viewpoint_think(input);

	// operate the weapons
	ac_game_player_think(input);

	// generate another viewpoint for gun shakes
	if (g_time - g_shake_time <= SHAKE_TIME) {
		float lerp = expf(-4 * (g_time - g_shake_time) / SHAKE_TIME);
		memcpy(&vp, &g_viewpoint, sizeof(vp));
		vp.angles[0] += (-0.018 + 0.000036 * (rand() % 1001)) * lerp;
		vp.angles[1] += (-0.018 + 0.000036 * (rand() % 1001)) * lerp;
		ac_renderer_start_scene(gameTicks, &vp);
	} else
		ac_renderer_start_scene(gameTicks, &g_viewpoint);

	// advance the non-player elements of the world
	ac_game_advance_projectiles();
	ac_renderer_start_fx();
	ac_game_advance_particles();
	ac_renderer_finish_fx();

	ac_renderer_finish_3D();
	if (!g_paused)
		ac_game_drawHUD(neg);
	else {
		ac_game_draw_instructions();
		if (gameTicks == 0) {
			ac_renderer_draw_string("PRESS FIRE TO START", 0.125, 0.87,
				1.0);
			if (input->flags & INPUT_MOUSE_LEFT)
				g_paused = false;
		} else
			ac_renderer_draw_string("GAME PAUSED", 0.27, 0.87, 1.0);
	}
	ac_renderer_finish_2D();

	ac_renderer_composite(neg, expld);
}
