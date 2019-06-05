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

	m_graphcisBackend.Init(m_sdlWindow.get());

	m_camera.SetPerspection( 0.1f, 10.0f, glm::radians(45.f), glm::vec2(width, height));
	m_camera.m_position = { 2.f,2.f,2.f };
	m_camera.LookAt( glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

bool Application_Rasterizer::Update()
{

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (SDL_WINDOWEVENT == event.type && SDL_WINDOWEVENT_CLOSE == event.window.event)
		{
			m_graphcisBackend.WaitIdle();
			return false;
		}
		//handle_event(event);
	}
	m_graphcisBackend.Render(m_camera);
	SDL_UpdateWindowSurface(m_sdlWindow.get());
	return true;
}

