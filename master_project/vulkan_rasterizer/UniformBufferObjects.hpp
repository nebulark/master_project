#pragma once

#include "glm.hpp"

struct Ubo_GlobalRenderData
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
