#version 450 core

in vec2 uv;

out vec4 color;

uniform ivec2 tex_size;
uniform sampler2D tex;

void main()
{
	color = vec4(texture(tex, gl_FragCoord.xy / tex_size).rgb, 1);
}
