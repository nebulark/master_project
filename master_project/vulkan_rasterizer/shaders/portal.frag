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


layout(constant_id = 0) const int maxPortalCount = 4;

layout(set = 2, binding = 0) uniform ubo_cameraMats
{
	mat4 mats[];
} u_cMats;

layout(set = 5, binding = 0) buffer PortalIndexHelper {
    int indices[];
} pih;

layout(set = 4, binding = 0) buffer CameraIndices {
    int cIndices[];
} ci;


void main() 
{
#if 1
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
	
	int stencilVal = int(pc.layerStencilVal);
	gl_FragStencilRefARB = stencilVal;
	outRenderedDepth = gl_FragCoord.z;

	outColor =pc.debugColor;
#endif

#if 1
gl_FragStencilRefARB = 0;
	int currentViewMatIndex = pc.cameraIdx == 0 ? 0 :  ci.cIndices[pc.cameraIdx];

	int firstPortalCameraIndex = currentViewMatIndex * maxPortalCount + 1;
	int currentPortalCameraIndex = firstPortalCameraIndex + pc.portalCameraIndex;

	// count previous visible portals
	// we can use a fixed iteration count here, as the other portals won't have be processed / written and will always be zero
	int childNum = 0;
	for(int i = 0; i < maxPortalCount; ++i)
	{
		int index = i + pc.firstHelperIndex;

		// don't count myself, as I am currently writing to it an the value may be zero or 1
		if(index != pc.currentHelperIndex)
		{
			childNum+= (pih.indices[i + pc.firstHelperIndex]) == 0 ? 0 : 1;
		}
	}

	if(childNum >= pc.maxVisiblePortalCountForRecursion)
	{
		// to many visible portals, sadly we won't be visible,
		// we could set a value, so that subsequent portals won't be rendered to maybe improb perf
		discard;
	}

	// mark that we are a visible portal
	pih.indices[pc.currentHelperIndex] = childNum + 1;

	// skip zero to avoid ambiguities
	uint myStencilVal = childNum+1;
	myStencilVal = pc.portalCameraIndex + 1;

	uint stencilRef_ui = (pc.layerStencilVal | (myStencilVal << pc.numOfBitsToShiftChildStencilVal));
	int stencilRef_i = int(stencilRef_ui);
	gl_FragStencilRefARB = stencilRef_i;

	// write our camera index into camera index buffer
	//ci.cIndices[pc.firstCameraIndicesIndex + childNum] =  currentPortalCameraIndex;//pc.portalCameraIndex;
	outRenderedDepth = gl_FragCoord.z;

	outColor =pc.debugColor;
	#endif

}