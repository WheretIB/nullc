#pragma once

#include "Array.h"

struct CompilerStatistics
{
	CompilerStatistics()
	{
		startTime = 0;
		finishTime = 0;

		timers.push_back(Timer("Lexer", "lexer"));
		timers.push_back(Timer("Parse", "parse"));

		timers.push_back(Timer("Import", "import"));
		timers.push_back(Timer("Expressions", "analyze"));
		timers.push_back(Timer("Finalization", "(finalize)"));
		timers.push_back(Timer("Cleanup", "(cleanup)"));

		timers.push_back(Timer("IrCodeGen", "codegen"));
		timers.push_back(Timer("LlvmCodeGen", "llvm"));
		timers.push_back(Timer("IrOptimization", "optimization"));
		timers.push_back(Timer("IrFinalization", "(finalize)"));
		timers.push_back(Timer("IrLowering", "lower"));
		timers.push_back(Timer("Bytecode", "bytecode"));
		timers.push_back(Timer("BytecodeCache", "(cache)"));

		timers.push_back(Timer("Logging", "logs"));
		timers.push_back(Timer("Tracing", "trace"));
		timers.push_back(Timer("Extra", "extra"));
	}

	void Start(unsigned timeMicros)
	{
		assert(startTime == 0);

		startTime = timeMicros;
		finishTime = 0;
	}

	void Finish(const char *keyName, unsigned timeMicros)
	{
		assert(startTime);

		finishTime = timeMicros;

		InplaceStr key = InplaceStr(keyName);

		for(unsigned i = 0; i < timers.size(); i++)
		{
			if(timers[i].keyName == key)
			{
				timers[i].total += timeMicros - startTime;

				startTime = 0;
				return;
			}
		}

		assert(!"unknown timer");
	}

	void Add(CompilerStatistics& other)
	{
		for(unsigned i = 0; i < timers.size(); i++)
			timers[i].total += other.timers[i].total;

		assert(startTime == 0);
		finishTime = 0;
	}

	unsigned Total()
	{
		unsigned total = 0;

		for(unsigned i = 0; i < timers.size(); i++)
			total += timers[i].total;

		return total;
	}

	unsigned startTime;
	unsigned finishTime;

	struct Timer
	{
		Timer()
		{
			total = 0;
		}

		Timer(const char *keyName, const char *outputName)
		{
			this->keyName = InplaceStr(keyName);
			this->outputName = InplaceStr(outputName);
			this->total = 0;
		}

		InplaceStr keyName;
		InplaceStr outputName;
		unsigned total;
	};

	SmallArray<Timer, 16> timers;
};
