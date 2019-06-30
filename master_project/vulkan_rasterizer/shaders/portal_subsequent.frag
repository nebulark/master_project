#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_stencil_export : enable


layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out float outRenderedDepth;
layout(location = 1) out vec4 outColor;

out int gl_FragStencilRefARB;

layout (input_attachment_index = 0, set = 3, binding = 0) uniform subpassInput inputDepth;

void main() 
{

	if(gl_FragCoord.z < subpassLoad(inputDepth).r)
	{
		discard;
	}

    outColor = vec4(1.0, 0.0, 0.0, 1.0);
	gl_FragStencilRefARB = 1;
	outRenderedDepth = gl_FragCoord.z;
}