#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "schema.h"

struct Context
{
	Context() = default;

	bool infoMode = false;
	bool debugMode = false;

	bool nullcInitialized = false;

	int seq = 1;

	InitializeRequestArguments initArgs;
	LaunchRequestArguments launchArgs;

	std::string defaultModulePath;
	std::string modulePath;

	std::mutex outputMutex;

	std::atomic<bool> running;

	std::mutex breakpointMutex;
	std::condition_variable breakpointWait;

	std::atomic<bool> breakpointActive;
	std::atomic<unsigned> breakpointLastModule;
	std::atomic<unsigned> breakpointLastLine;
	std::atomic<unsigned> breakpointAction;

	std::thread applicationThread;

	std::atomic<bool> pendingRestart;

	struct VariableReference
	{
		VariableReference() = default;
		VariableReference(char *ptr, unsigned type): ptr(ptr), type(type)
		{
			assert(ptr);
		}

		char *ptr = nullptr;
		unsigned type = 0;
	};

	std::vector<VariableReference> variableReferences;

	std::map<uintptr_t, std::vector<unsigned>> sourceBreakpoints;
};
