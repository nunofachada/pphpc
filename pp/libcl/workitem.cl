/** 
 * @file
 * @brief Utility functions for OpenCL workitems.
 */

#ifndef LIBCL_WORKITEM_CL
#define LIBCL_WORKITEM_CL

#define GLOBAL_SIZE() (get_global_size(0) * get_global_size(1) * get_global_size(2))

#define GID1() (get_global_id(0) * get_global_size(0) + get_global_id(1) * get_global_size(1) + get_global_id(2))

#define GID2() ((uint2) (GID1(), GLOBAL_SIZE() + GID1()))
	
#define GID4() ((uint4) (GID1(), GLOBAL_SIZE() + GID1(), GLOBAL_SIZE() * 2 + GID1(), GLOBAL_SIZE() * 3 + GID1()))

#define GID8() ((uint8) (GID1(), GLOBAL_SIZE() + GID1(), GLOBAL_SIZE() * 2 + GID1(), GLOBAL_SIZE() * 3 + GID1(), GLOBAL_SIZE() * 4 + GID1(), GLOBAL_SIZE() * 5 + GID1(), GLOBAL_SIZE() * 6 + GID1(), GLOBAL_SIZE() * 7 + GID1()))

#endif
