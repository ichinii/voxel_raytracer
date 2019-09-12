#version 450 core

layout (location = 0) in vec2 vertex_position;
layout (location = 1) in vec2 vertex_uv;

out vec2 uv;

void main()
{
	gl_Position = vec4(vertex_position, 0, 1);
	uv = vertex_uv;
}
