#include <cmath>

#pragma warning(push, 0)
#include <FL/Fl.H>
#pragma warning(pop)

#include "utils.h"
#include "algebra.h"
#include "color-maps.h"
#include "input-parser.h"
#include "progress-dialog.h"
#include "soma.h"
#include "brain-model.h"
#include "voltages.h"

const firings_instance_t Voltages::NO_FIRINGS;

Voltages::Voltages(const Brain_Model *bm, size32_t nc) : Sim_Data(bm, new Thermal_Map()), _num_active_somas(0),
	_active_somas(NULL), _voltages(NULL), _firings(NULL) {
	_num_cycles = nc;
}

Voltages::~Voltages() {
	_num_active_somas = 0;
	delete [] _active_somas;
	delete [] _voltages;
	delete [] _firings;
}

size32_t Voltages::active_soma_relative_index(size32_t index) const {
	for (size32_t i = 0; i < _num_active_somas; i++) {
		if (_active_somas[i] == index) {
			return i;
		}
	}
	return NULL_INDEX;
}

const firings_instance_t &Voltages::firings_relative(size32_t i) const {
	return i < _num_active_somas ? _firings[i] : NO_FIRINGS;
}

float Voltages::scale(float v) const { // -100 <= v <= 120
	float s = 0.0f;
	if (v < -70.0f) {
		s = (v + 100.0f) / 300.0f;
	}
	else if (v < -40.0f) {
		s = (v + 70.0f) / 60.0f + 0.1f;
	}
	else {
		s = (v + 40.0f) / 400.0f + 0.6f;
	}
	return s;
}

float Voltages::quantity(float s) const { // 0 <= s <= 1
	float v = 0.0f;
	if (s < 0.1f) {
		v = s * 300.0f - 100.0f;
	}
	else if (s < 0.6f) {
		v = (s - 0.1f) * 60.0f - 70.0f;
	}
	else {
		v = (s - 0.6f) * 400.0f - 40.0f;
	}
	return v;
}

void Voltages::color(size32_t index, const Soma_Type *, float *cv, bool invert) const {
	const float *bgcv = invert ? INVERT_INACTIVE_SOMA_COLOR : INACTIVE_SOMA_COLOR;
	size32_t i = active_soma_relative_index(index);
	if (!active_relative(i)) {
		cv[0] = bgcv[0]; cv[1] = bgcv[1]; cv[2] = bgcv[2];
		return;
	}
	float v = voltage_relative(i);
	float s = scale(v);
	_color_map->map(s, cv);
}

Read_Status Voltages::read_from(Input_Parser &ip, Progress_Dialog *p) {
	size_t denom = 1;
	// Get the file name and size
	_filename = ip.filename();
	_filesize = ip.filesize();
	// Get the number of somas
	size32_t somas_in_model = ip.get_size32();
	if (somas_in_model != num_somas()) { return WRONG_NUM_SOMAS; }
	// Get the number of active somas
	_num_active_somas = ip.get_size32();
	/////////////////////////////////////////////////////if (!_num_active_somas) { return NO_ACTIVE_SOMAS; }
	// Initialize the array of active soma IDs
	delete [] _active_somas;
	_active_somas = _num_active_somas > 0 ? new(std::nothrow) size32_t[_num_active_somas] : NULL;
	if (_active_somas == NULL && _num_active_somas > 0) { return NO_MEMORY; }
	// Get each active soma ID
	for (size32_t i = 0; i < _num_active_somas; i++) {
		size32_t id = ip.get_size32();
		// False positive "C6001: Using uninitialized memory" error with Visual Studio 2013 code analysis
		size32_t index = _model->soma_index(id);
		if (index >= num_somas()) { return BAD_SOMA_ID; }
		_active_somas[i] = index;
	}
	// Get the number of cycles
	size32_t cycles_in_data = ip.get_size32();
	if (cycles_in_data != _num_cycles) { return WRONG_NUM_CYCLES; }
	// Prepare to show voltage-parsing progress
	if (p) {
		denom = _num_cycles / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing voltages...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of voltages
	delete [] _voltages;
	_voltages = _num_active_somas > 0 ? new(std::nothrow) float[_num_cycles * _num_active_somas]() : NULL;
	delete [] _firings;
	_firings = _num_active_somas > 0 ? new(std::nothrow) firings_instance_t[_num_active_somas]() : NULL;
	if ((_voltages == NULL || _firings == NULL) && _num_active_somas > 0) { return NO_MEMORY; }
	// Get each cycle
	for (size32_t i = 0; i < _num_cycles; i++) {
		// A line defining a cycle is formatted as:
		//     cycle_id soma_1_voltage soma_1_spike_kind soma_2_voltage soma_2_spike_kind ... soma_n_voltage soma_n_spike_kind
		size32_t t = ip.get_size32();
		size32_t r = _num_active_somas * t;
		for (size32_t j = 0; j < _num_active_somas; j++) {
			_voltages[r + j] = ip.get_float();
			_firings[j][t] = (Firing_State)ip.get_size8();
		}
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_cycles);
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
