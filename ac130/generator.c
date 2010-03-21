// AC-130 shooter - rendrer module
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Procedural content generation module

#include "ac130.h"

/// Seed for the internal pseudorandom number generator.
static uint		g_seed;

/// Internal pseudorandom number generator.
int ac_gen_rand(void) {
	g_seed = (16807LL * g_seed) % 2147483647;
	if (g_seed < 0)
		g_seed += 2147483647;
	return g_seed;
}

/// Perlin noise permutation table.
static int p[] = { 151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

#define fade(t)			((t) * (t) * (t) * ((t) * ((t) * 6 - 15) + 10))
#define lerp(t, a, b)	((a) + (t) * ((b) - (a)))
static float grad(int hash, float x, float y, float z) {
	int h = hash & 15;	// convert low 4 bits of hash code into 12 gradient
	float u = h < 8 ? x : y;	// directions
	float v = h < 4 ? y : (h == 12 || h==14 ? x : z);
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

/// Improved perlin noise generator.
float ac_gen_perlin(float x, float y, float z) {
	int X, Y, Z;
	int A, AA, AB, B, BA, BB;
	float u, v, w;
	// find unit cube that contains point
	X = (int)floorf(x) & 255;
	Y = (int)floorf(y) & 255;
	Z = (int)floorf(z) & 255;
	// find relative x, y, z of point in cube
	x -= floorf(x);
	y -= floorf(y);
	z -= floorf(z);
	// compute fade curves for each of x, y, z
	u = fade(x);
	v = fade(y);
	w = fade(z);
	// hash coordinates of the 8 cube corners...
	A  = p[X % 256] + Y;
	AA = p[A % 256] + Z;
	AB = p[(A + 1) % 256] + Z;
	B  = p[(X + 1) % 256] + Y;
	BA = p[B % 256] + Z;
	BB = p[(B + 1) % 256]+Z;
	// ...and add blended results from 8 corners of cube
	return lerp(w, lerp(v,  lerp(u, grad(p[ AA    % 256], x  , y  , z   ),
									grad(p[ BA    % 256], x-1, y  , z   )),
							lerp(u, grad(p[ AB    % 256], x  , y-1, z   ),
									grad(p[ BB    % 256], x-1, y-1, z   ))),
					lerp(v, lerp(u, grad(p[(AA+1) % 256], x  , y  , z-1 ),
									grad(p[(BA+1) % 256], x-1, y  , z-1 )),
							lerp(u, grad(p[(AB+1) % 256], x  , y-1, z-1 ),
									grad(p[(BB+1) % 256], x-1, y-1, z-1 ))));
}
#undef fade
#undef lerp

static void ac_gen_cloudmap(char *dst, int size) {
	int i, x, y, xoff, yoff, pix;
	char *submaps[3];
	char *p;
	float freq;

	freq = 0.015 + 0.000001 * (float)((ac_gen_rand() % 10000) - 5000);

	for (x = 0; x < sizeof(submaps) / sizeof(submaps[0]); x++)
		submaps[x] = malloc(size * size);

	// fill the maps with noise
	for (i = 0, p = dst;
		i < 1 + sizeof(submaps) / sizeof(submaps[0]);
		i++, freq *= 2.0) {
		if (i > 0)
			p = submaps[i - 1];
		xoff = ac_gen_rand() % (size * 2);
		yoff = ac_gen_rand() % (size * 2);
		for (y = 0; y < size; y++) {
			for (x = 0; x < size; x++)
				p[y * size + x] = (char)(127.f
					* ac_gen_perlin(
						(float)(x + xoff) * freq,
						(float)(y + yoff) * freq,
						sqrtf((x + xoff) * (y + yoff)) * freq));

		}
	}

	// combine maps
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			for (i = 0, p = submaps[i];
			i < sizeof(submaps) / sizeof(submaps[0]);
			i++, p = submaps[i]) {
				pix = (int)(dst[y * size + x])
					+ (int)(p[y * size + x]) / (2 << i);
				if (pix < -128)
					pix = -128;
				else if (pix > 127)
					pix = 127;
				dst[y * size + x] = (char)pix;
			}
		}
	}

	for (x = 0; x < sizeof(submaps) / sizeof(submaps[0]); x++)
		free(submaps[x]);
}

static uchar	g_heightmap[HEIGHTMAP_SIZE * HEIGHTMAP_SIZE];

const uchar *ac_gen_terrain(int seed) {
	int x, y, xoff, yoff, pix;
	char *cloudmap = malloc(sizeof(g_heightmap));
	float freq = 0.005 + 0.000001 * (float)((ac_gen_rand() % 6000) - 3000);

	g_seed = seed;
	xoff = ac_gen_rand() % (HEIGHTMAP_SIZE * 2);
	yoff = ac_gen_rand() % (HEIGHTMAP_SIZE * 2);

	memset(g_heightmap, 127, sizeof(g_heightmap));

	ac_gen_cloudmap(cloudmap, HEIGHTMAP_SIZE);

	for (y = 0; y < HEIGHTMAP_SIZE; y++) {
		for (x = 0; x < HEIGHTMAP_SIZE; x++) {
#if 1
			// pass 1 - rough topography
			((char *)g_heightmap)[y * HEIGHTMAP_SIZE + x] += (char)(127.f
				* ac_gen_perlin(
					(float)(x + xoff) * freq,
					(float)(y + yoff) * freq,
					sqrtf((x + xoff) * (y + yoff)) * freq));
#endif
#if 1
			// pass 2 - detail
			freq *= 10.0;
			((char *)g_heightmap)[y * HEIGHTMAP_SIZE + x] += (char)(16.f
				* ac_gen_perlin(
					(float)(x + xoff) * freq,
					(float)(y + yoff) * freq,
					sqrtf((x + xoff) * (y + yoff)) * freq));
#endif
#if 1
			// pass 3 - cloud detail
			pix = (int)(g_heightmap[y * HEIGHTMAP_SIZE + x]) +
				(int)(cloudmap[y * HEIGHTMAP_SIZE + x] / 2);
			if (pix < 0)
				pix = 0;
			else if (pix > 255)
				pix = 255;
			((char *)g_heightmap)[y * HEIGHTMAP_SIZE + x] = pix;
#endif
		}
	}

	free(cloudmap);

	return g_heightmap;
}
