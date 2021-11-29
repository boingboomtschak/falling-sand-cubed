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

// Pseudorandom values for more natural processing
uint rand(vec2 co, uint rmin = 0, uint rmax = 1){
    return uint(round( fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453) * rmax )) + rmin;
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
	uint p = grid[x][y][z];
	// Try to move down
	if (y > 0) {
		uint np = grid[x][y-1][z];
		if (np == P_AIR || isGas(np)) {
			uint wp = atomicCompSwap(grid[x][y-1][z], np, p);
			if (wp == np) {
				atomicExchange(grid[x][y][z], np);
				return;
			}
		}
	}
	// Else, try to move sideways
	for (int nx = 1; nx >= -1; nx--) {
		for (int nz = 1; nz >= -1; nz--) {
			if ((nx != 0 || nz != 0) && x + nx < GRID_SIZE && x + nx >= 0 && z + nz < GRID_SIZE && z + nz >= 0) {
				uint np = grid[x + nx][y][z + nz];
				if (np == P_AIR || isGas(np)) {
					uint wp = atomicCompSwap(grid[x + nx][y][z + nz], np, p);
					if (wp == np) {
						atomicExchange(grid[x][y][z], np);
						return;
					}
				}
			}
		}
	}
}

void processMovableSolids(uint x, uint y, uint z) {
	uint p = grid[x][y][z];
	if (y > 0) {
		// Try to move down
		uint np = grid[x][y-1][z];
		if (np == P_AIR || isLiquid(np) || isGas(np)) {
			uint wp = atomicCompSwap(grid[x][y-1][z], np, p);
			if (wp == np) {
				atomicExchange(grid[x][y][z], np);
				return;
			}
		}
		// Else, move diagonal downwards (if possible)
		for (int nx = -1; nx <= 1; nx++) {
			for (int nz = -1; nz <= 1; nz++) {
				if ((nx != 0 || nz != 0) && x + nx < GRID_SIZE && x + nx >= 0 && z + nz < GRID_SIZE && z + nz >= 0) {
					uint np = grid[x + nx][y-1][z + nz];
					if (np == P_AIR || isLiquid(np) || isGas(np)) {
						uint wp = atomicCompSwap(grid[x + nx][y - 1][z + nz], np, p);
						if (wp == np) {
							atomicExchange(grid[x][y][z], np);
							return;
						}
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
	uint p = grid[inv.x][inv.y][inv.z];
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

