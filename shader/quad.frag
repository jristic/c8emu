#version 330

// Set once
uniform sampler2D inputTex;

in vec2 UV;

void main()
{
	gl_FragColor = texture2D(inputTex, UV);
}