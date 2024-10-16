#version 450

layout (location = 0) in vec2 v2fTexCoord;

layout (set = 0, binding = 0) uniform sampler2D uScreen;

layout (location = 0) out vec4 oColor;

void main()
{
	// Output the image as the colour
	oColor = vec4(texture(uScreen, v2fTexCoord).rgb, 1);
}
