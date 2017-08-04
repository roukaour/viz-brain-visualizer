#ifndef WEIGHTS_H
#define WEIGHTS_H

#include "sim-data.h"

typedef std::pair<int16_t, int16_t> weight_pair_t;
typedef std::map<size32_t, weight_pair_t> weights_instance_t;

class Input_Parser;

class Weights : public virtual Sim_Data {
private:
	weights_instance_t *_weights;
	float _center, _spread;
public:
	Weights(const Brain_Model *bm, size32_t nc);
	~Weights();
	inline float center(void) const { return _center; }
	inline void center(float c) { _center = c; }
	inline float spread(void) const { return _spread; }
	inline void spread(float s) { _spread = s; }
	inline const weights_instance_t &weight_changes(void) const { return _weights[_time]; }
	size32_t prev_change_time(void) const;
	size32_t prev_change_time(size32_t index) const;
	size32_t next_change_time(void) const;
	size32_t next_change_time(size32_t index) const;
	inline virtual bool active(size32_t) const { return true; }
	inline float weight_before(size32_t index) const { return weights(index).first / 100.0f; }
	inline float weight_after(size32_t index) const { return weights(index).second / 100.0f; }
	virtual void color(size32_t index, const Soma_Type *t, float *cv, bool invert) const;
	virtual float scale(float w) const;
	virtual float quantity(float s) const;
	void synapse_color(size32_t index, float *cv, bool after) const;
	Read_Status read_from(Input_Parser &ip, Progress_Dialog *p);
	Read_Status read_prunings_from(Input_Parser &ip, Progress_Dialog *p);
private:
	inline const weight_pair_t weights(size32_t index) const { return _weights[_time].find(index)->second; }
	void rescale(float min_w, float max_w);
};

#endif
