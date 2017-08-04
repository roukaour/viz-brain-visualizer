#include <iostream>

#include "color-maps.h"
#include "brain-model.h"
#include "sim-data.h"

const float Sim_Data::INACTIVE_SOMA_COLOR[3] = {0.2f, 0.2f, 0.2f}; // dark gray
const float Sim_Data::INVERT_INACTIVE_SOMA_COLOR[3] = {0.8f, 0.8f, 0.8f}; // light gray

Sim_Data::Sim_Data(const Brain_Model *bm, const Color_Map *m) : From_File(), _model(bm), _num_cycles(0),
	_start_time(0), _time(0), _color_map(m) {}

Sim_Data::~Sim_Data() {
	_model = NULL;
	_num_cycles = 0;
	_start_time = _time = 0;
	delete _color_map;
}

size32_t Sim_Data::num_somas(void) const { return _model->num_somas(); }
