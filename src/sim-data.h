#ifndef ACTIVE_SET_H
#define ACTIVE_SET_H

#include <iostream>

#include "utils.h"
#include "from-file.h"

class Color_Map;
class Soma_Type;
class Brain_Model;

enum Firing_State { NOTHING = 0x0, NORMAL = 0x1, FORCED = 0x2, BINARY = 0x4, UNSUPPRESSED = 0x7, SUPPRESSED = 0x8,
	PEAK = 0xF0, PEAK_NORMAL = 0xF1, PEAK_FORCED = 0xF2, PEAK_BINARY = 0xF4, PEAK_UNSUPPRESSED = 0xF7, PEAK_SUPPRESSED = 0xF8 };

class Sim_Data : public virtual From_File {
public:
	static const float INACTIVE_SOMA_COLOR[3], INVERT_INACTIVE_SOMA_COLOR[3];
protected:
	const Brain_Model *_model;
	size32_t _num_cycles;
	size32_t _start_time, _time;
	const Color_Map *_color_map;
public:
	Sim_Data(const Brain_Model *bm, const Color_Map *m);
	virtual ~Sim_Data();
	size32_t num_somas(void) const;
	inline size32_t num_cycles(void) const { return _num_cycles; }
	inline size32_t max_time(void) const { return _num_cycles - 1; }
	inline size32_t const_start_time(void) const { return _start_time; }
	inline virtual size32_t start_time(size32_t t) { if (t < _num_cycles) { _start_time = _time = t; } return _time; }
	inline size32_t time(void) const { return _time; }
	inline virtual size32_t step_time(void) { if (_time < _num_cycles - 1) { _time++; } return _time; }
	inline size32_t duration(void) const { return _time - _start_time + 1; }
	inline const Color_Map *color_map(void) const { return _color_map; }
	virtual bool active(size32_t index) const = 0;
	virtual float scale(float q) const = 0;
	virtual float quantity(float s) const = 0;
	virtual void color(size32_t index, const Soma_Type *t, float *cv, bool invert) const = 0;
};

#endif
