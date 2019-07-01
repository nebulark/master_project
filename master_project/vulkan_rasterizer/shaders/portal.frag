#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_stencil_export : enable


layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out float outRenderedDepth;
layout(location = 1) out vec4 outColor;

out int gl_FragStencilRefARB;

layout(push_constant) uniform PushConstant {
    mat4 model;
	vec4 debugColor;
	uint cameraIdx;
	uint portalStencilVal;



	#if 0
	// the index of the first element in PortalIndexHelper we need to consider to calculate our childnum
	uint firstHelperIndex;
	// our index in  PortalIndexHelper
	uint currentHelperIndex;

	// this + our childnum gets us the index for the cameraindices Buffer element to write our camera index into
	uint firstCameraIndicesIndex;

	// the stencil value of the layer
	uint layerStencilVal;

	// the index we need to write into CameraIndices
	uint portalCameraIndex;

	// number of bits we need to shift our stencil val before ORing it with the layerStencilVal
	uint numOfBitsToShiftChildStencilVal;
	#endif
} pc;


#if 0
layout(constant_id = 0) const int maxPortalCount = 4;
layout(constant_id = 0) const int maxVisiblePortalCount = 3;

layout(set = 2, binding = 0) uniform ubo_cameraMats
{
	mat4 mats[cameraMatCount];
} u_cMats;

layout(set = 6, binding = 0) buffer PortalIndexHelper {
    int indices[];
} pih;

layout(set = 5, binding = 0) buffer CameraIndices {
    int cIndices[];
} ci;
#endif
void main() 
{
#if 1
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
	int stencilVal = int( pc.portalStencilVal);
	gl_FragStencilRefARB =stencilVal;
	outRenderedDepth = gl_FragCoord.z;

	outColor =pc.debugColor;
#endif

#if 0
	// count previous visible portals
	int childNum = 0;
	for(int i = pc.firstHelperIndex; i < pc.currentHelperIndex; ++i)
	{
		childNum+=1;
	}

	if(childNum >= maxVisiblePortalCount)
	{
		// to many visible portals, sadly we won't be visible,
		// we could set a value, so that subsequent portals won't be rendered to maybe improb perf
		discard;
	}

	// mark that we are a visible portal
	ih.indices[pc.currentHelperIndex] = 1;

	// skip zero to avoid ambiguities
	uint myStencilVal = childNum+1;

	gl_FragStencilRefARB = pc.layerStencilVal | (myStencilVal << pc.numOfBitsToShiftChildStencilVal);

	// write our camera index into camera index buffer
	ci.cIndices[pc.firstCameraIndicesIndex + childNum] = pc.portalCameraIndex;
	outRenderedDepth = gl_FragCoord.z;

	outColor =pc.debugColor;
	#endif

}