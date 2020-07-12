#include "../NULLC/nullc.h"

#if defined(_MSC_VER)
#pragma warning(disable: 4996)
#endif

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

bool AddSourceFile(char *&buf, const char *name)
{
	if(FILE *file = fopen(name, "r"))
	{
		*(buf++) = ' ';
		strcpy(buf, name);
		buf += strlen(buf);

		fclose(file);

		return true;
	}

	return false;
}

bool SearchAndAddSourceFile(char*& buf, const char* name)
{
	char tmp[256];
	sprintf(tmp, "%s", name);

	if(AddSourceFile(buf, tmp))
		return true;

	sprintf(tmp, "translation/%s", name);

	if(AddSourceFile(buf, tmp))
		return true;

	sprintf(tmp, "../NULLC/translation/%s", name);

	if(AddSourceFile(buf, tmp))
		return true;

	return false;
}

int main(int argc, char** argv)
{
	nullcInit();
	nullcAddImportPath("Modules/");

	if(argc == 1)
	{
		printf("usage: nullcl [-o output.ncm] file.nc [-m module.name] [file2.nc [-m module.name] ...]\n");
		printf("usage: nullcl -c output.cpp file.nc\n");
		printf("usage: nullcl -x output.exe file.nc\n");
		return 1;
	}

	int argIndex = 1;
	FILE *mergeFile = NULL;
	bool verbose = false;

	if(strcmp("-v", argv[argIndex]) == 0)
	{
		argIndex++;

		verbose = true;
	}

	if(strcmp("-o", argv[argIndex]) == 0)
	{
		argIndex++;
		if(argIndex == argc)
		{
			printf("Output file name not found after -o\n");
			nullcTerminate();
			return 1;
		}
		mergeFile = fopen(argv[argIndex], "wb");
		if(!mergeFile)
		{
			printf("Cannot create output file %s\n", argv[argIndex]);
			nullcTerminate();
			return 1;
		}
		argIndex++;
	}else if(strcmp("-c", argv[argIndex]) == 0 || strcmp("-x", argv[argIndex]) == 0){
		bool link = strcmp("-x", argv[argIndex]) == 0;
		argIndex++;
		if(argIndex == argc)
		{
			printf("Output file name not found after -o\n");
			nullcTerminate();
			return 1;
		}
		const char *outputName = argv[argIndex++];

		if(argIndex == argc)
		{
			printf("Input file name not found\n");
			nullcTerminate();
			return 1;
		}

		const char *fileName = argv[argIndex++];
		FILE *ncFile = fopen(fileName, "rb");
		if(!ncFile)
		{
			printf("Cannot open file %s\n", fileName);
			nullcTerminate();
			return 1;
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
			return 1;
		}

		if(!nullcTranslateToC(link ? "__temp.cpp" : outputName, "main", AddDependency))
		{
			printf("Compilation of %s failed with error:\n%s\n", fileName, nullcGetLastError());
			delete[] fileContent;
			return 1;
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

			strcpy(pos, " -Itranslation");
			pos += strlen(pos);

			strcpy(pos, " -I../NULLC/translation");
			pos += strlen(pos);

			strcpy(pos, " -O2");
			pos += strlen(pos);

			if(!SearchAndAddSourceFile(pos, "runtime.cpp"))
				printf("Failed to find 'runtime.cpp' input file");

			for(unsigned i = 0; i < translationDependencyCount; i++)
			{
				const char *dependency = translationDependencies[i];

				*(pos++) = ' ';

				strcpy(pos, dependency);
				pos += strlen(pos);

				if(strstr(dependency, "import_"))
				{
					char tmp[256];
					sprintf(tmp, "%s", dependency + strlen("import_"));

					if(char *pos = strstr(tmp, "_nc.cpp"))
						strcpy(pos, "_bind.cpp");

					if(!SearchAndAddSourceFile(pos, tmp))
						printf("Failed to find '%s' input file", tmp);
				}
			}
			
			if (verbose)
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
			nullcTerminate();
			return 1;
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

				delete[] bytecode;
				break;
			}

			if(strlen(argv[argIndex]) + 1 >= 1024)
			{
				printf("Module name is too long\n");

				delete[] bytecode;
				break;
			}

			strcpy(moduleName, argv[argIndex]);

			argIndex++;
		}
		else
		{
			if(strlen(fileName) + 1 >= 1024)
			{
				printf("File name is too long\n");

				delete[] bytecode;
				break;
			}

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

			// Ont extra character for 'm' appended at the end
			if(strlen(fileName) + 1 >= 1024 - 1)
			{
				printf("File name is too long\n");

				delete[] bytecode;
				break;
			}

			strcpy(newName, fileName);
			strcat(newName, "m");
			FILE *nmcFile = fopen(newName, "wb");
			if(!nmcFile)
			{
				printf("Cannot create output file %s\n", newName);

				delete[] bytecode;
				break;
			}
			fwrite(moduleName, 1, strlen(moduleName) + 1, nmcFile);
			fwrite(bytecode, 1, *bytecode, nmcFile);
			fclose(nmcFile);
		}else{
			fwrite(moduleName, 1, strlen(moduleName) + 1, mergeFile);
			fwrite(bytecode, 1, *bytecode, mergeFile);
		}

		delete[] bytecode;
	}
	if(currIndex == argIndex)
		printf("None of the input files were found\n");

	if(mergeFile)
		fclose(mergeFile);

	nullcTerminate();

	return argIndex != argc;	
}
