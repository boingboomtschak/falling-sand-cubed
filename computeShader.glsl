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
	//uint x_offset = inv.x, z_offset = inv.z;
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
	/*
	// Iterate through rows upwards
	for (uint y = 0; y < GRID_SIZE; y++) {
		for (uint x = 0; x < COL_SIZE; x++) {
			for (uint z = 0; z < COL_SIZE; z++) {
				x = x + (COL_SIZE * x_offset);
				z = z + (COL_SIZE * z_offset);
				// Process particle at x,y,z
				uint p = imageLoad(srcGrid, ivec3(x, y, z)).r;
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
	*/
}

