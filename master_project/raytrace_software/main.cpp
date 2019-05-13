// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"



int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window = SDL_CreateWindow(
		"An SDL2 window",                  // window title
		SDL_WINDOWPOS_UNDEFINED,           // initial x position
		SDL_WINDOWPOS_UNDEFINED,           // initial y position
		640,                               // width, in pixels
		480,                               // height, in pixels
		SDL_WINDOW_OPENGL                  // flags - see below
	);

	// Check that the window was successfully created
	if (window == nullptr) {
		// In the case that the window could not be made...
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	while (true) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type)
			{
				case SDL_WINDOWEVENT:
				{
					if (event.window.event == SDL_WINDOWEVENT_CLOSE)
					{
						goto end_event_loop;
					}
				}
			}

			/* handle your event here */
		}
		/* do some other stuff here -- draw your app, etc. */
	}
	end_event_loop:

	// Close and destroy the window
	SDL_DestroyWindow(window);

	// Clean up
	SDL_Quit();
	return 0;

}


