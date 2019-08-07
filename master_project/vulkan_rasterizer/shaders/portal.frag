#version 450

layout(location = 0) in flat int inInstanceIndex;

layout(location = 0) out float outRenderedDepth;
layout(location = 1) out vec4 outColor;
layout(location = 2) out uint outRenderedStencil;


#ifdef SUBSEQUENT_PASS
layout (input_attachment_index = 0, set = 3, binding = 0) uniform subpassInput inputDepth;
layout (input_attachment_index = 1, set = 3, binding = 1) uniform usubpassInput inputStencil;
#endif

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

layout(push_constant) uniform PushConstant {
	mat4 model;
	vec4 debugColor;
	int layerStartIndex;
	int nextLayerStartIndex;
	int portalIndex;
	int maxVisiblePortalCount;
	int currentPortalCount;
} pc;

void main() 
{
	
	int cameraIndexAndStencilCompare = inInstanceIndex + pc.layerStartIndex;

#ifdef SUBSEQUENT_PASS
	if(cameraIndexAndStencilCompare != subpassLoad(inputStencil).r)
	{
		discard;
	}

	float renderedDepthValueAndFacing = subpassLoad(inputDepth).r;
	bool isRenderedDepthFrontFacing = renderedDepthValueAndFacing >= 0.0;
	float renderedDepthValue = abs(renderedDepthValueAndFacing);
	bool isSameFacing = gl_FrontFacing == isRenderedDepthFrontFacing;

	// if the facing is the same add a little bit bias to easily discard almost equal values
	// this way we can discard fragments of the partner portal, which would fight for depth with original portal
	if(isSameFacing)
	{
		renderedDepthValue *= 0.9999;
	}

	if(gl_FragCoord.z >= renderedDepthValue)
	{
		discard;
	}

#endif

	bool isLastPortalPass = pc.maxVisiblePortalCount == 0;

	if(isLastPortalPass)
	{
		outRenderedStencil = 0;
		outColor = gl_FrontFacing ? vec4(0.75) : vec4(0.25);

		// don't touch helper index and camera indices as we might be out of range
		// TODO: this if is know at compile time actually
	}
	else
	{
		int firstHelperIndex = cameraIndexAndStencilCompare * pc.currentPortalCount;
		int helperIndex = firstHelperIndex + pc.portalIndex;

		int currentViewMatIndex = cameraIndexAndStencilCompare == 0 ? 0 :  ci.cIndices[cameraIndexAndStencilCompare];

		int firstPortalCameraIndex = currentViewMatIndex * pc.currentPortalCount + 1;
		int currentPortalCameraIndex = firstPortalCameraIndex + pc.portalIndex;

		int firstCameraIndicesIndexAndStencilWrite = pc.nextLayerStartIndex + (inInstanceIndex * pc.maxVisiblePortalCount);
		// count previous visible portals
		// we can use a fixed iteration count here, as the other portals won't have be processed / written and will always be zero
		int previousVisiblePortals = 0;
		for(int i = firstHelperIndex; i < helperIndex; ++i)
		{
			previousVisiblePortals+= (pih.indices[i]) <= 0 ? 0 : 1;
		}

		if (previousVisiblePortals < pc.maxVisiblePortalCount)
		{
			// mark that we are a visible portal
			atomicExchange(pih.indices[helperIndex], previousVisiblePortals + 1);
			// write our camera index into camera index buffer
			atomicExchange(ci.cIndices[firstCameraIndicesIndexAndStencilWrite + previousVisiblePortals],  currentPortalCameraIndex);
			outRenderedStencil = firstCameraIndicesIndexAndStencilWrite + previousVisiblePortals;
			outColor = vec4(vec3(0.0),1.f);// pc.debugColor;
		}
		else
		{
			atomicExchange(pih.indices[helperIndex],-1);
			outRenderedStencil = 0;
			outColor = vec4(1.f);
		}
	}

	if(gl_FrontFacing)
	{
		outRenderedDepth = gl_FragCoord.z;
	}
	else
	{
		outRenderedDepth = -gl_FragCoord.z;
	}
}