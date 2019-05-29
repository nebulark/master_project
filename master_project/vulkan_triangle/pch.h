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
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "stb/stb_image.h"

#pragma warning(push, 1)   
#include <vulkan/vulkan.hpp>
#pragma warning(pop)
