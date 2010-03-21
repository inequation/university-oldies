// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Renderer module

#include "ac130.h"
#include <GL/glew.h>
#define NO_SDL_GLEXT	// GLEW takes care of extensions
#include <SDL/SDL_opengl.h>

SDL_Surface	*g_screen;

bool ac_renderer_init(void) {
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1);

	SDL_WM_SetCaption("AC-130", "AC-130");
	SDL_ShowCursor(0);

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
	// initialize matrices
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set face culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	return true;
}

void ac_renderer_shutdown(void) {
	SDL_QuitSubSystem(SDL_INIT_VIDEO);	// FIXME: this shuts input down as well
}
