#pragma once
#include "glm.hpp"

struct PushConstant_ModelMat
{
	alignas(16) glm::mat4 model;
	uint32_t cameraIdx;
};

constexpr size_t PushConstant_ModelViewProjection_Size = sizeof(PushConstant_ModelMat);

static_assert(sizeof(PushConstant_ModelMat) <= 128, "Push Constant must be small or equal to 128 Byte");
