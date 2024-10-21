#version 450

// Bring in the vertex buffer values
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in int inMaterialID;

// The world view uniform
layout(set = 0, binding = 0) uniform worldView
{
	mat4 projectionCameraMatrix;
	vec3 cameraPosition;
} view;

layout(set = 2, binding = 0, std140) uniform LightBuffer { 
    mat4 lightDirection;
    vec3 lightPosition;
	vec3 lightColour;
} light;

const mat4 bias = mat4( 
  0.5, 0.0, 0.0, 0.0,
  0.0, 0.5, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.5, 0.5, 0.0, 1.0 );

// The values to output to the fragment shader
layout (location = 0) out vec2 outTexCoord; 
layout (location = 1) out vec3 outPosition;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec4 outTangent;
layout (location = 4) out int outMatID;
layout (location = 5) out vec4 outLightDir;

void main()
{
	// Set the output values to go to the fragment shader
	outTexCoord = inTexCoord;
	outPosition = inPosition;
	outNormal = inNormal;
	outTangent = inTangent;
	outMatID = inMaterialID;

	outLightDir = bias * light.lightDirection * vec4(inPosition, 1.f);

	// The position of the vertex as shown to screen
	gl_Position = view.projectionCameraMatrix * vec4(inPosition, 1.f);
}
