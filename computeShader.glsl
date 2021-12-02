#version 430

const int GRID_SIZE = 128;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding=0) buffer src_buffer {
	uint srcGrid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
}; 
layout (std430, binding=1) buffer dst_buffer {
	uint dstGrid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
};

// Particle types
const int P_AIR = 0;
const int P_STONE = 1;
const int P_WATER = 2;
const int P_SAND = 3;
const int P_OIL = 4;
const int P_SALT = 5;

ivec3 inv = ivec3(gl_GlobalInvocationID);

// Pseudorandom values for more natural processing
uint pcg_hash(uint input) {
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

void shuffleMoves(inout uint[8] arr) {
	for (uint i = 0; i < 8; i++) {
		uint a = uint(floor((float(pcg_hash(inv.x + i)) / 4294967296.0) * 8.0));
		uint b = uint(floor((float(pcg_hash(inv.y - i)) / 4294967296.0) * 8.0));
		uint c = arr[a];
		arr[a] = arr[b];
		arr[b] = c;
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
	uint p = srcGrid[x][y][z];
	// Try to move down
	if (y > 0) {
		uint np = srcGrid[x][y-1][z];
		if (np == P_AIR || isGas(np)) {
			uint wp = atomicCompSwap(dstGrid[x][y-1][z], np, p);
			if (wp == np) {
				atomicExchange(dstGrid[x][y][z], np);
				return;
			}
		}
	}
	// Else, try to move sideways
	uint[8] moves;
	for (uint i = 0; i < 8; i++) moves[i] = i;
	shuffleMoves(moves);
	for (uint m = 0; m < 8; m++) {
		ivec3 move;
		switch(moves[m]) {
			case 0:
				move = ivec3(x - 1, y, z - 1);
				break;
			case 1:
				move = ivec3(x, y, z - 1);
				break;
			case 2:
				move = ivec3(x + 1, y, z - 1);
				break;
			case 3:
				move = ivec3(x - 1, y, z);
				break;
			case 4:
				move = ivec3(x + 1, y, z);
				break;
			case 5:
				move = ivec3(x - 1, y, z + 1);
				break;
			case 6:
				move = ivec3(x, y, z + 1);
				break;
			case 7:
				move = ivec3(x + 1, y, z + 1);
				break;
		}
		if (move.x >= 0 && move.x < GRID_SIZE && move.z >= 0 && move.z < GRID_SIZE) {
			uint np = srcGrid[move.x][move.y][move.z];
			if (np == P_AIR || isGas(np)) {
				uint wp = atomicCompSwap(dstGrid[move.x][move.y][move.z], np, p);
				if (wp == np) {
					atomicExchange(dstGrid[x][y][z], np);
					return;
				}
			}
		}
	}
	/*
	for (int nx = 1; nx >= -1; nx--) {
		for (int nz = 1; nz >= -1; nz--) {
			if ((nx != 0 || nz != 0) && x + nx < GRID_SIZE && x + nx >= 0 && z + nz < GRID_SIZE && z + nz >= 0) {
				uint np = srcGrid[x + nx][y][z + nz];
				if (np == P_AIR || isGas(np)) {
					uint wp = atomicCompSwap(dstGrid[x + nx][y][z + nz], np, p);
					if (wp == np) {
						atomicExchange(dstGrid[x][y][z], np);
						return;
					}
				}
			}
		}
	}
	*/
}

void processMovableSolids(uint x, uint y, uint z) {
	uint p = srcGrid[x][y][z];
	if (y > 0) {
		// Try to move down
		uint np = srcGrid[x][y-1][z];
		if (np == P_AIR || isLiquid(np) || isGas(np)) {
			uint wp = atomicCompSwap(dstGrid[x][y-1][z], np, p);
			if (wp == np) {
				atomicExchange(dstGrid[x][y][z], np);
				return;
			}
		}
		// Else, move diagonal downwards (if possible)
		for (int nx = -1; nx <= 1; nx++) {
			for (int nz = -1; nz <= 1; nz++) {
				if ((nx != 0 || nz != 0) && x + nx < GRID_SIZE && x + nx >= 0 && z + nz < GRID_SIZE && z + nz >= 0) {
					uint np = srcGrid[x + nx][y-1][z + nz];
					if (np == P_AIR || isLiquid(np) || isGas(np)) {
						uint wp = atomicCompSwap(dstGrid[x + nx][y - 1][z + nz], np, p);
						if (wp == np) {
							atomicExchange(dstGrid[x][y][z], np);
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
	uint p = srcGrid[inv.x][inv.y][inv.z];
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

