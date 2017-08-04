#ifndef BOUNDS_H
#define BOUNDS_H

#include "algebra.h"
#include "coords.h"

class Bounds {
private:
	coord_t _min[3], _max[3];
	coord_t _center[3], _range[3];
public:
	Bounds();
	inline const coord_t *min(void) const { return _min; }
	inline const coord_t *max(void) const { return _max; }
	inline const coord_t *center(void) const { return _center; }
	inline const coord_t *range(void) const { return _range; }
	inline coord_t max_range(void) const { return MAX(_range[0], MAX(_range[1], _range[2])); }
	bool contains(const coord_t p[3]) const;
	void recenter(void);
	void reset(void);
	void update(const coord_t p[3]);
};

#endif
