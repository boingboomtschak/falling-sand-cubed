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
//#include "CameraControls.h"
//#include "GeomUtils.h"
//#include "dCube.h"

using std::vector;

GLuint computeProgram = 0, computeBuffer = 0;
GLuint renderProgram = 0;
GLuint tempCubeBuffer = 0;
GLuint outerCubeProgram = 0, outerCubeBuffer = 0;

int win_width = 800, win_height = 800;

Camera camera((float)win_width / win_height, vec3(0, 0, 0), vec3(0, 0, -5));
vec3 lightPos = vec3(1, 1, 0);

float rand_float(float min = 0, float max = 1) { return min + (float)rand() / (RAND_MAX / (max - min)); }
float dist(vec3 p1, vec3 p2) { return (float)sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2) + pow(p2.z - p1.z, 2)); }

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
vector<vec3> outer_cube_points = { {-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1} };
vector<int4> outer_cube_faces = { {0, 1, 2, 3}, {2, 3, 4, 5}, {4, 5, 6, 7}, {6, 7, 0, 1}, {0, 3, 4, 7}, {1, 2, 5, 6} };

const int GRID_SIZE = 32;
const int COL_SIZE = 4;

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
	GLuint srcTex = 0, dstTex = 0;
	ParticleGrid() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					grid[i][j][k] = AIR;
	}
	void writeGrid() {
		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		GLvoid* buf = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		memcpy(buf, &grid, sizeof(grid));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		*/
		printf("WRITE 1\n");
		PrintGLErrors();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, srcTex);
		printf("WRITE 2\n");
		PrintGLErrors();
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &grid);
		printf("WRITE 3\n");
		PrintGLErrors();
	}
	void readGrid() {
		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(grid), &grid);
		*/
		printf("READ 1\n");
		PrintGLErrors();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, srcTex);
		printf("READ 2\n");
		PrintGLErrors();
		glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_UNSIGNED_INT, &grid);
		printf("READ 3\n");
		PrintGLErrors();
	}
	void loadBuffer() {
		/*
		glGenBuffers(1, &computeBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(grid), NULL, GL_DYNAMIC_COPY);
		writeGrid();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffer); 
		*/
		glEnable(GL_TEXTURE_3D);
		glGenTextures(1, &srcTex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, srcTex);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, dstTex);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	}
	void unloadBuffer() {
		/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDeleteBuffers(1, &computeBuffer); 
		*/
		glActiveTexture(GL_TEXTURE0);
		glBindBuffer(GL_TEXTURE_3D, 0);
		glDeleteTextures(1, &srcTex);
		glActiveTexture(GL_TEXTURE1);
		glBindBuffer(GL_TEXTURE_3D, 0);
		glDeleteTextures(1, &dstTex);
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
	void compute() {
		// Dispatch compute shader
		glUseProgram(computeProgram);
		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, srcTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, dstTex);
		//glBindImageTexture(0, tex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RED);
		glDispatchCompute(GRID_SIZE, GRID_SIZE, GRID_SIZE);
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// Copy dstTex to srcTex
		glCopyImageSubData(dstTex, 1, 0, 0, 0, 0, srcTex, 0, 0, 0, 0, 0, GRID_SIZE, GRID_SIZE, GRID_SIZE);
	}
	void render() {
		// Send necessary data to tess shaders

	}
	
};

ParticleGrid grid;

const char* computeShader = R"(
	#version 430

	const int GRID_SIZE = 32;
	const int COL_SIZE = 4;
	const int SHUFFLE_ITERS = 4;

	layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
	/*
	layout(std430, binding=0) buffer particle_data {
		uint grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
	};
	*/
	layout (r32ui, binding=0) uniform uimage3D srcGrid;
	layout (r32ui, binding=1) uniform uimage3D dstGrid;

	// Particle types
	const int P_AIR = 0;
	const int P_STONE = 1;
	const int P_WATER = 2;
	const int P_SAND = 3;
	const int P_OIL = 4;
	const int P_SALT = 5;

	uniform int solidFloor = 1;

	// Pseudorandom values for more natural processing
	uint rand(vec2 co, uint rmin = 0, uint rmax = 1){
		return uint(round( fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453) * rmax )) + rmin;
	}

	void shuffleRow(inout uint row[COL_SIZE]) {
		for (uint i = 0; i < COL_SIZE; i++) row[i] = i;
		for (uint s = 0; s < SHUFFLE_ITERS; s++) {
			uint a = rand(vec2(float(row[0]), float(row[1])), 0, COL_SIZE);
			uint b = rand(vec2(float(row[2]), float(row[2])), 0, COL_SIZE);
			uint c = row[a];
			row[a] = row[b];
			row[b] = c;
		}
	}

	bool isLiquid(uint p) {
		switch (p) {
			case P_WATER:
				return true;
			case P_OIL:
				return true;
		}
		return false;
	}

	bool isMovableSolid(uint p) {
		switch(p) {
			case P_SAND:
				return true;
			case P_SALT:
				return true;
		}
		return false;
	}

	bool isImmovableSolid(uint p) {
		switch(p) {
			case P_STONE:
				return true;
		}
		return false;
	}

	bool isGas(uint p) {
		return false;
	}

	void processLiquids(uint x, uint y, uint z) {
		if (y > 0) {
		
		}
	}

	void processMovableSolids(uint x, uint y, uint z) {
		uint p = imageLoad(srcGrid, ivec3(x, y, z)).r;
		if (y > 0) {
			// Try to move down
			uint np = imageLoad(srcGrid, ivec3(x, y-1, z)).r;
			if (np == P_AIR || isLiquid(np) || isGas(np)) {
				uint wp = imageAtomicCompSwap(dstGrid, ivec3(x, y-1, z), np, p);
				if (wp == np) {
					imageAtomicExchange(srcGrid, ivec3(x, y, z), P_AIR);
					return;
				}
			}
			// Else, move diagonal
			for (uint nx = -1; nx <= 1; nx++) {
				for (uint nz = -1; nz <= 1; nz++) {
					if ((nx != 0 || nz != 0) && x + nx < GRID_SIZE && x + nx > 0 && z + nz < GRID_SIZE && z + nz > 0) {
						uint np = imageLoad(srcGrid, ivec3(x + nx, y - 1, z + nz)).r;
						if (np == P_AIR || isLiquid(np) || isGas(np)) {
							uint wp = imageAtomicCompSwap(dstGrid, ivec3(x + nx, y-1, z + nz), np, p);
							if (wp == np) {
								imageAtomicExchange(srcGrid, ivec3(x, y, z), np);
								return;
							}
						}
					}
				}
			}
		}
		imageAtomicExchange(dstGrid, ivec3(x, y, z), p);
	}

	void processImmovableSolids(uint x, uint y, uint z) {

	}

	void processGases(uint x, uint y, uint z) {
	
	}

	void main() {
		ivec3 inv = ivec3(gl_GlobalInvocationID);
		uint p = imageLoad(srcGrid, ivec3(inv.x, inv.y, inv.z)).r;
		if (isLiquid(p)) {
			processLiquids(inv.x, inv.y, inv.z);
		} else if (isMovableSolid(p)) {
			processMovableSolids(inv.x, inv.y, inv.z);
		} else if(isImmovableSolid(p)) {
			processImmovableSolids(inv.x, inv.y, inv.z);
		} else if (isGas(p)) {
			processGases(inv.x, inv.y, inv.z);
		}
	}
)";

const char* vertexShader = R"(
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
)";

const char* fragmentShader = R"(
	#version 130

	in vec3 vPoint;
	in vec3 vNormal;

	out vec4 pColor;

	uniform vec4 color = vec4(1, 0, 0, 1);
	uniform vec4 light_color = vec4(1, 1, 1, 1);
	uniform vec3 light_pos = vec3(-.2, .1, -3);
	uniform float amb = 0.5; // ambient intensity
	uniform float dif = 0.4; // diffusivity
	uniform float spc = 0.7; // specularity

	void main() {
		vec3 N = normalize(vNormal);       // surface normal
		vec3 L = normalize(light_pos-vPoint);  // light vector
		vec3 E = normalize(vPoint);        // eye vertex
		vec3 R = reflect(L, N);            // highlight vector
		float d = dif*max(0, dot(N, L)); // one-sided Lambert
		float h = max(0, dot(R, E)); // highlight term
		float s = spc*pow(h, 100); // specular term
		float intensity = clamp(amb+d+s, 0, 1);
		pColor = vec4(intensity*light_color.rgb, 1) * color;
	}
)";

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

void CompileShaders() {
	computeProgram = LinkProgramViaCode(&computeShader);
	if (!computeProgram) {
		fprintf(stderr, "SHADER: Error linking compute shader! Exiting...\n");
		exit(1);
	}
	renderProgram = LinkProgramViaCode(&vertexShader, &fragmentShader);
	if (!renderProgram) {
		fprintf(stderr, "SHADER: Error linking render shader! Exiting...\n");
		exit(1);
	}
	outerCubeProgram = LinkProgramViaCode(&cubeVertShader, &cubeFragShader);
	if (!renderProgram) {
		fprintf(stderr, "SHADER: Error linking outer cube shader! Exiting...\n");
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
						SetUniform(renderProgram, "color", vec4(0.1f, 0.1f, 0.7f, 1.0f));
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

void Resize(GLFWwindow* window, int width, int height) {
	camera.Resize(win_width = width, win_height = height);
	glViewport(0, 0, win_width, win_height);
}

void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

bool Shift(GLFWwindow* w) {
	return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
		glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	y = win_height - y;
	if (action == GLFW_PRESS)
		camera.MouseDown((int)x, (int)y);
	if (action == GLFW_RELEASE)
		camera.MouseUp();
}

void MouseMove(GLFWwindow* w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
		y = win_height - y;
		camera.MouseDrag((int)x, (int)y, Shift(w));
	}
}

void MouseWheel(GLFWwindow* w, double ignore, double spin) {
	camera.MouseWheel(spin > 0, Shift(w));
}

void LoadBuffers() {
	glGenBuffers(1, &tempCubeBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, tempCubeBuffer);
	size_t size = sizeof(cube_points) + sizeof(cube_normals);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_points), cube_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(cube_points), sizeof(cube_normals), cube_normals);
	// Generate compute shader buffer
	grid.loadBuffer();
	// Loading outer cube buffer
	glGenBuffers(1, &outerCubeBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, outerCubeBuffer);
	size_t sizePts = outer_cube_points.size() * sizeof(vec3);
	glBufferData(GL_ARRAY_BUFFER, sizePts, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizePts, &outer_cube_points[0]);
}

void UnloadBuffers() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &tempCubeBuffer);
	glDeleteBuffers(1, &outerCubeBuffer);
	grid.unloadBuffer();
}

void Display() {
	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Draw outer cube
	glUseProgram(outerCubeProgram);
	glBindBuffer(GL_ARRAY_BUFFER, outerCubeBuffer);
	VertexAttribPointer(outerCubeProgram, "point", 3, 0, (void*)0);
	SetUniform(outerCubeProgram, "persp", camera.persp);
	SetUniform(outerCubeProgram, "modelview", camera.modelview);
	for (size_t i = 0; i < 6; i++) {
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, &outer_cube_faces[i]);
	}
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
	glfwSetCursorPosCallback(window, MouseMove);
	glfwSetMouseButtonCallback(window, MouseButton);
	glfwSetScrollCallback(window, MouseWheel);
	glfwSetKeyCallback(window, Keyboard);
	glfwSetWindowSizeCallback(window, Resize);
	glfwSwapInterval(1);
	int frame = 0;
	int x = 0;
	int z = 0;
	while (!glfwWindowShouldClose(window)) {
		grid.compute();
		grid.readGrid();
		if (frame % 4 == 0) {
			//int x = (int)round(rand_float() * GRID_SIZE); // d
			//int z = (int)round(rand_float() * GRID_SIZE); // d
			grid.grid[x][31][z] = SAND; // d
			grid.writeGrid();
			x++;
			if (x >= 32) {
				x = 0;
				z++;
				if (z >= 32) {
					z = 0;
				}
			}
		}
		Display();
		glfwPollEvents();
		glfwSwapBuffers(window);
		frame++;
	}
	UnloadBuffers();
	glfwDestroyWindow(window);
	glfwTerminate();
}