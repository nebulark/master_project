#pragma once

#include "glm.hpp"




struct Portal
{
	int meshIndex;
	glm::mat4 a_modelmat;
	glm::mat4 b_modelmat;

	glm::mat4 a_to_b;
	glm::mat4 b_to_a;

	static Portal CreateWithModelMats(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& b_modelmat);
	static Portal CreateWithModelMatAndTranslation(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& a_to_b);
};

inline Portal Portal::CreateWithModelMats(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& b_modelmat)
{
	Portal result = { meshIndex, a_modelmat, b_modelmat };
	result.a_to_b = b_modelmat * glm::inverse(a_modelmat);
	result.b_to_a = glm::inverse(result.a_to_b);
	return result;
}

inline Portal Portal::CreateWithModelMatAndTranslation(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& a_to_b)
{
	Portal result;
	result.meshIndex = meshIndex;
	result.a_modelmat = a_modelmat;
	result.a_to_b = a_to_b;
	result.b_modelmat = a_to_b * a_modelmat;
	result.b_to_a = glm::inverse(a_to_b);
	return result;
}

