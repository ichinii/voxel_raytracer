#version 450 core

in vec2 uv;

out vec4 color;

uniform ivec2 viewport_size;
uniform ivec2 tex_size;
uniform sampler2D tex;

void main()
{
	color = vec4(texture(tex, (vec2(gl_FragCoord.xy) / viewport_size) * (vec2(viewport_size) / tex_size)).rgb, 1);
}
