#version 430

const int GRID_SIZE = 32;
const int COL_SIZE = 32;
const int SHUFFLE_ITERS = 4;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding=0) buffer particle_data {
	uint grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
}; 

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
	for (uint s = 0; s < SHUFFLE_ITERS; s++) {
		uint a = rand(vec2(float(row[0]), float(row[1])), 0, COL_SIZE - 1);
		uint b = rand(vec2(float(row[2]), float(row[3])), 0, COL_SIZE - 1);
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

void swap(uint x1, uint y1, uint z1, uint x2, uint y2, uint z2) {
	uint p = grid[x1][y1][z1];
	grid[x1][y1][z1] = grid[x2][y2][z2];
	grid[x2][y2][z2] = p;
}

void processLiquids(uint x, uint y, uint z) {
	// Try to move down
	if (y > 0) {
		uint np = grid[x][y-1][z];
		if (np == P_AIR || isGas(np)) {
			swap(x, y, z, x, y-1, z);
			return;
		}
	}
	// Else, try to move sideways
	for (int nx = 1; nx >= -1; nx--) {
		for (int nz = 1; nz >= -1; nz--) {
			if ((nx != 0 || nz != 0) && x + nx < GRID_SIZE && x + nx >= 0 && z + nz < GRID_SIZE && z + nz >= 0) {
				uint np = grid[x + nx][y][z + nz];
				if (np == P_AIR || isGas(np)) {
					swap(x, y, z, x + nx, y, z + nz);
					return;
				}
			}
		}
	}
}

void processMovableSolids(uint x, uint y, uint z) {
	if (y > 0) {
		// Try to move down
		uint np = grid[x][y-1][z];
		if (np == P_AIR || isLiquid(np) || isGas(np)) {
			swap(x, y, z, x, y-1, z);
			return;
		}
		// Else, move diagonal downwards (if possible)
		for (int nx = -1; nx <= 1; nx++) {
			for (int nz = -1; nz <= 1; nz++) {
				if ((nx != 0 || nz != 0) && x + nx < GRID_SIZE && x + nx >= 0 && z + nz < GRID_SIZE && z + nz >= 0) {
					uint np = grid[x + nx][y-1][z + nz];
					if (np == P_AIR || isLiquid(np) || isGas(np)) {
						swap(x, y, z, x + nx, y - 1, z + nz);
						return;
					}
				}
			}
		}
	}
}

void processImmovableSolids(uint x, uint y, uint z) {

}

void processGases(uint x, uint y, uint z) {
	
}

void main() {
	ivec3 inv = ivec3(gl_GlobalInvocationID);
	uint x_offset = inv.x, z_offset = inv.z;

	// Iterate through rows upwards
	for (uint y = 0; y < GRID_SIZE; y++) {
		// Update particles in row in random order
		uint[COL_SIZE] rowX;
		uint[COL_SIZE] rowZ;
		for (uint i = 0; i < COL_SIZE; i++) {
			rowX[i] = i;
			rowZ[i] = i;
		}
		//shuffleRow(rowX);
		//shuffleRow(rowZ);
		for (uint xi = 0; xi < COL_SIZE; xi++) {
			for (uint zi = 0; zi < COL_SIZE; zi++) {
				uint x = rowX[xi] + (COL_SIZE * x_offset);
				uint z = rowZ[zi] + (COL_SIZE * z_offset);
				// Process particle at x,y,z
				uint p = grid[x][y][z];
				if (isLiquid(p)) {
					processLiquids(x, y, z);
				} else if (isMovableSolid(p)) {
					processMovableSolids(x, y, z);
				} else if(isImmovableSolid(p)) {
					processImmovableSolids(x, y, z);
				} else if (isGas(p)) {
					processGases(x, y, z);
				}
			}
		}
	}
}

