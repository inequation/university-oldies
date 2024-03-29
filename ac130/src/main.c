// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main module

#include <string.h>
#include <time.h>
#include "ac130.h"

int m_screen_width = 1024;
int m_screen_height = 768;
bool m_full_screen = true;

static void parse_args(int argc, char *argv[]) {
	int i;

	if (argc < 2)
		return;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-w")) {
			m_full_screen = false;
			continue;
		}
		if (!strcmp(argv[i], "-r") && i + 1 < argc) {
			char buf[32];
			float aspect;

			strncpy(buf, argv[++i], sizeof(buf) - 1);
			char *x = strchr(buf, 'x');
			if (x == NULL)
				continue;
			*x++ = 0;
			m_screen_width = atoi(buf);
			m_screen_height = atoi(x);
			// sanitize the values
			if (m_screen_width <= 0)
				m_screen_width = 1024;
			if (m_screen_height <= 0)
				m_screen_height = 768;
			// enforce a 4:3 or 5:4 aspect ratio
			aspect = (float)m_screen_width / (float)m_screen_height;
			if (fabs(aspect - 4.f / 3.f) > 0.01
				&& fabs(aspect - 5.f / 4.f) > 0.01) {
				// OK, this aspect isn't right, shrink the window to get 4:3
				if (aspect > 1.334)
					m_screen_width = 1.33 * m_screen_height;
				else
					m_screen_height = 0.75 * m_screen_width;
			}
		}
	}
}

int main (int argc, char *argv[]) {
	Uint32		prevTime;	/// Time of the previous frame time in milliseconds.
	Uint32		curTime;	/// Time of the current frame time in milliseconds.
	float		frameTime;	/// \ref curTime - \ref prevTime / 1000 (in seconds)
	SDL_Event	event;
	ac_input_t	prevInput;
	ac_input_t	curInput;
	bool		done;
	uint		frameCount = 0;
	uint		vertCount = 0;
	uint		triCount = 0;
	uint		dpCount = 0;
	uint		cpCount = 0;
	uint		frameCountTime;

	parse_args(argc, argv);

	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}

	// initialize renderer
	if (!r_init(&vertCount, &triCount, &dpCount, &cpCount)) {
		fprintf(stderr, "Unable to init renderer\n");
		return 1;
	}

	// initialize the system random number generator
	srand((uint)time(NULL));

	// set window caption to say that we're working
	SDL_WM_SetCaption("AC-130 - Generating resources, please wait...",
					"AC-130");

	// hide mouse cursor and grab input
	SDL_ShowCursor(0);
#ifdef NDEBUG
	SDL_WM_GrabInput(SDL_GRAB_ON);
#else
	bool grab = m_full_screen;
	SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif // NDEBUG

	// make sure SDL cleans up before exit
	atexit(SDL_Quit);

	// clear out input structs
	memset(&prevInput, 0, sizeof(prevInput));
	memset(&curInput, 0, sizeof(curInput));

	// initialize game logic
	g_init();

	// update window caption to say that we're done generating stuff
	SDL_WM_SetCaption("AC-130", "AC-130");

	memset(&prevInput, 0, sizeof(prevInput));

	// initialize tick counter
	frameCountTime = SDL_GetTicks();
	// hardcode the first frame time at 20ms to get a bit more accurate results
	// of the FPS counter on the first FPS calculation
	prevTime = frameCountTime - 20;

#ifndef NDEBUG
	// grab the input in debug
	if (!grab) {
		SDL_WM_GrabInput(SDL_GRAB_ON);
		grab = true;
	}
#endif

	// program main loop
	done = false;
	while (!done) {
		curTime = SDL_GetTicks();
		frameTime = (float)(curTime - prevTime) * 0.001;
		prevTime = curTime;

		memset(&curInput, 0, sizeof(curInput));
		// copy buttons from last frame in case there was no MOUSEBUTTONUP event
		curInput.flags |= prevInput.flags
			& (INPUT_MOUSE_LEFT | INPUT_MOUSE_RIGHT);
		// dispatch events
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				// exit if the window is closed
				case SDL_QUIT:
					done = true;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							done = true;
							break;
						case SDLK_f:
							curInput.flags |= INPUT_NEGATIVE;
							break;
						case SDLK_1:
							curInput.flags |= INPUT_1;
							break;
						case SDLK_2:
							curInput.flags |= INPUT_2;
							break;
						case SDLK_3:
							curInput.flags |= INPUT_3;
							break;
						case SDLK_p:
							curInput.flags |= INPUT_PAUSE;
							break;
#ifndef NDEBUG
						case SDLK_g:
							grab = !grab;
							if (grab) {
								SDL_WM_GrabInput(SDL_GRAB_ON);
								SDL_ShowCursor(0);
							} else {
								SDL_WM_GrabInput(SDL_GRAB_OFF);
								SDL_ShowCursor(1);
							}
							break;
#endif // NDEBUG
						default:	// shut up compiler
							break;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if (event.button.state == SDL_PRESSED)
						curInput.flags |= event.button.button == SDL_BUTTON_LEFT
							? INPUT_MOUSE_LEFT : INPUT_MOUSE_RIGHT;
					else
						curInput.flags &= event.button.button == SDL_BUTTON_LEFT
							? ~INPUT_MOUSE_LEFT : ~INPUT_MOUSE_RIGHT;
					break;
				case SDL_MOUSEMOTION:
					curInput.deltaX = event.motion.xrel;
					curInput.deltaY = event.motion.yrel;
					break;
			}
		}

		// show fps
		if (curTime - frameCountTime >= 2000) {
			float perFrameScale = 1.f / (float)frameCount;
			printf("%.0f FPS, %.0f tris/%.0f verts, "
					"%.0f/%.0f terrain patches culled (per frame)\n",
					(float)frameCount
						/ ((float)(curTime - frameCountTime) * 0.001),
					(float)triCount * perFrameScale,
					(float)vertCount * perFrameScale,
					(float)cpCount * perFrameScale,
					(float)(dpCount + cpCount) * perFrameScale);
			frameCountTime = curTime;
			frameCount = triCount = vertCount = dpCount = cpCount = 0;
		}

		g_frame(curTime, frameTime, &curInput);
		prevInput = curInput;
		frameCount++;

#if	0
		// bad performance simulation
		SDL_Delay(100);
#endif
	} // end main loop

	// show mouse cursor and release input
	SDL_ShowCursor(1);
	SDL_WM_GrabInput(SDL_GRAB_OFF);

	// shut all subsystems down
	r_shutdown();
	g_shutdown();

	return 0;
}
