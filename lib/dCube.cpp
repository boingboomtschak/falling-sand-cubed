// dCube.cpp : Implementation for dCube.h

#include <glad.h>
#include <stdio.h>
#include <vector>
#include "Camera.h"
#include "dCube.h"
#include "VecMat.h"
#include "GLXtras.h"
#include "Misc.h"

namespace {
	GLuint cubeShader = 0;

	const char* cubeVertShader = R"(
		#version 130
		in vec3 point;
		uniform mat4 modelview;
		uniform mat4 persp;
		void main() {
			gl_Position = persp * modelview * vec4(point, 1);
		}
	)";

	const char* cubeFragShader = R"(
		#version 130
		out vec4 pColor;
		void main() {
			pColor = vec4(1);
		}
	)";
}

void dCube::loadBuffer() {
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	size_t sizePts = points.size() * sizeof(vec3);
	glBufferData(GL_ARRAY_BUFFER, sizePts, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizePts, &points[0]);
}

void dCube::unloadBuffer() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void dCube::display(Camera camera, mat4 *m) {
	if (vBuffer == 0) {
		fprintf(stderr, "dCube: Cube buffer not loaded into memory\n");
		return;
	}
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	int shader = useCubeShader();
	VertexAttribPointer(shader, "point", 3, 0, (void*)0);
	SetUniform(shader, "persp", camera.persp);
	mat4 modelview = camera.modelview;
	if (m != NULL) modelview = modelview * *m;
	modelview = modelview * transform;
	SetUniform(shader, "modelview", modelview);
	for (size_t i = 0; i < 6; i++) {
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, &faces[i]);
	}
}

GLuint dCube::getCubeShader() {
	if (!cubeShader)
		cubeShader = LinkProgramViaCode(&cubeVertShader, &cubeFragShader);
	return cubeShader;
}

GLuint dCube::useCubeShader() {
	GLuint s = getCubeShader();
	GLint prog = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
	if (prog != s) {
		glUseProgram(s);
	}
	return s;
}
