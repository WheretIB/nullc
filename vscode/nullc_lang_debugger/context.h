#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

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

	std::string modulePath;

	std::mutex outputMutex;

	std::atomic<bool> running;

	std::mutex breakpointMutex;
	std::condition_variable breakpointWait;

	std::thread applicationThread;
};
