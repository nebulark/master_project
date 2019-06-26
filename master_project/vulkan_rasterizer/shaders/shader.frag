#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


layout(constant_id = 0) const int useInputDepth = 0;

#if useInputDepth
layout (input_attachment_index = 0, set = 3, binding = 0) uniform subpassInput inputDepth;
#endif

void main() {

	#if useInputDepth
	if(gl_FragCoord.z < subpassLoad(inputDepth))
	{
		discard;
	}
	#endif

    outColor = texture(texSampler,fragTexCoord);
}