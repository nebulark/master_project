#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 1) const int cameraMatCount = 3;

layout(push_constant) uniform PushConstant {
    mat4 model;
	uint cameraIdx;
	uint portalStencilVal;
} pc;

layout(set = 1, binding = 0) uniform Ubo_GlobalRenderData {
    mat4 view;
    mat4 proj;
} u_grd;

layout(set = 2, binding = 0) uniform ubo_cameraMats
{
	mat4 mats[cameraMatCount];
} u_cMats;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main() {
   	mat4 viewMat = u_cMats.mats[pc.cameraIdx];

    gl_Position = 
	u_grd.proj *
	viewMat * //u_grd.view *
	pc.model *
	vec4(inPosition, 1.0);

//	gl_Position = vec4(clamp(inPosition, vec3(-1.0), vec3(+1.0)), 1.0);
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}