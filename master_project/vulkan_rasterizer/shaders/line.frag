#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 incolor;

out int gl_FragStencilRefARB;

#ifdef SUBSEQUENT_PASS
layout (input_attachment_index = 0, set = 3, binding = 0) uniform subpassInput inputDepth;
#endif

layout(push_constant) uniform PushConstant {
	vec4 posA;
	vec4 posB;
	vec4 debugColorA;
	vec4 debugColorB;
	int cameraIdx;

} pc;

layout(location = 0) out vec4 outColor;
void main() 
{
	
#ifdef SUBSEQUENT_PASS
	if(gl_FragCoord.z <= subpassLoad(inputDepth).r) 
	{
		discard;
	}
#endif

	outColor = incolor;
}