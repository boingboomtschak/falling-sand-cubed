// fallingSandCubed - Devon McKee, Michael Pablo

#define _USE_MATH_DEFINES

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include <vector>
#include <string>
#include "VecMat.h"
#include "Camera.h"
#include "Misc.h"
#include "GLXtras.h"
#include "CameraControls.h"
#include "GeomUtils.h"
#include "dCube.h"
#include "fallingSandGui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

using std::vector;
using std::string;

GLuint computeProgram = 0, renderProgram = 0, gridRenderProgram = 0, stencilProgram = 0;
GLuint srcBuffer = 0, dstBuffer = 0, voxelBuffer = 0, particleBuffer = 0;

const int GRID_SIZE = 64;
const int MAX_PARTICLES = GRID_SIZE * GRID_SIZE * GRID_SIZE;
const char* render_glsl_version = "#version 130";

int win_width = 800, win_height = 800;
Camera camera((float)win_width / win_height, vec3(0, 0, 0), vec3(0, 0, -5));
GLFWwindow* window;
vec3 lightPos = vec3(1, 1, 0);
dCube cube;
vec3 brushPos = vec3((int)(GRID_SIZE / 2), (int)(GRID_SIZE - 3), (int)(GRID_SIZE / 2));
time_t start;
int renderer = 1; // 0 = CPU-managed, 1 = Instanced
int simulationFpsLimit = 60;
float ambient = 0.7, diffuse = 0.4, specular = 0.4;

// Colors
float bgColor[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
float brushColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float stencilColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float stoneColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
float waterColor[4] = { 0.1f, 0.1f, 0.7f, 0.5f };
float sandColor[4] = { 0.906f, 0.702f, 0.498f, 1.0f };
float oilColor[4] = { 0.133f, 0.067f, 0.223f, 0.5f };
float saltColor[4] = { 0.902f, 0.906f, 0.910f, 1.0f };
float steamColor[4] = { 0.714f, 0.976f, 1.0f, 0.5f };

// Window show toggles
static bool showAbout = false;
static bool showHelp = false;
static bool showElements = false;
static bool showOverlay = false;
static bool showGridStats = false;
static bool showDemo = false; // debug
static bool showMetrics = false; // debug
int overlayCorner = 1; // Defaults to top-right
static bool themeClassic = true, themeLight = false, themeDark = false;
static bool drawBrushHighlight = true;

float cube_points[][3] = { {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}, {1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {-1 , 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1} };
float cube_normals[][3] = { {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0} };
int cube_triangles[][3] = { {0, 1, 2}, {2, 3, 0}, {6, 5, 4}, {4, 7, 6}, {10, 9, 8}, {8, 11, 10}, {12, 13, 14}, {14, 15, 12}, {16, 17, 18}, {18, 19, 16}, {22, 21, 20}, {20, 23, 22} };

enum ParticleType {
	AIR = 0,
	STONE = 1,
	WATER = 2,
	SAND = 3,
	OIL = 4,
	SALT = 5,
	STEAM = 6,
};

int brushElement = SAND;
int brushRadius = 3;
int brushShape = 0; // 0 = Circle, 1 = Cube
string brushElementString = "SAND";
string brushShapeString = "SPHERE";

struct Particle {
	GLint x, y, z, t; // x, y, z, type
};

struct ParticleGrid {
	Particle particles[MAX_PARTICLES];
	int num_particles = 0, stone = 0, water = 0, sand = 0, oil = 0, salt = 0, steam = 0;
	GLuint grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
	ParticleGrid() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					grid[i][j][k] = AIR;
		for (size_t i = 0; i < MAX_PARTICLES; i++) {
			particles[i] = Particle();
		}
	}
	void writeGrid() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, srcBuffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(grid), &grid);
	}
	void readGrid() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, srcBuffer);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(grid), &grid);
	}
	void writeParticles() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Particle) * num_particles, &particles);
	}
	void loadBuffer() {
		// Create and write srcBuffer
		glGenBuffers(1, &srcBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, srcBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(grid), NULL, GL_DYNAMIC_COPY);
		writeGrid();
		// Create dstBuffer
		glGenBuffers(1, &dstBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, dstBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(grid), NULL, GL_DYNAMIC_COPY);
		// Create particleBuffer
		glGenBuffers(1, &particleBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(particles), NULL, GL_DYNAMIC_DRAW);
	}
	void unloadBuffer() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDeleteBuffers(1, &srcBuffer);
		glDeleteBuffers(1, &dstBuffer);
		glDeleteBuffers(1, &particleBuffer);
	}
	void clear() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					grid[i][j][k] = AIR;
		writeGrid();
	}
	void updateParticles() {
		num_particles = 0, stone = 0, water = 0, sand = 0, oil = 0, salt = 0, steam = 0;
		for (int i = 0; i < GRID_SIZE; i++) {
			for (int j = 0; j < GRID_SIZE; j++) {
				for (int k = 0; k < GRID_SIZE; k++) {
					if (grid[i][j][k] != AIR) {
						if (renderer == 1) {
							particles[num_particles].x = i;
							particles[num_particles].y = j;
							particles[num_particles].z = k;
							particles[num_particles].t = grid[i][j][k];
						}
						if (grid[i][j][k] == STONE) stone++;
						if (grid[i][j][k] == WATER) water++;
						if (grid[i][j][k] == SAND) sand++;
						if (grid[i][j][k] == OIL) oil++;
						if (grid[i][j][k] == SALT) salt++;
						if (grid[i][j][k] == STEAM) steam++;
						num_particles++;
					}
				}
			}
		}
		if (renderer == 1) writeParticles();
	}
	void WriteSphere(vec3 center, int radius, int pType) {
		int xmin = center.x - radius; xmin = xmin > 0 ? xmin : 0;
		int xmax = center.x + radius; xmax = xmax < GRID_SIZE ? xmax : GRID_SIZE - 1;
		int ymin = center.y - radius; ymin = ymin > 0 ? ymin : 0;
		int ymax = center.y + radius; ymax = ymax < GRID_SIZE ? ymax : GRID_SIZE - 1;
		int zmin = center.z - radius; zmin = zmin > 0 ? zmin : 0;
		int zmax = center.z + radius; zmax = zmax < GRID_SIZE ? zmax : GRID_SIZE - 1;
		for (int i = xmin; i < xmax; i++) {
			for (int j = ymin; j < ymax; j++) {
				for (int k = zmin; k < zmax; k++) {
					vec3 p = vec3(i, j, k);
					float p_dist = dist(center, p);
					if (p_dist <= radius) {
						grid[i][j][k] = pType;
					}
				}
			}
		}
	}
	void WriteCube(vec3 center, int radius, int pType) {
		int xmin = center.x - radius; xmin = xmin > 0 ? xmin : 0;
		int xmax = center.x + radius; xmax = xmax < GRID_SIZE ? xmax : GRID_SIZE - 1;
		int ymin = center.y - radius; ymin = ymin > 0 ? ymin : 0;
		int ymax = center.y + radius; ymax = ymax < GRID_SIZE ? ymax : GRID_SIZE - 1;
		int zmin = center.z - radius; zmin = zmin > 0 ? zmin : 0;
		int zmax = center.z + radius; zmax = zmax < GRID_SIZE ? zmax : GRID_SIZE - 1;
		for (int i = xmin; i < xmax; i++) {
			for (int j = ymin; j < ymax; j++) {
				for (int k = zmin; k < zmax; k++) {
					grid[i][j][k] = pType;
				}
			}
		}
	}
	void compute() {
		writeGrid();
		// Copy src to dst (dst will be modified by compute program)
		glBindBuffer(GL_COPY_READ_BUFFER, srcBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, dstBuffer);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(grid));
		// Dispatch compute shader
		glUseProgram(computeProgram);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, srcBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dstBuffer);
		SetUniform(computeProgram, "rngSeed", (int)(clock() - start));
		glDispatchCompute(GRID_SIZE, GRID_SIZE, GRID_SIZE);
		// Wait for writes to memory to finish
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// Copy dst to src
		glBindBuffer(GL_COPY_READ_BUFFER, dstBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, srcBuffer);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(grid));
		readGrid();
	}
	void cpuManagedRender() {
		updateParticles();
		glUseProgram(renderProgram);
		glStencilMask(0x00);
		glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
		VertexAttribPointer(renderProgram, "point", 3, 0, (void*)0);
		VertexAttribPointer(renderProgram, "normal", 3, 0, (void*)(sizeof(cube_points)));
		SetUniform(renderProgram, "persp", camera.persp);
		SetUniform(renderProgram, "light_pos", lightPos);
		SetUniform(renderProgram, "amb", ambient);
		SetUniform(renderProgram, "dif", diffuse);
		SetUniform(renderProgram, "spc", specular);
		for (int i = 0; i < GRID_SIZE; i++) {
			for (int j = 0; j < GRID_SIZE; j++) {
				for (int k = 0; k < GRID_SIZE; k++) {
					if (grid[i][j][k] != AIR) {
						mat4 scale = Scale((float)(1.0f / GRID_SIZE));
						mat4 trans = Translate((i - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (j - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (k - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE));
						SetUniform(renderProgram, "modelview", camera.modelview * trans * scale);
						switch (grid[i][j][k]) {
						case STONE:
							SetUniform(renderProgram, "color", vec4(stoneColor[0], stoneColor[1], stoneColor[2], stoneColor[3]));
							break;
						case WATER:
							SetUniform(renderProgram, "color", vec4(waterColor[0], waterColor[1], waterColor[2], waterColor[3]));
							break;
						case SAND:
							SetUniform(renderProgram, "color", vec4(sandColor[0], sandColor[1], sandColor[2], sandColor[3]));
							break;
						case OIL:
							SetUniform(renderProgram, "color", vec4(oilColor[0], oilColor[1], oilColor[2], oilColor[3]));
							break;
						case SALT:
							SetUniform(renderProgram, "color", vec4(saltColor[0], saltColor[1], saltColor[2], saltColor[3]));
							break;
						case STEAM:
							SetUniform(renderProgram, "color", vec4(steamColor[0], steamColor[1], steamColor[2], steamColor[3]));
							break;
						}
						glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
					}
				}
			}
		}
	}
	void instancedRender() {
		updateParticles();
		// Instanced rendering of grid with shader storage
		glUseProgram(gridRenderProgram);
		glStencilMask(0x00);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer);
		SetUniform(gridRenderProgram, "grid_size", GRID_SIZE);
		SetUniform(gridRenderProgram, "stoneColor", vec4(stoneColor[0], stoneColor[1], stoneColor[2], stoneColor[3]));
		SetUniform(gridRenderProgram, "waterColor", vec4(waterColor[0], waterColor[1], waterColor[2], waterColor[3]));
		SetUniform(gridRenderProgram, "sandColor", vec4(sandColor[0], sandColor[1], sandColor[2], sandColor[3]));
		SetUniform(gridRenderProgram, "oilColor", vec4(oilColor[0], oilColor[1], oilColor[2], oilColor[3]));
		SetUniform(gridRenderProgram, "saltColor", vec4(saltColor[0], saltColor[1], saltColor[2], saltColor[3]));
		SetUniform(gridRenderProgram, "steamColor", vec4(steamColor[0], steamColor[1], steamColor[2], steamColor[3]));
		SetUniform(gridRenderProgram, "persp", camera.persp);
		SetUniform(gridRenderProgram, "modelview", camera.modelview);
		SetUniform(gridRenderProgram, "light_pos", lightPos);
		SetUniform(gridRenderProgram, "amb", ambient);
		SetUniform(gridRenderProgram, "dif", diffuse);
		SetUniform(gridRenderProgram, "spc", specular);
		glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
		VertexAttribPointer(renderProgram, "point", 3, 0, (void*)0);
		VertexAttribPointer(renderProgram, "normal", 3, 0, (void*)(sizeof(cube_points)));
		glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles, num_particles);
	}
	
};

ParticleGrid grid;

void CompileShaders() {
	computeProgram = LinkProgramViaFile("shaders/compute.comp");
	if (!computeProgram) {
		fprintf(stderr, "SHADER: Error linking compute shader! Exiting...\n");
		exit(1);
	}
	renderProgram = LinkProgramViaFile("shaders/render.vert", "shaders/render.frag");
	if (!renderProgram) {
		fprintf(stderr, "SHADER: Error linking render shader! Exiting...\n");
		exit(1);
	}
	gridRenderProgram = LinkProgramViaFile("shaders/gridRender.vert", "shaders/gridRender.frag");
	if (!gridRenderProgram) {
		fprintf(stderr, "SHADER: Error linking grid render shader! Exiting...\n");
		exit(1);
	}
	stencilProgram = LinkProgramViaFile("shaders/stencil.vert", "shaders/stencil.frag");
	if (!stencilProgram) {
		fprintf(stderr, "SHADER: Error linking stencil shader! Exiting...\n");
		exit(1);
	}
}

void RenderBrush() {
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
	glUseProgram(renderProgram);
	glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
	VertexAttribPointer(renderProgram, "point", 3, 0, (void*)0);
	VertexAttribPointer(renderProgram, "normal", 3, 0, (void*)(sizeof(cube_points)));
	SetUniform(renderProgram, "persp", camera.persp);
	SetUniform(renderProgram, "light_pos", lightPos);
	mat4 scale = Scale((float)(1.0f / GRID_SIZE));
	mat4 trans = Translate((brushPos.x - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (brushPos.y - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (brushPos.z - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE));
	SetUniform(renderProgram, "modelview", camera.modelview * trans * scale);
	SetUniform(renderProgram, "color", vec4(brushColor[0], brushColor[1], brushColor[2], brushColor[3]));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
}

void RenderBrushStencil() {
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(stencilProgram);
	glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
	VertexAttribPointer(stencilProgram, "point", 3, 0, (void*)0);
	SetUniform(stencilProgram, "persp", camera.persp);
	SetUniform(stencilProgram, "stencilColor", vec4(stencilColor[0], stencilColor[1], stencilColor[2], stencilColor[3]));
	mat4 scale = Scale((float)(1.0f / GRID_SIZE) * 1.2);
	mat4 trans = Translate((brushPos.x - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (brushPos.y - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (brushPos.z - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE));
	SetUniform(stencilProgram, "modelview", camera.modelview * trans * scale);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
	glEnable(GL_DEPTH_TEST);
}

void ChangeBrushElement (int type) {
	brushElement = type;
	switch (type) {
	case AIR:
		brushElementString = "AIR";
		break;
	case STONE:
		brushElementString = "STONE";
		break;
	case WATER:
		brushElementString = "WATER";
		break;
	case SAND:
		brushElementString = "SAND";
		break;
	case OIL:
		brushElementString = "OIL";
		break;
	case SALT:
		brushElementString = "SALT";
		break;
	case STEAM:
		brushElementString = "STEAM";
		break;
	}
}

void ChangeBrushShape(int shape) {
	brushShape = shape;
	switch (shape) {
	case 0:
		brushShapeString = "SPHERE";
		break;
	case 1:
		brushShapeString = "CUBE";
		break;
	}
}

void S_MouseButton(GLFWwindow* w, int butn, int action, int mods) {
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	y = win_height - y;
	if (action == GLFW_PRESS)
		camera.MouseDown((int)x, (int)y);
	if (action == GLFW_RELEASE)
		camera.MouseUp();
}

void S_MouseMove(GLFWwindow* w, double x, double y) {
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
		y = win_height - y;
		camera.MouseDrag((int)x, (int)y, Shift(w));
	}
}

// Overriding default camera control callback
void S_Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch (key) {
		case GLFW_KEY_R:
			if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) grid.clear();
			break;
		case GLFW_KEY_SPACE:
			if (brushShape == 0) grid.WriteSphere(brushPos, brushRadius, brushElement);
			else if (brushShape == 1) grid.WriteCube(brushPos, brushRadius, brushElement);
			break;
		case GLFW_KEY_H:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) showHelp = true;
			break;
		case GLFW_KEY_W:
			if (brushPos.x < GRID_SIZE - 1)
				brushPos.x += 1;
			break;
		case GLFW_KEY_S:
			if (brushPos.x > 0)
				brushPos.x -= 1;
			break;
		case GLFW_KEY_D:
			if (brushPos.z < GRID_SIZE - 1)
				brushPos.z += 1;
			break;
		case GLFW_KEY_A:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
				showAbout = true; return;
			}
			if (brushPos.z > 0)
				brushPos.z -= 1;
			break;
		case GLFW_KEY_Q:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
				glfwSetWindowShouldClose(window, GLFW_TRUE); return; 
			}
			if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
				grid.clear(); return;
			}
			if (brushPos.y < GRID_SIZE - 1)
				brushPos.y += 1;
			break;
		case GLFW_KEY_E:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
				showElements = true; return;
			}
			if (brushPos.y > 0)
				brushPos.y -= 1;
			break;
		case GLFW_KEY_O:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
				if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
					showGridStats = true; return;
				}
				showOverlay = true; return;
			}
		case GLFW_KEY_1:
			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
				ChangeBrushShape(0); return;
			}
			ChangeBrushElement(AIR);
			break;
		case GLFW_KEY_2:
			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
				ChangeBrushShape(1); return;
			}
			ChangeBrushElement(STONE);
			break;
		case GLFW_KEY_3:
			ChangeBrushElement(WATER);
			break;
		case GLFW_KEY_4:
			ChangeBrushElement(SAND);
			break;
		case GLFW_KEY_5:
			ChangeBrushElement(OIL);
			break;
		case GLFW_KEY_6:
			ChangeBrushElement(SALT);
			break;
		case GLFW_KEY_7:
			ChangeBrushElement(STEAM);
			break;
	}
}

void LoadBuffers() {
	glGenBuffers(1, &voxelBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
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
	glDeleteBuffers(1, &voxelBuffer);
	cube.unloadBuffer();
	grid.unloadBuffer();
}

void ShowAboutWindow(bool* p_open) {
	int about_width = 400, about_height = 240;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(ImVec2((win_width / 2) - (about_width / 2), (win_height / 2) - (about_height / 2)), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(about_width, about_height), ImGuiCond_Once);
	if (!ImGui::Begin("About", &showAbout, window_flags)) ImGui::End();
	else {
		ImGui::Text("Falling Sand Cubed (FSC)");
		ImGui::Separator();
		ImGui::TextWrapped(FSGui::AboutText);
		ImGui::End();
	}
}

void ShowHelpWindow(bool* p_open) {
	int help_width = 400, help_height = 240;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(ImVec2((win_width / 2) - (help_width / 2), (win_height / 2) - (help_height / 2)), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(help_width, help_height), ImGuiCond_Once);
	if (!ImGui::Begin("Help", &showHelp, window_flags)) ImGui::End();
	else {
		ImGui::BulletText("Use the mouse to drag the simulation cube around.");
		ImGui::BulletText("Use the mouse wheel to zoom the camera in and out.");
		ImGui::BulletText("Use the WASD keys to move the brush cursor \n(shown as a floating cube) horizontally.");
		ImGui::BulletText("Use the QE keys to move the brush cursor up and \ndown.");
		ImGui::BulletText("Use the SPACE key to draw using the brush cursor.");
		ImGui::BulletText("Use the numeric keys (1-6) to quick select an \nelement, or use the 'Brush' tab on the menu bar \nabove.");
		ImGui::BulletText("Most functions can be accessed using the menu bar,\nand certain options have an associated key \ncombination shown next to them.");
		ImGui::End();
	}
}

void ShowOverlayWindow(bool* p_open) {
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
	const float PAD = 10.0f;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos; 
	ImVec2 work_size = viewport->WorkSize;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = (overlayCorner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
	window_pos.y = (overlayCorner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
	window_pos_pivot.x = (overlayCorner & 1) ? 1.0f : 0.0f;
	window_pos_pivot.y = (overlayCorner & 2) ? 1.0f : 0.0f;
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	ImGui::SetNextWindowBgAlpha(0.35f); 
	if (ImGui::Begin("Overlay", p_open, window_flags)) {
		ImGui::Text("Framerate: %.0f fps", io.Framerate);
		ImGui::Text("Particles: %i", grid.num_particles);
		ImGui::Text("Brush: (%i,%i,%i)", (int)brushPos.x, (int)brushPos.y, (int)brushPos.z);
		ImGui::Text("Element: %s", brushElementString.c_str());
		ImGui::Text("Radius: %i", brushRadius);
		ImGui::Text("Shape: %s", brushShapeString.c_str());
		if (showGridStats) {
			ImGui::Separator();
			ImGui::Text("Stone: %i", grid.stone);
			ImGui::Text("Water: %i", grid.water);
			ImGui::Text("Sand:  %i", grid.sand);
			ImGui::Text("Oil:   %i", grid.oil);
			ImGui::Text("Salt:  %i", grid.salt);
			ImGui::Text("Steam: %i", grid.steam);
			ImGui::Text("Grid: %ix%ix%i", GRID_SIZE, GRID_SIZE, GRID_SIZE);
			ImGui::Text("(Total %i)", GRID_SIZE * GRID_SIZE * GRID_SIZE);
		}
	}
	ImGui::End();
}

void ShowElementsWindow(bool* p_open) {
	int elements_width = 400, elements_height = 140;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2(elements_width, elements_height), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2((win_width / 2) - (elements_width / 2), (win_height / 2) - (elements_height / 2)), ImGuiCond_Once);
	if (ImGui::Begin("Elements", p_open, window_flags)) {
		static int selected = AIR;
		ImGui::BeginChild("left pane", ImVec2(80, 0), true);
		if (ImGui::Selectable("Air", selected == AIR)) selected = AIR;
		if (ImGui::Selectable("Stone", selected == STONE)) selected = STONE;
		if (ImGui::Selectable("Water", selected == WATER)) selected = WATER;
		if (ImGui::Selectable("Sand", selected == SAND)) selected = SAND;
		if (ImGui::Selectable("Oil", selected == OIL)) selected = OIL;
		if (ImGui::Selectable("Salt", selected == SALT)) selected = SALT;
		if (ImGui::Selectable("Steam", selected == STEAM)) selected = STEAM;
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("item view", ImVec2(0, 0), true);
		if (selected == AIR) ImGui::TextWrapped(FSGui::ElementsAirText);
		if (selected == STONE) ImGui::TextWrapped(FSGui::ElementsStoneText);
		if (selected == WATER) ImGui::TextWrapped(FSGui::ElementsWaterText);
		if (selected == SAND) ImGui::TextWrapped(FSGui::ElementsSandText);
		if (selected == OIL) ImGui::TextWrapped(FSGui::ElementsOilText);
		if (selected == SALT) ImGui::TextWrapped(FSGui::ElementsSaltText);
		if (selected == STEAM) ImGui::TextWrapped(FSGui::ElementsSteamText);
		ImGui::EndChild();
	}
	ImGui::End();
}

void RenderImGui() {
	// Tell ImGui we're rendering a new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create main menu bar
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("FallingSandCubed")) {
		if (ImGui::MenuItem("About", "CTRL + A", false)) showAbout = !showAbout;
		if (ImGui::MenuItem("Help", "CTRL + H", false)) showHelp = !showHelp;
		if (ImGui::MenuItem("Elements", "CTRL + E", false)) showElements = !showElements;
		if (ImGui::BeginMenu("Overlay")) {
			if (ImGui::MenuItem("Toggle", "CTRL + O", showOverlay)) showOverlay = !showOverlay;
			if (ImGui::MenuItem("Grid Stats", "CTRL + SHIFT + O", showGridStats && showOverlay, showOverlay)) showGridStats = !showGridStats;
			if (ImGui::MenuItem("Top-left", NULL, overlayCorner == 0 && showOverlay, showOverlay)) overlayCorner = 0;
			if (ImGui::MenuItem("Top-right", NULL, overlayCorner == 1 && showOverlay, showOverlay)) overlayCorner = 1;
			if (ImGui::MenuItem("Bottom-left", NULL, overlayCorner == 2 && showOverlay, showOverlay)) overlayCorner = 2;
			if (ImGui::MenuItem("Bottom-right", NULL, overlayCorner == 3 && showOverlay, showOverlay)) overlayCorner = 3;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Themes")) {
			if (ImGui::MenuItem("Classic", NULL, themeClassic)) {
				themeClassic = true, themeLight = false, themeDark = false;
				ImGui::StyleColorsClassic();
			}
			if (ImGui::MenuItem("Light", NULL, themeLight)) {
				themeClassic = false, themeLight = true, themeDark = false;
				ImGui::StyleColorsLight();
			}
			if (ImGui::MenuItem("Dark", NULL, themeDark)) {
				themeClassic = false, themeLight = false, themeDark = true;
				ImGui::StyleColorsDark();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Renderer")) {
			ImGui::Text("Renderer");
			ImGui::RadioButton("CPU-managed", &renderer, 0);
			ImGui::RadioButton("Instanced", &renderer, 1);
			ImGui::Separator();
			ImGui::Text("Simulation FPS");
			ImGui::RadioButton("144 FPS", &simulationFpsLimit, 144);
			ImGui::RadioButton("60 FPS", &simulationFpsLimit, 60);
			ImGui::RadioButton("30 FPS", &simulationFpsLimit, 30);
			ImGui::Separator;
			ImGui::Text("Lighting");
			ImGui::SliderFloat("Ambient", &ambient, 0, 1);
			ImGui::SliderFloat("Diffuse", &diffuse, 0, 1);
			ImGui::SliderFloat("Specular", &specular, 0, 1);
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Quit", "CTRL + Q", false)) glfwSetWindowShouldClose(window, GLFW_TRUE);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Brush")) {
		if (ImGui::BeginMenu("Element")) {
			if (ImGui::MenuItem("Air", "1", brushElement == AIR)) ChangeBrushElement(AIR);
			if (ImGui::MenuItem("Stone", "2", brushElement == STONE)) ChangeBrushElement(STONE);
			if (ImGui::MenuItem("Water", "3", brushElement == WATER)) ChangeBrushElement(WATER);
			if (ImGui::MenuItem("Sand", "4", brushElement == SAND)) ChangeBrushElement(SAND);
			if (ImGui::MenuItem("Oil", "5", brushElement == OIL)) ChangeBrushElement(OIL);
			if (ImGui::MenuItem("Salt", "6", brushElement == SALT)) ChangeBrushElement(SALT);
			if (ImGui::MenuItem("Steam", "7", brushElement == STEAM)) ChangeBrushElement(STEAM);
			ImGui::EndMenu();
		}
		ImGui::SliderInt("Radius", &brushRadius, 1, 20);
		if (ImGui::BeginMenu("Shape")) {
			if (ImGui::MenuItem("Sphere", "SHIFT + 1", brushShape == 0)) ChangeBrushShape(0);
			if (ImGui::MenuItem("Cube", "SHIFT + 2", brushShape == 1)) ChangeBrushShape(1);
			ImGui::EndMenu();
		}
		ImGui::MenuItem("Toggle Highlight", NULL, &drawBrushHighlight);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Grid")) {
		if (ImGui::MenuItem("Clear", "ALT + Q", false)) grid.clear();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Colors")) {
		ImGui::ColorEdit4("BG Color", bgColor);
		ImGui::ColorEdit4("Brush Color", brushColor);
		ImGui::ColorEdit4("Brush Stencil Color", stencilColor);
		ImGui::ColorEdit4("Stone Color", stoneColor);
		ImGui::ColorEdit4("Water Color", waterColor);
		ImGui::ColorEdit4("Sand Color", sandColor);
		ImGui::ColorEdit4("Oil Color", oilColor);
		ImGui::ColorEdit4("Salt Color", saltColor);
		ImGui::ColorEdit4("Steam Color", steamColor);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Debug")) {
		if (ImGui::MenuItem("ImGui Demo", NULL, showDemo)) showDemo = !showDemo;
		if (ImGui::MenuItem("ImGui Metrics", NULL, showMetrics)) showMetrics = !showMetrics;
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar(); 

	// Show windows
	if (showAbout) ShowAboutWindow(&showAbout);
	if (showHelp) ShowHelpWindow(&showHelp);
	if (showElements) ShowElementsWindow(&showElements);
	if (showOverlay) ShowOverlayWindow(&showOverlay);
	if (showDemo) ImGui::ShowDemoWindow(); // debug
	if (showMetrics) ImGui::ShowMetricsWindow(); // debug

	// Render ImGui windows
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Display() {
	glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	cube.display(camera);
	if (renderer == 0) grid.cpuManagedRender();
	else if (renderer == 1) grid.instancedRender();
	RenderBrush();
	if (drawBrushHighlight) RenderBrushStencil();
	RenderImGui();
	glFlush();
}

int main() {
	srand((int)time(NULL));
	if (!glfwInit())
		return 1;
	window = glfwCreateWindow(win_width, win_height, "Falling Sand: Cubed", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwSetWindowPos(window, 100, 100);
	glfwMakeContextCurrent(window);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(render_glsl_version);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	PrintGLErrors();
	CompileShaders();
	LoadBuffers();
	glfwWindowHint(GLFW_SAMPLES, 4);
	InitializeCallbacks(window);
	glfwSetCursorPosCallback(window, S_MouseMove);
	glfwSetMouseButtonCallback(window, S_MouseButton);
	glfwSetKeyCallback(window, S_Keyboard);
	glfwSwapInterval(1);
	start = clock();
	double lastFrame = 0, lastSim = 0;
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	while (!glfwWindowShouldClose(window)) {
		double now = glfwGetTime();
		double deltaTime = now - lastFrame;
		if ((now - lastSim) >= (1.0 / simulationFpsLimit)) {
			grid.compute();
			lastSim = now;
		}
		lastFrame = now;
		Display();
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	UnloadBuffers();
	glfwDestroyWindow(window);
	glfwTerminate();
}