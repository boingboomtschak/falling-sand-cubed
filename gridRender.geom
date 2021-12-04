#version 330 core

const int GRID_SIZE = 64;

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

uniform mat4 persp;
uniform mat4 modelview;

out vec3 gPt;
out vec3 gNrm;
out vec3 gColor;

void main() {
	
}