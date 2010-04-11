// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Renderer module

#include "ac130.h"
#include <GL/glew.h>
#define NO_SDL_GLEXT	// GLEW takes care of extensions
#include <SDL/SDL_opengl.h>
#include <GL/glu.h>

// convenience define for a GL 4x4 matrix
typedef GLfloat	GLmatrix_t[16];

SDL_Surface	*g_screen;

// terrain resources
uchar		*g_heightmap = NULL;
GLuint		g_hmapTex;
ac_vertex_t	g_ter_verts[TERRAIN_NUM_VERTS];
ushort		g_ter_indices[TERRAIN_NUM_INDICES];
int			g_ter_maxLevels;

void ac_renderer_set_frustum(ac_vec4_t fwd, ac_vec4_t up, float x, float y,
							float zNear, float zFar) {

}

bool ac_renderer_cull_bbox(ac_vec4_t mins, ac_vec4_t maxs) {
	return false;
}

void ac_renderer_create_terrain_indices(void) {
	short	i, j;	// must be signed
	ushort	*p = g_ter_indices;

	// patch body
	for (i = 0; i < TERRAIN_PATCH_SIZE - 1; i++) {
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
			*(p++) = i * TERRAIN_PATCH_SIZE + j;
			*(p++) = (i + 1) * TERRAIN_PATCH_SIZE + j;
		}
		if (i < TERRAIN_PATCH_SIZE - 2) {	// add a degenerate triangle
	        *p = *(p - 1);
	        p++;
	        *(p++) = (i + 1) * TERRAIN_PATCH_SIZE;
	    }
	}

	// add a degenerate triangle to connect body with skirt
	*(p++) = (TERRAIN_PATCH_SIZE - 1) * TERRAIN_PATCH_SIZE + TERRAIN_PATCH_SIZE - 1;
	*(p++) = TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;

	// skirt
	// a single strip that goes around the entire patch
	// -Z edge
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;
		*(p++) = i;
	}
	// -X edge
	for (i = 1; i < TERRAIN_PATCH_SIZE - 1; i++) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE + 2) - 1;
		*(p++) = i * TERRAIN_PATCH_SIZE + TERRAIN_PATCH_SIZE - 1;
	}
	// +Z edge
	for (i = TERRAIN_PATCH_SIZE - 1; i >= 0; i--) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE + 1;
		*(p++) = i + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE - 1);
	}
	// +X edge
	for (i = TERRAIN_PATCH_SIZE - 2; i >= 1; i--) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE + 2) - 2;
		*(p++) = i * TERRAIN_PATCH_SIZE;
	}
	*(p++) = TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;
	*(p++) = 0;
	//assert(p - g_ter_indices == TERRAIN_NUM_INDICES);
}

void ac_renderer_calc_terrain_lodlevels(void) {
	// calculate max LOD levels
	int i, pow2 = (HEIGHTMAP_SIZE - 1) / (TERRAIN_PATCH_SIZE - 1);
	g_ter_maxLevels = 0;
	for (i = 1; i < pow2; i *= 2)
		g_ter_maxLevels++;
}

inline float ac_renderer_sample_height(float s, float t) {
	int x = roundf(s * (HEIGHTMAP_SIZE - 1));
	int y = roundf(t * (HEIGHTMAP_SIZE - 1));
	return 50.f * (float)g_heightmap[y * HEIGHTMAP_SIZE + x] / 255.f;
}

void ac_renderer_terrain_patch(float bu, float bv, float scale, int level) {
	int i, j;
	const float invScaleX = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
	const float invScaleY = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
	float s, t;

	// fill patch body vertices
	ac_vertex_t *v = g_ter_verts;
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		t = i * invScaleY;
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
			s = j * invScaleX;
			v->st[0] = bu + s * scale;
			v->st[1] = bv + t * scale;
			v->pos = ac_vec_set(v->st[0] * HEIGHTMAP_SIZE,
						ac_renderer_sample_height(v->st[0], v->st[1]),
						v->st[1] * HEIGHTMAP_SIZE,
						0.f);
			v++;
		}
	}

	// fill skirt vertices
	for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
		s = j * invScaleX;
		v->st[0] = bu + s * scale;
		v->st[1] = 0.f;
		v->pos = ac_vec_set(v->st[0] * HEIGHTMAP_SIZE,
						-50.f,
						v->st[1] * HEIGHTMAP_SIZE,
						0.f);
		v++;
		v->st[0] = bu + s * scale;
		v->st[1] = 1.f;
		v->pos = ac_vec_set(v->st[0] * HEIGHTMAP_SIZE,
						-50.f,
						v->st[1] * HEIGHTMAP_SIZE,
						0.f);
		v++;
	}

	for (i = 1; i < TERRAIN_PATCH_SIZE - 1; i++) {
		t = i * invScaleY;
		v->st[0] = 0.f;
		v->st[1] = bu + t * scale;
		v->pos = ac_vec_set(v->st[0] * HEIGHTMAP_SIZE,
						-50.f,
						v->st[1] * HEIGHTMAP_SIZE,
						0.f);
		v++;
		v->st[0] = 1.f;
		v->st[1] = bu + t * scale;
		v->pos = ac_vec_set(v->st[0] * HEIGHTMAP_SIZE,
						-50.f,
						v->st[1] * HEIGHTMAP_SIZE,
						0.f);
		v++;
	}

	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t),
					&g_ter_verts[0].pos.f[0]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ac_vertex_t),
					&g_ter_verts[0].st[0]);
	glDrawElements(GL_TRIANGLE_STRIP,
					TERRAIN_NUM_INDICES,
					GL_UNSIGNED_SHORT,
					&g_ter_indices[0]);
}

void ac_renderer_recurse_terrain(ac_vec4_t cam,
								float minU, float minV,
								float maxU, float maxV,
								int level, float scale) {
	ac_vec4_t v, mins, maxs;
	float halfU = (minU + maxU) * 0.5;
	float halfV = (minV + maxV) * 0.5;

	// apply frustum culling
	mins = ac_vec_set((minU - 0.5) * HEIGHTMAP_SIZE,
					-10.f,
					(minV - 0.5) * HEIGHTMAP_SIZE,
					0.f);
	maxs = ac_vec_set((maxU - 0.5) * HEIGHTMAP_SIZE,
					50.f,
					(maxV - 0.5) * HEIGHTMAP_SIZE,
					0.f);
	if (ac_renderer_cull_bbox(mins, maxs))
		return;

	float d2 = (maxU - minU) * (float)HEIGHTMAP_SIZE
				/ (TERRAIN_PATCH_SIZE_F - 1.f);
	d2 *= d2;

	v = ac_vec_set((halfU - 0.5) * (float)HEIGHTMAP_SIZE,
				128.f,
				(halfV - 0.5) * (float)HEIGHTMAP_SIZE,
				0.f);
	v = ac_vec_sub(v, cam);

	// use distances squared
	float f2 = ac_vec_dot(v, v) / d2;

	if (f2 > TERRAIN_LOD * TERRAIN_LOD || level < 1)
		ac_renderer_terrain_patch(minU, minV, scale, level);
	else {
		scale *= 0.5;
		ac_renderer_recurse_terrain(cam, minU, minV, halfU, halfV, level - 1,
									scale);
		ac_renderer_recurse_terrain(cam, halfU, minV, maxU, halfV, level - 1,
									scale);
		ac_renderer_recurse_terrain(cam, minU, halfV, halfU, maxV, level - 1,
									scale);
		ac_renderer_recurse_terrain(cam, halfU, halfV, maxU, maxV, level - 1,
									scale);
	}
}

void ac_renderer_terrain_bruteforce(void) {
	int x, y, k;
	float c;

	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(1, 1, 1);
	for (y = 0; y < HEIGHTMAP_SIZE - 1; y++) {
		for (x = 0; x < HEIGHTMAP_SIZE - 1; x++) {
			k = y * HEIGHTMAP_SIZE + x;
			c = ((float)g_heightmap[k]) / 255.f;
			//glColor3f(c, c, c);
			glTexCoord2f(((float)x + 0.5) / (float)HEIGHTMAP_SIZE, ((float)y + 0.5) / (float)HEIGHTMAP_SIZE);
			glVertex3f((float)(x - HEIGHTMAP_SIZE / 2), c * 50.f, (float)(y - HEIGHTMAP_SIZE / 2));
			k = (y + 1) * HEIGHTMAP_SIZE + x;
			c = ((float)g_heightmap[k]) / 255.f;
			//glColor3f(c, c, c);
			glTexCoord2f(((float)x + 0.5) / (float)HEIGHTMAP_SIZE, ((float)y + 1.5) / (float)HEIGHTMAP_SIZE);
			glVertex3f((float)(x - HEIGHTMAP_SIZE / 2), c * 50.f, (float)(y - HEIGHTMAP_SIZE / 2 + 1));
			k = y * HEIGHTMAP_SIZE + x + 1;
			c = ((float)g_heightmap[k]) / 255.f;
			//glColor3f(c, c, c);
			glTexCoord2f(((float)x + 1.5) / (float)HEIGHTMAP_SIZE, ((float)y + 0.5) / (float)HEIGHTMAP_SIZE);
			glVertex3f((float)(x - HEIGHTMAP_SIZE / 2 + 1), c * 50.f, (float)(y - HEIGHTMAP_SIZE / 2));
			k = (y + 1) * HEIGHTMAP_SIZE + x + 1;
			c = ((float)g_heightmap[k]) / 255.f;
			//glColor3f(c, c, c);
			glTexCoord2f(((float)x + 1.5) / (float)HEIGHTMAP_SIZE, ((float)y + 1.5) / (float)HEIGHTMAP_SIZE);
			glVertex3f((float)(x - HEIGHTMAP_SIZE / 2 + 1), c * 50.f, (float)(y - HEIGHTMAP_SIZE / 2 + 1));
		}
		// degenerate triangle
		//glColor3f(c, 0, 0);
		//glTexCoord2f(0, 0);
		glVertex3f((float)(x - HEIGHTMAP_SIZE / 2), c * 50.f, (float)(y - HEIGHTMAP_SIZE / 2 + 1));
		k = (y + 1) * HEIGHTMAP_SIZE;
		c = ((float)g_heightmap[k]) / 255.f;
		//glColor3f(0, 0, c);
		//glTexCoord2f(0, 0);
		glVertex3f((float)(0 - HEIGHTMAP_SIZE / 2), c * 50.f, (float)(y - HEIGHTMAP_SIZE / 2 + 1));
	}
	glEnd();
}

void ac_renderer_draw_terrain(ac_vec4_t cam) {
	glPushMatrix();

	/*//glLoadIdentity();
	glPushMatrix();
	//glTranslatef(20, 20, 0);
	glBegin(GL_TRIANGLES);
	glColor3f(1, 0, 0);
	glVertex3f(10, 0, 0);
	glColor3f(0, 1, 0);
	glVertex3f(0, 10, 0);
	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 10);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glBegin(GL_QUADS);
	glColor3f(1, 0, 0);
	glVertex3f(-3, 9, -10);
	glColor3f(0, 1, 0);
	glVertex3f(-1, 9, -10);
	glColor3f(0, 0, 1);
	glVertex3f(-1, 11, -10);
	glColor3f(1, 1, 1);
	glVertex3f(-3, 11, -10);
	glEnd();
	glPopMatrix();*/

#if 1
	// centre the terrain
	glTranslatef(-HEIGHTMAP_SIZE / 2, 0, -HEIGHTMAP_SIZE / 2);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_hmapTex);

	// traverse the quadtree
	ac_renderer_recurse_terrain(cam,
								0.f, 0.f, 1.f, 1.f,
								g_ter_maxLevels, 1.f);
#else
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_hmapTex);
	ac_renderer_terrain_bruteforce();
#endif
	glPopMatrix();
}

bool ac_renderer_init(void) {
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	if (!(g_screen = SDL_SetVideoMode(1024, 768, 24, SDL_OPENGL))) {
		fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
		return false;
	}

	SDL_WM_SetCaption("AC-130", "AC-130");

	// initialize the extension wrangler
	glewInit();
	// check for required features
	if (!GLEW_EXT_framebuffer_object)
		return false;

	// all geometry uses vertex and index arrays
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// initialize matrices
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set face culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// generate resources
	ac_renderer_create_terrain_indices();
	ac_renderer_calc_terrain_lodlevels();

	return true;
}

void ac_renderer_shutdown(void) {
	if (g_heightmap != NULL)
		glDeleteTextures(1, &g_hmapTex);
	// close SDL down
	SDL_QuitSubSystem(SDL_INIT_VIDEO);	// FIXME: this shuts input down as well
}

void ac_renderer_set_heightmap(uchar *hmap) {
	if (g_heightmap != NULL)
		glDeleteTextures(1, &g_hmapTex);
	g_heightmap = hmap;
	glGenTextures(1, &g_hmapTex);
	glBindTexture(GL_TEXTURE_2D, g_hmapTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,
				HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, g_heightmap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void ac_renderer_start_scene(ac_viewpoint_t *vp) {
	GLmatrix_t m = {
		1, 0, 0, 0,	// 0
		0, 1, 0, 0,	// 4
		0, 0, 1, 0,	// 8
		0, 0, 0, 1	// 12
	};
	double zNear = 0.1, zFar = 600.0;
	double x, y;
	float cy, cp, sy, sp;
	ac_vec4_t fwd, up;

	// clear buffers
	//glClearColor(0.f, 0.75f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set camera matrix
	glMatrixMode(GL_MODELVIEW);

	cy = cosf(vp->angles[0]);
	sy = sinf(vp->angles[0]);
	cp = cosf(vp->angles[1]);
	sp = sinf(vp->angles[1]);
	// column 1
	m[0] = cy;
	m[1] = sp * sy;
	m[2] = cp * sy;
	m[3] = 0;
	// column 2
	m[4] = 0;
	m[5] = cp;
	m[6] = -sp;
	m[7] = 0;
	// column 3
	m[8] = -sy;
	m[9] = sp * cy;
	m[10] = cp * cy;
	m[11] = 0;
	// column 4
	m[12] = /*-vp->origin.f[0] * m[0]
		- vp->origin.f[1] * m[4]
		- vp->origin.f[2] * m[8]*/0;
	m[13] = /*-vp->origin.f[0] * m[1]
		- vp->origin.f[1] * m[5]
		- vp->origin.f[2] * m[9]*/0;
	m[14] = /*-vp->origin.f[0] * m[2]
		- vp->origin.f[1] * m[6]
		- vp->origin.f[2] * m[10]*/0;
	m[15] = 1;
	glLoadMatrixf(m);

	// set up the GL projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	x = zNear * tan(vp->fov);
	y = zNear * tan(vp->fov * 0.75);
	glFrustum(-x, x, -y, y, zNear, zFar);
	// calculate frustum planes
	fwd = ac_vec_set(-m[2], -m[6], -m[10], 0);
	up = ac_vec_set(m[1], m[5], m[9], 0);
	ac_renderer_set_frustum(fwd, up, x, y, zNear, zFar);

	// draw terrain
	ac_renderer_draw_terrain(vp->origin);
}

void ac_renderer_finish_3D(void) {

}

void ac_renderer_composite(bool negative) {
	// dump everything to screen
	SDL_GL_SwapBuffers();
}
