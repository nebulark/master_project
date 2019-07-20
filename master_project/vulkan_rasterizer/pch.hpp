#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <optional>
#include <unordered_set>
#include <array>
#include <functional>

#include <gsl/gsl>
#include "SDL.h"
#include "SDL_vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan uses depth range 0 to 1, instead of the gl range -1 to 1
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/hash.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/quaternion.hpp"
#include "gtx/quaternion.hpp"


#include "stb/stb_image.h"
#include "tinyObj/tiny_obj_loader.h"

#pragma warning(push, 1)   
#include <vulkan/vulkan.hpp>
#pragma warning(pop)
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_iterators.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidxml/rapidxml_utils.hpp"
