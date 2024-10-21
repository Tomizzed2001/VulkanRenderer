#version 450

#define NUMLIGHTS 1

layout(location = 0) in vec3 iPosition;

// The lighting uniform
layout(set = 0, binding = 0, std140) uniform LightBuffer { 
	mat4 lightDirection;
    vec3 lightPosition;
	vec3 lightColour;
} light;

void main()
{
	gl_Position = light.lightDirection * vec4(iPosition, 1.f);
}
