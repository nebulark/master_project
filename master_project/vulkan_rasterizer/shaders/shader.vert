#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstant {
 	mat4 model;
	vec4 debugColor;
	int cameraIdx;
	uint compareStencilVal;
} pc;

layout(set = 1, binding = 0) uniform Ubo_GlobalRenderData {
    mat4 proj;
} u_grd;

layout(constant_id = 1) const int cameraMatCount = 21;
layout(set = 2, binding = 0) uniform ubo_cameraMats
{
	mat4 mats[cameraMatCount];
} u_cMats;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

layout(constant_id = 0) const int maxPortalCount = 4;

layout(set = 4, binding = 0) buffer CameraIndices {
    int cIndices[];
} ci;

const uint invalid_matIndex = ~0;

void main() {

	uint viewMatIndex = pc.cameraIdx == 0 ? 0 :  ci.cIndices[pc.cameraIdx];
	//viewMatIndex = pc.cameraIdx;

	if(viewMatIndex != invalid_matIndex)
	{
		mat4 viewMat = u_cMats.mats[viewMatIndex];

		gl_Position = 
		u_grd.proj *
		viewMat *
		pc.model *
		vec4(inPosition, 1.0);
	}
	else
	{
		gl_Position = vec4(1);
	}

    fragNormal = inNormal;
    fragTexCoord = inTexCoord;

 }