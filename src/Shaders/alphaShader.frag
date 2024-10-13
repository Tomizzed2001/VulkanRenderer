#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec3 inPosition;
layout (location = 4) flat in int inMatID;

layout(set = 0, binding = 0) uniform worldView
{
	mat4 projectionCameraMatrix;
	vec3 cameraPosition;
} view;

layout (set = 1, binding = 0) uniform sampler2D textureColour[];

layout (location = 0) out vec4 outColor;

void main()
{
	if (texture(textureColour[inMatID], vec2(inTexCoord.x, inTexCoord.y)).a < 0.5){
		discard;
	}
	outColor = vec4(texture(textureColour[inMatID], vec2(inTexCoord.x, inTexCoord.y)).rgb, 1.f);
}
