#pragma once
#include "glm.hpp"

struct PushConstant_ModelViewProjection
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 viewprojection;
};

constexpr size_t PushConstant_ModelViewProjection_Size = sizeof(PushConstant_ModelViewProjection);

static_assert(sizeof(PushConstant_ModelViewProjection) <= 128, "Push Constant must be small or equal to 128 Byte");
