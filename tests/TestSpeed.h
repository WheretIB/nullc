#pragma once

#if !defined(_DEBUG) && !defined(ANDROID)
	#define SPEED_TEST
	#define SPEED_TEST_EXTRA
#endif

void RunSpeedTests();
void SpeedTestText(const char* name, const char* text);
