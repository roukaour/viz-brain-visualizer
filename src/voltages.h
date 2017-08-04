#ifndef VOLTAGES_H
#define VOLTAGES_H

#include <map>

#include "sim-data.h"
#include "utils.h"

typedef std::map<size32_t, Firing_State> firings_instance_t;

class Input_Parser;
class Progress_Dialog;

class Voltages : public virtual Sim_Data {
private:
	static const firings_instance_t NO_FIRINGS;
private:
	size32_t _num_active_somas;
	size32_t *_active_somas;
	float *_voltages;
	firings_instance_t *_firings;
public:
	Voltages(const Brain_Model *bm, size32_t nc);
	~Voltages();
	inline size32_t num_active_somas(void) const { return _num_active_somas; }
	inline size32_t active_soma_index(size32_t i) const { return _active_somas[i]; }
	size32_t active_soma_relative_index(size32_t index) const;
	inline virtual bool active(size32_t index) const { return active_relative(active_soma_relative_index(index)); }
	inline bool active_relative(size32_t i) const { return i < _num_active_somas; }
	inline float voltage(size32_t index) const { return voltage_relative(active_soma_relative_index(index)); }
	inline float voltage(size32_t index, size32_t t) const { return voltage_relative(active_soma_relative_index(index), t); }
	inline float voltage_relative(size32_t i) const { return voltage_relative(i, _time); }
	inline float voltage_relative(size32_t i, size32_t t) const { return _voltages[_num_active_somas * t + i]; }
	inline const firings_instance_t &firings(size32_t index) const { return firings_relative(active_soma_relative_index(index)); }
	const firings_instance_t &firings_relative(size32_t i) const;
	virtual float scale(float v) const;
	virtual float quantity(float s) const;
	virtual void color(size32_t index, const Soma_Type *t, float *cv, bool invert) const;
	Read_Status read_from(Input_Parser &ip, Progress_Dialog *p);
};

#endif
