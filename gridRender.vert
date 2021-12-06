#version 430 core


in vec3 point;
in vec3 normal;

struct Particle {
	int x, y, z, t;
};

layout (std430, binding = 0) buffer particle_buffer {
	Particle particles[];
};

// Particle types
const int P_AIR = 0;
const int P_STONE = 1;
const int P_WATER = 2;
const int P_SAND = 3;
const int P_OIL = 4;
const int P_SALT = 5;

uniform int grid_size;
uniform vec4 stoneColor;
uniform vec4 waterColor;
uniform vec4 sandColor;
uniform vec4 oilColor;
uniform vec4 saltColor;
uniform mat4 persp;
uniform mat4 modelview;

out vec3 vPoint;
out vec3 vNormal;
out vec4 color;

void main() {
	// Transform point and normal to vec4 for matrix mult
	vec4 point_ = vec4(point, 1);
	vec4 normal_ = vec4(normal, 0);

	Particle p = particles[gl_InstanceID];

	// Create combined translation / scale matrix
	vec3 trans_vec = vec3((float(p.x) - float(grid_size / 2) + 0.5) * (2.0 / float(grid_size)), (float(p.y) - float(grid_size / 2) + 0.5) * (2.0 / float(grid_size)), (float(p.z) - float(grid_size / 2) + 0.5) * (2.0 / float(grid_size)));
	float scale_fac = 1.0 / float(grid_size);
	mat4 m = mat4(
		scale_fac, 0.0, 0.0, 0.0,
		0.0, scale_fac, 0.0, 0.0,
		0.0, 0.0, scale_fac, 0.0,
		trans_vec.x, trans_vec.y, trans_vec.z, 1.0
	);

	// Create transformed points and normals for frag shader lighting
	vPoint = (modelview * m * point_).xyz;
	vNormal = (modelview * m  * normal_).xyz;

	// Select color to pass forward
	switch(p.t) {
		case P_STONE:
			color = stoneColor;
			break;
		case P_WATER:
			color = waterColor;
			break;
		case P_SAND:
			color = sandColor;
			break;
		case P_OIL:
			color = oilColor;
			break;
		case P_SALT:
			color = saltColor;
			break;
	}
	
	gl_Position = persp * vec4(vPoint, 1);
}