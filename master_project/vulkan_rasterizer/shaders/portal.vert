#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform Ubo_GlobalRenderData {
    mat4 proj;
} u_grd;

layout(constant_id = 1) const int maxCameraMatCount = 3257437;
layout(set = 2, binding = 0) uniform ubo_cameraMats
{
	mat4 mats[maxCameraMatCount];
} u_cMats;

layout(set = 4, binding = 0) buffer CameraIndices {
    int cIndices[];
} ci;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out flat int outInstanceIndex;

const uint invalid_matIndex = ~0;

layout(push_constant) uniform PushConstant {
	mat4 model;
	vec4 debugColor;
	int layerStartIndex;
	int nextLayerStartIndex;
	int portalIndex;
	int maxVisiblePortalCount;
	int currentPortalCount;
} pc;

void main() {
	
	int cameraIndicesIndex = gl_InstanceIndex + pc.layerStartIndex;
	outInstanceIndex = gl_InstanceIndex;


	uint viewMatIndex = cameraIndicesIndex == 0 ? 0 :  ci.cIndices[cameraIndicesIndex];


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

	//gl_Position = vec4(clamp(inPosition, vec3(-1.f), vec3(1)), 1.0);
}