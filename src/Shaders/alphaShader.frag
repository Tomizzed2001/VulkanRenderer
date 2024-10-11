#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 v2fTexCoord;
layout (location = 1) flat in int matID;

layout (set = 1, binding = 0) uniform sampler2D uTextureColour[];

layout (location = 0) out vec4 oColor;

void main()
{
	if (texture(uTextureColour[matID], vec2(v2fTexCoord.x, v2fTexCoord.y)).a < 0.5){
		discard;
	}
	oColor = vec4(texture(uTextureColour[matID], vec2(v2fTexCoord.x, v2fTexCoord.y)).rgb, 1.f);
}
