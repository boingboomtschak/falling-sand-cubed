#version 430

const int GRID_SIZE = 8;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding=0) buffer particle_data {
	int grid[GRID_SIZE][GRID_SIZE][GRID_SIZE][2];
}; 

void main() {
	ivec3 inv = ivec3(gl_GlobalInvocationID);
	int pbuffer[2] = grid[inv.x][inv.y][inv.z];
	
	// find "top" particle to move elsewhere
	//if (inv.y > 0) {
	//	int below[2] = grid[inv.x][inv.y-1][inv.z];
	//} 

	// Process liquids
	if (pbuffer[0] == 2 && inv.y > 0) {
		grid[inv.x][inv.y-1][inv.z][0] = pbuffer[0];
		grid[inv.x][inv.y][inv.z][0] = 0;
	}


	// Process solids



	// Process gases



}