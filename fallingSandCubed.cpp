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
vec3 lightPos = vec3(1, 1, 0);
dCube cube;

//float cube_points[][3] = { {1, 1, 1}, {-1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {1, -1, 1}, {-1, -1, 1}, {-1, -1, -1}, {1, -1, -1} };
//int cube_triangle_strip[] = { 3, 2, 6, 7, 4, 2, 0, 3, 1, 6, 5, 4, 1, 0 };
float cube_points[][3] = {
	{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, // front (0, 1, 2, 3)
	{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, // back (4, 5, 6, 7)
	{-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}, // left (8, 9, 10, 11)
	{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, // right (12, 13, 14, 15)
	{-1 , 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, // top  (16, 17, 18, 19)
	{-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}  // bottom (20, 21, 22, 23)
};
float cube_normals[][3] = {
	{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, // front
	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, // back
	{-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, // left
	{1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, // right
	{0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, // top
	{0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}  // bottom
};
int cube_triangles[][3] = {
	{0, 1, 2}, {2, 3, 0}, // front
	{4, 5, 6}, {6, 7, 4}, // back
	{8, 9, 10}, {10, 11, 8}, // left
	{12, 13, 14}, {14, 15, 12}, // right
	{16, 17, 18}, {18, 19, 16}, // top
	{20, 21, 22}, {22, 23, 20}  // bottom
};

const int GRID_SIZE = 32;
const int COL_SIZE = 32;

enum ParticleType {
	AIR = 0,
	STONE = 1,
	WATER = 2,
	SAND = 3,
	OIL = 4,
	SALT = 5,
};

struct ParticleGrid {
	GLuint grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
	ParticleGrid() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					grid[i][j][k] = AIR;
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
					printf("%i", grid[i][j][k]);
				}
				printf("\n");
			}
			printf("\n\n\n");
		}
	}
	void clear() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					grid[i][j][k] = AIR;
		writeGrid();
	}
	void compute() {
		// Dispatch compute shader
		glUseProgram(computeProgram);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		glDispatchCompute((GLuint)(GRID_SIZE / COL_SIZE), 1, (GLuint)(GRID_SIZE / COL_SIZE));
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
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
	VertexAttribPointer(renderProgram, "normal", 3, 0, (void*)(sizeof(cube_points)));
	SetUniform(renderProgram, "persp", camera.persp);
	SetUniform(renderProgram, "light_pos", lightPos);
	for (int i = 0; i < GRID_SIZE; i++) {
		for (int j = 0; j < GRID_SIZE; j++) {
			for (int k = 0; k < GRID_SIZE; k++) {
				if (grid.grid[i][j][k] != AIR) {
					mat4 scale = Scale((float)(1.0f / GRID_SIZE));
					mat4 trans = Translate((i - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (j - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (k - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE));
					SetUniform(renderProgram, "modelview", camera.modelview * trans * scale);
					switch (grid.grid[i][j][k]) {
					case AIR:
						SetUniform(renderProgram, "color", vec4(0.0f, 0.0f, 0.0f, 0.05f));
						break;
					case STONE:
						SetUniform(renderProgram, "color", vec4(0.1f, 0.1f, 0.1f, 1.0f));
						break;
					case WATER:
						SetUniform(renderProgram, "color", vec4(0.1f, 0.1f, 0.7f, 0.5f));
						break;
					case SAND:
						SetUniform(renderProgram, "color", vec4(0.906f, 0.702f, 0.498f, 1.0f));
						break;
					}
					glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
				}
			}
		}
	}
}

void WriteSphere(vec3 center, int radius, ParticleType pType) {
	grid.readGrid();
	int xmin = center.x - radius; xmin = xmin > 0 ? xmin : 0;
	int xmax = center.x + radius; xmax = xmax < GRID_SIZE ? xmax : GRID_SIZE - 1;
	int ymin = center.y - radius; ymin = ymin > 0 ? ymin : 0;
	int ymax = center.y + radius; ymax = ymax < GRID_SIZE ? ymax : GRID_SIZE - 1;
	int zmin = center.z - radius; zmin = zmin > 0 ? zmin : 0;
	int zmax = center.z + radius; zmax = zmax < GRID_SIZE ? ymax : GRID_SIZE - 1;
	for (int i = xmin; i < xmax; i++) {
		for (int j = ymin; j < ymax; j++) {
			for (int k = zmin; k < zmax; k++) {
				vec3 p = vec3(i, j, k);
				float p_dist = dist(center, p);
				if (p_dist <= radius) {
					grid.grid[i][j][k] = pType;
				}
			}
		}
	}
	grid.writeGrid();
}

// Overriding default camera control callback
void S_Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	} else if (key == GLFW_KEY_S) {
		WriteSphere(vec3(16, 28, 16), 3, SAND);
	} else if (key == GLFW_KEY_D) {
		WriteSphere(vec3(16, 28, 16), 3, WATER);
	} else if (key == GLFW_KEY_R) {
		grid.clear();
	}
}

void LoadBuffers() {
	glGenBuffers(1, &tempCubeBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, tempCubeBuffer);
	size_t size = sizeof(cube_points) + sizeof(cube_normals);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_points), cube_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(cube_points), sizeof(cube_normals), cube_normals);
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
	LoadBuffers();
	glfwWindowHint(GLFW_SAMPLES, 4);
	InitializeCallbacks(window);
	glfwSetKeyCallback(window, S_Keyboard);
	glfwSwapInterval(1);
	int frame = 0;
	while (!glfwWindowShouldClose(window)) {
		grid.compute();
		Display();
		glfwPollEvents();
		glfwSwapBuffers(window);
		frame++;
	}
	UnloadBuffers();
	glfwDestroyWindow(window);
	glfwTerminate();
}