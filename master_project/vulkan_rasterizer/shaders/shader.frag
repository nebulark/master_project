#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

#ifdef SUBSEQUENT_PASS
layout (input_attachment_index = 0, set = 3, binding = 0) uniform subpassInput inputDepth;
#endif

const vec3 directionalLightDir = normalize(vec3(1.0,1.0,1.0));

layout(push_constant) uniform PushConstant {
    mat4 model;
	vec4 debugColor;
	int cameraIdx;

	// the index of the first element in PortalIndexHelper we need to consider to calculate our childnum
	int firstHelperIndex;
	// our index in  PortalIndexHelper
	int currentHelperIndex;

	// this + our childnum gets us the index for the cameraindices Buffer element to write our camera index into
	int firstCameraIndicesIndex;

	// the stencil value of the layer
	uint layerStencilVal;

	// the index we need to write into CameraIndices
	int portalCameraIndex;

	// number of bits we need to shift our stencil val before ORing it with the layerStencilVal
	int numOfBitsToShiftChildStencilVal;

	int maxVisiblePortalCountForRecursion;
} pc;

void main() {

#ifdef SUBSEQUENT_PASS
	if(gl_FragCoord.z <= subpassLoad(inputDepth).r) 
	{
		discard;
	}
#endif

	vec3 color = texture(texSampler,fragTexCoord).xyz;
	if(pc.debugColor.w != 0)
	{
		color = pc.debugColor.xyz;
	}


	const float ambient = 0.2;

	vec3 normal = normalize(fragNormal);
	float diffuse = max(0, dot(directionalLightDir, normal));

	color = color * min(diffuse + ambient, 1.0);


	outColor = vec4(color,1.0);
}