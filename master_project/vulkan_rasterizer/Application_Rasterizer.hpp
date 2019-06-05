#pragma once

#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "VmaRAII.hpp"
#include "common/Deleters.hpp"
#include "GraphicBackend.hpp"
#include "Camera.hpp"

class Application_Rasterizer
{

public:
	Application_Rasterizer();
	bool Update();

private:


	WindowPtr m_sdlWindow;
	GraphicsBackend m_graphcisBackend;
	Camera m_camera;
};
