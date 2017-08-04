#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
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
#include "firing-spikes.h"

const float Firing_Spikes::FADE_ALPHA = 0.96f;

Firing_Spikes::Firing_Spikes(const Brain_Model *bm) : Sim_Data(bm, new Rainbow_Map()), _timescale(1.0f),
	_spike_counts(NULL), _fade_strengths(NULL), _suppression_strengths(NULL), _spikes(NULL) {
	size32_t n = num_somas();
	_spike_counts = new(std::nothrow) size16_t[n]();
	_fade_strengths = new(std::nothrow) float[n]();
	_suppression_strengths = new(std::nothrow) size8_t[n]();
}

Firing_Spikes::~Firing_Spikes() {
	delete [] _spike_counts;
	delete [] _fade_strengths;
	delete [] _suppression_strengths;
	delete [] _spikes;
}

size32_t Firing_Spikes::start_time(size32_t t) {
	size32_t prev_time = _time;
	size32_t new_time = Sim_Data::start_time(t);
	if (new_time == prev_time) { return new_time; }
	for (size32_t i = 0; i < num_somas(); i++) {
		_spike_counts[i] = 0;
		_fade_strengths[i] = 0.0f;
		_suppression_strengths[i] = 0;
	}
	const spikes_instance_t &firing = _spikes[_time];
	for (spikes_instance_t::const_iterator f = firing.begin(); f != firing.end(); ++f) {
		size32_t i = f->first;
		if (f->second & SUPPRESSED) {
			_suppression_strengths[i] = SUPPRESSION_DURATION;
		}
		else {
			_spike_counts[i] = 1;
			_fade_strengths[i] = 1.0f;
			_suppression_strengths[i] = 0;
		}
	}
	return new_time;
}

size32_t Firing_Spikes::step_time() {
	size32_t prev_time = _time;
	size32_t new_time = Sim_Data::step_time();
	if (new_time == prev_time) { return new_time; }
	// Decay fade and suppression strengths for all somas
	for (size32_t i = 0; i < num_somas(); i++) {
		_fade_strengths[i] *= FADE_ALPHA;
		if (_suppression_strengths[i] > 0) { _suppression_strengths[i]--; }
	}
	// Increment spike counts for somas fired now
	const spikes_instance_t &firing = _spikes[_time];
	for (spikes_instance_t::const_iterator f = firing.begin(); f != firing.end(); ++f) {
		size32_t i = f->first;
		if (f->second & SUPPRESSED) {
			_suppression_strengths[i] = SUPPRESSION_DURATION;
		}
		else {
			_spike_counts[i]++;
			_fade_strengths[i] = 1.0f;
			_suppression_strengths[i] = 0;
		}
	}
	if (_time - _start_time >= WINDOW_SIZE) {
		// Decrement spike counts for somas fired just outside the window
		const spikes_instance_t &fired = _spikes[_time - WINDOW_SIZE];
		for (spikes_instance_t::const_iterator f = fired.begin(); f != fired.end(); ++f) {
			if (f->second & UNSUPPRESSED) {
				_spike_counts[f->first]--;
			}
		}
	}
	return new_time;
}

bool Firing_Spikes::firing_or_suppressing(size32_t index) const {
	spikes_instance_t::const_iterator it = _spikes[_time].find(index);
	return it != _spikes[_time].end();
}

bool Firing_Spikes::firing(size32_t index) const {
	spikes_instance_t::const_iterator it = _spikes[_time].find(index);
	return it != _spikes[_time].end() && (it->second & UNSUPPRESSED);
}

bool Firing_Spikes::suppressing(size32_t index) const {
	spikes_instance_t::const_iterator it = _spikes[_time].find(index);
	return it != _spikes[_time].end() && (it->second & SUPPRESSED);
}

float Firing_Spikes::hertz(size32_t index) const {
	return _spike_counts[index] * _timescale * 1000.0f / MIN(WINDOW_SIZE, MAX(_time - _start_time + 1, MIN_WINDOW_SIZE));
}

float Firing_Spikes::scale(float hz) const { // 1 <= hz <= 1000
	float s = 0.0f;
	if (hz >= 1.0f) {
		float nlhz = (float)log10(hz) / 3.0f;
		s = (float)pow(nlhz, 1.25f);
	}
	return s;
}

float Firing_Spikes::quantity(float s) const { // 0 <= s <= 1
	float hz = 1.0f;
	if (s > 0.0f) {
		hz = pow(1000.0f, pow(s, 0.8f));
	}
	return hz;
}

void Firing_Spikes::color(size32_t index, const Soma_Type *, float *cv, bool invert) const {
	const float *incv = invert ? INVERT_INACTIVE_SOMA_COLOR : INACTIVE_SOMA_COLOR;
	if (active(index)) {
		float strength = 0.0f;
		if (_suppression_strengths[index] > 0) {
			// Color suppressing somas purple
			float intensity = (incv[0] + incv[1] + incv[2]) / 3.0f;
			if (intensity < 0.5f) {
				cv[0] = 0.21875f; cv[1] = 0.0f; cv[2] = 0.15625f;
			}
			else {
				cv[0] = 1.0f; cv[1] = 0.6875f; cv[2] = 0.875f;
			}
			strength = (float)_suppression_strengths[index] / SUPPRESSION_DURATION;
		}
		else {
			// Color firing somas by frequency
			float hz = hertz(index);
			float s = scale(hz);
			_color_map->map(s, cv);
			strength = _fade_strengths[index];
		}
		cv[0] = cv[0] * strength + incv[0] * (1.0f - strength);
		cv[1] = cv[1] * strength + incv[1] * (1.0f - strength);
		cv[2] = cv[2] * strength + incv[2] * (1.0f - strength);
	}
	else {
		// Color inactive somas an inactive color
		cv[0] = incv[0]; cv[1] = incv[1]; cv[2] = incv[2];
	}
}

void Firing_Spikes::bright_color(size32_t index, const Soma_Type *, float *cv, bool invert) const {
	const float *incv = invert ? INVERT_INACTIVE_SOMA_COLOR : INACTIVE_SOMA_COLOR;
	if (active(index)) {
		if (_suppression_strengths[index] > 0) {
			// Color suppressing somas purple
			float intensity = (incv[0] + incv[1] + incv[2]) / 3.0f;
			if (intensity < 0.5f) {
				cv[0] = 0.21875f; cv[1] = 0.0f; cv[2] = 0.15625f;
			}
			else {
				cv[0] = 1.0f; cv[1] = 0.6875f; cv[2] = 0.875f;
			}
		}
		else {
			// Color firing somas by frequency
			float hz = hertz(index);
			float s = scale(hz);
			_color_map->map(s, cv);
		}
	}
	else {
		// Color inactive somas an inactive color
		cv[0] = incv[0]; cv[1] = incv[1]; cv[2] = incv[2];
	}
}

Read_Status Firing_Spikes::read_from(Input_Parser &ip, Progress_Dialog *p) {
	if (_spike_counts == NULL || _fade_strengths == NULL) { return NO_MEMORY; }
	size_t denom = 1;
	// Get the file name and size
	_filename = ip.filename();
	_filesize = ip.filesize();
	// Get the version number (optional)
	size32_t version = 1;
	if (ip.peek() == 'v') {
		ip.get_char();
		version = ip.get_size32();
	}
	if (version < 1 || version > 2) { return BAD_VERSION; }
	// Get the microseconds per cycle
	size32_t micros_per_cycle = ip.get_size32();
	_timescale = 1000.0f / micros_per_cycle;
	// Get the number of somas
	size32_t somas_in_model = ip.get_size32();
	if (somas_in_model != num_somas()) { return WRONG_NUM_SOMAS; }
	// Get the number of cycles
	_num_cycles = ip.get_size32();
	if (!_num_cycles) { return NO_CYCLES; }
	// Prepare to show spike-parsing progress
	if (p) {
		denom = _num_cycles / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing firing spikes...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of firing spikes
	delete [] _spikes;
	_spikes = new(std::nothrow) spikes_instance_t[_num_cycles];
	if (_spikes == NULL) { return NO_MEMORY; }
	// Get each cycle
	for (size32_t i = 0; i < _num_cycles; i++) {
		// A line defining a cycle is formatted as:
		//     cycle_id num_spikes soma_id_1 soma_id_2 ... soma_id_n
		// Or in version 2 files, as:
		//     cycle_id num_spikes soma_id_1 spike_state_1 soma_id_2 spike_state_2 ... soma_id_n spike_state_n
		size32_t t = ip.get_size32();
		spikes_instance_t &firing_somas = _spikes[t];
		size32_t num_spikes = ip.get_size32();
		for (size32_t j = 0; j < num_spikes; j++) {
			size32_t id = ip.get_size32();
			Firing_State state = version >= 2 ? (Firing_State)ip.get_size32() : NORMAL;
			if (state == NOTHING) { continue; }
			size32_t index = _model->soma_index(id);
			if (index >= num_somas()) { return BAD_SOMA_ID; }
			firing_somas[index] = state;
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
