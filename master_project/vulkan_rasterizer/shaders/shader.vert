#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstant {
    mat4 model;
} pc;

layout(set = 1, binding = 0) uniform Ubo_GlobalRenderData {
    mat4 view;
    mat4 proj;
} u_grd;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = 
	u_grd.proj *
	u_grd.view *
	pc.model *
	vec4(inPosition, 1.0);

//	gl_Position = vec4(clamp(inPosition, vec3(-1.0), vec3(+1.0)), 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}