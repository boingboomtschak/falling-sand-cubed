#version 130

out vec4 pColor;

uniform vec4 stencilColor = vec4(1.0, 1.0, 1.0, 1.0);

void main() {
	pColor = stencilColor;
}