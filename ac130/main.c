// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main module

#include "ac130.h"

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

	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}

	// initialize renderer
	if (!ac_renderer_init(&vertCount, &triCount, &dpCount, &cpCount)) {
		fprintf(stderr, "Unable to init renderer\n");
		return 1;
	}

	// set window caption to say that we're working
	SDL_WM_SetCaption("AC-130 - Generating resources, please wait...",
					"AC-130");

	// make sure SDL cleans up before exit
	atexit(SDL_Quit);

	// clear out input structs
	memset(&prevInput, 0, sizeof(prevInput));
	memset(&curInput, 0, sizeof(curInput));

	// initialize game logic
	ac_game_init();

	// update window caption to say that we're done generating stuff
	SDL_WM_SetCaption("AC-130", "AC-130");
	// hide mouse cursor and grab input
	//SDL_ShowCursor(0);
	//SDL_WM_GrabInput(SDL_GRAB_ON);
	//bool grab = false;

	memset(&prevInput, 0, sizeof(prevInput));

	// initialize tick counter
	frameCountTime = SDL_GetTicks();
	// hardcode the first frame time at 20ms to get a bit more accurate results
	// of the FPS counter on the first FPS calculation
	prevTime = frameCountTime - 20;

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
					if (event.key.keysym.sym == SDLK_ESCAPE)
						done = true;
					else if (event.key.keysym.sym == 'N'
								|| event.key.keysym.sym == 'n')
						curInput.flags |= INPUT_NEGATIVE;
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if (event.button.state == SDL_PRESSED) {
						curInput.flags |= event.button.button == SDL_BUTTON_LEFT
							? INPUT_MOUSE_LEFT : INPUT_MOUSE_RIGHT;
						/*grab = !grab;
						if (grab) {
							SDL_WM_GrabInput(SDL_GRAB_ON);
							SDL_ShowCursor(0);
						} else {
							SDL_WM_GrabInput(SDL_GRAB_OFF);
							SDL_ShowCursor(1);
						}*/
					} else
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

		ac_game_frame(curTime, frameTime, &curInput);
		prevInput = curInput;
		frameCount++;
	} // end main loop

	// show mouse cursor and release input
	SDL_ShowCursor(1);
	SDL_WM_GrabInput(SDL_GRAB_OFF);

	// shut all subsystems down
	ac_renderer_shutdown();
	ac_game_shutdown();

	return 0;
}
