#pragma once

bool nullcInitIOModule(void *context, unsigned (*writeFunc)(void *context, char *data, unsigned length), unsigned (*readFunc)(void *context, char *target, unsigned length));
bool nullcInitIOModule();
