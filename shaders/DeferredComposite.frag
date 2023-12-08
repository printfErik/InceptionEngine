#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput gBufferA;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput gBufferB;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput gBufferC;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 gbufferA = subpassLoad(gBufferA);
	vec4 gbufferB = subpassLoad(gBufferB);
	vec4 gbufferC = subpassLoad(gBufferC);

    
}