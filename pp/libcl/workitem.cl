/** 
 * @file
 * @brief Utility functions for OpenCL workitems.
 */

#ifndef LIBCL_WORKITEM_CL
#define LIBCL_WORKITEM_CL

/** 
 * @brief Returns the absolute index of the current workitem.
 * 
 * @return The absolute index of the current workitem.
 */
uint getWorkitemIndex() {
	uint index;
	uint dims = get_work_dim();
	if (dims == 1)
		index = get_global_id(0);
	else if (dims == 2)
		index = get_global_id(0) * get_global_size(0) + get_global_id(1);
	else
		index = get_global_id(0) * get_global_size(0) + get_global_id(1) * get_global_size(1) + get_global_id(2);
	return index;
}
#endif
