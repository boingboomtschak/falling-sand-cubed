#version 430

const int GRID_SIZE = 32;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding=0) buffer particle_data {
	int grid[GRID_SIZE][GRID_SIZE][GRID_SIZE][2];
}; 

// Particle types
const int P_AIR = 0;
const int P_WALL = 1;
const int P_WATER = 2;
const int P_SAND = 3;
const int P_OIL = 4;
const int P_SALT = 5;

uniform int solidFloor = 1;

void processLiquids() {

}

void processSolids() {

}

void processGases() {
	
}

void main() {
	ivec3 inv = ivec3(gl_GlobalInvocationID);
	int pbuffer[2] = grid[inv.x][inv.y][inv.z];
	
	// find "top" particle to move elsewhere
	//if (inv.y > 0) {
	//	int below[2] = grid[inv.x][inv.y-1][inv.z];
	//} 

	// Process liquids
	if (pbuffer[0] == 2) {
		bool moved = false;
		// Flow down
		if (inv.y > 0) {
			if (grid[inv.x][inv.y-1][inv.z][0] == 0) {
				grid[inv.x][inv.y-1][inv.z][0] = pbuffer[0];
				grid[inv.x][inv.y][inv.z][0] = 0;
				moved = true;
			}
		}
		// Flow to side 
		if (!moved && inv.z + 1 < GRID_SIZE && grid[inv.x][inv.y][inv.z+1][0] == 0) {
			grid[inv.x][inv.y][inv.z+1][0] = pbuffer[0];
			grid[inv.x][inv.y][inv.z][0] = 0;
			moved = true;
		}
		if (!moved && inv.z - 1 > 0 && grid[inv.x][inv.y][inv.z-1][0] == 0) {
			grid[inv.x+1][inv.y][inv.z-1][0] = pbuffer[0];
			grid[inv.x][inv.y][inv.z][0] = 0;
			moved = true;
		}
		if (!moved && inv.x + 1 < GRID_SIZE && grid[inv.x+1][inv.y][inv.z][0] == 0) {
			grid[inv.x+1][inv.y][inv.z][0] = pbuffer[0];
			grid[inv.x][inv.y][inv.z][0] = 0;
			moved = true;
		}
		if (!moved && inv.x - 1 > 0 && grid[inv.x-1][inv.y][inv.z][0] == 0) {
			grid[inv.x-1][inv.y][inv.z][0] = pbuffer[0];
			grid[inv.x][inv.y][inv.z][0] = 0;
			moved = true;
		}

	}


	// Process solids



	// Process gases



}

