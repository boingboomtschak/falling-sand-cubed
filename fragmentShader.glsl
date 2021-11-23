#version 130

out vec4 pColor;

uniform vec4 color = vec4(1, 0, 0, 1);

void main() {
	pColor = color;
}