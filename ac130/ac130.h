// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main header file

#ifndef AC130_H
#define AC130_H

// some includes shared by all the modules
#include <stdlib.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <assert.h>

// our own math module
#include "ac_math.h"

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;

// =========================================================
// Renderer interface
// =========================================================

#define TERRAIN_PATCH_SIZE			17
#define TERRAIN_PATCH_SIZE_F		17.f
#define TERRAIN_NUM_VERTS			(										\
										TERRAIN_PATCH_SIZE					\
										* TERRAIN_PATCH_SIZE				\
										+ TERRAIN_PATCH_SIZE * 4 - 4		\
									)	// plus skirt verts at each edge minus
										// duplicated corners
#define TERRAIN_NUM_BODY_INDICES	(										\
										(2 * TERRAIN_PATCH_SIZE + 2)		\
										* (TERRAIN_PATCH_SIZE - 1)			\
									)
#define TERRAIN_NUM_SKIRT_INDICES	(										\
										2 * (TERRAIN_PATCH_SIZE * 2			\
											+ 2 * (TERRAIN_PATCH_SIZE - 2))	\
										+ 2									\
									)	// + 2 degenerate triangles
#define TERRAIN_NUM_INDICES			(TERRAIN_NUM_BODY_INDICES				\
										+ TERRAIN_NUM_SKIRT_INDICES)
#define TERRAIN_LOD					50.f

/// Viewpoint definition structure.
typedef struct {
	ac_vec4_t	origin;		/// camera position
	float		angles[2];	/// camera direction defined by yaw and pitch angles
	float		fov;		/// field of view angle in radians
} ac_viewpoint_t;

/// Geometry vertex.
typedef struct {
	ac_vec4_t	pos;		/// vertex position
	float		st[2];		/// texture coordinates
} ac_vertex_t;

/// Initializes the renderer.
/// \param vcounter		vertex counter address (for performance measurement)
/// \param tcounter		triangle counter address (for performance measurement)
/// \return				true on success
bool ac_renderer_init(uint *vcounter, uint *tcounter);

/// Shuts the renderer down.
void ac_renderer_shutdown(void);

/// Sets new terrain heightmap.
void ac_renderer_set_heightmap(uchar *hmap);

/// Starts the rendering of the next frame. Also sts the point of view.
/// \note				Must be called *before* \ref ac_renderer_finish3D
void ac_renderer_start_scene(ac_viewpoint_t *vp);

/// Adds a tree to the scene at the given position.
void ac_renderer_add_tree(ac_vec4_t origin);

/// Adds a building to the scene.
/// \param origin		position of the building's origin
/// \param dimensions	dimensions of the building
/// \param angle		building's yaw angle
/// \param slantedRoof	"architectural" setting - if true, adds a building with
///						a slanted roof, otherwise a flat one
void ac_renderer_add_bldg(ac_vec4_t origin, ac_vec4_t dimensions, float angle,
						bool slantedRoof);

/// Finishes the 3D rendering stage - flushes the scene to the render target and
/// switches to the 2D (HUD) stage.
/// \note				Must be called *after* \ref ac_renderer_viewpoint and
///						*before* \ref ac_renderer_finish2D
void ac_renderer_finish_3D(void);

/// Finishes the 2D rendering stage - flushes the 2D (HUD) elements to the
/// render target.
/// \note				Must be called *after* \ref ac_renderer_finish3D and
///						*before* \ref ac_renderer_composite
void ac_renderer_finish_2D(void);

/// Combines the 3D and 2D parts of the scene, runs post-processing effects and
/// outputs the frame to screen.
/// \param negative		if true, inverts display colours
void ac_renderer_composite(bool negative);

// =========================================================
// Content generator interface
// =========================================================

#define HEIGHTMAP_SIZE		512
#define HEIGHT_SCALE		(50.f / 255.f)

/// Generates the terrain heightmap.
/// \note				The heightmap is stored in stack memory, therefore it
///						should not be freed.
/// \return				constant pointer to the heightmap
/// \param seed			random number seed; ensures identical random number
///						sequence each run
const uchar *ac_gen_terrain(int seed);

/// Generates tree resources.
/// \note				All the resources are stored in heap memory, therefore
///						freeing it is the client's responsibility.
/// \param texture		texture pointer address
/// \param verts		vertex array pointer address
/// \param indices		index array pointer address
void ac_gen_tree(uchar **texture, ac_vertex_t **verts, int **indices);

/// Generates unit-size (1x1x1) building resources.
/// \note				All the resources are stored in heap memory, therefore
///						freeing it is the client's responsibility.
/// \param texture		texture pointer address
/// \param flatVerts	vertex array pointer address for the flat roof building
///						variant
/// \param flatIndices	index array pointer address for the flat roof building
///						variant
/// \param slantedVerts	vertex array pointer address for the slanted roof
///						building variant
/// \param slantedIndices	index array pointer address for the slanted roof
///						building variant
void ac_gen_buildings(uchar **texture,
					ac_vertex_t **flatVerts, int **flatIndices,
					ac_vertex_t **slantedVerts, int **slantedIndices);

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
/// \param ticks		number of ticks (milliseconds) since the start of game
/// \param frameTime	time elapsed since last frame in seconds
/// \param input		current state of player input
void ac_game_frame(int ticks, float frameTime, ac_input_t *input);

#endif // AC130_H
