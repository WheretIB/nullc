#include "../NULLC/nullc.h"

#define BUILD_FOR_WINDOWS

#pragma warning(disable : 4996)

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <time.h>

// NULLC modules
#include "../NULLC/includes/file.h"
#include "../NULLC/includes/math.h"
#include "../NULLC/includes/string.h"

#include "../NULLC/includes/canvas.h"

#ifdef BUILD_FOR_WINDOWS
	#include "../NULLC/includes/window.h"
	#include "../NULLC/includes/io.h"
#endif

int main(int argc, char** argv)
{
	nullcInit("Modules\\");

	nullcAddExternalFunction((void (*)())clock, "int clock();");
	
	nullcInitFileModule();
	nullcInitMathModule();
	nullcInitStringModule();

	nullcInitCanvasModule();

#ifdef BUILD_FOR_WINDOWS
	nullcInitIOModule();
	nullcInitWindowModule();
#endif

	if(argc == 1)
	{
		printf("usage: ConsoleCalc [-x86] file");
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
		printf("File must be specified");
		return 0;
	}

	nullcSetExecutor(useX86 ? NULLC_X86 : NULLC_VM);
	
	FILE *ncFile = fopen(fileName, "rb");
	if(!ncFile)
	{
		printf("File not found");
		return 0;
	}
	fseek(ncFile, 0, SEEK_END);
	unsigned int textSize = ftell(ncFile);
	fseek(ncFile, 0, SEEK_SET);
	char *fileContent = new char[textSize+1];
	fread(fileContent, 1, textSize, ncFile);
	fileContent[textSize] = 0;

	char *bytecode = NULL;

	if(profile)
	{
		int start = clock();
		for(unsigned int i = 0; i < 5000; i++)
			nullres good = nullcCompile(fileContent);
		int end = clock();
		printf("5000 compilations: %dms Single: %.2fms\r\n", end - start, (end - start) / 5000.0);

		start = clock();
		for(unsigned int i = 0; i < 5000; i++)
		{
			nullres good = nullcCompile(fileContent);
			nullcGetBytecode(&bytecode);

			nullcClean();
			nullcLinkCode(bytecode, 0);
		}
		end = clock();
		printf("5000 comp. + link: %dms Single: %.2fms\r\n", end - start, (end - start) / 5000.0);
	}

	nullres good = nullcCompile(fileContent);
	if(!good)
	{
		printf("Compilation failed: %s\r\n", nullcGetCompilationError());
	}else if(!profile){
		nullcGetBytecode(&bytecode);

		nullcClean();
		nullcLinkCode(bytecode, 0);

		nullres goodRun = nullcRun();
		if(goodRun)
		{
			const char* val = nullcGetResult();
			printf("\r\n%s\r\n", val);
		}else{
			printf("Execution failed: %s\r\n", nullcGetRuntimeError());
		}
		delete[] bytecode;
	}

	delete[] fileContent;

	nullcDeinit();
}
