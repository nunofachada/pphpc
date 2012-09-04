/*** Functions to parse errors in OpenCL ***/
#include "clerrors.h"

void PrintErrorCreateContext( cl_int error, const char * xtra ) 
{
	printf("Context creation error: ");
	switch (error) {
		case CL_INVALID_PLATFORM : printf("Invalid platform!\n"); break;
		case CL_INVALID_PROPERTY: printf("Invalid property!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value!\n"); break;
		case CL_INVALID_DEVICE: printf("Invalid device!\n"); break;
		case CL_DEVICE_NOT_AVAILABLE: printf("Device not available!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorCreateBuffer ( cl_int error, const char * xtra )
{
	printf("CreateBuffer error: ");
	switch (error) {
		case CL_INVALID_CONTEXT: printf("Invalid context!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value!\n"); break;
		case CL_INVALID_BUFFER_SIZE: printf("Invalid buffer size (size is 0!!)!\n"); break;
		case CL_INVALID_HOST_PTR: printf("Invalid host pointer (host_ptr is NULL and CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or if host_ptr is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set in flags)!\n"); break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE : printf("Failure to allocate memory object (there is a failure to allocate memory for buffer object)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorCreateCommandQueue( cl_int error, const char * xtra ) 
{
	printf("Command queue creation error: ");
	switch (error) {
		case CL_INVALID_CONTEXT: printf("Invalid context!\n"); break;
		case CL_INVALID_DEVICE: printf("Invalid device!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value!\n"); break;
		case CL_INVALID_QUEUE_PROPERTIES: printf("Invalid queue properties!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorGetDeviceInfo( cl_int error, const char * xtra )
{
	printf("Get device info error: ");
	switch (error) {
		case CL_INVALID_DEVICE: printf("Invalid device!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorGetDeviceIDs( cl_int error, const char * xtra )
{
	printf("Get device IDs error: ");
	switch (error) {
		case CL_INVALID_PLATFORM : printf("Invalid platform!\n"); break;
		case CL_INVALID_DEVICE_TYPE : printf("Invalid device type!\n"); break;
		case CL_DEVICE_NOT_FOUND: printf("Device not found!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorGetPlatformIDs( cl_int error, const char * xtra )
{
	printf("Get platform IDs error: ");
	switch (error) {
		case CL_INVALID_VALUE: printf("Invalid value!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorSetKernelArg( cl_int error, const char * xtra )
{
	printf("SetKernelArg error: ");
	switch (error) {
		case CL_INVALID_KERNEL : printf("Invalid kernel (kernel is not a valid kernel object)!\n"); break;
		case CL_INVALID_ARG_INDEX : printf("Invalid argument index!\n"); break;
		case CL_INVALID_ARG_VALUE : printf("Invalid argument value!\n"); break;
		case CL_INVALID_MEM_OBJECT : printf("Invalid memory object given as argument!\n"); break;
		case CL_INVALID_SAMPLER : printf("Invalid sampler!\n"); break;
		case CL_INVALID_ARG_SIZE : printf("Invalid argument size (arg_size does not match the size of the data type for an argument that is not a memory object or if the argument is a memory object and arg_size != sizeof(cl_mem) or if arg_size is zero and the argument is declared with the __local qualifier or if the argument is a sampler and arg_size != sizeof(cl_sampler).)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorEnqueueNDRangeKernel( cl_int error, const char * xtra )
{
	printf("clEnqueueNDRangeKernel error: ");
	switch (error) {
		case CL_INVALID_PROGRAM_EXECUTABLE : printf("Invalid program executable (there is no successfully built program executable available for device associated with the command queue)!\n"); break;
		case CL_INVALID_COMMAND_QUEUE : printf("Invalid command queue (command queue object is not a valid command-queue)!\n"); break;
		case CL_INVALID_KERNEL : printf("Invalid kernel (kernel is not a valid kernel object)!\n"); break;
		case CL_INVALID_CONTEXT : printf("Invalid context (context associated with command queue and kernel is not the same or context associated with command ueue and events in event_wait_list is not the same)!\n"); break;
		case CL_INVALID_KERNEL_ARGS : printf("Invalid kernel arguments (kernel argument values have not been specified)!\n"); break;
		case CL_INVALID_WORK_DIMENSION : printf("Invalid work dimension (work_dim is not a valid value bettwen 1-3)!\n"); break;
		case CL_INVALID_GLOBAL_WORK_SIZE : printf("Invalid global work size ( global_work_size is NULL, or if any of the values specified in global_work_size[0], ... global_work_size[work_dim – 1] are 0 or exceed the range given by the sizeof(size_t) for the device on which the kernel execution will be enqueued)!\n"); break;
		case CL_INVALID_GLOBAL_OFFSET : printf("Invalid global offset (the value specified in global_work_size + the corresponding values in global_work_offset for any dimensions is greater than the sizeof(size t) for the device on which the kernel execution will be enqueued)!\n"); break;
		case CL_INVALID_WORK_GROUP_SIZE : printf("Invalid work group size!\n"); break;
		case CL_DEVICE_MAX_WORK_GROUP_SIZE : printf("Work group size is larger than device's maximum!\n"); break;
		case CL_INVALID_WORK_ITEM_SIZE : printf("Invalid work item size (the number of work-items specified in any of local_work_size[0], ... local_work_size[work_dim – 1] is greater than the corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0], ....)!\n"); break;
		case CL_DEVICE_MAX_WORK_ITEM_SIZES : printf("Work items size is larger than device's maximum!!\n"); break;
		case CL_MISALIGNED_SUB_BUFFER_OFFSET : printf("Misaligned sub-buffer offset (a sub-buffer object is specified as the value for an argument that is a buffer object and the offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue.!\n"); break;
		case CL_INVALID_IMAGE_SIZE : printf("Invalid image size (n image object is specified as an argument value and the image dimensions (image width, height, specified or compute row and/or slice pitch) are not supported by device associated with queue.)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_DEVICE_MAX_READ_IMAGE_ARGS : printf("Read image arguments are larger than device's maximum (value for device or the number of write-only image args used in kernel exceed the CL_DEVICE_MAX_WRITE_IMAGE_ARGS value for device or the number of samplers used in kernel exceed CL_DEVICE_MAX_SAMPLERS for device.)!\n"); break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE : printf("Failure to allocate memory object (there is a failure to allocate memory for data store associated with image or buffer objects specified as arguments to kernel)!\n"); break;
		case CL_INVALID_EVENT_WAIT_LIST : printf("Invalid event wait list (event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events)!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorCreateProgramWithSource ( cl_int error, const char * xtra ) {
	printf("clCreateProgramWithSource error: ");
	switch (error) {
		case CL_INVALID_CONTEXT : printf("Invalid context (context is not a valid context)!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value (count is zero or strings or any entry in strings is NULL)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorBuildProgram( cl_int error, const char * xtra ) {
	printf("clBuildProgram error: ");
	switch (error) {
		case CL_INVALID_PROGRAM : printf("Invalid program (program is not a valid program object)!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value (device_list is NULL and num_devices is greater than zero, or device_list is not NULL and num_devices is zero, or pfn_notify is NULL but user_data is not NULL)!\n"); break;
		case CL_INVALID_DEVICE: printf("Invalid device (OpenCL devices listed in device_list are not in the list of devices associated with program)!\n"); break;
		case CL_INVALID_BINARY : printf("Invalid binary (program was created with clCreateWithProgramBinary but devices listed in device_list do not have a valid program binary loaded)!\n"); break;
		case CL_INVALID_BUILD_OPTIONS : printf("Invalid build options (build options specified by options are invalid)!\n"); break;
		case CL_INVALID_OPERATION : printf("Invalid operation (the build of a program executable for any of the devices listed in device_list by a previous call to clBuildProgram for program has not completed OR there are kernel objects attached to program)!\n"); break;
		case CL_COMPILER_NOT_AVAILABLE : printf("Compiler not available (program is created with clCreateProgramWithSource and a compiler is not available i.e. CL_DEVICE_COMPILER_AVAILABLE is set to CL_FALSE)!\n"); break;
		case CL_BUILD_PROGRAM_FAILURE : printf("Build program failure (there is a failure to build the program executable; this occurs if clBuildProgram does not return until the build has completed)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorGetProgramBuildInfo( cl_int error, const char * xtra ) {
	printf("GetProgramBuildInfo error: ");
	switch (error) {
		case CL_INVALID_PROGRAM : printf("Invalid program (program is not a valid program object)!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value (param_name is not valid, or if size in bytes specified by param_value_size is < size of return type as described in table 5.12 and param_value is not NULL)!\n"); break;
		case CL_INVALID_DEVICE: printf("Invalid device (device is not in the list of devices associated with program)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorCreateKernel( cl_int error, const char * xtra ) {
	printf("Create Kernel: ");
	switch (error) {
		case CL_INVALID_PROGRAM : printf("Invalid program (program is not a valid program object)!\n"); break;
		case CL_INVALID_PROGRAM_EXECUTABLE : printf("Invalid program executable (there is no successfully built executable for program)!\n"); break;
		case CL_INVALID_KERNEL_NAME: printf("Invalid kernel name (kernel_name is not found in program)!\n"); break;
		case CL_INVALID_KERNEL_DEFINITION: printf("Invalid kernel definition (the function definition for __kernel function given by kernel_name such as the number of arguments, the argument types are not the same for all devices for which the program executable has been built)!\n"); break;

		case CL_INVALID_VALUE: printf("Invalid value (kernel_name is NULL)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorEnqueueReadWriteBuffer( cl_int error, const char * xtra ) {
	printf("EnqueueReadBuffer or EnqueueWriteBuffer error: ");
	switch (error) {
		case CL_INVALID_COMMAND_QUEUE : printf("Invalid command queue (command queue object is not a valid command-queue)!\n"); break;
		case CL_INVALID_CONTEXT : printf("Invalid context (the context associated with command_queue and buffer are not the same or if the context associated with command_queue and events in event_wait_list are not the same)!\n"); break;
		case CL_INVALID_MEM_OBJECT: printf("Invalid mem object (buffer is not a valid buffer object)!\n"); break;
		case CL_INVALID_EVENT_WAIT_LIST : printf("Invalid event wait list (event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events)!\n"); break;
		case CL_MISALIGNED_SUB_BUFFER_OFFSET: printf("Invalid sub-buffer offset (buffer is a sub-buffer object and offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue)!\n"); break;
		case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: printf("Execution status error for events in wait list (the read and write operations are blocking and the execution status of any of the events in event_wait_list is a negative integer value)!\n"); break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE: printf("Mem object allocation failure (there is a failure to allocate memory for data store associated with buffer\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value (the region being read or written specified by (offset, cb) is out of bounds or ptr is a NULL value)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorWaitForEvents( cl_int error, const char * xtra ) {
	printf("WaitForEvents error: ");
	switch (error) {
		case CL_INVALID_CONTEXT : printf("Invalid context (events specified in event_list do not belong to the same context)!\n"); break;
		case CL_INVALID_EVENT : printf("Invalid event (event objects specified in event_list are not valid event objects)!\n"); break;
		case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST : printf("Execution status error for events in wait list (the execution status of any of the events in event_list is a negative integer value.)!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value (num_events is zero or event_list is NULL)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorEnqueueBarrier( cl_int error, const char * xtra ) {
	printf("EnqueueBarrier error: ");
	switch (error) {
		case CL_INVALID_COMMAND_QUEUE: printf("Invalid command queue (command_queue is not a valid command-queue)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorEnqueueWaitForEvents( cl_int error, const char * xtra ) {
	printf("WaitForEvents error: ");
	switch (error) {
		case CL_INVALID_COMMAND_QUEUE: printf("Invalid command queue (command_queue is not a valid command-queue)!\n"); break;
		case CL_INVALID_CONTEXT : printf("Invalid context (context associated with command_queue and events in event_list are not the same)!\n"); break;
		case CL_INVALID_VALUE: printf("Invalid value (num_events is zero or event_list is NULL)!\n"); break;
		case CL_INVALID_EVENT : printf("Invalid event (event objects specified in event_list are not valid events)!\n"); break;
		case CL_OUT_OF_RESOURCES: printf("Out of resources!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory!\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}

void PrintErrorGetPlatformInfo( cl_int error, const char * xtra ) {
	printf("GetPlatformInfo error: ");
	switch (error) {
		case CL_INVALID_VALUE: printf("Invalid value (param_name is not one of the supported values OR size in bytes specified by param_value_size is < size of return type and param_value is not a NULL value.)!\n"); break;
		case CL_INVALID_PLATFORM : printf(" Invalid platform (platform is not a valid platform)!\n"); break;
		case CL_OUT_OF_HOST_MEMORY: printf("Out of host memory! Failure to allocate resources required by the OpenCL implementation on the host.\n"); break;
		default: printf("Unknown reason!\n"); break;
	}
	if (xtra != NULL) printf("Additional info: %s\n", xtra);
	return;
}


