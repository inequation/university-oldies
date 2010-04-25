// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Renderer module

#include "ac130.h"
#ifdef WIN32	// enable static linking on win32
	#define GLEW_STATIC
#endif // WIN32
#include <GL/glew.h>
#define NO_SDL_GLEXT	// GLEW takes care of extensions
#include <SDL/SDL_opengl.h>
#include <GL/glu.h>

// convenience define for a GL 4x4 matrix
typedef GLfloat	GLmatrix_t[16];

typedef enum {
	CR_OUTSIDE,
	CR_INSIDE,
	CR_INTERSECT
} cullResult_t;

SDL_Surface	*g_screen;

// terrain resources
GLuint		g_hmapTex;
ac_vertex_t	g_ter_verts[TERRAIN_NUM_VERTS];
ushort		g_ter_indices[TERRAIN_NUM_INDICES];
int			g_ter_maxLevels;

// prop resources
ac_vertex_t	g_tree_verts[TREE_BASE + 1			// LOD 0
							+ TREE_BASE - 2		// LOD 1
							+ TREE_BASE - 4];	// LOD 2
uchar		g_tree_indices[TREE_BASE + 2		// LOD 0
							+ TREE_BASE			// LOD 1
							+ TREE_BASE - 2];	// LOD 2
uchar		g_tree_texture[TREE_TEXTURE_SIZE];
GLuint		g_treeTex;
uint		g_propVBOs[2];

// counters for measuring performance
uint		*g_triCounter;
uint		*g_vertCounter;
uint		*g_displayedPatchCounter;
uint		*g_culledPatchCounter;

// frustum planes
ac_vec4_t	g_frustum[6];

ac_vec4_t	g_viewpoint;

void ac_renderer_set_frustum(ac_vec4_t pos,
							ac_vec4_t fwd, ac_vec4_t right, ac_vec4_t up,
							float x, float y, float zNear, float zFar) {
	ac_vec4_t v1, v2;

	// culling debug (look straight down to best see it at work)
#if 0
	x *= 0.5;
	y *= 0.5;
#endif

	// near plane
	g_frustum[0] = fwd;
	g_frustum[0].f[3] = ac_vec_dot(fwd, pos) + zNear;

	// far plane
	g_frustum[1] = ac_vec_mulf(fwd, -1.f);
	g_frustum[1].f[3] = -ac_vec_dot(fwd, pos) - zFar;

	// right plane
	// v1 = fwd * zNear + right * x + up * -y
	v1 = ac_vec_add(ac_vec_mulf(fwd, zNear),
			ac_vec_add(ac_vec_mulf(right, x), ac_vec_mulf(up, -y)));
	// v2 = p1 + up * 2y
	v2 = ac_vec_add(v1, ac_vec_mulf(up, 2.f * y));
	g_frustum[2] = ac_vec_normalize(ac_vec_cross(v2, v1));
	g_frustum[2].f[3] = ac_vec_dot(g_frustum[2], ac_vec_add(v1, pos));

	// left plane
	// v1 -= right * 2 * x
	v1 = ac_vec_add(v1, ac_vec_mulf(right, -2.f * x));
	// v2 -= right * 2 * x
	v2 = ac_vec_add(v2, ac_vec_mulf(right, -2.f * x));
	g_frustum[3] = ac_vec_normalize(ac_vec_cross(v1, v2));
	g_frustum[3].f[3] = ac_vec_dot(g_frustum[3], ac_vec_add(v1, pos));

	// top plane
	// v2 = v1 + right * 2 * x
	v1 = ac_vec_add(v2, ac_vec_mulf(right, 2.f * x));
	g_frustum[4] = ac_vec_normalize(ac_vec_cross(v2, v1));
	g_frustum[4].f[3] = ac_vec_dot(g_frustum[4], ac_vec_add(v1, pos));

	// bottom plane
	// v1 -= up * 2 * y
	v1 = ac_vec_add(v1, ac_vec_mulf(up, -2.f * y));
	// v2 -= up * 2 * y
	v2 = ac_vec_add(v2, ac_vec_mulf(up, -2.f * y));
	g_frustum[5] = ac_vec_normalize(ac_vec_cross(v1, v2));
	g_frustum[5].f[3] = ac_vec_dot(g_frustum[5], ac_vec_add(v1, pos));

	assert(ac_vec_dot(g_frustum[0], g_frustum[1]) < 0.f);
	assert(ac_vec_dot(g_frustum[2], g_frustum[3]) < 1.f);
	assert(ac_vec_dot(g_frustum[4], g_frustum[5]) < 1.f);
}

bool ac_renderer_cull_sphere(ac_vec4_t p, float radius) {
	int i;
	for (i = 0; i < 6; i++) {
		if (ac_vec_dot(p, g_frustum[i]) + radius < g_frustum[i].f[3])
			// sphere is behind one of the planes
			return true;
	}
	return false;
}

cullResult_t ac_renderer_cull_bbox(ac_vec4_t bounds[2]) {
	ac_vec4_t v;
	int i, x, y, z;
	bool intersect = false;

	for (i = 0; i < 6; i++) {
		// floating point magic! extract the sign bits
#define AS_INT(f)		(*(int *)(&(f)))
		x = (AS_INT(g_frustum[i].f[0]) & 0x80000000) >> 31;
		y = (AS_INT(g_frustum[i].f[1]) & 0x80000000) >> 31;
		z = (AS_INT(g_frustum[i].f[2]) & 0x80000000) >> 31;
#undef AS_INT
		// test the negative far point against the plane
		v = ac_vec_set(
				bounds[1 - x].f[0],
				bounds[1 - y].f[1],
				bounds[1 - z].f[2],
				0);
		if (ac_vec_dot(v, g_frustum[i]) < g_frustum[i].f[3])
			// negative far point behind plane -> box outside frustum
			return CR_OUTSIDE;
		// test the positive far point against the plane
		v = ac_vec_set(
				bounds[x].f[0],
				bounds[y].f[1],
				bounds[z].f[2],
				0);
		if (ac_vec_dot(v, g_frustum[i]) < g_frustum[i].f[3])
			intersect = true;
	}
	return intersect ? CR_INTERSECT : CR_INSIDE;
}

void ac_renderer_fill_terrain_indices(void) {
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

void ac_renderer_fill_terrain_vertices(void) {
	int i, j;
	float s, t;
	const float invScaleX = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
	const float invScaleY = 1.f / (TERRAIN_PATCH_SIZE - 1.f);

	// fill patch body vertices
	ac_vertex_t *v = g_ter_verts;
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		t = 1.f - i * invScaleY;
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
			s = j * invScaleX;
			v->st[0] = s;
			v->st[1] = t;
			v->pos = ac_vec_set(s, 0.f, t, 0.f);
			v++;
		}
	}

	// fill skirt vertices
	for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
		s = j * invScaleX;
		v->st[0] = s;
		v->st[1] = 1.f;
		v->pos = ac_vec_set(s, -10.f, 1.f, 0.f);
		v++;
		v->st[0] = s;
		v->st[1] = 0.f;
		v->pos = ac_vec_set(s, -10.f, 0.f, 0.f);
		v++;
	}

	for (i = 1; i < TERRAIN_PATCH_SIZE - 1; i++) {
		t = 1.f - i * invScaleY;
		v->st[0] = 0.f;
		v->st[1] = t;
		v->pos = ac_vec_set(0.f, -10.f, t, 0.f);
		v++;
		v->st[0] = 1.f;
		v->st[1] = t;
		v->pos = ac_vec_set(1.f, -10.f, t, 0.f);
		v++;
	}
	assert(v - g_ter_verts == TERRAIN_NUM_VERTS);
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
	return (float)g_heightmap[y * HEIGHTMAP_SIZE + x];
}

void ac_renderer_terrain_patch(float bu, float bv, float scale, int level) {
	int i, j;
	GLmatrix_t m = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// create the texture matrix for the patch...
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	// column 1
	m[0] = scale;
	//m[1] = 0;
	//m[2] = 0;
	//m[3] = 0;
	// column 2
	//m[4] = 0;
	m[5] = scale;
	//m[6] = 0;
	//m[7] = 0;
	// column 3
	//m[8] = 0;
	//m[9] = 0;
	//m[10] = 1;
	//m[11] = 0;
	// column 4
	m[12] = bu;
	m[13] = bv;
	//m[14] = 0;
	//m[15] = 1;
	glLoadMatrixf(m);
	// ...and the modelview one
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	// only make the necessary changes
	m[0] *= HEIGHTMAP_SIZE;
	m[5] = HEIGHT_SCALE;
	m[10] = scale * HEIGHTMAP_SIZE;
	m[12] = bu * HEIGHTMAP_SIZE;
	m[13] = 0;
	m[14] = bv * HEIGHTMAP_SIZE;
	glMultMatrixf(m);

	// sample the heightmap and set vertex Y component
	ac_vertex_t *v = g_ter_verts;
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++, v++)
			v->pos.f[1] = ac_renderer_sample_height(bu + v->st[0] * scale,
													bv + v->st[1] * scale);
	}

	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t),
					&g_ter_verts[0].pos.f[0]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ac_vertex_t),
					&g_ter_verts[0].st[0]);
	glDrawElements(GL_TRIANGLE_STRIP,
					TERRAIN_NUM_INDICES,
					GL_UNSIGNED_SHORT,
					&g_ter_indices[0]);
	*g_vertCounter += TERRAIN_NUM_VERTS;
	*g_triCounter += TERRAIN_NUM_INDICES - 2;
	(*g_displayedPatchCounter)++;

	// pop both matrices
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void ac_renderer_recurse_terrain(float minU, float minV,
								float maxU, float maxV,
								int level, float scale) {
	ac_vec4_t v, bounds[2];
	float halfU = (minU + maxU) * 0.5;
	float halfV = (minV + maxV) * 0.5;

	// apply frustum culling
	bounds[0] = ac_vec_set((minU - 0.5) * HEIGHTMAP_SIZE,
					-10.f,
					(minV - 0.5) * HEIGHTMAP_SIZE,
					0.f);
	bounds[1] = ac_vec_set((maxU - 0.5) * HEIGHTMAP_SIZE,
					HEIGHT,
					(maxV - 0.5) * HEIGHTMAP_SIZE,
					0.f);
	if (ac_renderer_cull_bbox(bounds) == CR_OUTSIDE) {
		(*g_culledPatchCounter)++;
		return;
	}

	float d2 = (maxU - minU) * (float)HEIGHTMAP_SIZE
				/ (TERRAIN_PATCH_SIZE_F - 1.f);
	d2 *= d2;

	v = ac_vec_set((halfU - 0.5) * (float)HEIGHTMAP_SIZE,
				128.f,
				(halfV - 0.5) * (float)HEIGHTMAP_SIZE,
				0.f);
	v = ac_vec_sub(v, g_viewpoint);

	// use distances squared
	float f2 = ac_vec_dot(v, v) / d2;

	if (f2 > TERRAIN_LOD * TERRAIN_LOD || level < 1)
		ac_renderer_terrain_patch(minU, minV, scale, level);
	else {
		scale *= 0.5;
		ac_renderer_recurse_terrain(minU, minV, halfU, halfV, level - 1,
									scale);
		ac_renderer_recurse_terrain(halfU, minV, maxU, halfV, level - 1,
									scale);
		ac_renderer_recurse_terrain(minU, halfV, halfU, maxV, level - 1,
									scale);
		ac_renderer_recurse_terrain(halfU, halfV, maxU, maxV, level - 1,
									scale);
	}
}

void ac_renderer_draw_terrain() {
	glPushMatrix();

#if 0
	// debug coordinate system triangle
	//glLoadIdentity();
	glPushMatrix();
	glTranslatef(0, HEIGHT, 0);
	glBegin(GL_TRIANGLES);
	glColor3f(1, 0, 0);
	glVertex3f(10, 0, 0);
	glColor3f(0, 1, 0);
	glVertex3f(0, 10, 0);
	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 10);
	glEnd();
	glPopMatrix();
	glColor3f(1, 1, 1);
#endif

	// centre the terrain
	glTranslatef(-HEIGHTMAP_SIZE / 2, 0, -HEIGHTMAP_SIZE / 2);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_hmapTex);

	// traverse the quadtree
	ac_renderer_recurse_terrain(0.f, 0.f, 1.f, 1.f,
								g_ter_maxLevels, 1.f);

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}

void ac_renderer_create_tree(void) {
	ac_gen_tree(g_tree_texture, g_tree_verts, g_tree_indices);

	// generate texture
	glGenTextures(1, &g_treeTex);
	glBindTexture(GL_TEXTURE_1D, g_treeTex);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_LUMINANCE8,
				TREE_TEXTURE_SIZE, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, g_tree_texture);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// generate VBOs
	glGenBuffers(2, g_propVBOs);
	glBindBuffer(GL_ARRAY_BUFFER, g_propVBOs[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_propVBOs[1]);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(g_tree_verts), g_tree_verts, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(g_tree_indices), g_tree_indices, GL_STATIC_DRAW);
	// unbind VBOs
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ac_renderer_recurse_proptree_drawall(ac_prop_t *node) {
	static GLmatrix_t m = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		1, 0, 0, 1
	};

	if (node->trees != NULL) {
		int i;
		float c, s, d2;
		ac_tree_t *t;
		int ofs, num;
		// pick level of detail
		ac_vec4_t l = ac_vec_mulf(
			ac_vec_add(node->bounds[0], node->bounds[1]), 0.5);
		l = ac_vec_sub(l, g_viewpoint);
		d2 = ac_vec_dot(l, l);

		if (d2 < PROP_LOD_DISTANCE * PROP_LOD_DISTANCE) {
			ofs = 0;
			num = TREE_BASE + 2;
		} else if (d2 < PROP_LOD_DISTANCE * PROP_LOD_DISTANCE * 4) {
			ofs = TREE_BASE + 2;
			num = TREE_BASE;
		} else {
			ofs = TREE_BASE + 2 + TREE_BASE;
			num = TREE_BASE - 2;
		}

		for (t = node->trees, i = 0; i < TREES_PER_FIELD; i++, t++) {
			glPushMatrix();

			c = cosf(t->ang);
			s = sinf(t->ang);
			// column 1
			m[0] = t->XZscale * c;
			//m[1] = 0;
			m[2] = t->XZscale * -s;
			//m[3] = 0;
			// column 2
			//m[4] = 0;
			m[5] = t->Yscale;
			//m[6] = 0;
			//m[7] = 0;
			// column 3
			m[8] = t->XZscale * s;
			//m[9] = 0;
			m[10] = t->XZscale * c;
			//m[11] = 0;
			// column 4
			/*m[12] = t->pos.f[0];
			m[13] = t->pos.f[1];
			m[14] = t->pos.f[2];
			//m[15] = 1;*/
			ac_vec_tofloat(t->pos, &m[12]);
			glMultMatrixf(m);

			glDrawElements(GL_TRIANGLE_FAN, num, GL_UNSIGNED_BYTE, (void *)ofs);
			*g_vertCounter += num - 1;
			*g_triCounter += num - 2;

			glPopMatrix();
		}
	} else if (node->bldgs != NULL) {
	} else if (node->child[0] != NULL) {
		// it's enough to just check the existence of the 1st child because a
		// node will always either have 4 children or none
		ac_renderer_recurse_proptree_drawall(node->child[0]);
		ac_renderer_recurse_proptree_drawall(node->child[1]);
		ac_renderer_recurse_proptree_drawall(node->child[2]);
		ac_renderer_recurse_proptree_drawall(node->child[3]);
	}
}

void ac_renderer_recurse_proptree(ac_prop_t *node, int step) {
	if (step < 1) {
		ac_renderer_recurse_proptree_drawall(node);
		return;
	}
	switch (ac_renderer_cull_bbox(node->bounds)) {
		case CR_OUTSIDE:
			return;
		case CR_INSIDE:
			ac_renderer_recurse_proptree_drawall(node);
			break;
		case CR_INTERSECT:
			step /= 2;
			ac_renderer_recurse_proptree(node->child[0], step);
			ac_renderer_recurse_proptree(node->child[1], step);
			ac_renderer_recurse_proptree(node->child[2], step);
			ac_renderer_recurse_proptree(node->child[3], step);
			break;
	}
}

void ac_renderer_draw_props(void) {
	// make the necessary state changes
	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, g_treeTex);
	glBindBuffer(GL_ARRAY_BUFFER, g_propVBOs[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_propVBOs[1]);
	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, pos.f[0]));
	glTexCoordPointer(1, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, st[0]));

	ac_renderer_recurse_proptree(g_proptree, PROPMAP_SIZE / 2);
	// bring the previous state back

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);
}

bool ac_renderer_init(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter) {
	float fogcolour[] = {0, 0, 0, 1};

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

	if (!vcounter || !tcounter || !dpcounter || !cpcounter)
		return false;

	g_vertCounter = vcounter;
	g_triCounter = tcounter;
	g_displayedPatchCounter = dpcounter;
	g_culledPatchCounter = cpcounter;

	SDL_WM_SetCaption("AC-130", "AC-130");

	// initialize the extension wrangler
	glewInit();
	// check for required features
	if (!GLEW_EXT_framebuffer_object) {
		fprintf(stderr, "Hardware does not support frame buffer objects\n");
		return false;
	}
	if (!GLEW_ARB_vertex_buffer_object) {
		fprintf(stderr, "Hardware does not support vertex buffer objects\n");
		return false;
	}

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
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	// set up fog
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogfv(GL_FOG_COLOR, fogcolour);
	glFogf(GL_FOG_DENSITY, 0.005);

	// generate resources
	ac_renderer_fill_terrain_indices();
	ac_renderer_fill_terrain_vertices();
	ac_renderer_calc_terrain_lodlevels();
	ac_renderer_create_tree();

	return true;
}

void ac_renderer_shutdown(void) {
	// FIXME: move to somewhere more appropriate
	ac_gen_free_proptree(NULL);

	glDeleteBuffers(2, g_propVBOs);

	glDeleteTextures(1, &g_hmapTex);
	glDeleteTextures(1, &g_treeTex);
	// close SDL down
	SDL_QuitSubSystem(SDL_INIT_VIDEO);	// FIXME: this shuts input down as well
}

void ac_renderer_set_heightmap(void) {
	if (g_heightmap != NULL)
		glDeleteTextures(1, &g_hmapTex);
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
	static GLmatrix_t m = {
		1, 0, 0, 0,	// 0
		0, 1, 0, 0,	// 4
		0, 0, 1, 0,	// 8
		0, 0, 0, 1	// 12
	};
	double zNear = 0.1, zFar = 800.0;
	double x, y;
	float cy, cp, sy, sp;
	ac_vec4_t fwd, right, up;

	g_viewpoint = vp->origin;

	// clear buffers
	//glClearColor(0.f, 0.75f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// flick some switches for the 3D rendering
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);

	// set up the GL projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	x = zNear * tan(vp->fov);
	y = zNear * tan(vp->fov * 0.75);
	glFrustum(-x, x, -y, y, zNear, zFar);

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
	//m[3] = 0;
	// column 2
	//m[4] = 0;
	m[5] = cp;
	m[6] = -sp;
	//m[7] = 0;
	// column 3
	m[8] = -sy;
	m[9] = sp * cy;
	m[10] = cp * cy;
	//m[11] = 0;
	// column 4
	m[12] = -vp->origin.f[0] * m[0]
		- vp->origin.f[1] * m[4]
		- vp->origin.f[2] * m[8];
	m[13] = -vp->origin.f[0] * m[1]
		- vp->origin.f[1] * m[5]
		- vp->origin.f[2] * m[9];
	m[14] = -vp->origin.f[0] * m[2]
		- vp->origin.f[1] * m[6]
		- vp->origin.f[2] * m[10];
	//m[15] = 1;
	glLoadMatrixf(m);

	// calculate frustum planes
	fwd = ac_vec_set(-m[2], -m[6], -m[10], 0);
	right = ac_vec_set(m[0], m[4], m[8], 0);
	up = ac_vec_set(m[1], m[5], m[9], 0);
	ac_renderer_set_frustum(vp->origin, fwd, right, up, x, y, zNear, zFar);

	// draw terrain
	ac_renderer_draw_terrain();

	// draw props
	ac_renderer_draw_props();
}

void ac_renderer_finish_3D(void) {

}

void ac_renderer_composite(bool negative) {
	// dump everything to screen
	SDL_GL_SwapBuffers();
}
