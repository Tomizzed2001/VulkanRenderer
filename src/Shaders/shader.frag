#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 v2fTexCoord;
layout (location = 1) flat in int matID;

layout (set = 1, binding = 0) uniform sampler2DArray uTexColor;
layout (set = 2, binding = 0) uniform sampler2D uTex[];

layout (location = 0) out vec4 oColor;

void main()
{
	oColor = vec4(texture(uTex[matID], vec2(v2fTexCoord.x, v2fTexCoord.y)).rgb, 1.f);
	//oColor = vec4(texture(uTexColor, vec3(v2fTexCoord.x, v2fTexCoord.y, matID)).rgb, 1.f);
	//oColor = vec4(texture(uTexColor, vec2(v2fTexCoord.x, v2fTexCoord.y)).rgb, 1.f);
	//oColor = vec4(v2fTexCoord.x, v2fTexCoord.y, 0, 1);
}
