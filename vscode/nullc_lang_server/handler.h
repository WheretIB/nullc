#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include "rapidjson/fwd.h"

struct Context;
struct Document;

void UpdateDiagnostics(Context& ctx, Document &document);

bool HandleMessage(Context& ctx, char *message, unsigned length);
bool HandleMessage(Context& ctx, unsigned idNumber, const char *idString, const char *method, rapidjson::Value& arguments);
bool HandleNotification(Context& ctx, const char *method, rapidjson::Value& arguments);
