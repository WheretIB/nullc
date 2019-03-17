#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include "external/rapidjson/fwd.h"

struct Context;

void SendEventInitialized(Context& ctx);
void SendEventProcess(Context& ctx);

struct ThreadEventData;
void SendEventThread(Context& ctx, const ThreadEventData& data);

void SendEventTerminated(Context& ctx);
void SendEventExited(Context& ctx, int exitCode);

struct OutputEventData;
void SendEventOutput(Context& ctx, const OutputEventData& data);

struct StoppedEventData;
void SendEventStopped(Context& ctx, const StoppedEventData& data);

bool HandleMessage(Context& ctx, char *json, unsigned length);
