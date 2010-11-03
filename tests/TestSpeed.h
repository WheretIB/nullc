#pragma once

#ifndef _DEBUG
	#define SPEED_TEST
	#define SPEED_TEST_EXTRA
#endif

void RunSpeedTests();
void SpeedTestText(const char* name, const char* text);
