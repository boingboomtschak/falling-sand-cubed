// fallingSandCubed - Devon McKee, Michael Pablo

#define _USE_MATH_DEFINES

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include <vector>
#include "VecMat.h"
#include "Camera.h"
#include "Misc.h"
#include "GLXtras.h"
#include "CameraControls.h"
#include "GeomUtils.h"
#include "dCube.h"

using std::vector;

GLuint computeProgram = 0, computeBuffer = 0;
GLuint renderProgram = 0;
GLuint tempCubeBuffer = 0;

int win_width = 800, win_height = 800;

Camera camera((float)win_width / win_height, vec3(0, 0, 0), vec3(0, 0, -5));
vec3 lightSource = vec3(1, 1, 0);
dCube cube;

float cube_points[][3] = {
	{1, 1, 1}, {-1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, 
	{1, -1, 1}, {-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}
};
int cube_triangle_strip[] = {
	3, 2, 6, 7, 4, 2, 0,
	3, 1, 6, 5, 4, 1, 0
};

const int GRID_SIZE = 32;

enum ParticleType {
	AIR = 0,
	WALL = 1,
	WATER = 2,
	SAND = 3,
	OIL = 4,
	SALT = 5,
};

struct ParticleGrid {
	int grid[GRID_SIZE][GRID_SIZE][GRID_SIZE][2];
	ParticleGrid() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					for (int l = 0; l < 2; l++)
						grid[i][j][k][l] = AIR;
	}
	void writeGrid() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		GLvoid* buf = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		memcpy(buf, &grid, sizeof(grid));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
	void readGrid() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(grid), &grid);
	}
	void loadBuffer() {
		glGenBuffers(1, &computeBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(grid), NULL, GL_DYNAMIC_COPY);
		writeGrid();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffer);
	}
	void unloadBuffer() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDeleteBuffers(1, &computeBuffer);
	}
	void printGrid() {
		readGrid();
		for (int i = 0; i < GRID_SIZE; i++) {
			for (int j = 0; j < GRID_SIZE; j++) {
				for (int k = 0; k < GRID_SIZE; k++) {
					printf("[%i %i]", grid[i][j][k][0], grid[i][j][k][1]);
				}
				printf("\n");
			}
			printf("\n\n\n");
		}
	}
	void compute() {
		// Dispatch compute shader
		glUseProgram(computeProgram);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		glDispatchCompute((GLuint)GRID_SIZE, (GLuint)GRID_SIZE, (GLuint)GRID_SIZE);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	void render() {
		// Send necessary data to tess shaders

	}
	
};

ParticleGrid grid;

void CompileShaders() {
	computeProgram = LinkProgramViaFile("computeShader.glsl");
	if (!computeProgram) {
		fprintf(stderr, "SHADER: Error linking compute shader! Exiting...\n");
		exit(1);
	}
	renderProgram = LinkProgramViaFile("vertexShader.glsl", "fragmentShader.glsl");
	if (!renderProgram) {
		fprintf(stderr, "SHADER: Error linking render shader! Exiting...\n");
		exit(1);
	}
	/*
	GLuint vshader = CompileShaderViaFile("vertexShader.glsl", GL_VERTEX_SHADER);
	if (!vshader) {
		fprintf(stderr, "SHADER: Error compiling vertex shader! Exiting...\n");
		exit(1);
	}
	GLuint tcshader = CompileShaderViaFile("tessControlShader.glsl", GL_TESS_CONTROL_SHADER);
	if (!tcshader) {
		fprintf(stderr, "SHADER: Error compiling tessellation control shader! Exiting...\n");
		exit(1);
	}
	GLuint teshader = CompileShaderViaFile("tessEvalShader.glsl", GL_TESS_EVALUATION_SHADER);
	if (!teshader) {
		fprintf(stderr, "SHADER: Error compiling tessellation evaluation shader! Exiting...\n");
		exit(1);
	}
	GLuint fshader = CompileShaderViaFile("fragmentShader.glsl", GL_FRAGMENT_SHADER);
	if (!fshader) {
		fprintf(stderr, "SHADER: Error compiling fragment shader! Exiting...\n");
		exit(1);
	}
	renderProgram = LinkProgram(vshader, tcshader, teshader, 0, fshader);
	if (!renderProgram) {
		fprintf(stderr, "SHADER: Error linking render program! Exiting...\n");
		exit(1);
	}
	*/
}

void cpuRenderGrid() {
	grid.readGrid();
	glUseProgram(renderProgram);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glBindBuffer(GL_ARRAY_BUFFER, tempCubeBuffer);
	VertexAttribPointer(renderProgram, "point", 3, 0, (void*)0);
	SetUniform(renderProgram, "persp", camera.persp);
	for (int i = 0; i < GRID_SIZE; i++) {
		for (int j = 0; j < GRID_SIZE; j++) {
			for (int k = 0; k < GRID_SIZE; k++) {
				if (grid.grid[i][j][k][0] != AIR) {
					mat4 scale = Scale((float)(1.0f / GRID_SIZE));
					mat4 trans = Translate((i - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (j - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (k - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE));
					SetUniform(renderProgram, "modelview", camera.modelview * trans * scale);
					switch (grid.grid[i][j][k][0]) {
					case AIR:
						SetUniform(renderProgram, "color", vec4(0.0f, 0.0f, 0.0f, 0.05f));
						break;
					case WALL:
						SetUniform(renderProgram, "color", vec4(0.1f, 0.1f, 0.1f, 1.0f));
						break;
					case WATER:
						SetUniform(renderProgram, "color", vec4(0.1f, 0.1f, 0.7f, 1.0f));
						break;
					}
					glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, cube_triangle_strip);
				}
			}
		}
	}
}

void LoadBuffers() {
	glGenBuffers(1, &tempCubeBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, tempCubeBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_points), cube_points, GL_STATIC_DRAW);
	// Load enclosing cube buffer into GPU
	cube.loadBuffer();
	// Generate compute shader buffer
	grid.loadBuffer();
}

void UnloadBuffers() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &tempCubeBuffer);
	cube.unloadBuffer();
	grid.unloadBuffer();
}

void Display() {
	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cube.display(camera);
	cpuRenderGrid();
	glFlush();
}

int main() {
	srand((int)time(NULL));
	if (!glfwInit())
		return 1;
	GLFWwindow* window = glfwCreateWindow(win_width, win_height, "Falling Sand: Cubed", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwSetWindowPos(window, 100, 100);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	PrintGLErrors();
	CompileShaders();
	grid.grid[4][4][4][0] = WATER; // debug
	LoadBuffers();
	glfwWindowHint(GLFW_SAMPLES, 4);
	InitializeCallbacks(window);
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window)) {
		grid.compute();
		Display();
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	UnloadBuffers();
	glfwDestroyWindow(window);
	glfwTerminate();
}