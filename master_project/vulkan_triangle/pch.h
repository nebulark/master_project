#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <optional>

#include <gsl/gsl>
#include "SDL.h"
#include "SDL_vulkan.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan uses depth range 0 to 1, instead of the gl range -1 to 1
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "stb/stb_image.h"

#pragma warning(push, 1)   
#include <vulkan/vulkan.hpp>
#pragma warning(pop)
