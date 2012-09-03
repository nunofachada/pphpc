#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable


/*
 * Agent movement kernel
 */
__kernel void RandomWalk(__global uint4 * array,
				const uint max_x,
				const uint max_y)
{
	// Global id for this work-item
	uint gid = get_global_id(0);
	// Pseudo-randomly determine direction
	uint4 agent = array[gid];
	if (agent.z > 0)
	{
		uint direction = ((uint) ((agent.x ^ agent.y ^ agent.z) % 5));
		// Perform the actual walk
		if (direction == 1) 
		{
			agent.x++;
			if (agent.x >= max_x) agent.x = 0;
		}
		else if (direction == 2) 
		{
			if (agent.x == 0)
				agent.x = max_x - 1;
			else
				agent.x--;
		}
		else if (direction == 3)
		{
			agent.y++;
			if (agent.y >= max_y) agent.y = 0;
		}
		else if (direction == 4)
		{
			if (agent.y == 0)
				agent.y = max_y - 1;
			else
				agent.y--;
		}
		// Lose energy
		agent.z--;
		// For debug
		//agent.w = direction;
		// Update global mem
		array[gid] = agent;
	}
}

/*
 * Bitonic sort kernel
 */
__kernel void BitonicSort(__global uint4 * array,
				const uint stage,
				const uint step)
{
	// Global id for this work-item
	uint gid = get_global_id(0);

	// Determine what to compare and possibly swap
	uint pair_stride = (uint) pown((float) 2, (int) step-1);
	uint index1 = gid + (gid / pair_stride) * pair_stride;
	uint index2 = index1 + pair_stride;
	//bool swap = 0;
	// Get values from global memory
	uint4 auxValue1 = array[index1];
	uint4 auxValue2 = array[index2];
	// Determine if ascending or descending
	bool desc = (bool) (0x1 & (gid >> stage-1));
	// Determine if auxValue1 > auxValue2 according to our criteria
	bool gt = 0;

	if (auxValue2.z != 0)
	{
		if (auxValue1.z == 0) 
		{
			// If energy is zero, we want to put sheep in end of list, so we set "greater than" = true
			gt = 1;
		}
		else if (auxValue1.x > auxValue2.x)
		{
			// x is the main dimension to check if value1 is "greater than" value2
			gt = 1;
		}
		else if (auxValue1.x == auxValue2.x)
		{
			// Break tie by checking y position
			gt = auxValue1.y > auxValue2.y;
		}
	}

	/* Perform swap if needed */ 
	if ( (desc && !gt) || (!desc && gt))
	{
		//swap = 1;
		array[index1] = auxValue2;
		array[index2] = auxValue1;
	}
}


/*
 * Grass kernel
 */
__kernel void Grass(__global uint * matrix, 
			const uint width_x )
{
	// Grid position for this work-item
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	// Decrement counter if grass is dead
	uint index = x + width_x * y;
	if (matrix[index] > 0)
		matrix[index]--;
}

/*
 * Count agents. Assumes dead agents are at the end of the list.
 */
__kernel void CountAgents(__global uint4 * agents, __global uint * stats, const uint index)
{
	// Position of the array this work-item will check
	uint gid = get_global_id(0);
	// Check if this is the limit
	if (gid <  get_global_size(0) - 1)
		if ((agents[gid].z > 0) && (agents[gid + 1].z == 0))
			stats[index] = (uint) (gid + 1);
	if (gid == 0)
		if (agents[0].z == 0)
			stats[index] = 0;
}

/*
 * Count grass part 1.
 */
__kernel void CountGrass1(__global uint * grass,
			__global uint * gcounter,
			__local uint * lcounter/*,
			__global uint4 * debug*/)
{
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	lcounter[lid] = (grass[gid] == 0 ? 1 : 0);
	//barrier(CLK_LOCAL_MEM_FENCE);
	//debug[gid].x = lcounter[lid];
	barrier(CLK_LOCAL_MEM_FENCE);
	uint lsbits = 8*sizeof(uint) - clz(get_local_size(0)) - 1;
	uint stages = ((1 << lsbits) == get_local_size(0)) ? lsbits : lsbits + 1;
	for (int i = 0; i < stages; i++) {
		uint stride = (uint) pown(2.0f, i);
		uint divisible = (uint) stride * 2;
		if ((lid % divisible) == 0) {
			if (lid + stride < get_local_size(0)) {
				lcounter[lid] += lcounter[lid + stride];
			}
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (lid == 0)
		gcounter[get_group_id(0)] = lcounter[lid];
	//debug[gid].x = lcounter[lid];
}

/*
 * Count grass part 2.
 */
__kernel void CountGrass2(__global uint * gcounter,
			__local uint * lcounter,
			__global uint * stats)
{
	uint lid = get_local_id(0);
	lcounter[lid] = gcounter[lid];
	barrier(CLK_LOCAL_MEM_FENCE);
	uint lsbits = 8*sizeof(uint) - clz(get_local_size(0)) - 1;
	uint stages = ((1 << lsbits) == get_local_size(0)) ? lsbits : lsbits + 1;
	for (int i = 0; i < stages; i++) {
		uint stride = (uint) pown(2.0f, i);
		uint divisible = (uint) stride * 2;
		if ((lid % divisible) == 0) {
			if (lid + stride < get_local_size(0)) {
				lcounter[lid] += lcounter[lid + stride];
			}
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (lid == 0)
		stats[2] = lcounter[0];
	
}

/*
 * Sheep action kernel
 */
__kernel void Sheep( __global uint4 * sheeps, 
			__global uint * grass, 
			const uint width_x, 
			const uint gain_from_food, 
			const uint grass_restart, 
			const uint reproduce_threshold, 
			const uint reproduce_prob, /* between 1 and 100 */
			__global uint * stats)
{
	// Global id for this work-item
	uint gid = get_global_id(0);
	// Get my sheep
	uint4 sheep = sheeps[gid];
	// If sheep is alive, do stuff
	if (sheep.z > 0) {
		// If there is grass, eat it (and I can be the only one to do so)!
		uint index = sheep.x + sheep.y * width_x;
		uint grassState = atom_cmpxchg(&grass[index], (unsigned int) 0, (unsigned int) grass_restart);
		if (grassState == 0) {
			// There is grass, sheep eats it and gains energy
			sheep.z += gain_from_food;
		}
		// Perhaps sheep will reproduce if energy > reproduce_threshold
		if (sheep.z > reproduce_threshold) {
			// Throw some kind of dice to see if sheep reproduces
			if (((sheep.x ^ sheep.y ^ sheep.z) % 100) < reproduce_prob ) {
				// Sheep will reproduce! Let's see if there is space...
				uint4 newSheep;
				uint position = atom_inc( &stats[0] );
				if (position < get_global_size(0) - 2)
				{
					// There is space, lets put new sheep!
					newSheep = (uint4) (sheep.x, sheep.y, sheep.z / 2, 444);
					sheeps[position] = newSheep;
				}
				// Current sheep's energy will be halved also
				sheep.z = sheep.z - newSheep.z;
				
			}
		}
		// Put sheep back in array
		sheeps[gid] = sheep;
	}
}

/*
 * Wolf action kernel.
 */
__kernel void Wolves( __global uint4 * wolves, 
			__global uint4 * sheep, 
			const uint width_x, 
			const uint gain_from_food, 
			const uint reproduce_threshold, 
			const uint reproduce_prob, /* between 1 and 100 */
			__global uint * stats)
{
	uint lid = get_local_id(0);
	lcounter[lid] = gcounter[lid];
	barrier(CLK_LOCAL_MEM_FENCE);
	uint lsbits = 8*sizeof(uint) - clz(get_local_size(0)) - 1;
	uint stages = ((1 << lsbits) == get_local_size(0)) ? lsbits : lsbits + 1;
	for (int i = 0; i < stages; i++) {
		uint stride = (uint) pown(2.0f, i);
		uint divisible = (uint) stride * 2;
		if ((lid % divisible) == 0) {
			if (lid + stride < get_local_size(0)) {
				lcounter[lid] += lcounter[lid + stride];
			}
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (lid == 0)
		stats[2] = lcounter[0];
	
}

