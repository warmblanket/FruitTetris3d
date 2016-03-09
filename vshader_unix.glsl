#version 130

in vec4 v_position;
in vec4 v_color;
out vec4 color;

uniform mat4 MVP;

void main() 
{
	gl_Position = MVP * v_position;

	color = v_color;	
} 
