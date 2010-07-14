#include "../NULLC/nullc.h"

#pragma warning(disable : 4996)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	nullcInit("Modules\\");

	if(argc == 1)
	{
		printf("usage: nullcl [-o output.ncm] file.nc [-m module.name] [file2.nc [-m module.name] ...]\n");
#ifdef NULLC_ENABLE_C_TRANSLATION
		printf("usage: nullcl -c output.cpp file.nc\n");
		printf("usage: nullcl -x output.exe file.nc\n");
#endif
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
#ifdef NULLC_ENABLE_C_TRANSLATION
		bool link = strcmp("-x", argv[argIndex]) == 0;
		argIndex++;
		if(argIndex == argc)
		{
			printf("Output file name not found after -o\n");
			nullcTerminate();
			return 0;
		}
		const char *outputName = argv[argIndex++];
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
		nullcTranslateToC(link ? "__temp.cpp" : outputName, "main");

		if(link)
		{
			// $$$ move this to a dependency file?
			char cmdLine[1024];
			strcpy(cmdLine, "gcc.exe -o ");
			strcat(cmdLine, outputName);
			strcat(cmdLine, " __temp.cpp");
			strcat(cmdLine, " translation\\runtime.cpp -lstdc++");
			if(strstr(fileContent, "std.math;"))
			{
				strcat(cmdLine, " translation\\std_math.cpp");
				strcat(cmdLine, " translation\\std_math_bind.cpp");
			}
			if(strstr(fileContent, "std.typeinfo;"))
			{
				strcat(cmdLine, " translation\\std_typeinfo.cpp");
				strcat(cmdLine, " translation\\std_typeinfo_bind.cpp");
			}
			if(strstr(fileContent, "std.file;"))
			{
				strcat(cmdLine, " translation\\std_file.cpp");
				strcat(cmdLine, " translation\\std_file_bind.cpp");
			}
			if(strstr(fileContent, "std.vector;"))
			{
				strcat(cmdLine, " translation\\std_vector.cpp");
				strcat(cmdLine, " translation\\std_vector_bind.cpp");
				if(!strstr(fileContent, "std.typeinfo;"))
				{
					strcat(cmdLine, " translation\\std_typeinfo.cpp");
					strcat(cmdLine, " translation\\std_typeinfo_bind.cpp");
				}
			}
			if(strstr(fileContent, "std.list;"))
			{
				strcat(cmdLine, " translation\\std_list.cpp");
				if(!strstr(fileContent, "std.typeinfo;"))
				{
					strcat(cmdLine, " translation\\std_typeinfo.cpp");
					strcat(cmdLine, " translation\\std_typeinfo_bind.cpp");
				}
			}
			if(strstr(fileContent, "std.range;"))
			{
				strcat(cmdLine, " translation\\std_range.cpp");
			}
			if(strstr(fileContent, "std.gc;"))
			{
				strcat(cmdLine, " translation\\std_gc.cpp");
				strcat(cmdLine, " translation\\std_gc_bind.cpp");
			}
			if(strstr(fileContent, "std.io;"))
			{
				strcat(cmdLine, " translation\\std_io.cpp");
				strcat(cmdLine, " translation\\std_io_bind.cpp");
			}
			if(strstr(fileContent, "std.dynamic;"))
			{
				strcat(cmdLine, " translation\\std_dynamic.cpp");
				strcat(cmdLine, " translation\\std_dynamic_bind.cpp");
			}
			if(strstr(fileContent, "std.time;"))
			{
				strcat(cmdLine, " translation\\std_time.cpp");
				strcat(cmdLine, " translation\\std_time_bind.cpp");
			}
			if(strstr(fileContent, "img.canvas;"))
			{
				strcat(cmdLine, " translation\\img_canvas.cpp");
				strcat(cmdLine, " translation\\img_canvas_bind.cpp");
			}
			if(strstr(fileContent, "win.window;"))
			{
				strcat(cmdLine, " translation\\win_window.cpp");
				strcat(cmdLine, " translation\\win_window_bind.cpp");
				strcat(cmdLine, " translation\\win_window_ex.cpp");
				strcat(cmdLine, " translation\\win_window_ex_bind.cpp -mwindows -mconsole");
				if(!strstr(fileContent, "img.canvas;"))
				{
					strcat(cmdLine, " translation\\img_canvas.cpp");
					strcat(cmdLine, " translation\\img_canvas_bind.cpp");
				}
			}
			printf("Command line: %s\n", cmdLine);
			system(cmdLine);
		}
		delete[] fileContent;
		nullcTerminate();
		fclose(ncFile);
#else
		printf("To use this flag, compile with NULLC_ENABLE_C_TRANSLATION defined\n");
#endif
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
		delete[] fileContent;
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
