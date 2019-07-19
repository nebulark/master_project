#pragma once
#include "glm.hpp"

struct PushConstant_portal
{
	alignas(16) glm::mat4 model;
	glm::vec4 debugColor;
	int layerStartIndex;
	int nextLayerStartIndex;
	int portalIndex;
	int maxVisiblePortalCount;
};

constexpr size_t PushConstant_Size = sizeof(PushConstant_portal);

static_assert(PushConstant_Size <= 128, "Push Constant must be small or equal to 128 Byte");


struct PushConstant_lines
{
	glm::vec4 posA;
	glm::vec4 posB;
	glm::vec4 debugColorA;
	glm::vec4 debugColorB;
	int32_t cameraIndexAndStencilCompare;
};
constexpr size_t PushConstant_lines_size = sizeof(PushConstant_lines);
static_assert(PushConstant_lines_size <= 128, "Push Constant must be small or equal to 128 Byte");

struct PushConstant_sceneObject
{
	alignas(16) glm::mat4 model;
	glm::vec4 debugColor;
	int32_t cameraIndexAndStencilCompare;
};

