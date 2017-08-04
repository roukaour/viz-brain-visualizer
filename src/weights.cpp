#include <cmath>
#include <map>
#include <utility>

#pragma warning(push, 0)
#include <FL/Fl.H>
#pragma warning(pop)

#include "utils.h"
#include "algebra.h"
#include "soma-type.h"
#include "color.h"
#include "color-maps.h"
#include "input-parser.h"
#include "progress-dialog.h"
#include "brain-model.h"
#include "weights.h"

Weights::Weights(const Brain_Model *bm, size32_t nc) : Sim_Data(bm, new Opposed_Map()), _weights(NULL),
	_center(0.0f), _spread(0.0f) {
	_num_cycles = nc;
}

Weights::~Weights() {
	delete [] _weights;
}

size32_t Weights::prev_change_time(void) const {
	for (size32_t i = _time; i > 0;) {
		weights_instance_t &wc = _weights[--i];
		if (!wc.empty()) { return i; }
	}
	return _time;
}

size32_t Weights::prev_change_time(size32_t index) const {
	for (size32_t i = _time; i > 0;) {
		weights_instance_t &wc = _weights[--i];
		for (weights_instance_t::const_iterator it = wc.begin(); it != wc.end(); ++it) {
			size32_t y_index = it->first;
			const Synapse *y = _model->synapse(y_index);
			if (y->axon_soma_index() == index || y->den_soma_index() == index) { return i; }
		}
	}
	return _time;
}

size32_t Weights::next_change_time(void) const {
	for (size32_t i = _time; i < _num_cycles - 1;) {
		weights_instance_t &wc = _weights[++i];
		if (!wc.empty()) { return i; }
	}
	return _time;
}

size32_t Weights::next_change_time(size32_t index) const {
	for (size32_t i = _time; i < _num_cycles - 1;) {
		weights_instance_t &wc = _weights[++i];
		for (weights_instance_t::const_iterator it = wc.begin(); it != wc.end(); ++it) {
			size32_t y_index = it->first;
			const Synapse *y = _model->synapse(y_index);
			if (y->axon_soma_index() == index || y->den_soma_index() == index) { return i; }
		}
	}
	return _time;
}

float Weights::scale(float w) const { // -163.84 <= w <= 163.83
	float nlw, limit;
	if (fabs(_spread) < EPSILON) {
		nlw = log1p(fabs(w - _center));
		limit = log(164.84f);
	}
	else {
		nlw = (pow(fabs(w - _center) + 1.0f, _spread) - 1.0f) / _spread;
		limit = (pow(164.84f, _spread) - 1.0f) / _spread;
	}
	if (w < _center) { nlw *= -1; }
	float s = (nlw + limit) / (2.0f * limit);
	return s;
}

float Weights::quantity(float s) const { // 0 <= s <= 1
	float as = s;
	if (s < 0.5f) { as = 1.0f - as; }
	float limit, nlw, wmc;
	if (fabs(_spread) < EPSILON) {
		limit = log(164.84f);
		nlw = as * 2.0f * limit - limit;
		wmc = expm1(nlw);
	}
	else {
		limit = (pow(164.84f, _spread) - 1.0f) / _spread;
		nlw = as * 2.0f * limit - limit;
		wmc = pow(nlw * _spread + 1.0f, 1.0f / _spread) - 1.0f;
	}
	if (s < 0.5f) { wmc *= -1; }
	float w = wmc + _center;
	return w;
}

void Weights::color(size32_t, const Soma_Type *t, float *cv, bool) const {
	t->color()->rgb(cv);
}

void Weights::synapse_color(size32_t index, float *cv, bool after) const {
	float w = after ? weight_after(index) : weight_before(index);
	float s = scale(w);
	_color_map->map(s, cv);
}

void Weights::rescale(float min_w, float max_w) {
	if (max_w <= min_w) {
		_center = _spread = 0.0f;
		return;
	}
	_center = floor((min_w + max_w) / 2.0f + 0.5f);
	float range = MAX(max_w - _center, _center - min_w);
	// Can't easily solve for _spread given range, so just test all values for _spread
	// (range <= ((cap * 2 - 1) * 164.84^spread + (2 - cap * 2))^(1 / spread) - 1; cap == 0.95)
	for (_spread = -3.0f; _spread <= 3.0f; _spread += 0.1f) {
		if (range <= quantity(0.95f) - _center) { break; }
	}
	if (_spread > 2.0f) { _spread = 2.0f; }
}

Read_Status Weights::read_from(Input_Parser &ip, Progress_Dialog *p) {
	size_t denom = 1;
	// Get the file name and size
	_filename = ip.filename();
	_filesize = ip.filesize();
	// Get the total number of somas
	size32_t somas_in_model = ip.get_size32();
	if (somas_in_model != num_somas()) { return WRONG_NUM_SOMAS; }
	// Get the total number of synapses
	size32_t synapses_in_model = ip.get_size32();
	if (synapses_in_model != _model->num_synapses()) { return WRONG_NUM_SYNAPSES; }
	// Get the number of cycles
	size32_t cycles_in_data = ip.get_size32();
	if (cycles_in_data != _num_cycles) { return WRONG_NUM_CYCLES; }
	// Initialize the array of weight changes
	delete [] _weights;
	_weights = new(std::nothrow) weights_instance_t[_num_cycles]();
	if (_weights == NULL) { return NO_MEMORY; }
	// Approximate number of changes (assuming 40 characters per line)
	size_t approx_num_changes = ip.filesize() / 40;
	// Prepare to show weight-parsing progress
	if (p) {
		denom = approx_num_changes / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing weights...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the weight range
	int16_t min_w = (std::numeric_limits<int16_t>::max)();
	int16_t max_w = (std::numeric_limits<int16_t>::min)();
	// Get each weight change
	size_t num_weights = 0;
	for (size_t i = 0; ip.good() && !ip.done(); i++) {
		// A line defining a weight change is formatted as:
		//     cycle_id synapse_init_index axon_id den_id synapse_runsim_id weight_before weight_after
		size16_t t = ip.get_size16();
		size32_t y_index = ip.get_size32();
		ip.get_size32(); // axonal soma ID
		ip.get_size32(); // dendritic soma ID
		ip.get_size32(); // RUNSIM synapse ID
		int16_t w_before = ip.get_int16();
		int16_t w_after = ip.get_int16();
		_weights[t][y_index] = std::make_pair(w_before, w_after);
		num_weights++;
		// Keep track of weight range
		if (w_before < min_w) { min_w = w_before; }
		if (w_before > max_w) { max_w = w_before; }
		if (w_after < min_w) { min_w = w_after; }
		if (w_after > max_w) { max_w = w_after; }
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / approx_num_changes);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	rescale(min_w / 100.0f, max_w / 100.0f);
	if (p) {
		p->progress(1.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	return SUCCESS;
}

Read_Status Weights::read_prunings_from(Input_Parser &ip, Progress_Dialog *p) {
	size_t denom = 1;
	// Get the file name and size
	_filename = ip.filename();
	_filesize = ip.filesize();
	// Get the total number of somas
	size32_t somas_in_model = ip.get_size32();
	if (somas_in_model != num_somas()) { return WRONG_NUM_SOMAS; }
	// Get the total number of synapses
	size32_t synapses_in_model = ip.get_size32();
	if (synapses_in_model != _model->num_synapses()) { return WRONG_NUM_SYNAPSES; }
	// Get the number of cycles
	size32_t cycles_in_data = ip.get_size32();
	if (cycles_in_data != _num_cycles) { return WRONG_NUM_CYCLES; }
	// Approximate number of changes (assuming 40 characters per line)
	size_t approx_num_changes = ip.filesize() / 40;
	// Prepare to show weight-parsing progress
	if (p) {
		denom = approx_num_changes / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing prunings...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Get each weight change
	for (size_t i = 0; ip.good() && !ip.done(); i++) {
		// A line defining a weight change is formatted as:
		//     cycle_id synapse_init_index axon_id den_id synapse_runsim_id weight_before weight_after
		size16_t t = ip.get_size16();
		size32_t y_index = ip.get_size32();
		ip.get_size32(); // axonal soma ID
		ip.get_size32(); // dendritic soma ID
		ip.get_size32(); // RUNSIM synapse ID
		int16_t w_before = ip.get_int16();
		int16_t w_after = ip.get_int16();
		if (_weights[t].find(y_index) == _weights[t].end()) {
			if (_weights[t][y_index].first == w_after) {
				_weights[t][y_index].first = w_before;
			}
			else if (_weights[t][y_index].second == w_before) {
				_weights[t][y_index].second = w_after;
			}
			else {
				_weights[t][y_index].first = w_before;
				_weights[t][y_index].second = w_after;
			}
		}
		else {
			_weights[t][y_index] = std::make_pair(w_before, w_after);
		}
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / approx_num_changes);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	if (p) {
		p->progress(1.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	return SUCCESS;
}
