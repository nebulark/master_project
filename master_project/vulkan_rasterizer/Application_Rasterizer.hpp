#pragma once

#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "VmaRAII.hpp"
#include "common/Deleters.hpp"
#include "GraphicBackend.hpp"
#include "Camera.hpp"
#include "InputManager.hpp"
#include "LineDrawer.hpp"

class Application_Rasterizer
{

public:
	Application_Rasterizer();
	bool Update();

private:

	using ClockType = std::chrono::steady_clock;
	using FloatSeconds = std::chrono::duration<float>;

	WindowPtr m_sdlWindow;
	GraphicsBackend m_graphcisBackend;
	Camera m_camera;
	InputManager m_inputManager;
	void HandleEvent(SDL_Event event);
	void GameUpdate(float Seconds);
	ClockType::time_point m_lastTime;
	std::vector<Line> m_extraLines;
};
