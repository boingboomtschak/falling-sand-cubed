#version 130

in vec3 point;

uniform mat4 modelview;
uniform mat4 persp;

void main() {
	gl_Position = persp * modelview * vec4(point, 1);
}