#version 430 core

const int GRID_SIZE = 64;

in vec3 point;

layout (std430, binding = 0) buffer grid_buffer {
	uint grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
};

uniform vec4 colors[];
uniform mat4 persp;
uniform mat4 modelview;

out vec3 vPoint;
out vec3 vNormal;
out vec4 color;

void main() {
	gl_Position = vec4(point, 1);
}