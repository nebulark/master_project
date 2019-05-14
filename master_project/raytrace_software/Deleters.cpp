#include "pch.h"
#include "Deleters.hpp"

void Deleters::RendererDeleter::operator()(SDL_Renderer* ptr)
{
	SDL_DestroyRenderer(ptr);
}

void Deleters::WindowDeleter::operator()(SDL_Window* ptr)
{
	SDL_DestroyWindow(ptr);
}
