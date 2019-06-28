#pragma once
#include "glm.hpp"

struct PushConstant
{
	alignas(16) glm::mat4 model;
	glm::vec4 debugColor;
	uint32_t cameraIdx;
	uint32_t portalStencilVal;
};

constexpr size_t PushConstant_Size = sizeof(PushConstant);

static_assert(PushConstant_Size <= 128, "Push Constant must be small or equal to 128 Byte");
