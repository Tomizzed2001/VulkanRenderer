#version 450
#extension GL_EXT_nonuniform_qualifier : require

// Bring in the values from the vertex shader
layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec3 inPosition;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) flat in int inMatID;

// The world view uniform
layout(set = 0, binding = 0) uniform worldView
{
	mat4 projectionCameraMatrix;
	vec3 cameraPosition;
} view;

// The texture uniforms (Indexed by material ID)
layout (set = 1, binding = 0) uniform sampler2D textureColour[];

// The screen colour output
layout (location = 0) out vec4 outColor;

void main()
{
	// Calculate the view direction and normalize
	vec3 viewDirection = normalize(view.cameraPosition - inPosition);
	
	// This is the normal
	vec3 normal = normalize(inNormal).rgb;

	// Output the colour
	outColor = vec4(texture(textureColour[inMatID], vec2(inTexCoord.x, inTexCoord.y)).rgb, 1.f);
}
