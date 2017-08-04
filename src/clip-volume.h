#ifndef CLIP_VOLUME_H
#define CLIP_VOLUME_H

#include "coords.h"

class Clip_Volume {
public:
	static void enable(void);
	static void disable(void);
private:
	// The four clipping planes' A, B, C, and D coefficients (Ax + By + Cz + D = 0 defines a plane)
	double _top[4], _right[4], _bottom[4], _left[4];
public:
	Clip_Volume();
	inline const double *top(void) const { return _top; }
	inline void top(const double *v) { _top[0] = v[0]; _top[1] = v[1]; _top[2] = v[2]; _top[3] = v[3]; }
	inline const double *right(void) const { return _right; }
	inline void right(const double *v) { _right[0] = v[0]; _right[1] = v[1]; _right[2] = v[2]; _right[3] = v[3]; }
	inline const double *bottom(void) const { return _bottom; }
	inline void bottom(const double *v) { _bottom[0] = v[0]; _bottom[1] = v[1]; _bottom[2] = v[2]; _bottom[3] = v[3]; }
	inline const double *left(void) const { return _left; }
	inline void left(const double *v) { _left[0] = v[0]; _left[1] = v[1]; _left[2] = v[2]; _left[3] = v[3]; }
	void specify(void) const;
	void acquire(void);
	void copy(const Clip_Volume &v);
	void reset(void);
	bool contains(const coord_t c[3]) const;
};

#endif
