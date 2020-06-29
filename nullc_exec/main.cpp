#include "../NULLC/nullc.h"

#if defined(_MSC_VER)
	#define BUILD_FOR_WINDOWS
#endif

#if defined(_MSC_VER)
#pragma warning(disable: 4996)
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

// NULLC modules
#include "../NULLC/includes/file.h"
#include "../NULLC/includes/io.h"
#include "../NULLC/includes/math.h"
#include "../NULLC/includes/string.h"
#include "../NULLC/includes/vector.h"
#include "../NULLC/includes/random.h"
#include "../NULLC/includes/time.h"
#include "../NULLC/includes/gc.h"
#include "../NULLC/includes/memory.h"

#include "../NULLC/includes/canvas.h"

#include "../NULLC/includes/pugi.h"

#ifdef BUILD_FOR_WINDOWS
	#include <windows.h>
	#include "../NULLC/includes/window.h"
#endif

typedef nullres (*externalInit)(nullres (*)(const char*, const char*), nullres (*)(const char*, void (*)(), const char*, int));

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		printf("usage: nullc_exec [-x86] [-p] [-v] file\n");
		printf("\t -x86\texecute using x86 JiT compilation\n");
		printf("\t -p\tprofile code compilation speed\n");
		printf("\t -v\tverbose output (module import warnings, program result)\n");
		printf("\t -log\tenable compiler debug output\n");
		return 0;
	}
	bool useX86 = false;
	bool profile = false;
	bool verbose = false;
	bool logging = false;
	const char *fileName = NULL;
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-x86") == 0)
			useX86 = true;
		if(strcmp(argv[i], "-p") == 0)
			profile = true;
		if(strcmp(argv[i], "-v") == 0)
			verbose = true;
		if(strcmp(argv[i], "-log") == 0)
			logging = true;
		if(strstr(argv[i], ".nc"))
			fileName = argv[i];
	}
	if(!fileName)
	{
		printf("File must be specified\n");
		return 0;
	}

	nullcInit();
	nullcAddImportPath("Modules/");

#ifdef __linux
	#define X64_LIB "nullclib.ncm"
#else
	#define X64_LIB "nullclib_x64.ncm"
#endif

	FILE *modulePack = fopen(sizeof(void*) == sizeof(int) ? "nullclib.ncm" : X64_LIB, "rb");
	if(!modulePack)
	{
		if(verbose)
		{
			printf("WARNING: Failed to open precompiled module file ");
			printf(sizeof(void*) == sizeof(int) ? "nullclib.ncm\r\n" : X64_LIB"\r\n");
		}
	}
	else
	{
		fseek(modulePack, 0, SEEK_END);
		unsigned int fileSize = ftell(modulePack);
		fseek(modulePack, 0, SEEK_SET);
		char *fileContent = new char[fileSize];
		fread(fileContent, 1, fileSize, modulePack);
		fclose(modulePack);

		char *filePos = fileContent;
		while((unsigned int)(filePos - fileContent) < fileSize)
		{
			char *moduleName = filePos;
			filePos += strlen(moduleName) + 1;
			char *binaryCode = filePos;
			filePos += *(unsigned int*)binaryCode;
			nullcLoadModuleByBinary(moduleName, binaryCode);
		}

		delete[] fileContent;
	}

	if(!nullcInitTypeinfoModule() && verbose)
		printf("ERROR: Failed to init std.typeinfo module\r\n");
	if(!nullcInitDynamicModule() && verbose)
		printf("ERROR: Failed to init std.dynamic module\r\n");

	if(!nullcInitFileModule() && verbose)
		printf("ERROR: Failed to init std.file module\r\n");
	if(!nullcInitIOModule() && verbose)
		printf("ERROR: Failed to init std.io module\r\n");
	if(!nullcInitMathModule() && verbose)
		printf("ERROR: Failed to init std.math module\r\n");
	if(!nullcInitStringModule() && verbose)
		printf("ERROR: Failed to init std.string module\r\n");

	if(!nullcInitCanvasModule() && verbose)
		printf("ERROR: Failed to init img.canvas module\r\n");
#ifdef BUILD_FOR_WINDOWS
	if(!nullcInitWindowModule() && verbose)
		printf("ERROR: Failed to init win.window module\r\n");
#endif
	if(!nullcInitVectorModule() && verbose)
		printf("ERROR: Failed to init std.vector module\r\n");
	if(!nullcInitRandomModule() && verbose)
		printf("ERROR: Failed to init std.random module\r\n");
	if(!nullcInitTimeModule() && verbose)
		printf("ERROR: Failed to init std.time module\r\n");
	if(!nullcInitGCModule() && verbose)
		printf("ERROR: Failed to init std.gc module\r\n");
	if(!nullcInitMemoryModule() && verbose)
		printf("ERROR: Failed to init std.memory module\r\n");

	if(!nullcInitPugiXMLModule() && verbose)
		printf("ERROR: Failed to init ext.pugixml module\r\n");

#ifdef BUILD_FOR_WINDOWS
	WIN32_FIND_DATA	findData;
	memset(&findData, 0, sizeof(findData));

	const char *path = "NC_*.dll";
	HANDLE hFind = FindFirstFile(path, &findData);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if(verbose)
					printf("Loading bindings from %s\n", findData.cFileName);
				HMODULE module = LoadLibrary(findData.cFileName);
				externalInit init = (externalInit)GetProcAddress(module, "InitModule");
				if(!init)
				{
					if(verbose)
						printf("Failed to find initialization function InitModule\n");
				}else{
					if(!init(nullcLoadModuleBySource, nullcBindModuleFunction) && verbose)
						printf("Failed to load bindings from %s\n", findData.cFileName);
				}
			}
		}while(FindNextFile(hFind, &findData));
		FindClose(hFind);
	}
#endif

	nullcSetExecutor(useX86 ? NULLC_X86 : NULLC_REG_VM);

	if(logging)
		nullcSetEnableLogFiles(true, NULL, NULL, NULL);

	FILE *ncFile = fopen(fileName, "rb");
	if(!ncFile)
	{
		printf("File not found\n");
		return 0;
	}
	fseek(ncFile, 0, SEEK_END);
	unsigned int textSize = ftell(ncFile);
	fseek(ncFile, 0, SEEK_SET);
	char *fileContent = new char[textSize+1];
	fread(fileContent, 1, textSize, ncFile);
	fileContent[textSize] = 0;
	fclose(ncFile);

	if(profile)
	{
		int start = clock();
		for(unsigned int i = 0; i < 5000; i++)
			nullcCompile(fileContent);
		int end = clock();
		printf("5000 compilations: %dms Single: %.2fms\n", end - start, (end - start) / 5000.0);

		start = clock();
		for(unsigned int i = 0; i < 5000; i++)
			nullcBuild(fileContent);
		end = clock();
		printf("5000 comp. + link: %dms Single: %.2fms\n", end - start, (end - start) / 5000.0);
	}

	int result = 1;

	nullres good = nullcBuild(fileContent);
	if(!good)
	{
		printf("Build failed: %s\n", nullcGetLastError());
	}else if(!profile){
		nullres goodRun = nullcRun();
		if(goodRun)
		{
			if(verbose)
			{
				const char* val = nullcGetResult();
				printf("\n%s\n", val);

				result = 0;
			}else{
				result = nullcGetResultInt();
			}
		}else{
			printf("Execution failed: %s\n", nullcGetLastError());
		}
	}

	delete[] fileContent;

	nullcTerminate();

	return result;
}
