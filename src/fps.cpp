#include <stdlib.h>

#include "fps.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

const float FPS::MEAN_ALPHA = 0.9f;

FPS::FPS() : _mean_elapsed_ticks(1.0f), _ticks_per_second(0), _start_tick(0) {
#ifdef _WIN32
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	_ticks_per_second = (size_t)freq.QuadPart;
#else
	_ticks_per_second = 1000000;
#endif
}

void FPS::start() {
#ifdef _WIN32
	LARGE_INTEGER start_counter;
	QueryPerformanceCounter(&start_counter);
	_start_tick = (size_t)start_counter.QuadPart;
#else
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	_start_tick = (size_t)(start_time.tv_sec * 1000000 + start_time.tv_usec);
#endif
}

void FPS::stop() {
#ifdef _WIN32
	LARGE_INTEGER stop_counter;
	QueryPerformanceCounter(&stop_counter);
	size_t stop_tick = (size_t)stop_counter.QuadPart;
#else
	struct timeval stop_time;
	gettimeofday(&stop_time, NULL);
	size_t stop_tick = (size_t)(stop_time.tv_sec * 1000000 + stop_time.tv_usec);
#endif
	size_t elapsed_ticks = stop_tick - _start_tick;
	_mean_elapsed_ticks = MEAN_ALPHA * _mean_elapsed_ticks + (1.0f - MEAN_ALPHA) * elapsed_ticks;
	if (_mean_elapsed_ticks < 1.0f) { _mean_elapsed_ticks = 1.0f; }
}
