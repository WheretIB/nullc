#include "../NULLC/nullc.h"

#pragma warning(disable : 4996)

#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
	nullcInit("Modules\\");

	if(argc == 1)
	{
		printf("usage: nullcl [-o output.ncm] file.nc [-m module.name] [file2.nc [-m module.name] ...]\n");
		return 0;
	}
	int argIndex = 1;
	FILE *mergeFile = NULL;
	if(strcmp("-o", argv[argIndex]) == 0)
	{
		argIndex++;
		if(argIndex == argc)
		{
			printf("Output file name not found after -o\n");
			nullcTerminate();
			return 0;
		}
		mergeFile = fopen(argv[argIndex], "wb");
		if(!mergeFile)
		{
			printf("Cannot create output file %s\n", argv[argIndex]);
			nullcTerminate();
			return 0;
		}
		argIndex++;
	}
	int currIndex = argIndex;
	while(argIndex < argc)
	{
		const char *fileName = argv[argIndex++];
		FILE *ncFile = fopen(fileName, "rb");
		if(!ncFile)
		{
			printf("Cannot open file %s\n", fileName);
			break;
		}

		fseek(ncFile, 0, SEEK_END);
		unsigned int textSize = ftell(ncFile);
		fseek(ncFile, 0, SEEK_SET);
		char *fileContent = new char[textSize+1];
		fread(fileContent, 1, textSize, ncFile);
		fileContent[textSize] = 0;
		fclose(ncFile);

		if(!nullcCompile(fileContent))
		{
			printf("Compilation of %s failed with error:\n%s\n", fileName, nullcGetLastError());
			return false;
		}
		unsigned int *bytecode = NULL;
		nullcGetBytecode((char**)&bytecode);

		// Create module name
		char	moduleName[1024];
		if(argIndex < argc && strcmp("-m", argv[argIndex]) == 0)
		{
			argIndex++;
			if(argIndex == argc)
			{
				printf("Module name not found after -m\n");
				break;
			}
			strcpy(moduleName, argv[argIndex++]);
		}else{
			strcpy(moduleName, fileName);
			if(char *extensionPos = strchr(moduleName, '.'))
				*extensionPos = NULL;
			char	*pos = moduleName;
			while(*pos)
			{
				if(*pos++ == '\\' || *pos++ == '/')
					pos[-1] = '.';
			}
		}
		nullcLoadModuleByBinary(moduleName, (const char*)bytecode);

		if(!mergeFile)
		{
			char newName[1024];
			strcpy(newName, fileName);
			strcat(newName, "m");
			FILE *nmcFile = fopen(newName, "wb");
			if(!nmcFile)
			{
				printf("Cannot create output file %s\n", newName);
				break;
			}
			fwrite(moduleName, 1, strlen(moduleName) + 1, nmcFile);
			fwrite(bytecode, 1, *bytecode, nmcFile);
			fclose(nmcFile);
		}else{
			fwrite(moduleName, 1, strlen(moduleName) + 1, mergeFile);
			fwrite(bytecode, 1, *bytecode, mergeFile);
		}
	}
	if(currIndex == argIndex)
		printf("None of the input files were found\n");

	if(mergeFile)
		fclose(mergeFile);

	nullcTerminate();
}
