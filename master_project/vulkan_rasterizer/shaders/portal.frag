#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_stencil_export : enable

layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput inputDepth;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outRenderedDepth;

out int gl_FragStencilRefARB;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
	gl_FragStencilRefARB = 1;
	outRenderedDepth = gl_FragCoord.z;
}