#version 330

in vec3 Position;

out vec2 UV;

void main()
{
	gl_Position = vec4(Position,1.0);
	UV = (Position.xy + 1.0) / 2.0;
}
