#include "../NULLC/nullc.h"

#pragma warning(disable : 4996)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char* translationDependencies[128];
unsigned translationDependencyCount = 0;

void AddDependency(const char *fileName)
{
	if(translationDependencyCount < 128)
		translationDependencies[translationDependencyCount++] = fileName;
}

int main(int argc, char** argv)
{
	nullcInit("Modules/");

	if(argc == 1)
	{
		printf("usage: nullcl [-o output.ncm] file.nc [-m module.name] [file2.nc [-m module.name] ...]\n");
		printf("usage: nullcl -c output.cpp file.nc\n");
		printf("usage: nullcl -x output.exe file.nc\n");
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
	}else if(strcmp("-c", argv[argIndex]) == 0 || strcmp("-x", argv[argIndex]) == 0){
		bool link = strcmp("-x", argv[argIndex]) == 0;
		argIndex++;
		if(argIndex == argc)
		{
			printf("Output file name not found after -o\n");
			nullcTerminate();
			return 0;
		}
		const char *outputName = argv[argIndex++];

		if(argIndex == argc)
		{
			printf("Input file name not found\n");
			nullcTerminate();
			return 0;
		}

		const char *fileName = argv[argIndex++];
		FILE *ncFile = fopen(fileName, "rb");
		if(!ncFile)
		{
			printf("Cannot open file %s\n", fileName);
			nullcTerminate();
			return 0;
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
			delete[] fileContent;
			return false;
		}

		if(!nullcTranslateToC(link ? "__temp.cpp" : outputName, "main", AddDependency))
		{
			printf("Compilation of %s failed with error:\n%s\n", fileName, nullcGetLastError());
			delete[] fileContent;
			return false;
		}

		if(link)
		{
			// $$$ move this to a dependency file?
			char cmdLine[4096];

			char *pos = cmdLine;
			strcpy(pos, "gcc -g -o ");
			pos += strlen(pos);

			strcpy(pos, outputName);
			pos += strlen(pos);

			strcpy(pos, " __temp.cpp");
			pos += strlen(pos);

			strcpy(pos, " -lstdc++");
			pos += strlen(pos);

			char tmp[256];
			sprintf(tmp, "runtime.cpp");

			if(FILE *file = fopen(tmp, "r"))
			{
				*(pos++) = ' ';
				strcpy(pos, tmp);
				pos += strlen(pos);

				fclose(file);
			}
			else
			{
				sprintf(tmp, "translation/runtime.cpp");

				if(FILE *file = fopen(tmp, "r"))
				{
					*(pos++) = ' ';
					strcpy(pos, tmp);
					pos += strlen(pos);

					fclose(file);
				}
			}

			for(unsigned i = 0; i < translationDependencyCount; i++)
			{
				const char *dependency = translationDependencies[i];

				*(pos++) = ' ';

				strcpy(pos, dependency);
				pos += strlen(pos);

				if(strstr(dependency, "import_"))
				{
					sprintf(tmp, "%s", dependency + strlen("import_"));

					if(char *pos = strstr(tmp, "_nc.cpp"))
						strcpy(pos, "_bind.cpp");

					if(FILE *file = fopen(tmp, "r"))
					{
						*(pos++) = ' ';
						strcpy(pos, tmp);
						pos += strlen(pos);

						fclose(file);
					}
					else
					{
						sprintf(tmp, "translation/%s", dependency + strlen("import_"));

						if(char *pos = strstr(tmp, "_nc.cpp"))
							strcpy(pos, "_bind.cpp");

						if(FILE *file = fopen(tmp, "r"))
						{
							*(pos++) = ' ';
							strcpy(pos, tmp);
							pos += strlen(pos);

							fclose(file);
						}
					}
				}
			}
			
			printf("Command line: %s\n", cmdLine);

			

			system(cmdLine);
		}
		delete[] fileContent;
		nullcTerminate();
		return 0;
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
			delete[] fileContent;
			return false;
		}
		unsigned int *bytecode = NULL;
		nullcGetBytecode((char**)&bytecode);
		delete[] fileContent;

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
				*extensionPos = '\0';
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

	return argIndex != argc;	
}
