#include <limits>

#include "coords.h"
#include "bounds.h"

Bounds::Bounds() {
	reset();
}

bool Bounds::contains(const coord_t p[3]) const {
	return p[0] >= _min[0] && p[0] <= _max[0] &&
		p[1] >= _min[1] && p[1] <= _max[1] &&
		p[2] >= _min[2] && p[2] <= _max[2];
}

void Bounds::recenter() {
	// The center point is midway between the extremes
	_center[0] = (_min[0] + _max[0]) / 2;
	_center[1] = (_min[1] + _max[1]) / 2;
	_center[2] = (_min[2] + _max[2]) / 2;
	// The range is half the distance between the extremes
	_range[0] = (_max[0] - _min[0]) / 2;
	_range[1] = (_max[1] - _min[1]) / 2;
	_range[2] = (_max[2] - _min[2]) / 2;
}

void Bounds::reset() {
	_min[0] = _min[1] = _min[2] = MAX_COORD;
	_max[0] = _max[1] = _max[2] = MIN_COORD;
	_center[0] = _center[1] = _center[2] = ZERO_COORD;
	_range[0] = _range[1] = _range[2] = UNIT_COORD;
}

void Bounds::update(const coord_t p[3]) {
	if (p[0] < _min[0]) { _min[0] = p[0]; }
	if (p[1] < _min[1]) { _min[1] = p[1]; }
	if (p[2] < _min[2]) { _min[2] = p[2]; }
	if (p[0] > _max[0]) { _max[0] = p[0]; }
	if (p[1] > _max[1]) { _max[1] = p[1]; }
	if (p[2] > _max[2]) { _max[2] = p[2]; }
}
