#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstant {
	vec4 posA;
	vec4 posB;
	vec4 debugColorA;
	vec4 debugColorB;
	int cameraIndexAndStencilCompare;
} pc;

layout(set = 1, binding = 0) uniform Ubo_GlobalRenderData {
    mat4 proj;
} u_grd;

layout(constant_id = 1) const int maxCameraMatCount = 33825;
layout(set = 2, binding = 0) uniform ubo_cameraMats
{
	mat4 mats[maxCameraMatCount];
} u_cMats;


layout(constant_id = 0) const int maxPortalCount = 4;

layout(set = 4, binding = 0) buffer CameraIndices {
    int cIndices[];
} ci;

const uint invalid_matIndex = ~0;

layout(location = 0) out vec4 outcolor;

void main() {

	vec4 worldPos = gl_VertexIndex == 0 ? pc.posA : pc.posB;
	outcolor = gl_VertexIndex == 0 ? pc.debugColorA : pc.debugColorB;


	uint viewMatIndex = pc.cameraIndexAndStencilCompare == 0 ? 0 :  ci.cIndices[pc.cameraIndexAndStencilCompare];

	if(viewMatIndex != invalid_matIndex)
	{
		mat4 viewMat = u_cMats.mats[viewMatIndex];

		gl_Position = 
		u_grd.proj *
		viewMat *
		worldPos;
	}
	else
	{
		gl_Position = vec4(1);
	}
 }