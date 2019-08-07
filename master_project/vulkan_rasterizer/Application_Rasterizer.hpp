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
	using DoubleSeconds = std::chrono::duration<double>;

	WindowPtr m_sdlWindow;
	GraphicsBackend m_graphcisBackend;
	Camera m_camera;
	InputManager m_inputManager;
	bool m_shouldLockrotation = false;
	void HandleEvent(SDL_Event event);
	void SetTestCase(int i);
	void GameUpdate(float Seconds);
	ClockType::time_point m_lastTime;
	DrawOptions m_drawOptions;
	glm::vec3 m_savedLocation;
	glm::vec3 m_oldCameraPos;
	bool m_showRenderMilliseconds = true;

	constexpr static int frameTimeBuckedSize = 128;
	std::array<double, frameTimeBuckedSize> m_frameTimeBuckets;
	int m_currentFrameBucketIndex = 0;

	std::vector<std::vector<int>> testCases;

	int currentCase = -1;

	static constexpr int sampleWaitFrames = frameTimeBuckedSize * 2;
	int framesTillSample;
};
