#include <assert.h>
#include <stdio.h>

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <string>
#include <vector>

#include "../../NULLC/nullc.h"

#include "context.h"
#include "handler.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

int main(int argc, char **argv)
{
	Context ctx;

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0)
		{
			ctx.infoMode = true;
			ctx.debugMode = true;
		}
		else if(strncmp(argv[i], "--module_path=", strlen("--module_path=")) == 0)
		{
			ctx.defaultModulePath = argv[i] + strlen("--module_path=");
		}
		else if(strncmp(argv[i], "-mp=", strlen("-mp=")) == 0)
		{
			ctx.defaultModulePath = argv[i] + strlen("--mp=");
		}
	}

#if defined(_WIN32)
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif

	if(ctx.infoMode)
	{
		fprintf(stderr, "INFO: Launching\n");
		fprintf(stderr, "INFO: Default module path: %s\n", ctx.defaultModulePath.c_str());
	}

	std::vector<char> header;
	std::vector<char> message;

	unsigned expectedLength = 0;

	char ch = 0;

	while(std::cin.get(ch))
	{
		// Header field ends with \r\n
		if(ch == '\r')
		{
			if(!std::cin.get(ch))
				break;

			if(ch != '\n')
			{
				fprintf(stderr, "ERROR: Expected '\\n' after '\\r' after the header\r\n");
				return 1;
			}

			// If buffer is empty that means headers have ended and we can read the message
			if(header.empty())
			{
				message.resize(expectedLength + 1);

				std::cin.read(message.data(), expectedLength);

				if(ctx.debugMode)
					fprintf(stderr, "DEBUG: Received message '%.*s'\r\n", (int)expectedLength, message.data());

				message.back() = 0;

				if(!HandleMessage(ctx, message.data(), expectedLength))
					return 1;

				expectedLength = 0;
			}
			else
			{
				header.push_back(0);

				if(strstr(header.data(), "Content-Length") == header.data())
				{
					const char *pos = strstr(header.data(), ": ");

					if(!pos)
					{
						fprintf(stderr, "ERROR: Content-Length is not followed by ': '\r\n");
						return 1;
					}

					pos += 2;

					expectedLength = strtoul(pos, nullptr, 10);
					
					header.clear();
				}
				else if(strstr(header.data(), "Content-Type") == header.data())
				{
					// Don't care about encoding, we support all defined by spec
					header.clear();
				}
				else
				{
					fprintf(stderr, "WARNING: Unknown header '%.*s'\r\n", (int)header.size(), header.data());
					header.clear();
				}
			}
		}
		else
		{
			header.push_back(ch);
		}
	}

	if(ctx.nullcInitialized)
		nullcTerminate();
}
