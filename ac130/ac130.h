// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main header file

#ifndef AC130_H
#define AC130_H

// some includes shared by all the modules
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL/SDL.h>

// our own math module
#include "ac_math.h"

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;

// =========================================================
// Renderer interface
// =========================================================

///
typedef struct {
	ac_vec4_t	origin;
	float		angles[2];
} ac_viewpont_t;

/// Initializes the renderer.
/// \return true on success
bool ac_renderer_init(void);

/// Shuts the renderer down.
void ac_renderer_shutdown(void);

/// Sets the point of view.
void ac_renderer_viewpoint(ac_viewpont_t *vp);

// =========================================================
// Content generator interface
// =========================================================

#define HEIGHTMAP_SIZE		256

/// Generates the terrain heightmap.
/// \return constant pointer to the heightmap
const uchar *ac_gen_terrain(int seed);

// =========================================================
// Game logic interface
// =========================================================

#define INPUT_NEGATIVE		0x01
#define INPUT_MOUSE_LEFT	0x02
#define INPUT_MOUSE_RIGHT	0x04

typedef struct {
	int			flags;
	short		deltaX, deltaY;
} ac_input_t;

/// Initializes the game logic.
/// \return true on success
bool ac_game_init(void);

/// Shuts the game logic down.
void ac_game_shutdown(void);

/// Advances the game world by one frame.
/// \param frameTime time elapsed since last frame in seconds
/// \param input current state of player input
void ac_game_frame(float frameTime, ac_input_t *input);

#endif // AC130_H
