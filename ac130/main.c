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

	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}

	// initialize renderer
	if (!ac_renderer_init()) {
		fprintf(stderr, "Unable to init renderer\n");
		return 1;
	}

	// initialize tick counter
	prevTime = SDL_GetTicks();

	// make sure SDL cleans up before exit
	atexit(SDL_Quit);

	// clear out input structs
	memset(&prevInput, 0, sizeof(prevInput));
	memset(&curInput, 0, sizeof(curInput));

	// initialize game logic
	ac_game_init();

	// program main loop
	done = false;
	while (!done) {
		curTime = SDL_GetTicks();
		frameTime = (float)(curTime - prevTime) * 0.001;
		prevTime = curTime;

		memset(&curInput, 0, sizeof(curInput));
		memset(&prevInput, 0, sizeof(prevInput));
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

		ac_game_frame(frameTime, &curInput);
		prevInput = curInput;
	} // end main loop

	// shut all subsystems down
	ac_renderer_shutdown();
	ac_game_shutdown();

	return 0;
}
