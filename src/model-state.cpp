#include <cstdlib>
#include <fstream>

#pragma warning(push, 0)
#include <FL/Fl.H>
#pragma warning(pop)

#include "algebra.h"
#include "bounds.h"
#include "input-parser.h"
#include "soma.h"
#include "brain-model.h"
#include "model-state.h"

// Zoom factors get multiplied by a 45-degree field of view
const double Model_State::MIN_ZOOM = 0.0001; // 0.0045 degrees
const double Model_State::MAX_ZOOM = 4.0;    // 180 degrees

Model_State::Model_State() : _rotate_matrix(), _pan_vector(), _zoom_factor(1.0), _pivot(), _clip_volume(),
	_clipped(false), _bounds(), _num_selected(0), _selected(), _selected_indices(), _marked_syns() {
	reset();
}

void Model_State::rotate(const double m[16]) {
	for (int i = 0; i < 16; i++) {
		_rotate_matrix[i] = m[i];
	}
}

void Model_State::zoom(double z) {
	_zoom_factor = z;
	if (_zoom_factor < MIN_ZOOM) { _zoom_factor = MIN_ZOOM; }
	else if (_zoom_factor > MAX_ZOOM) { _zoom_factor = MAX_ZOOM; }
}

void Model_State::reclip(Bounds &b) {
	_pan_vector[0] = _pan_vector[1] = 0.0;
	_zoom_factor = 1.0;
	_clipped = true;
	b.recenter();
	bound(b);
}

bool Model_State::is_selected(const Soma *s) const {
	for (size_t i = 0; i < _num_selected; i++) {
		if (_selected[i] == s) {
			return true;
		}
	}
	return false;
}

bool Model_State::select(const Soma *s, size32_t index) {
	if (_num_selected >= MAX_SELECTED) { return false; }
	_selected[_num_selected] = s;
	_selected_indices[_num_selected] = index;
	_num_selected++;
	return true;
}

bool Model_State::deselect(const Soma *s) {
	for (size_t i = 0; i < _num_selected; i++) {
		if (_selected[i] == s) {
			_num_selected--;
			for (size_t j = i; j < _num_selected; j++) {
				_selected[j] = _selected[j + 1];
			}
			return true;
		}
	}
	return false;
}

bool Model_State::is_marked(const Synapse *y, size32_t index) const {
	marked_syn_t p = std::make_pair(y, index);
	return _marked_syns.find(p) != _marked_syns.end();
}

bool Model_State::mark(const Synapse *y, size32_t index) {
	size_t n = _marked_syns.size();
	_marked_syns.emplace(y, index);
	return _marked_syns.size() > n;
}

bool Model_State::unmark(const Synapse *y, size32_t index) {
	size_t n = _marked_syns.size();
	marked_syn_t p = std::make_pair(y, index);
	_marked_syns.erase(p);
	return _marked_syns.size() < n;
}

void Model_State::reset() {
	identity_matrix(_rotate_matrix);
	_pan_vector[0] = _pan_vector[1] = 0.0;
	_zoom_factor = 1.0;
	_pivot[0] = _pivot[1] = _pivot[2] = ZERO_COORD;
	_clipped = false;
	_bounds.reset();
	deselect_all();
	unmark_all();
}

Read_Status Model_State::read_from(Input_Parser &ip, const Brain_Model &bm) {
	// Get the rotation matrix
	for (int i = 0; i < 16; i++) {
		_rotate_matrix[i] = ip.get_double();
	}
	// Get the pan vector
	_pan_vector[0] = ip.get_double(); _pan_vector[1] = ip.get_double();
	// Get the zoom factor
	_zoom_factor = ip.get_double();
	// Get the pivot point
	_pivot[0] = ip.get_coord(); _pivot[1] = ip.get_coord(); _pivot[2] = ip.get_coord();
	// Get the clipping planes
	_clipped = ip.get_bool();
	double v[4];
	v[0] = ip.get_double(); v[1] = ip.get_double(); v[2] = ip.get_double(); v[3] = ip.get_double();
	_clip_volume.top(v);
	v[0] = ip.get_double(); v[1] = ip.get_double(); v[2] = ip.get_double(); v[3] = ip.get_double();
	_clip_volume.right(v);
	v[0] = ip.get_double(); v[1] = ip.get_double(); v[2] = ip.get_double(); v[3] = ip.get_double();
	_clip_volume.bottom(v);
	v[0] = ip.get_double(); v[1] = ip.get_double(); v[2] = ip.get_double(); v[3] = ip.get_double();
	_clip_volume.left(v);
	// Get the clipping center and range
	coord_t p[3];
	_bounds.reset();
	p[0] = ip.get_coord(); p[1] = ip.get_coord(); p[2] = ip.get_coord();
	_bounds.update(p);
	p[0] = ip.get_coord(); p[1] = ip.get_coord(); p[2] = ip.get_coord();
	_bounds.update(p);
	_bounds.recenter();
	// Get the selected soma IDs
	_num_selected = ip.get_size32();
	for (size_t i = 0; i < _num_selected; i++) {
		size32_t index = bm.soma_index(ip.get_size32());
		if (index >= bm.num_somas()) { return BAD_SOMA_ID; }
		_selected[i] = bm.soma(index);
		_selected_indices[i] = index;
	}
	// Get the marked synapse indexes
	size_t n = ip.get_size32();
	for (size_t i = 0; i < n; i++) {
		size32_t index = ip.get_size32();
		_marked_syns.emplace(bm.synapse(index), index);
	}
	return SUCCESS;
}

void Model_State::write_to(std::ofstream &ofs) const {
	ofs.imbue(std::locale("C"));
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(16);
	// Write the rotation matrix
	ofs << "# rotation matrix (column-major order)\n";
	for (int i = 0; i < 16; i++) {
		ofs << _rotate_matrix[i];
		if (!((i + 1) % 4)) { ofs << "\n"; }
		else if (i < 15) { ofs << " "; }
	}
	// Write the pan vector
	ofs << "# pan vector\n";
	ofs << _pan_vector[0] << " " << _pan_vector[1] << "\n";
	// Write the zoom factor
	ofs << "# zoom factor\n";
	ofs << _zoom_factor << "\n";
	// Write the pivot point
	ofs << "# pivot point\n";
	ofs << _pivot[0] << " " << _pivot[1] << " " << _pivot[2] << "\n";
	// Write the clipping planes
	ofs << "# clipping planes\n";
	ofs << (_clipped ? 1 : 0) << "\n";
	const double *v = _clip_volume.top();
	ofs << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << " # top\n";
	v = _clip_volume.right();
	ofs << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << " # right\n";
	v = _clip_volume.bottom();
	ofs << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << " # bottom\n";
	v = _clip_volume.left();
	ofs << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << " # left\n";
	// Write the clipping min and max
	ofs << "# clipping min and max\n";
	const coord_t *p = _bounds.min();
	ofs << p[0] << " " << p[1] << " " << p[2] << "\n";
	p = _bounds.max();
	ofs << p[0] << " " << p[1] << " " << p[2] << "\n";
	// Write the selected soma IDs
	ofs << "# selected somas\n";
	ofs << _num_selected << " # number of selected somas\n";
	for (size_t i = 0; i < _num_selected; i++) {
		ofs << _selected[i]->id();
		if (i == _num_selected - 1) { ofs << "\n"; }
		else { ofs << " "; }
	}
	// Write the marked synapse indexes
	ofs << "# marked synapses\n";
	size_t n = _marked_syns.size();
	ofs << n << " # number of marked synapses\n";
	for (marked_syns_t::const_iterator it = _marked_syns.begin(); it != _marked_syns.end(); ++it) {
		ofs << it->second << "\n";
	}
}
