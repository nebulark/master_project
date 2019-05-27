// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "Application.hpp"



int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	{
		Application app{};
		while (app.Update());
	}
	SDL_Quit();
		

	return 0;
	
}


