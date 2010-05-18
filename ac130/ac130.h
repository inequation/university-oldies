// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main header file

#ifndef AC130_H
#define AC130_H

// some includes shared by all the modules
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <SDL/SDL.h>
#include <assert.h>

// our own math module
#include "ac_math.h"

// =========================================================
// Type definitions
// =========================================================

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;

/// Tree prop structure.
typedef struct {
	/// position at which to spawn the tree
	ac_vec4_t				pos;
	/// orientation (rotation around the Y axis) of the tree
	float					ang;
	/// horizontal scale factor of the tree
	float					XZscale;
	/// vertical scale factor of the tree
	float					Yscale;
} ac_tree_t;

/// Building prop structure.
typedef struct {
	/// position of the building's origin
	ac_vec4_t				pos;
	/// orientation (rotation around the Y axis) of the building
	float					ang;
	/// horiz. X scale factor of the building
	float					Xscale;
	/// horiz. Z scale factor of the building
	float					Yscale;
	/// vertical scale factor of the building
	float					Zscale;
	/// "architectural" setting - if true, adds a building with a slanted roof,
	/// otherwise a flat one
	bool					slantedRoof;
} ac_bldg_t;

/// Prop tree node structure.
typedef struct ac_prop_s {
	ac_vec4_t				bounds[2];	/// 2 points describing the AABB (axis-
										/// aligned bounding box) of the node
	struct ac_prop_s		*child[4];	/// pointers to children nodes
	ac_tree_t				*trees;		/// tree array (NULL if node is a branch
										/// or a building prop leaf)
	ac_bldg_t				*bldgs;		/// building array (NULL if node is a
										/// branch or a tree prop leaf)
} ac_prop_t;

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

// =========================================================
// Content generator interface
// =========================================================

#define HEIGHTMAP_SIZE		1024
#define HEIGHT				50.f
#define HEIGHT_SCALE		(HEIGHT / 255.f)

#define TREE_BASE			7
#define BLDG_FLAT_VERTS		8
#define BLDG_FLAT_INDICES	16
#define BLDG_SLNT_VERTS		10
#define BLDG_SLNT_INDICES	28
#define PROP_TEXTURE_SIZE	64

#define PROPMAP_SHIFT		4
#define PROPMAP_SIZE		(HEIGHTMAP_SIZE >> PROPMAP_SHIFT)
#define TREE_COVERAGE		0.6
#define BLDG_COVERAGE		0.1
#define TREES_PER_FIELD		25
#define BLDGS_PER_FIELD		1

#define MAX_NUM_TREES		(TREES_PER_FIELD								\
								* PROPMAP_SIZE * PROPMAP_SIZE * TREE_COVERAGE)
#define MAX_NUM_BLDGS		(BLDGS_PER_FIELD								\
								* PROPMAP_SIZE * PROPMAP_SIZE * BLDG_COVERAGE)

#define FX_TEXTURE_SIZE		256

extern uchar				g_heightmap[];
extern ac_prop_t			*g_proptree;

/// Generates the terrain heightmap.
/// \note				The heightmap is stored in stack memory, therefore it
///						should not be freed.
/// \return				constant pointer to the heightmap
/// \param seed			random number seed; ensures identical random number
///						sequence each run
void ac_gen_terrain(int seed);

/// Generates props (trees, buildings) resources.
/// \param texture		texture byte array
/// \param verts		vertex array
/// \param indices		index array
void ac_gen_props(uchar *texture, ac_vertex_t *verts, uchar *indices);


/// Generates FX (bullet tracers, explosions, smoke plums etc.) resources.
/// \param texture		texture byte array
/// \param verts		vertex array
/// \param indices		index array
void ac_gen_fx(uchar *texture, ac_vertex_t *verts, uchar *indices);

/// Generates prop (tree and buildings) lists.
/// \note				Both trees and bldgs must be preallocated by the caller.
/// \param numTrees		pointer to where to store the tree count
/// \param trees		the array to which to write the prop placement
///						information
/// \param numBldgs		pointer to where to store the building count
/// \param bldgs		the array to which to write the prop placement
///						information
void ac_gen_proplists(int *numTrees, ac_tree_t *trees,
					int *numBldgs, ac_bldg_t *bldgs);

/// Frees the memory allocated for the given prop tree node and all of its
/// children.
/// \param n			pointer to the node of the tree to free (pass NULL to
///						free the entire tree)
void ac_gen_free_proptree(ac_prop_t *n);

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
#define TERRAIN_LOD					40.f

#define PROP_LOD_DISTANCE	250.f

/// Initializes the renderer.
/// \param vcounter		vertex counter address (for performance measurement)
/// \param tcounter		triangle counter address (for performance measurement)
/// \param dpcounter	displayed terrain patch counter address
/// \param cpcounter	culled terrain patch counter address
/// \return				true on success
bool ac_renderer_init(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter);

/// Shuts the renderer down.
void ac_renderer_shutdown(void);

/// Sets new terrain heightmap.
void ac_renderer_set_heightmap();

/// Starts the rendering of the next frame. Also sts the point of view.
/// \note				Must be called *before* \ref ac_renderer_finish3D
void ac_renderer_start_scene(int time, ac_viewpoint_t *vp);

/// Starts the FX rendering stage - calls the necessary state changes, etc.
/// \sa ac_renderer_finish_fx
/// \sa ac_renderer_draw_tracer
void ac_renderer_start_fx(void);

/// Finishes the FX rendering stage - calls the necessary state changes, etc.
/// \sa ac_renderer_start_fx
/// \sa ac_renderer_draw_tracer
void ac_renderer_finish_fx(void);

/// Draws a smoke particle at the given position with the given scale and alpha.
/// \param pos			position of the smoke particle
/// \param scale		scale of the particle
/// \param alpha		alpha (transparency) value of the particle
/// \param angle		angle by which to rotate the particle
void ac_renderer_draw_fx(ac_vec4_t pos, float scale, float alpha, float angle);

/// Draws a bullet tracer at the given position in the given direction.
/// \note				Must be called *after* \ref ac_renderer_start_fx and
///						*before* \ref ac_renderer_finish_fx
/// \param pos			position of the tracer
/// \param dir			tracer's direction
/// \param scale		scale of the tracer (length in metres, width in pixels)
void ac_renderer_draw_tracer(ac_vec4_t pos, ac_vec4_t dir, float scale);

/// Finishes the 3D rendering stage - flushes the scene to the render target and
/// switches to the 2D (HUD) stage.
/// \note				Must be called *after* \ref ac_renderer_start_scene and
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
