#pragma once

#include "glm.hpp"
#include "NTree.hpp"
#include "GetSizeUint32.hpp"
#include "Transform.hpp"



struct Portal
{
	int meshIndex;
	Transform a_transform;
	Transform b_transform;

	Transform a_to_b;
	Transform b_to_a;

	static Portal CreateWithPortalTransforms(int meshIndex, const Transform& a_transform, const Transform& b_transform);
	static Portal CreateWithTransformAndAtoB(int meshIndex, const Transform& a_transform, const Transform& a_to_b);
};

inline Portal Portal::CreateWithPortalTransforms(int meshIndex, const Transform& a_transform, const Transform& b_transform)
{
	Portal result = { meshIndex, a_transform, b_transform };
	result.a_to_b = Transform::CalcAtToB(a_transform, b_transform);
	result.b_to_a = result.a_to_b.CalcInversion();
	return result;
}

inline Portal Portal::CreateWithTransformAndAtoB(int meshIndex, const Transform& a_modelmat, const Transform& a_to_b)
{
	Portal result;
	result.meshIndex = meshIndex;
	result.a_transform = a_modelmat;
	result.a_to_b = a_to_b;
	result.b_transform = a_to_b * a_modelmat;
	result.b_to_a = a_to_b.CalcInversion();
	return result;
}



