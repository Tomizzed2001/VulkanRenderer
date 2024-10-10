#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in int iMaterialID;


layout(set = 0, binding = 0) uniform UScene
{
	mat4 camera;
	mat4 projection;
	mat4 projCam;
} uScene;

// v2f means vertex to fragment
layout(location = 0) out vec2 v2fTexCoord; 
layout(location = 1) out int matID;

void main()
{
	v2fTexCoord = iTexCoord;
	matID = iMaterialID;

	gl_Position = uScene.projCam * vec4(iPosition, 1.f);
}
