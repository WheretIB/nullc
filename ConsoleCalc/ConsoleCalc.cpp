#include "../NULLC/nullc.h"

#if defined(_MSC_VER)
	#define BUILD_FOR_WINDOWS
#endif

#pragma warning(disable : 4996)

#include <stdio.h>
#include <string.h>
#include <time.h>

// NULLC modules
#include "../NULLC/includes/file.h"
#include "../NULLC/includes/io.h"
#include "../NULLC/includes/math.h"
#include "../NULLC/includes/string.h"
#include "../NULLC/includes/vector.h"
#include "../NULLC/includes/list.h"
#include "../NULLC/includes/map.h"
#include "../NULLC/includes/hashmap.h"
#include "../NULLC/includes/random.h"
#include "../NULLC/includes/time.h"
#include "../NULLC/includes/gc.h"

#include "../NULLC/includes/canvas.h"

#include "../NULLC/includes/pugi.h"

#ifdef BUILD_FOR_WINDOWS
	#include "../NULLC/includes/window.h"
#endif

int main(int argc, char** argv)
{
	nullcInit("Modules\\");

	FILE *modulePack = fopen(sizeof(void*) == sizeof(int) ? "nullclib.ncm" : "nullclib_x64.ncm", "rb");
	if(!modulePack)
	{
		printf("WARNING: Failed to open precompiled module file");
		printf(sizeof(void*) == sizeof(int) ? "nullclib.ncm\r\n" : "nullclib_x64.ncm\r\n");
	}else{
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

	if(!nullcInitTypeinfoModule())
		printf("ERROR: Failed to init std.typeinfo module\r\n");
	if(!nullcInitDynamicModule())
		printf("ERROR: Failed to init std.dynamic module\r\n");

	if(!nullcInitFileModule())
		printf("ERROR: Failed to init std.file module\r\n");
	if(!nullcInitIOModule())
		printf("ERROR: Failed to init std.io module\r\n");
	if(!nullcInitMathModule())
		printf("ERROR: Failed to init std.math module\r\n");
	if(!nullcInitStringModule())
		printf("ERROR: Failed to init std.string module\r\n");

	if(!nullcInitCanvasModule())
		printf("ERROR: Failed to init img.canvas module\r\n");
#ifdef BUILD_FOR_WINDOWS
	if(!nullcInitWindowModule())
		printf("ERROR: Failed to init win.window module\r\n");
#endif
	if(!nullcInitVectorModule())
		printf("ERROR: Failed to init std.vector module\r\n");
	if(!nullcInitListModule())
		printf("ERROR: Failed to init std.list module\r\n");
	if(!nullcInitMapModule())
		printf("ERROR: Failed to init std.map module\r\n");
	if(!nullcInitHashmapModule())
		printf("ERROR: Failed to init std.hashmap module\r\n");
	if(!nullcInitRandomModule())
		printf("ERROR: Failed to init std.random module\r\n");
	if(!nullcInitTimeModule())
		printf("ERROR: Failed to init std.time module\r\n");
	if(!nullcInitGCModule())
		printf("ERROR: Failed to init std.gc module\r\n");

	if(!nullcInitPugiXMLModule())
		printf("ERROR: Failed to init ext.pugixml module\r\n");
	/*
	nullcInitFileModule();
	nullcInitMathModule();
	nullcInitStringModule();

	nullcInitCanvasModule();
	nullcInitIOModule();

#ifdef BUILD_FOR_WINDOWS
	nullcInitWindowModule();
#endif
*/
	if(argc == 1)
	{
		printf("usage: ConsoleCalc [-x86] file\n");
		return 0;
	}
	bool useX86 = false;
	bool profile = false;
	const char *fileName = NULL;
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-x86") == 0)
			useX86 = true;
		if(strcmp(argv[i], "-p") == 0)
			profile = true;
		if(strstr(argv[i], ".nc"))
			fileName = argv[i];
	}
	if(!fileName)
	{
		printf("File must be specified\n");
		return 0;
	}

	nullcSetExecutor(useX86 ? NULLC_X86 : NULLC_VM);
	
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

	nullres good = nullcBuild(fileContent);
	if(!good)
	{
		printf("Build failed: %s\n", nullcGetLastError());
	}else if(!profile){
		nullres goodRun = nullcRun();
		if(goodRun)
		{
			const char* val = nullcGetResult();
			printf("\n%s\n", val);
		}else{
			printf("Execution failed: %s\n", nullcGetLastError());
		}
	}

	delete[] fileContent;

	nullcTerminate();
}
