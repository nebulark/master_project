#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_stencil_export : enable


layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outRenderedDepth;

out int gl_FragStencilRefARB;

layout(push_constant) uniform PushConstant {
    mat4 model;
	uint cameraIdx;
	uint portalStencilVal;
} pc;

void main() 
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
	int stencilVal = int( pc.portalStencilVal);
	gl_FragStencilRefARB =stencilVal;
	outRenderedDepth = gl_FragCoord.z;
}