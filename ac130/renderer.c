// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Renderer module

#include "ac130.h"
#include <GL/glew.h>
#define NO_SDL_GLEXT	// GLEW takes care of extensions
#include <SDL/SDL_opengl.h>

// convenience define for a GL 4x4 matrix
typedef GLfloat	GLmatrix_t[16];

SDL_Surface	*g_screen;

// terrain resources
uchar		*g_heightmap = NULL;
GLuint		g_hmapTex;
ac_vertex_t	g_ter_verts[TERRAIN_NUM_VERTS];
ushort		g_ter_indices[TERRAIN_NUM_INDICES];
int			g_ter_maxLevels;
GLmatrix_t	g_ter_matrix;

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
	        *(p++) = *(p - 1);
	        *(p++) = (i + 1) * TERRAIN_PATCH_SIZE;
	    }
	}

	// add a degenerate triangle to connect body with skirt
	*(p++) = (TERRAIN_PATCH_SIZE - 1) * TERRAIN_PATCH_SIZE + TERRAIN_PATCH_SIZE - 1;
	*(p++) = TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;

	// skirt
	// a single strip that goes around the entire patch
	// -Y edge
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;
		*(p++) = i;
	}
	// -X edge
	for (i = 1; i < TERRAIN_PATCH_SIZE - 1; i++) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE + 2) - 1;
		*(p++) = i * TERRAIN_PATCH_SIZE + TERRAIN_PATCH_SIZE * 2 - 1;
	}
	// +Y edge
	for (i = TERRAIN_PATCH_SIZE - 1; i >= 0; i--) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE + 1;
		*(p++) = i + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE - 1);
	}
	// +X edge
	for (i = TERRAIN_PATCH_SIZE - 2; i >= 1; i--) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE + 2);
		*(p++) = i * TERRAIN_PATCH_SIZE;
	}
	*(p++) = 0;
	*(p++) = TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;
	assert(p - g_ter_indices == TERRAIN_NUM_INDICES);
}

void ac_renderer_calc_terrain_lodlevels(void) {
	// calculate max LOD levels
	int i, pow2 = (HEIGHTMAP_SIZE - 1) / (TERRAIN_PATCH_SIZE - 1);
	g_ter_maxLevels = 0;
	for (i = 1; i < pow2; i *= 2)
		g_ter_maxLevels++;
}

inline float ac_renderer_sample_height(float s, float t) {
	return (float)g_heightmap[(int)(roundf(s * (float)HEIGHTMAP_SIZE))
								* HEIGHTMAP_SIZE
							+ (int)(roundf(t * (float)HEIGHTMAP_SIZE))];
}

void ac_renderer_terrain_patch(float bu, float bv, float scale, int level) {
	int i, j;
	const float invScaleX = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
	const float invScaleY = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
	float s, t;

	// use these temporary SSE vectors to speed up the scale and bias transform
	ac_vec4_t svec, bias;
	ac_vec_setall(svec, scale * (float)HEIGHTMAP_SIZE);
	ac_vec_set(bias, bu, 0.f, bv, 0.f);

	// fill patch body vertices
	ac_vertex_t *v = g_ter_verts;
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		t = i * invScaleY;
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
			s = j * invScaleX;
			ac_vec_set(v->pos,
						1.f - t,
						ac_renderer_sample_height(t, s),
						s,
						0.f);
			ac_vec_ma(v->pos, svec, bias, v->pos);
			v->st[0] = 1.f - s;
			v->st[1] = t;
			v++;
		}
	}

	// fill skirt vertices
	for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
		s = j * invScaleX;
		ac_vec_set(v->pos, 1.f, -10.f, s, 0.f);
		ac_vec_ma(v->pos, svec, bias, v->pos);
		v->st[0] = 1.f - s;
		v->st[1] = 1.f;
		v++;
		ac_vec_set(v->pos, 0.f, -10.f, s, 0.f);
		ac_vec_ma(v->pos, svec, bias, v->pos);
		v->st[0] = 1.f - s;
		v->st[1] = 0.f;
		v++;
	}

	for (i = 1; i < TERRAIN_PATCH_SIZE - 1; i++) {
		t = i * invScaleY;
		ac_vec_set(v->pos, 1.f - t, -10.f, 1.f, 0.f);
		ac_vec_ma(v->pos, svec, bias, v->pos);
		v->st[0] = 1.f;
		v->st[1] = t;
		v++;
		ac_vec_set(v->pos, 1.f - t, -10.f, 0.f, 0.f);
		ac_vec_ma(v->pos, svec, bias, v->pos);
		v->st[0] = 0.f;
		v->st[1] = t;
		v++;
	}

	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t),
					&g_ter_verts[0].pos.f[0]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ac_vertex_t),
					&g_ter_verts[0].st[0]);
	glDrawElements(GL_TRIANGLE_STRIP, 3 /*TERRAIN_NUM_INDICES * 3*/, GL_UNSIGNED_SHORT,
					&g_ter_indices[0]);
}

void ac_renderer_recurse_terrain(ac_vec4_t cam,
								float minU, float minV,
								float maxU, float maxV,
								int level, float scale) {
	ac_vec4_t v;
	float halfU = (minU + maxU) * 0.5;
	float halfV = (minV + maxV) * 0.5;

	/*if (!v_noCull->getInt()) {
		math::vec3 bounds[2] = {
			math::vec3((minU - 0.5) * this->m_hMapSize,
						(minV - 0.5) * this->m_hMapSize,
						gVideo->getAssetMgr()->getHeightMap()->getBias()),
			math::vec3((maxU - 0.5) * this->m_hMapSize,
						(maxV - 0.5) * this->m_hMapSize,
						gVideo->getAssetMgr()->getHeightMap()->getScale())
		};
		if (gVideo->getSceneMgr()->frustumCullBBox(bounds)) {
			gVideo->getSceneMgr()->addTerrainStats(true);
			return;
		}
	}*/

	float d2 = (maxU - minU) * (float)HEIGHTMAP_SIZE
				/ (TERRAIN_PATCH_SIZE_F - 1.f);
	d2 *= d2;

	ac_vec_set(v,
				-(halfV - 0.5) * (float)HEIGHTMAP_SIZE,
				128.f,
				(halfU - 0.5) * (float)HEIGHTMAP_SIZE,
				0.f);
	ac_vec_sub(v, cam, v);

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

void ac_renderer_draw_terrain(ac_vec4_t cam) {
	glPushMatrix();
	// apply our model view matrix
	glMultMatrixf(g_ter_matrix);

	glBindTexture(GL_TEXTURE_2D, g_hmapTex);

	ac_renderer_recurse_terrain(cam, 0.f, 0.f, 1.f, 1.f, g_ter_maxLevels, 1.f);
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
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1);

	// set window caption to say that we're working
	SDL_WM_SetCaption("AC-130 - Generating resources, please wait...",
					"AC-130");

	if (!(g_screen = SDL_SetVideoMode(1024, 768, 24, SDL_OPENGL))) {
		fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
		return false;
	}

	// initialize the extension wrangler
	glewInit();
	// check for required features
	if (!GLEW_EXT_framebuffer_object)
		return false;

	// all geometry uses vertex and index arrays
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_INDEX_ARRAY);
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
				GL_LUMINANCE, GL_BYTE, g_heightmap);
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
	float sy, cy, sp, cp;
	ac_vec4_t tmp;
	int i;

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set up the GL projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	x = zNear * tan(vp->fov);
	y = zNear * tan(vp->fov * 0.75);
	glFrustum(-x, x, -y, y, zNear, zFar);

	// restore modelview matrix mode
	glMatrixMode(GL_MODELVIEW);
	// calculate rotation matrix
	sy = sinf(vp->angles[0]);
	cy = cosf(vp->angles[0]);
	sp = sinf(vp->angles[1]);
	cp = cosf(vp->angles[1]);
	// forward vector
	m[8]	= -cp * sy;
	m[9]	= sp;
	m[10]	= cp * cy;
	// up vector
	m[4]	= -sp * sy;
	m[5]	= cp;
	m[6]	= -sp * cy;
	// right vector
	m[0]	= cy;
	m[1]	= 0.f;
	m[2]	= -sy;
	/*m[0]	= m[9] * m[6] - m[10] * m[5];
	m[1]	= m[10] * m[4] - m[8] * m[6];
	m[2]	= m[8] * m[5] - m[9] * m[4];*/
	// now set the camera position
	for (i = 0; i < 3; i++) {
		ac_vec_tosse(&m[4 * i], tmp);
		m[12 + i] = ac_vec_dot(vp->origin, tmp);
	}
	m[15]	= 1.f;
	glLoadMatrixf(m);

	// draw terrain
	ac_renderer_draw_terrain(vp->origin);
}

void ac_renderer_finish_3D(void) {

}

void ac_renderer_composite(bool negative) {
	// dump everything to screen
	SDL_GL_SwapBuffers();
}
