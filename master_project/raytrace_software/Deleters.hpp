#pragma once
#include <memory>


struct SDL_Renderer;
struct SDL_Window;

namespace Deleters
{
	struct RendererDeleter
	{
		void operator()(SDL_Renderer* ptr);
	};

	struct WindowDeleter
	{
		void operator()(SDL_Window* ptr);
	};
}

using RendererPtr = std::unique_ptr<SDL_Renderer, Deleters::RendererDeleter>;
using WindowPtr = std::unique_ptr<SDL_Window, Deleters::WindowDeleter>;