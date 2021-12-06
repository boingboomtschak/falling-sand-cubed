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

GLuint computeProgram = 0, srcBuffer = 0, dstBuffer = 0;
GLuint renderProgram = 0, gridRenderProgram = 0;
GLuint tempCubeBuffer = 0;

const int GRID_SIZE = 64;
const char* render_glsl_version = "#version 130";

int win_width = 800, win_height = 800;
Camera camera((float)win_width / win_height, vec3(0, 0, 0), vec3(0, 0, -5));
GLFWwindow* window;
vec3 lightPos = vec3(1, 1, 0);
dCube cube;
vec3 brushPos = vec3((int)(GRID_SIZE / 2), (int)(GRID_SIZE - 3), (int)(GRID_SIZE / 2));
time_t start;
int livingParticles = 0;

// Colors
float bgColor[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
float brushColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float stoneColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
float waterColor[4] = { 0.1f, 0.1f, 0.7f, 0.5f };
float sandColor[4] = { 0.906f, 0.702f, 0.498f, 1.0f };
float oilColor[4] = { 0.133f, 0.067f, 0.223f, 0.5f };
float saltColor[4] = { 0.902f, 0.906f, 0.910f, 1.0f };

// Window show toggles
static bool showAbout = false;
static bool showHelp = false;
static bool showOverlay = false;
static bool showDemo = false; // debug
static bool showMetrics = false; // debug
int overlayCorner = 1; // Defaults to top-right
static bool themeClassic = true, themeLight = false, themeDark = false;

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
string brushElementString = "SAND";
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
		glUseProgram(gridRenderProgram);

	}
	
};

ParticleGrid grid;

void CompileShaders() {
	computeProgram = LinkProgramViaFile("compute.comp");
	if (!computeProgram) {
		fprintf(stderr, "SHADER: Error linking compute shader! Exiting...\n");
		exit(1);
	}
	renderProgram = LinkProgramViaFile("render.vert", "render.frag");
	if (!renderProgram) {
		fprintf(stderr, "SHADER: Error linking render shader! Exiting...\n");
		exit(1);
	}
	gridRenderProgram = LinkProgramViaFile("gridRender.vert", "gridRender.frag");
	if (!gridRenderProgram) {
		fprintf(stderr, "SHADER: Error linking grid render shader! Exiting...\n");
		exit(1);
	}
}

void RenderGrid() {
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
					}
					glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
				}
			}
		}
	}
}

void RenderDropper() {
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
	mat4 trans = Translate((brushPos.x - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (brushPos.y - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE), (brushPos.z - (GRID_SIZE / 2) + 0.5) * (2.0f / GRID_SIZE));
	SetUniform(renderProgram, "modelview", camera.modelview * trans * scale);
	SetUniform(renderProgram, "color", vec4(brushColor[0], brushColor[1], brushColor[2], brushColor[3]));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cube_triangles);
}

void WriteSphere(vec3 center, int radius, int pType) {
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

int FindLivingParticles() {
	int p = 0;
	for (size_t i = 0; i < GRID_SIZE; i++)
		for (size_t j = 0; j < GRID_SIZE; j++)
			for (size_t k = 0; k < GRID_SIZE; k++)
				if (grid.grid[i][j][k] != AIR)
					p++;
	return p;
}

void ChangeBrush(int pType) {
	brushElement = pType;
	switch (pType) {
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
			WriteSphere(brushPos, brushRadius, brushElement);
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
				showAbout = true;
				return;
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
			if (brushPos.y > 0)
				brushPos.y -= 1;
			break;
		case GLFW_KEY_O:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
				showOverlay = true;
				return;
			}
		case GLFW_KEY_1:
			ChangeBrush(AIR);
			break;
		case GLFW_KEY_2:
			ChangeBrush(STONE);
			break;
		case GLFW_KEY_3:
			ChangeBrush(WATER);
			break;
		case GLFW_KEY_4:
			ChangeBrush(SAND);
			break;
		case GLFW_KEY_5:
			ChangeBrush(OIL);
			break;
		case GLFW_KEY_6:
			ChangeBrush(SALT);
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

void ShowAboutWindow(bool* p_open) {
	int about_width = 400, about_height = 200;
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
		ImGui::Text("Particles: %i", livingParticles);
		ImGui::Text("Brush: (%i,%i,%i)", (int)brushPos.x, (int)brushPos.y, (int)brushPos.z);
		ImGui::Text("Element: %s", brushElementString.c_str());
		ImGui::Text("Grid: %ix%ix%i", GRID_SIZE, GRID_SIZE, GRID_SIZE);
		ImGui::Text("(Total %i)", GRID_SIZE * GRID_SIZE * GRID_SIZE);
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
		if (ImGui::BeginMenu("Overlay")) {
			if (ImGui::MenuItem("Toggle", "CTRL + O", showOverlay)) showOverlay = !showOverlay;
			if (ImGui::MenuItem("Top-left", NULL, overlayCorner == 0)) overlayCorner = 0;
			if (ImGui::MenuItem("Top-right", NULL, overlayCorner == 1)) overlayCorner = 1;
			if (ImGui::MenuItem("Bottom-left", NULL, overlayCorner == 2)) overlayCorner = 2;
			if (ImGui::MenuItem("Bottom-right", NULL, overlayCorner == 3)) overlayCorner = 3;
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
		if (ImGui::MenuItem("Quit", "CTRL + Q", false)) glfwSetWindowShouldClose(window, GLFW_TRUE);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Brush")) {
		ImGui::BeginChild("brush menu", ImVec2(150, 40));
		if (ImGui::BeginMenu("Element")) {
			if (ImGui::MenuItem("Air", "1", brushElement == AIR)) ChangeBrush(AIR);
			if (ImGui::MenuItem("Stone", "2", brushElement == STONE)) ChangeBrush(STONE);
			if (ImGui::MenuItem("Water", "3", brushElement == WATER)) ChangeBrush(WATER);
			if (ImGui::MenuItem("Sand", "4", brushElement == SAND)) ChangeBrush(SAND);
			if (ImGui::MenuItem("Oil", "5", brushElement == OIL)) ChangeBrush(OIL);
			if (ImGui::MenuItem("Salt", "6", brushElement == SALT)) ChangeBrush(SALT);
			ImGui::EndMenu();
		}
		ImGui::InputInt("Radius", &brushRadius, 1, 5);
		if (brushRadius < 1) brushRadius = 1;
		if (brushRadius > 10) brushRadius = 10;
		ImGui::EndChild();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Grid")) {
		if (ImGui::MenuItem("Clear", "ALT + Q", false)) grid.clear();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Colors")) {
		ImGui::ColorEdit4("BG Color", bgColor);
		ImGui::ColorEdit4("Dropper Color", brushColor);
		ImGui::ColorEdit4("Stone Color", stoneColor);
		ImGui::ColorEdit4("Water Color", waterColor);
		ImGui::ColorEdit4("Sand Color", sandColor);
		ImGui::ColorEdit4("Oil Color", oilColor);
		ImGui::ColorEdit4("Salt Color", saltColor);
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
	if (showOverlay) ShowOverlayWindow(&showOverlay);
	if (showDemo) ImGui::ShowDemoWindow(); // debug
	if (showMetrics) ImGui::ShowMetricsWindow(); // debug

	// Render ImGui windows
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Display() {
	glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cube.display(camera);
	RenderGrid();
	//grid.render();
	RenderDropper();
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
	int frame = 0;
	start = clock();
	while (!glfwWindowShouldClose(window)) {
		grid.compute();
		livingParticles = FindLivingParticles();
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