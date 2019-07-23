#pragma once
#include "glm.hpp"

inline constexpr glm::vec4 ColorFromHSV(float hDegrees, float s, float v) 
{

	assert(0.f < hDegrees && hDegrees < 360.f);
	
	if (s == 0.f)
	{
		return glm::vec4(glm::vec3(v), 1.f);
	}
	else
	{
		

		float hBy60 = hDegrees / 60.f;
		int i = static_cast<int>(hBy60);
		float frac = hBy60 - i;

		float p = v * (1.0f - s);
		float q = v * (1.0f - (s * frac));
		float t = v * (1.0f - (s * (1.0f - frac)));

		switch (i)
		{
		case 0:
			return glm::vec4(v, t, p, 1.f);
		case 1:
			return glm::vec4(q, v, p, 1.f);
		case 2:
			return glm::vec4(p, v, t, 1.f);
		case 3:
			return glm::vec4(p, q, v, 1.f);
		case 4:
			return glm::vec4(t, p, v, 1.f);
		default:
			return glm::vec4(v, p, q, 1.f);
		}

	}
}
