#version 130

in vec3 point;
in vec3 normal;

out vec3 vPoint;
out vec3 vNormal;

uniform mat4 modelview;
uniform mat4 persp;

void main() {
	vPoint = (modelview * vec4(point, 1)).xyz;
	vNormal = (modelview * vec4(normal, 0)).xyz;
	gl_Position = persp * vec4(vPoint, 1);
}