#include "pch.hpp"
#include "Application_Rasterizer.hpp"


namespace
{
	

}

Application_Rasterizer::Application_Rasterizer()
{
	constexpr int width = 1920 / 2;
	constexpr int height = 1080 / 2;

	m_sdlWindow = WindowPtr{
		SDL_CreateWindow(
		"An SDL2 window",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WindowFlags::SDL_WINDOW_VULKAN | SDL_WindowFlags::SDL_WINDOW_RESIZABLE
		)
	};


}

bool Application_Rasterizer::Update()
{
	VmaAllocator
}
