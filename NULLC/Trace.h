#pragma once

#include "nullcdef.h"

#if defined(NULLC_NO_EXECUTOR)
#pragma detect_mismatch("NULLC_NO_EXECUTOR", "1")
#else
#pragma detect_mismatch("NULLC_NO_EXECUTOR", "0")
#endif

#if defined(NULLC_TIME_TRACE)
#pragma detect_mismatch("NULLC_TIME_TRACE", "1")
#else
#pragma detect_mismatch("NULLC_TIME_TRACE", "0")
#endif

#if !defined(NULLC_NO_EXECUTOR) && defined(NULLC_TIME_TRACE)

#include <assert.h>

#include "Output.h"

namespace NULLCTime
{
	void clockMicroInit();
	unsigned clockMicro();
}

namespace NULLC
{
	template<typename T>
	struct TraceArray
	{
		TraceArray(unsigned size)
		{
			count = 0;
			max = size;

			if(size)
				data = new T[size];
			else
				data = NULL;
		}

		~TraceArray()
		{
			delete[] data;
		}

		void Resize(unsigned size)
		{
			if(size <= max)
				return;

			// Only for empty arrays
			assert(count == 0);

			delete[] data;
			max = size;
			data = new T[size];
		}

		unsigned count;
		unsigned max;
		T *data;
	};

	struct TraceScopeToken
	{
		TraceScopeToken()
		{
			category = "";
			name = "";
		}

		TraceScopeToken(const char *category, const char *name) : category(category), name(name)
		{
		}

		const char *category;
		const char *name;
	};

	struct TraceEvent
	{
		unsigned isEnter : 1;
		unsigned isLabel : 1;
		unsigned token : 30;

		unsigned ts;
	};

	void TraceDump();

	struct TraceContext
	{
		TraceContext(): scopeTokens(1024), labels(8192), events(8192), outputBuf(0), tempBuf(0)
		{
			outputEnabled = false;

			needComma = false;
			openEvent = false;
			depth = 0;
			outputDepth = 0;
		}

		~TraceContext()
		{
			TraceDump();

			if(output.stream)
			{
				output.Print(']');
				output.Flush();

				output.closeStream(output.stream);
				output.stream = NULL;
			}
		}

		OutputContext output;

		bool outputEnabled;

		bool needComma;
		bool openEvent;
		unsigned depth;
		unsigned outputDepth;

		TraceArray<TraceScopeToken> scopeTokens;
		TraceArray<char> labels;

		TraceArray<TraceEvent> events;

		TraceArray<char> outputBuf;
		TraceArray<char> tempBuf;
	};

	inline TraceContext* TraceGetContext()
	{
		static TraceContext context;

		NULLCTime::clockMicroInit();

		return &context;
	}

	extern TraceContext *traceContext;

	inline void TraceSetEnabled(bool enabled)
	{
		TraceContext &context = *traceContext;

		if(context.outputEnabled == enabled)
			return;

		if(enabled)
		{
			context.outputEnabled = true;

			// Clear what was already recorded
			context.events.count = 0;
			context.labels.count = 0;

			if(!context.output.stream)
			{
				context.outputBuf.Resize(32768);
				context.tempBuf.Resize(1024);

				context.output.stream = context.output.openStream("trace.json");

				context.output.outputBuf = context.outputBuf.data;
				context.output.outputBufSize = context.outputBuf.max;

				context.output.tempBuf = context.tempBuf.data;
				context.output.tempBufSize = context.tempBuf.max;

				context.output.Print('[');

				context.labels.Resize(262144);
				context.events.Resize(131072);
			}
		}
		else
		{
			context.outputEnabled = false;

			TraceDump();

			context.output.Flush();
		}
	}

	inline unsigned TraceGetToken(const char *category, const char *name)
	{
		TraceContext &context = *traceContext;

		assert(context.scopeTokens.count < context.scopeTokens.max);

		if(context.scopeTokens.count < context.scopeTokens.max)
			context.scopeTokens.data[context.scopeTokens.count] = TraceScopeToken(category, name);

		return context.scopeTokens.count++;
	}

	inline unsigned TraceGetDepth()
	{
		TraceContext &context = *traceContext;

		return context.depth;
	}

	inline double TraceEnter(unsigned token, unsigned &eventPos, unsigned &labelPos)
	{
		TraceContext &context = *traceContext;

		if(context.events.count == context.events.max)
			TraceDump();

		TraceEvent &traceEvent = context.events.data[context.events.count];

		traceEvent.isEnter = true;
		traceEvent.isLabel = false;
		traceEvent.token = token;
		traceEvent.ts = NULLCTime::clockMicro();

		context.depth++;

		eventPos = context.events.count++;
		labelPos = context.labels.count;

		return traceEvent.ts;
	}

	inline void TraceLeave(double ts, unsigned eventPos, unsigned labelPos)
	{
		TraceContext &context = *traceContext;

		if(context.events.count == context.events.max)
			TraceDump();

		unsigned currTs = NULLCTime::clockMicro();

		if(context.events.count > eventPos && context.events.data[eventPos].ts == ts)
		{
			TraceEvent &enterEvent = context.events.data[eventPos];

			assert(context.labels.count >= labelPos);
			assert(enterEvent.isEnter);

			if(currTs - enterEvent.ts < 50)
			{
				context.events.count = eventPos;
				context.labels.count = labelPos;

				assert(context.depth != 0);
				context.depth--;

				return;
			}
		}

		TraceEvent &traceEvent = context.events.data[context.events.count++];

		traceEvent.isEnter = false;
		traceEvent.isLabel = false;
		traceEvent.token = 0;
		traceEvent.ts = currTs;

		assert(context.depth != 0);
		context.depth--;
	}

	inline void TraceLeaveTo(unsigned depth)
	{
		TraceContext &context = *traceContext;

		while(context.depth > depth)
			TraceLeave(0.0, 0, 0);
	}

	inline void TraceLabel(const char *str)
	{
		TraceContext &context = *traceContext;

		unsigned count = unsigned(strlen(str)) + 1;

		if(context.labels.count + count >= context.labels.max || context.events.count == context.events.max)
			TraceDump();

		assert(count < context.labels.max);

		unsigned token = context.labels.count;

		memcpy(&context.labels.data[context.labels.count], str, count);
		context.labels.count += count;

		TraceEvent &traceEvent = context.events.data[context.events.count++];

		traceEvent.isEnter = false;
		traceEvent.isLabel = true;
		traceEvent.token = token;
		traceEvent.ts = 0;
	}

	inline void TraceLabel(const char *begin, const char *end)
	{
		TraceContext &context = *traceContext;

		unsigned count = unsigned(end - begin);

		if(!count)
			return;

		if(context.labels.count + count + 1 >= context.labels.max || context.events.count == context.events.max)
			TraceDump();

		assert(count + 1 < context.labels.max);

		unsigned token = context.labels.count;

		memcpy(&context.labels.data[context.labels.count], begin, count);
		context.labels.data[context.labels.count + count] = 0;
		context.labels.count += count + 1;

		TraceEvent &traceEvent = context.events.data[context.events.count++];

		traceEvent.isEnter = false;
		traceEvent.isLabel = true;
		traceEvent.token = token;
		traceEvent.ts = 0;
	}

	struct TraceScope
	{
		TraceScope(unsigned token)
		{
			ts = TraceEnter(token, lastEventPos, lastLabelPos);
		}

		~TraceScope()
		{
			TraceLeave(ts, lastEventPos, lastLabelPos);
		}

		double ts;
		unsigned lastEventPos;
		unsigned lastLabelPos;
	};

	inline void TraceDump()
	{
		TraceContext &context = *traceContext;

		if(!context.outputEnabled)
		{
			context.events.count = 0;
			context.labels.count = 0;
			return;
		}

		unsigned currentLabel = 0;

		for(unsigned i = 0; i < context.events.count; i++)
		{
			TraceEvent &traceEvent = context.events.data[i];

			if(traceEvent.isEnter)
			{
				TraceScopeToken &token = context.scopeTokens.data[traceEvent.token];

				if(context.openEvent)
				{
					if(currentLabel != 0)
					{
						context.output.Printf("}");
						currentLabel = 0;
					}

					context.output.Printf("},\n");
				}
				else if(context.needComma)
				{
					context.output.Printf(",\n");
					context.needComma = false;
				}

				context.openEvent = false;

				context.outputDepth++;

				if(context.outputDepth > 32)
					continue;

				context.output.Printf("{\"ph\":\"B\",\"ts\":%d,\"pid\":1,\"tid\":1,\"name\":\"%s\",\"cat\":\"%s\"", traceEvent.ts, token.name, token.category);

				context.openEvent = true;
			}
			else if(traceEvent.isLabel)
			{
				if(context.outputDepth > 32)
					continue;

				if(currentLabel == 0)
					context.output.Printf(",\"args\":{");
				else
					context.output.Printf(",");

				context.output.Printf("\"label %d\":\"%s\"", currentLabel, &context.labels.data[traceEvent.token]);
				currentLabel++;
			}
			else
			{
				if(context.openEvent)
				{
					if(currentLabel != 0)
					{
						context.output.Printf("}");
						currentLabel = 0;
					}

					context.output.Printf("},\n");
				}
				else if(context.needComma)
				{
					context.output.Printf(",\n");
					context.needComma = false;
				}

				context.openEvent = false;

				if(context.outputDepth > 32)
				{
					context.outputDepth--;
					continue;
				}

				context.output.Printf("{\"ph\":\"E\",\"ts\":%d,\"pid\":1,\"tid\":1}", traceEvent.ts);

				context.needComma = true;

				context.outputDepth--;
			}
		}

		if(context.openEvent)
		{
			if(currentLabel != 0)
			{
				context.output.Printf("}");
				currentLabel = 0;
			}

			context.output.Printf("}");

			context.needComma = true;
		}

		context.openEvent = false;

		context.events.count = 0;
		context.labels.count = 0;
	}
}

#define TRACE_SCOPE(category, name) static unsigned token = NULLC::TraceGetToken(category, name); NULLC::TraceScope traceScope(token)
#define TRACE_LABEL(str) NULLC::TraceLabel(str)
#define TRACE_LABEL2(begin, end) NULLC::TraceLabel(begin, end)

#else

namespace NULLC
{
	struct TraceContext;

	inline TraceContext* TraceGetContext()
	{
		return NULL;
	}

	inline void TraceSetEnabled(bool enabled)
	{
		(void)enabled;
	}

	inline unsigned TraceGetDepth()
	{
		return 0;
	}

	inline void TraceLeaveTo(unsigned depth)
	{
		(void)depth;
	}

	inline void TraceDump()
	{
	}
}

#define TRACE_SCOPE(category, name) (void)0
#define TRACE_LABEL(str) (void)0
#define TRACE_LABEL2(begin, end) (void)0

#endif
