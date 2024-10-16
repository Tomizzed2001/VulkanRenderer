#version 460

layout (location = 0) out vec2 v2fTexCoord;

void main()
{
	// Create a full screen quad using 2 triangles
	v2fTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(v2fTexCoord * 2.0f + -1.0f, 0.0f, 1.0f);
}
