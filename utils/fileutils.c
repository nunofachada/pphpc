/*** Functions to import OpenCL kernels ***/
#include "fileutils.h"

char* importKernel( const char * filename ) 
{
	FILE * fp = fopen(filename, "r");
	fseek(fp, 0L, SEEK_END);
	int kernel_size = (int) ftell(fp);
	char * sourcetmp = (char*) malloc((kernel_size + 1)*sizeof(char));
	rewind(fp);
	int ch;
	int cnt = 0;
	while((ch = fgetc(fp)) != EOF) {
		sourcetmp[cnt] = (char) ch;
		cnt++;
	}
	sourcetmp[cnt] = '\0';
	fclose(fp);
	return sourcetmp;
}



