/*** Functions to import OpenCL kernels ***/
#include "fileutils.h"

char* loadSource( const char * filename ) 
{
	FILE * fp = fopen(filename, "r");
	char * sourcetmp;

	if (!fp) {
		sourcetmp = (char*) malloc(sizeof(char));
		sourcetmp[0] = '\0';
		return sourcetmp;
	}
	
	fseek(fp, 0L, SEEK_END);
	int prog_size = (int) ftell(fp);
	sourcetmp = (char*) malloc((prog_size + 1)*sizeof(char));
	rewind(fp);
	fread(sourcetmp, sizeof(char), prog_size, fp);
	sourcetmp[prog_size] = '\0';
	fclose(fp);
	return sourcetmp;
}

void freeSource( char* source) {
	free(source);
}

