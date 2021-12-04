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
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

using std::vector;

GLuint computeProgram = 0, srcBuffer = 0, dstBuffer = 0;
GLuint renderProgram = 0;
GLuint tempCubeBuffer = 0;

const int GRID_SIZE = 64;
const char* render_glsl_version = "#version 130";

int win_width = 800, win_height = 800;
Camera camera((float)win_width / win_height, vec3(0, 0, 0), vec3(0, 0, -5));
GLFWwindow* window;
vec3 lightPos = vec3(1, 1, 0);
dCube cube;
vec3 dropperPos = vec3((int)(GRID_SIZE / 2), (int)(GRID_SIZE - 3), (int)(GRID_SIZE / 2));
time_t start;

// Colors
float bgColor[3] = { 0.4f, 0.4f, 0.4f };
float dropperColor[3] = { 1.0f, 0.0f, 0.0f };
float stoneColor[3] = { 0.5f, 0.5f, 0.5f };
float waterColor[3] = { 0.1f, 0.1f, 0.7f };
float sandColor[3] = { 0.906f, 0.702f, 0.498f };
float oilColor[3] = { 0.133f, 0.067f, 0.223f };
float saltColor[3] = { 0.902f, 0.906f, 0.910f };
float liquidTranslucence = 0.5f;

float cube_points[][3] = { {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}, {1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {-1 , 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1} };
float cube_normals[][3] = { {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0} };
int cube_triangles[][3] = { {0, 1, 2}, {2, 3, 0}, {4, 5, 6}, {6, 7, 4}, {8, 9, 10}, {10, 11, 8}, {12, 13, 14}, {14, 15, 12}, {16, 17, 18}, {18, 19, 16}, {20, 21, 22}, {22, 23, 20} };

enum ParticleType {
	AIR = 0,
	STONE = 1,
	WATER = 2,
	SAND = 3,
	OIL = 4,
	SALT = 5,
};

int brushElement = SAND;
int brushRadius = 3;

struct ParticleGrid {
	GLuint grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
	ParticleGrid() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					grid[i][j][k] = AIR;
	}
	void writeGrid() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, srcBuffer);
		GLvoid* buf = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		memcpy(buf, &grid, sizeof(grid));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
	void readGrid() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, srcBuffer);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(grid), &grid);
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
	}
	void unloadBuffer() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDeleteBuffers(1, &srcBuffer);
		glDeleteBuffers(1, &dstBuffer);
	}
	void clear() {
		for (int i = 0; i < GRID_SIZE; i++)
			for (int j = 0; j < GRID_SIZE; j++)
				for (int k = 0; k < GRID_SIZE; k++)
					grid[i][j][k] = AIR;
		writeGrid();
	}
	void compute() {
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

void renderGrid() {
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
						case STONE:
							SetUniform(renderProgram, "color", vec4(stoneColor[0], stoneColor[1], stoneColor[2], 1.0f));
							break;
						case WATER:
							SetUniform(renderProgram, "color", vec4(waterColor[0], waterColor[1], waterColor[2], liquidTranslucence));
							break;
						case SAND:
							SetUniform(renderProgram, "color", vec4(sandColor[0], sandColor[1], sandColor[2], 1.0f));
							break;
						case OIL:
							SetUniform(renderProgram, "color", vec4(oilColor[0], oilColor[1], oilColor[2], liquidTranslucence));
							break;
						case SALT:
							SetUniform(renderProgram, "color", vec4(saltColor[0], saltColor[1], saltColor[2], 1.0f));
							break;
					}
					glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
				}
			}
		}
	}
}

void renderDropper() {
	glUseProgram(renderProgram);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glBindBuffer(GL_ARRAY_BUFFER, tempCubeBuffer);
	VertexAttribPointer(renderProgram, "point", 3, 0, (void*)0);
	VertexAttribPointer(renderProgram, "normal", 3, 0, (void*)(sizeof(cube_points)));
	SetUniform(renderProgram, "persp", camera.persp);
	SetUniform(renderProgram, "light_pos", lightPos);
	mat4 scale = Scale((float)(1.0f / GRID_SIZE));
	mat4 trans = Translate((dropperPos.x - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (dropperPos.y - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (dropperPos.z - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE));
	SetUniform(renderProgram, "modelview", camera.modelview * trans * scale);
	SetUniform(renderProgram, "color", vec4(dropperColor[0], dropperColor[1], dropperColor[2], 1.0f));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
}

void WriteSphere(vec3 center, int radius, int pType, float spawnProb) {
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
					if (rand_float() <= spawnProb) {
						grid.grid[i][j][k] = pType;
					}
				}
			}
		}
	}
	grid.writeGrid();
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
			WriteSphere(dropperPos, brushRadius, brushElement, 1.0);
			break;
		case GLFW_KEY_W:
			if (dropperPos.x < GRID_SIZE - 1)
				dropperPos.x += 1;
			break;
		case GLFW_KEY_S:
			if (dropperPos.x > 0)
				dropperPos.x -= 1;
			break;
		case GLFW_KEY_D:
			if (dropperPos.z < GRID_SIZE - 1)
				dropperPos.z += 1;
			break;
		case GLFW_KEY_A:
			if (dropperPos.z > 0)
				dropperPos.z -= 1;
			break;
		case GLFW_KEY_Q:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
			else if (dropperPos.y < GRID_SIZE - 1)
				dropperPos.y += 1;
			break;
		case GLFW_KEY_E:
			if (dropperPos.y > 0)
				dropperPos.y -= 1;
			break;
		case GLFW_KEY_1:
			brushElement = AIR; 
			break;
		case GLFW_KEY_2:
			brushElement = STONE; 
			break;
		case GLFW_KEY_3:
			brushElement = WATER; 
			break;
		case GLFW_KEY_4:
			brushElement = SAND; 
			break;
		case GLFW_KEY_5:
			brushElement = OIL; 
			break;
		case GLFW_KEY_6:
			brushElement = SALT; 
			break;
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

void renderImGui() {
	// Tell ImGui we're rendering a new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("FallingSandCubed")) {
		ImGui::MenuItem("About", NULL, false);
		if (ImGui::MenuItem("Quit", "CTRL + Q", false)) glfwSetWindowShouldClose(window, GLFW_TRUE);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Brush")) {
		if (ImGui::BeginMenu("Element")) {
			if (ImGui::MenuItem("Air", "1")) brushElement = AIR;
			if (ImGui::MenuItem("Stone", "2")) brushElement = STONE;
			if (ImGui::MenuItem("Water", "3")) brushElement = WATER;
			if (ImGui::MenuItem("Sand", "4")) brushElement = SAND;
			if (ImGui::MenuItem("Oil", "5")) brushElement = OIL;
			if (ImGui::MenuItem("Salt", "6")) brushElement = SALT;
			ImGui::EndMenu();
		}
		ImGui::SliderInt("Radius", &brushRadius, 1, 10);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Grid")) {
		if (ImGui::MenuItem("Clear", "ALT + Q", false)) grid.clear();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Colors")) {
		ImGui::ColorEdit3("BG Color", bgColor);
		ImGui::ColorEdit3("Dropper Color", dropperColor);
		ImGui::ColorEdit3("Stone Color", stoneColor);
		ImGui::ColorEdit3("Water Color", waterColor);
		ImGui::ColorEdit3("Sand Color", sandColor);
		ImGui::ColorEdit3("Oil Color", oilColor);
		ImGui::ColorEdit3("Salt Color", saltColor);
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar(); 

	//ImGui::ShowDemoWindow();

	// Render ImGui windows
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void Display() {
	glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cube.display(camera);
	renderGrid();
	renderDropper();
	renderImGui();
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
	int frame = 0;
	start = clock();
	while (!glfwWindowShouldClose(window)) {
		grid.compute();
		Display();
		glfwPollEvents();
		glfwSwapBuffers(window);
		frame++;
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	UnloadBuffers();
	glfwDestroyWindow(window);
	glfwTerminate();
}