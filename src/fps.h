#ifndef FPS_H
#define FPS_H

#include <cstdlib>

class FPS {
private:
	static const float MEAN_ALPHA;
private:
	float _mean_elapsed_ticks;
	size_t _ticks_per_second;
	size_t _start_tick;
public:
	FPS();
	void start(void);
	void stop(void);
	inline size_t fps(void) const { return (size_t)(_ticks_per_second / _mean_elapsed_ticks); }
	inline size_t spf(void) const { return (size_t)(_mean_elapsed_ticks / _ticks_per_second); }
};

#endif
