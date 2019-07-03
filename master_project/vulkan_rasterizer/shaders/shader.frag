#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

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

    outColor = texture(texSampler,fragTexCoord);
	if(pc.debugColor.w != 0)
	{
		outColor = pc.debugColor;
	}
}