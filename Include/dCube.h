// dCube.h : Object to enclose a wireframed cube to be drawn to screen

#include <glad.h>
#include <stdio.h>
#include <vector>
#include "Camera.h"
#include "VecMat.h"

using std::vector;

class dCube {
public:
	dCube() {};
	mat4 transform = mat4();
	void loadBuffer();
	void unloadBuffer();
	void display(Camera camera, mat4* m = NULL);
private:
	GLuint getCubeShader();
	GLuint useCubeShader();
	GLuint vBuffer = 0;
	vector<vec3> points = { {-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1} };
	vector<int4> faces = { {0, 1, 2, 3}, {2, 3, 4, 5}, {4, 5, 6, 7}, {6, 7, 0, 1}, {0, 3, 4, 7}, {1, 2, 5, 6} };
};