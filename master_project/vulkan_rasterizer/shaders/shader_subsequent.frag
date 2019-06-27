#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout (input_attachment_index = 0, set = 3, binding = 0) uniform subpassInput inputDepth;

void main() {

	if(gl_FragCoord.z < subpassLoad(inputDepth).r)
	{
		discard;
	}

    outColor = texture(texSampler,fragTexCoord);

	if(gl_FragCoord.z < subpassLoad(inputDepth).r)
	{
		outColor = vec4(1.f,0.f,0.f,1.f);
	}
	else
	{
		outColor = vec4(0.f,1.f,0.f,1.f);
	}

}