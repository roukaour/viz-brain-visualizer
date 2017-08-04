#ifndef FIRING_SPIKES_H
#define FIRING_SPIKES_H

#include <map>

#include "utils.h"
#include "algebra.h"
#include "sim-data.h"

typedef std::map<size32_t, Firing_State> spikes_instance_t;

class Progress_Dialog;
class Input_Parser;

class Firing_Spikes : public virtual Sim_Data {
private:
	static const size32_t WINDOW_SIZE = 1000;
	static const size32_t MIN_WINDOW_SIZE = 5;
	static const float FADE_ALPHA;
	static const size8_t SUPPRESSION_DURATION = 10;
private:
	float _timescale;
	size16_t *_spike_counts;
	float *_fade_strengths;
	size8_t *_suppression_strengths;
	spikes_instance_t *_spikes;
public:
	Firing_Spikes(const Brain_Model *bm);
	~Firing_Spikes();
	inline float timescale(void) const { return _timescale; }
	virtual size32_t start_time(size32_t t);
	virtual size32_t step_time(void);
	inline virtual bool active(size32_t index) const { return _spike_counts[index] || _suppression_strengths[index]; }
	bool firing_or_suppressing(size32_t index) const;
	bool firing(size32_t index) const;
	bool suppressing(size32_t index) const;
	float hertz(size32_t index) const;
	virtual float scale(float hz) const;
	virtual float quantity(float s) const;
	virtual void color(size32_t index, const Soma_Type *t, float *cv, bool invert) const;
	virtual void bright_color(size32_t index, const Soma_Type *t, float *cv, bool invert) const;
	Read_Status read_from(Input_Parser &ip, Progress_Dialog *p);
};

#endif
