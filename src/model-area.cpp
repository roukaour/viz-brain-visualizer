#include <cmath>
#include <cerrno>
#include <vector>
#include <deque>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <iostream>
#include <fstream>
#include <sstream>

#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#include <FL/glut.H>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "image.h"
#include "bounds.h"
#include "color.h"
#include "color-maps.h"
#include "algebra.h"
#include "viz-window.h"
#include "brain-model.h"
#include "sim-data.h"
#include "firing-spikes.h"
#include "voltages.h"
#include "weights.h"
#include "overview-area.h"
#include "progress-dialog.h"
#include "waiting-dialog.h"
#include "model-state.h"
#include "widgets.h"
#include "model-area.h"

const float Model_Area::BACKGROUND_COLOR[3] = {0.0f, 0.0f, 0.0f}; // black
const float Model_Area::INVERT_BACKGROUND_COLOR[3] = {1.0f, 1.0f, 1.0f}; // white

const float Model_Area::AXON_SYN_COLOR[3] = {1.0f, 0.0f, 0.0f}; // red
const float Model_Area::DEN_SYN_COLOR[3] = {0.67f, 0.0f, 0.67f}; // purple
const float Model_Area::MARKED_SYN_COLOR[3] = {1.0f, 0.5f, 0.0f}; // orange

const float Model_Area::GAP_JUNCTION_COLOR[3] = {1.0f, 1.0f, 0.0f}; // yellow

const float Model_Area::X_AXIS_COLOR[3] = {1.0f, 0.0f, 0.0f}; // red
const float Model_Area::Y_AXIS_COLOR[3] = {0.0f, 1.0f, 0.0f}; // green
const float Model_Area::Z_AXIS_COLOR[3] = {0.0f, 0.5f, 1.0f}; // blue

const double Model_Area::FOV_Y = 45.0;

const double Model_Area::NEAR_PLANE = 0.5;
const double Model_Area::FAR_PLANE = 12.0;

const double Model_Area::FOCAL_LENGTH = 5.0;

const double Model_Area::PAN_SCALE = 2.25;
const double Model_Area::ZOOM_SCALE = 6.0;

const double Model_Area::ROTATION_GUIDE_SCALE = 1.0;

const float Model_Area::SELECT_TOLERANCE = 12.0f;

const double Model_Area::ROTATE_AXIS_TOLERANCE = 0.2;

Model_Area::Model_Area(int x, int y, int w, int h, const char *l) : Fl_Gl_Window(x, y, w, h, l), _model(),
	_overview_area(NULL), _dnd_receiver(NULL), _state(), _prev_state(), _saved_state(), _history(MAX_HISTORY),
	_future(MAX_HISTORY), _draw_opts(), _fps(), _opened(false), _initialized(false), _dragging(false),
	_click_coords(), _drag_coords(), _rotation_mode(ARCBALL_3D), _scale_rotation(false), _invert_zoom(false) {
	mode(FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);
	action(SELECT);
	resizable(NULL);
	end();
}

void Model_Area::action(Model_Area::Action a) {
	type((uchar)a);
	if (Fl::event_inside(this)) {
		refresh_cursor();
	}
}

const Sim_Data *Model_Area::active_sim_data() const {
	const Sim_Data *sd = NULL;
	if (!_draw_opts.show_inactive_somas()) {
		if (_model.has_weights() && _draw_opts.display() == Draw_Options::WEIGHTS) {
			sd = _model.const_weights();
		}
		else if (_model.has_voltages() && _draw_opts.display() == Draw_Options::VOLTAGES) {
			sd = _model.const_voltages();
		}
		else if (_model.has_firing_spikes() && _draw_opts.display() == Draw_Options::FIRING_SPIKES) {
			sd = _model.const_firing_spikes();
		}
	}
	return sd;
}

void Model_Area::refresh() {
	if (!context_valid()) { return; }
	invalidate();
	redraw();
	flush();
}

void Model_Area::refresh_gl() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl_font(FL_SCREEN, 12); // fix font-drawing bug in Windows
}

void Model_Area::refresh_selected() const {
	Viz_Window *vw = static_cast<Viz_Window *>(parent());
	vw->refresh_selected();
}

void Model_Area::refresh_cursor() const {
	if (!_model.num_somas()) {
		fl_cursor(FL_CURSOR_DEFAULT);
		return;
	}
	switch (type()) {
	case SELECT:
		fl_cursor(FL_CURSOR_HAND);
		return;
	case CLIP:
		fl_cursor(FL_CURSOR_CROSS);
		return;
	case ROTATE:
		fl_cursor(FL_CURSOR_WE);
		return;
	case PAN:
		fl_cursor(FL_CURSOR_MOVE);
		return;
	case ZOOM:
		fl_cursor(FL_CURSOR_NS);
		return;
	case MARK:
		fl_cursor(FL_CURSOR_ARROW);
	}
}

void Model_Area::refresh_projection(Model_Area::Mode mode) {
	if (_draw_opts.invert_background()) {
		glClearColor(INVERT_BACKGROUND_COLOR[0], INVERT_BACKGROUND_COLOR[1], INVERT_BACKGROUND_COLOR[2], 1.0f);
	}
	else {
		glClearColor(BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2], 1.0f);
	}
	glClearDepth(1.0);
	glViewport(0, 0, w(), h());
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (mode == CLIPPING) {
		double x, y, w, h;
		x = (double)(_click_coords[0] + _drag_coords[0]) / 2.0;
		y = (double)(_click_coords[1] + _drag_coords[1]) / 2.0;
		w = (double)abs(_drag_coords[0] - _click_coords[0]);
		h = (double)abs(_drag_coords[1] - _click_coords[1]);
		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		gluPickMatrix(x, y, w, h, viewport);
	}
	double aspect = (double)w() / h();
	double r = (double)_state.max_range();
	if (_draw_opts.orthographic()) {
		double top = tan(FOV_Y * PI / 180.0 * _state.zoom() / 2.0) * NEAR_PLANE * r * FOCAL_LENGTH * 2.0;
		double right = top * aspect;
		glOrtho(-right, right, -top, top, NEAR_PLANE * r, FAR_PLANE * r);
	}
	else {
		gluPerspective(FOV_Y * _state.zoom(), aspect, NEAR_PLANE * r, FAR_PLANE * r);
	}
	if (mode == DRAWING && _draw_opts.left_handed()) {
		glScaled(-1.0, 1.0, 1.0); // mirror image
	}
}

void Model_Area::refresh_view() {
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	const coord_t *p = _state.pivot();
	double r = (double)_state.max_range();
	gluLookAt((double)p[0], (double)p[1], (double)p[2] + FOCAL_LENGTH * r, (double)p[0], (double)p[1], (double)p[2],
		0.0, 1.0, 0.0); // look down the z-axis; y-axis points up
	glTranslated(_state.pan_x() * r, _state.pan_y() * r, 0.0);
	glTranslatef((float)p[0], (float)p[1], (float)p[2]);
	glMultMatrixd(_state.rotate());
	glTranslatef(-(float)p[0], -(float)p[1], -(float)p[2]);
	if (_draw_opts.only_show_clipped() && _state.clipped()) {
		Clip_Volume::enable();
		_state.clip_volume().specify();
	}
	else {
		Clip_Volume::disable();
	}
}

void Model_Area::clear() {
	_model.clear();
	_opened = false;
	prepare();
}

void Model_Area::prepare() {
	_state.reset();
	_state.bound(_model.bounds());
	prepare_for_model();
	_prev_state = _state;
	_saved_state = _state;
	_history.clear();
	_future.clear();
	_dragging = false;
	refresh_cursor();
	refresh();
}

void Model_Area::prepare_for_model() {
	if (_model.num_types() == 12 && _model.type(0)->letter() == 'P' && _model.type(1)->letter() == 'N'
		&& _model.type(2)->letter() == 'G' && _model.type(3)->letter() == 'B' && _model.type(4)->letter() == 'A'
		&& _model.type(5)->letter() == 'S' && _model.type(6)->letter() == 'T' && _model.type(7)->letter() == 'I'
		&& _model.type(8)->letter() == 'C' && _model.type(9)->letter() == 'M' && _model.type(10)->letter() == 'R'
		&& _model.type(11)->letter() == 'D') {
		// InputCat model
		double rm[16], m1[16], m2[16];
		rotate_x_matrix(HALF_PI, m1);
		rotate_z_matrix(HALF_PI, m2);
		matrix_mul(m2, m1, rm);
		_state.rotate(rm);
	}
	else if (_model.num_types() == 2 && _model.type(0)->letter() == 'U' && _model.type(1)->letter() == 'L') {
		// InputSimLearning model
		double rm[16];
		identity_matrix(rm);
		_state.rotate(rm);
	}
}

void Model_Area::remember(const Model_State &s) {
	_future.clear();
	while (_history.size() >= MAX_HISTORY) { _history.pop_front(); }
	_history.push_back(s);
}

void Model_Area::undo() {
	if (_history.empty()) { return; }
	while (_future.size() >= MAX_HISTORY) { _future.pop_front(); }
	_future.push_back(_state);
	_state = _history.back();
	_history.pop_back();
	refresh_selected();
	refresh();
}

void Model_Area::redo() {
	if (_future.empty()) { return; }
	while (_history.size() >= MAX_HISTORY) { _history.pop_front(); }
	_history.push_back(_state);
	_state = _future.back();
	_future.pop_back();
	refresh_selected();
	refresh();
}

void Model_Area::copy() {
	_saved_state = _state;
}

void Model_Area::paste() {
	remember(_state);
	_prev_state = _state;
	_state = _saved_state;
	refresh_selected();
	refresh();
}

void Model_Area::reset() {
	remember(_state);
	_prev_state = _state;
	_state.reset();
	_state.bound(_model.bounds());
	prepare_for_model();
	refresh_selected();
	refresh();
}

void Model_Area::repeat_action() {
	remember(_state);
	double pan_x = _state.pan_x() + _state.pan_x() - _prev_state.pan_x();
	double pan_y = _state.pan_y() + _state.pan_y() - _prev_state.pan_y();
	double zoom = _state.zoom() * _state.zoom() / _prev_state.zoom();
	double rotate[16], p_inv[16], delta[16], q[4];
	const double *r = _state.rotate();
	const double *p = _prev_state.rotate();
	matrix_transpose(p, p_inv); // transpose of a rotation matrix is its inverse
	matrix_mul(r, p_inv, delta);
	matrix_mul(delta, r, rotate);
	matrix_to_quat(rotate, q);
	normalize_quat(q);
	quat_to_matrix(q, rotate);
	_prev_state = _state;
	_state.pan(pan_x, pan_y);
	_state.zoom(zoom);
	_state.rotate(rotate);
	refresh();
}

void Model_Area::snap_to_axes() {
	remember(_state);
	const double *r = _state.rotate();
	// Snappable axes
	double axes[6][3] = {
		{1.0, 0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 0.0, 1.0},
		{-1.0, 0.0, 0.0},
		{0.0, -1.0, 0.0},
		{0.0, 0.0, -1.0}
	};
	// Rotated +x, +y, and +z vectors
	double vectors[3][3] = {
		{r[0], r[4], r[8]},
		{r[1], r[5], r[9]},
		{r[2], r[6], r[10]}
	};
	// Find closest vector-axis pair
	size_t iv = 0, ia = 0;
	double min_d = 4.0;
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 6; j++) {
			double d = vector_distance(vectors[i], axes[j]);
			if (d < min_d) {
				iv = i;
				ia = j;
				min_d = d;
			}
		}
	}
	// Align vector with axis
	double aa[4];
	vector_cross(vectors[iv], axes[ia], aa);
	aa[3] = acos(vector_dot(vectors[iv], axes[ia]));
	normalize_axis_angle(aa);
	double align[16];
	axis_angle_to_matrix(aa, align);
	double r2[16];
	matrix_mul(align, r, r2);
	// Rotated +x, +y, and +z vectors
	double vectors2[3][3] = {
		{r2[0], r2[4], r2[8]},
		{r2[1], r2[5], r2[9]},
		{r2[2], r2[6], r2[10]}
	};
	// Find closest vector-axis pair
	size_t iv2 = 0;
	min_d = 4.0;
	for (size_t i = 0; i < 3; i++) {
		if (i == iv) { continue; }
		for (size_t j = 0; j < 6; j++) {
			double d = vector_distance(vectors2[i], axes[j]);
			if (d < min_d) {
				iv2 = i;
				ia = j;
				min_d = d;
			}
		}
	}
	// Align vector with axis
	vector_cross(vectors2[iv2], axes[ia], aa);
	aa[3] = acos(vector_dot(vectors2[iv2], axes[ia]));
	normalize_axis_angle(aa);
	axis_angle_to_matrix(aa, align);
	double rotate[16];
	matrix_mul(align, r2, rotate);
	_state.rotate(rotate);
	refresh();
}

void Model_Area::pivot_ith(size_t i) {
	remember(_state);
	_prev_state = _state;
	if (i < _state.num_selected()) {
		const Soma *s = _state.selected(i);
		_state.pivot(s->coords());
	}
	else {
		_state.pivot(_state.center());
	}
	_state.pan_x(0.0); _state.pan_y(0.0);
	refresh();
}

void Model_Area::select_top(int n) {
	remember(_state);
	_prev_state = _state;
	_state.deselect_all();
	std::unordered_set<size32_t> top;
	const Firing_Spikes *fd = _model.const_firing_spikes();
	size32_t n_somas = _model.num_somas();
	for (int i = 0; i < n; i++) {
		size32_t max_index = n_somas;
		float max_hz = -1.0f;
		for (size32_t j = 0; j < n_somas; j++) {
			const Soma *s = _model.soma(j);
			const Soma_Type *t = _model.type(s->type_index());
			if (t->display_state() == Soma_Type::DISABLED) { continue; }
			float hz = fd->hertz(j);
			if (hz > max_hz && top.find(j) == top.end()) {
				max_index = j;
				max_hz = hz;
			}
		}
		if (max_index < n_somas) {
			_state.select(_model.soma(max_index), max_index);
			top.insert(max_index);
		}
	}
	refresh_selected();
	refresh();
}

void Model_Area::select_id(size32_t id) {
	size32_t index = _model.soma_index(id);
	if (index >= _model.num_somas()) { return; }
	remember(_state);
	_prev_state = _state;
	const Soma *s = _model.soma(index);
	_state.reselect(s, index);
	refresh_selected();
	refresh();
}

void Model_Area::deselect_id(size32_t id) {
	size32_t index = _model.soma_index(id);
	if (index >= _model.num_somas()) { return; }
	remember(_state);
	_prev_state = _state;
	const Soma *s = _model.soma(index);
	_state.deselect(s);
	refresh_selected();
	refresh();
}

void Model_Area::deselect_ith(size_t i) {
	if (i >= _state.num_selected()) { return; }
	remember(_state);
	_prev_state = _state;
	const Soma *s = _state.selected(i);
	_state.deselect(s);
	refresh_selected();
	refresh();
}

size32_t Model_Area::select_syn_count(size8_t t_index, size_t y_count, bool count_den, Progress_Dialog *p) {
	size_t denom = 1;
	size32_t n = _model.num_somas();
	if (p) {
		denom = n / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Somas found: 0");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return 0; }
	}
	remember(_state);
	_prev_state = _state;
	size32_t n_found = 0;
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	if (count_den) {
		for (size32_t index = 0; index < n; index++) {
			const Soma *s = _model.soma(index);
			if (s->type_index() == t_index) {
				size_t d_count = 0;
				size32_t ny = _model.num_synapses();
				const Synapse *y = NULL;
				for (size32_t y_index = s->first_den_syn_index(); y_index < ny; y_index = y->next_den_syn_index()) {
					y = _model.synapse(y_index);
					d_count++;
				}
				if (d_count == y_count) {
					_state.reselect(s, index);
					n_found++;
					if (p) {
						ss.str("");
						ss << "Somas found: " << n_found;
						p->message(ss.str().c_str());
						Fl::check();
						if (p->canceled()) { return 0; }
					}
				}
			}
			if (p && !((index + 1) % denom)) {
				p->progress((float)(index + 1) / n);
				Fl::check();
				if (p->canceled()) { return n_found; }
			}
		}
	}
	else {
		for (size32_t index = 0; index < n; index++) {
			const Soma *s = _model.soma(index);
			if (s->type_index() == t_index) {
				size_t a_count = 0;
				size32_t ny = _model.num_synapses();
				const Synapse *y = NULL;
				for (size32_t y_index = s->first_axon_syn_index(); y_index < ny; y_index = y->next_axon_syn_index()) {
					y = _model.synapse(y_index);
					a_count++;
				}
				if (a_count == y_count) {
					_state.reselect(s, index);
					n_found++;
					if (p) {
						ss.str("");
						ss << "Somas found: " << n_found;
						p->message(ss.str().c_str());
						Fl::check();
						if (p->canceled()) { return 0; }
					}
				}
			}
			if (p && !((index + 1) % denom)) {
				p->progress((float)(index + 1) / n);
				Fl::check();
				if (p->canceled()) { return n_found; }
			}
		}
	}
	if (p) {
		p->progress(1.0f);
		Fl::check();
		if (p->canceled()) { return n_found; }
	}
	return n_found;
}

size32_t Model_Area::report_syn_count(std::ofstream &ofs, size8_t t_index, size_t y_count, bool count_den,
	Progress_Dialog *p) {
	size_t denom = 1;
	size32_t n = _model.num_somas();
	if (p) {
		denom = n / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Somas found: 0");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return 0; }
	}
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(0);
	ofs << "# " << _model.type(t_index)->name() << " somas with " << y_count << " " <<
		(count_den ? "dendritic" : "axonal") << " synapses:\n";
	ofs << "# <type> <id> <x> <y> <z>\n";
	remember(_state);
	_prev_state = _state;
	size32_t n_found = 0;
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	if (count_den) {
		for (size32_t index = 0; index < n; index++) {
			const Soma *s = _model.soma(index);
			if (s->type_index() == t_index) {
				size_t d_count = 0;
				size32_t ny = _model.num_synapses();
				const Synapse *y = NULL;
				for (size32_t y_index = s->first_den_syn_index(); y_index < ny; y_index = y->next_den_syn_index()) {
					y = _model.synapse(y_index);
					d_count++;
				}
				if (d_count == y_count) {
					const coord_t *c = s->coords();
					ofs << (size32_t)s->type_index() << " " << s->id() << " " << c[0] << " " << c[1] << " " << c[2] << "\n";
					n_found++;
					if (p) {
						ss.str("");
						ss << "Somas found: " << n_found;
						p->message(ss.str().c_str());
						Fl::check();
						if (p->canceled()) { return 0; }
					}
				}
			}
			if (p && !((index + 1) % denom)) {
				p->progress((float)(index + 1) / n);
				Fl::check();
				if (p->canceled()) { return n_found; }
			}
		}
	}
	else {
		for (size32_t index = 0; index < n; index++) {
			const Soma *s = _model.soma(index);
			if (s->type_index() == t_index) {
				size_t a_count = 0;
				size32_t ny = _model.num_synapses();
				const Synapse *y = NULL;
				for (size32_t y_index = s->first_axon_syn_index(); y_index < ny; y_index = y->next_axon_syn_index()) {
					y = _model.synapse(y_index);
					a_count++;
				}
				if (a_count == y_count) {
					const coord_t *c = s->coords();
					ofs << (size32_t)s->type_index() << " " << s->id() << " " << c[0] << " " << c[1] << " " << c[2] << "\n";
					n_found++;
					if (p) {
						ss.str("");
						ss << "Somas found: " << n_found;
						p->message(ss.str().c_str());
						Fl::check();
						if (p->canceled()) { return 0; }
					}
				}
			}
			if (p && !((index + 1) % denom)) {
				p->progress((float)(index + 1) / n);
				Fl::check();
				if (p->canceled()) { return n_found; }
			}
		}
	}
	ofs << "# Total: " << n_found << "\n";
	if (p) {
		p->progress(1.0f);
		Fl::check();
		if (p->canceled()) { return n_found; }
	}
	return n_found;
}

bool Model_Area::mark_conn_paths(size32_t a_index, size32_t d_index, size_t limit, bool include_disabled,
	std::deque<size32_t> &path_syns, size_t &n_paths, std::deque<soma_syn_t> &stack, Waiting_Dialog *w) {
	if (a_index == d_index) {
		for (std::deque<soma_syn_t>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
			path_syns.push_back(it->second);
		}
		n_paths++;
		if (w) {
			std::ostringstream ss;
			ss.imbue(std::locale(""));
			ss.setf(std::ios::fixed, std::ios::floatfield);
			ss << "Synapse paths found: " << n_paths;
			w->message(ss.str().c_str());
		}
		return true;
	}
	if (w) {
		Fl::check();
		if (w->canceled()) { return false; }
	}
	for (std::deque<soma_syn_t>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
		if (it->first == a_index) { return true; }
	}
	const Soma *a = _model.soma(a_index);
	if (!include_disabled) {
		const Soma_Type *t = _model.type(a->type_index());
		if (t->display_state() == Soma_Type::DISABLED) { return true; }
	}
	size32_t ny = _model.num_synapses();
	const Synapse *y = NULL;
	for (size32_t y_index = a->first_axon_syn_index(); y_index < ny; y_index = y->next_axon_syn_index()) {
		stack.push_back(std::make_pair(a_index, y_index));
		y = _model.synapse(y_index);
		size32_t b_index = y->den_soma_index();
		if (!mark_conn_paths(b_index, d_index, limit, include_disabled, path_syns, n_paths, stack, w)) {
			return false;
		}
		stack.pop_back();
		if (limit > 0 && n_paths >= limit) { break; }
	}
	return true;
}

size_t Model_Area::mark_conn_paths(size32_t a_id, size32_t d_id, size_t limit, bool include_disabled,
	Waiting_Dialog *w) {
	if (w) {
		w->canceled(false);
		w->message("Synapse paths found: 0");
	}
	size32_t a_index = _model.soma_index(a_id);
	size32_t d_index = _model.soma_index(d_id);
	size32_t n = _model.num_somas();
	if (a_index >= n || d_index >= n) { return 0; }
	std::deque<size32_t> path_syns;
	std::deque<soma_syn_t> stack;
	size_t n_paths = 0;
	mark_conn_paths(a_index, d_index, limit, include_disabled, path_syns, n_paths, stack, w);
	if (n_paths > 0) {
		remember(_state);
		_prev_state = _state;
		while (!path_syns.empty()) {
			size32_t index = path_syns.front();
			const Synapse *y = _model.synapse(index);
			_state.mark(y, index);
			path_syns.pop_front();
		}
		refresh_selected();
		refresh();
	}
	return n_paths;
}

bool Model_Area::select_conn_paths(size32_t a_index, size32_t d_index, size_t limit, bool include_disabled,
	std::deque<size32_t> &path_somas, size_t &n_paths, std::deque<size32_t> &stack, Waiting_Dialog *w) {
	if (a_index == d_index) {
		for (std::deque<size32_t>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
			path_somas.push_back(*it);
		}
		n_paths++;
		if (w) {
			std::ostringstream ss;
			ss.imbue(std::locale(""));
			ss.setf(std::ios::fixed, std::ios::floatfield);
			ss << "Synapse paths found: " << n_paths;
			w->message(ss.str().c_str());
		}
		return true;
	}
	if (w) {
		Fl::check();
		if (w->canceled()) { return false; }
	}
	if (std::find(stack.begin(), stack.end(), a_index) != stack.end()) { return true; }
	const Soma *a = _model.soma(a_index);
	if (!include_disabled) {
		const Soma_Type *t = _model.type(a->type_index());
		if (t->display_state() == Soma_Type::DISABLED) { return true; }
	}
	stack.push_back(a_index);
	size32_t ny = _model.num_synapses();
	const Synapse *y = NULL;
	for (size32_t y_index = a->first_axon_syn_index(); y_index < ny; y_index = y->next_axon_syn_index()) {
		y = _model.synapse(y_index);
		size32_t b_index = y->den_soma_index();
		if (!select_conn_paths(b_index, d_index, limit, include_disabled, path_somas, n_paths, stack, w)) {
			return false;
		}
		if (limit > 0 && n_paths >= limit) { break; }
	}
	stack.pop_back();
	return true;
}

size_t Model_Area::select_conn_paths(size32_t a_id, size32_t d_id, size_t limit, bool include_disabled,
	Waiting_Dialog *w) {
	if (w) {
		w->canceled(false);
		w->message("Synapse paths found: 0");
	}
	size32_t a_index = _model.soma_index(a_id);
	size32_t d_index = _model.soma_index(d_id);
	size32_t n = _model.num_somas();
	if (a_index >= n || d_index >= n) { return 0; }
	std::deque<size32_t> path_somas, stack;
	size_t n_paths = 0;
	path_somas.push_back(a_index);
	path_somas.push_back(d_index);
	select_conn_paths(a_index, d_index, limit, include_disabled, path_somas, n_paths, stack, w);
	if (n_paths > 0) {
		remember(_state);
		_prev_state = _state;
		while (!path_somas.empty()) {
			size32_t index = path_somas.front();
			const Soma *s = _model.soma(index);
			_state.reselect(s, index);
			path_somas.pop_front();
		}
		refresh_selected();
		refresh();
	}
	return n_paths;
}

bool Model_Area::report_conn_paths(std::ofstream &ofs, size32_t a_index, size32_t d_index, size_t limit,
	bool include_disabled, size_t &n_paths, std::deque<soma_syn_t> &stack, Waiting_Dialog *w) {
	if (a_index == d_index) {
		for (std::deque<soma_syn_t>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
			size32_t s_index = it->first;
			size32_t y_index = it->second;
			const Soma *s = _model.soma(s_index);
			const Soma_Type *t = _model.type(s->type_index());
			const Synapse *y = _model.synapse(y_index);
			const coord_t *c = y->coords();
			ofs << t->letter() << " #" << s->id() << " --(" << c[0] << ", " << c[1] << ", " << c[2] << ")-> ";
		}
		const Soma *d = _model.soma(d_index);
		const Soma_Type *u = _model.type(d->type_index());
		ofs << u->letter() << " #" << d->id() << "\n";
		n_paths++;
		if (w) {
			std::ostringstream ss;
			ss.imbue(std::locale(""));
			ss.setf(std::ios::fixed, std::ios::floatfield);
			ss << "Synapse paths found: " << n_paths;
			w->message(ss.str().c_str());
		}
		return true;
	}
	if (w) {
		Fl::check();
		if (w->canceled()) { return false; }
	}
	for (std::deque<soma_syn_t>::const_iterator it = stack.begin(); it != stack.end(); ++it) {
		if (it->first == a_index) { return true; }
	}
	const Soma *a = _model.soma(a_index);
	if (!include_disabled) {
		const Soma_Type *t = _model.type(a->type_index());
		if (t->display_state() == Soma_Type::DISABLED) { return true; }
	}
	size32_t ny = _model.num_synapses();
	const Synapse *y = NULL;
	for (size32_t y_index = a->first_axon_syn_index(); y_index < ny; y_index = y->next_axon_syn_index()) {
		stack.push_back(std::make_pair(a_index, y_index));
		y = _model.synapse(y_index);
		size32_t b_index = y->den_soma_index();
		if (!report_conn_paths(ofs, b_index, d_index, limit, include_disabled, n_paths, stack, w)) {
			return false;
		}
		stack.pop_back();
		if (limit > 0 && n_paths >= limit) { break; }
	}
	return true;
}

size_t Model_Area::report_conn_paths(std::ofstream &ofs, size32_t a_id, size32_t d_id, size_t limit,
	bool include_disabled, Waiting_Dialog *w) {
	if (w) {
		w->canceled(false);
		w->message("Synapse paths found: 0");
	}
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(0);
	size32_t a_index = _model.soma_index(a_id);
	size32_t d_index = _model.soma_index(d_id);
	size32_t n = _model.num_somas();
	if (a_index >= n || d_index >= n) {
		ofs << "Soma #" << a_id << " or #" << d_id << " does not exist\n";
		return 0;
	}
	ofs << "Synapse paths from soma #" << a_id << " to #" << d_id;
	if (limit > 0) { ofs << " (limit " << limit << ")"; }
	ofs << ":\n";
	std::deque<soma_syn_t> stack;
	size_t n_paths = 0;
	report_conn_paths(ofs, a_index, d_index, limit, include_disabled, n_paths, stack, w);
	ofs << "Total: " << n_paths << " paths\n";
	return n_paths;
}

Read_Status Model_Area::read_model_from(Input_Parser &ip, Progress_Dialog *p) {
	_opened = false;
	Read_Status status = _model.read_from(ip, p);
	if (p && p->canceled()) { status = CANCELED; }
	if (status == SUCCESS) { _opened = true; }
	return status;
}

Read_Status Model_Area::read_model_from(Binary_Parser &bp, Progress_Dialog *p) {
	_opened = false;
	Read_Status status = _model.read_from(bp, p);
	if (p && p->canceled()) { status = CANCELED; }
	if (status == SUCCESS) { _opened = true; }
	return status;
}

Read_Status Model_Area::read_state_from(Input_Parser &ip) {
	Model_State new_state;
	Read_Status status = new_state.read_from(ip, _model);
	if (status == SUCCESS) {
		remember(_state);
		_prev_state = _state;
		_state = new_state;
		refresh_selected();
		refresh();
	}
	return status;
}

int Model_Area::write_image(const char *f, Image::Format m) {
	make_current();
	Image *image = new(std::nothrow) Image(f, (size_t)w(), (size_t)h());
	if (image == NULL) { return ENOMEM; }
	image->write(m);
	int r = image->error();
	image->close();
	delete image;
	return r;
}

void Model_Area::write_selected_somas_to(std::ofstream &ofs, bool bounding_box, bool relations) const {
	size_t ns = _state.num_selected();
	Bounds b;
	for (size_t i = 0; i < ns; i++) {
		const Soma *s = _state.selected(i);
		const coord_t *c = s->coords();
		b.update(c);
	}
	size_t nr = ns;
	size32_t nm = _model.num_somas();
	for (size32_t index = 0; index < nm; index++) {
		const Soma *s = _model.soma(index);
		const coord_t *c = s->coords();
		if (b.contains(c) && !_state.is_selected(s)) { nr++; }
	}
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(0);
	ofs << "# " << _model.filename() << "\n";
	ofs << ns << " # number of selected somas\n";
	ofs << "# <type> <id> <x> <y> <z>\n";
	ofs << "# " << ns << " selected somas\n";
	ofs << "# (BOSS records voltages and weights of these and their parents+children)\n";
	for (size_t i = 0; i < ns; i++) {
		const Soma *s = _state.selected(i);
		const coord_t *c = s->coords();
		ofs << (size32_t)s->type_index() << " " << s->id() << " " << c[0] << " " << c[1] << " " << c[2] << "\n";
	}
	ofs << "# This line prevents BOSS from finding parents+children itself:\n";
	if (relations) {
		ofs << "# -1 0 0 0 0\n";
	}
	else {
		ofs << "-1 0 0 0 0\n";
	}
	if (bounding_box) {
		ofs << "# " << (nr - ns) << " other somas within bounding box of selected somas\n";
		ofs << "# (BOSS ignores these when recording voltages and weights)\n";
		const coord_t *min = b.min();
		const coord_t *max = b.max();
		ofs << "# (" << min[0] << ", " << min[1] << ", " << min[2] << ") to (" << max[0] << ", " << max[1] << ", " <<
			max[2] << ")\n";
		for (size32_t i = 0; i < nm; i++) {
			const Soma *s = _model.soma(i);
			const coord_t *c = s->coords();
			if (b.contains(c) && !_state.is_selected(s)) {
				ofs << (size32_t)s->type_index() << " " << s->id() << " " << c[0] << " " << c[1] << " " << c[2] << "\n";
			}
		}
	}
	else {
		ofs << "# Other somas within bounding box of selected somas are not output\n";
	}
}

void Model_Area::write_selected_synapses_to(std::ofstream &ofs) const {
	typedef std::unordered_set<const Synapse *> synapse_set_t;
	synapse_set_t synapses;
	size_t ns = _state.num_selected();
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(0);
	ofs << "# " << _model.filename() << "\n";
	ofs << ns << " # number of selected somas\n";
	ofs << "# <type> <id> <x> <y> <z>\n";
	for (size_t i = 0; i < ns; i++) {
		const Soma *s = _state.selected(i);
		const coord_t *c = s->coords();
		ofs << (size32_t)s->type_index() << " " << s->id() << " " << c[0] << " " << c[1] << " " << c[2] << "\n";
		for (size32_t a_index = s->first_axon_syn_index(); a_index != NULL_INDEX;) {
			const Synapse *y = _model.synapse(a_index);
			synapses.insert(y);
			a_index = y->next_axon_syn_index();
		}
		for (size32_t d_index = s->first_den_syn_index(); d_index != NULL_INDEX;) {
			const Synapse *y = _model.synapse(d_index);
			synapses.insert(y);
			d_index = y->next_den_syn_index();
		}
	}
	size_t nr = synapses.size();
	ofs << nr << " # number of reported synapses\n";
	ofs << "# <axonal id> <dendritic id> <x> <y> <z>\n";
	for (synapse_set_t::const_iterator it = synapses.begin(); it != synapses.end(); ++it) {
		const Synapse *y = *it;
		const coord_t *c = y->coords();
		ofs << _model.soma(y->axon_soma_index())->id() << " " << _model.soma(y->den_soma_index())->id()
			<< " " << c[0] << " " << c[1] << " " << c[2] << "\n";
	}
}

void Model_Area::write_marked_synapses_to(std::ofstream &ofs) const {
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(0);
	ofs << "# " << _model.filename() << "\n";
	ofs << _state.num_marked() << " # number of marked synapses\n";
	ofs << "# <axonal letter> #<axonal id> [via (<vx>, <vy>, <vz>)] thru (<x>, <y>, <z>) to <dendritic letter> #<dendritic id>\n";
	for (marked_syns_t::const_iterator it = _state.begin_marked_synapses(); it != _state.end_marked_synapses(); ++it) {
		const Synapse *y = it->first;
		const coord_t *c = y->coords();
		const Soma *a = _model.soma(y->axon_soma_index());
		const Soma_Type *t = _model.type(a->type_index());
		const Soma *d = _model.soma(y->den_soma_index());
		const Soma_Type *u = _model.type(d->type_index());
		ofs << t->letter() << " #" << a->id();
		if (y->has_via()) {
			const coord_t *v = y->via_coords();
			ofs << " via (" << v[0] << ", " << v[1] << ", " << v[2] << ")";
		}
		ofs << " thru (" << c[0] << ", " << c[1] << ", " << c[2] << ") to " << u->letter() << " #" << d->id() << "\n";
	}
}

void Model_Area::write_current_frequencies_to(std::ofstream &ofs) const {
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(0);
	const Firing_Spikes *fd = _model.const_firing_spikes();
	ofs << "# " << fd->filename() << "\n";
	ofs << "# " << fd->duration() << " cycles (" << fd->const_start_time() << " to " << fd->time() << ") at ";
	ofs.precision(1);
	ofs << (1.0f / fd->timescale()) << " ms/cycle\n";
	ofs << "# <type> <id> <x> <y> <z> <frequency>\n";
	ofs.precision(2);
	size32_t n = _model.num_somas();
	size_t n_selected = _state.num_selected();
	for (size32_t index = 0; index < n; index++) {
		const Soma *s = _model.soma(index);
		if (n_selected > 0 && !_state.is_selected(s)) { continue; }
		const Soma_Type *t = _model.type(s->type_index());
		if (t->display_state() == Soma_Type::DISABLED) { continue; }
		if (!fd->active(index)) { continue; }
		float hz = fd->hertz(index);
		if (hz == 0.0f) { continue; }
		const coord_t *c = s->coords();
		ofs << (size32_t)s->type_index() << " " << s->id() << " " << c[0] << " " << c[1] << " " << c[2] << " " << hz << "\n";
	}
}

void Model_Area::write_average_frequencies_to(std::ofstream &ofs, size32_t start, size32_t stop, size32_t step, Progress_Dialog *p) {
	size_t denom = 1;
	size32_t n = stop - start + 1;
	if (p) {
		denom = n / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Calculating frequencies...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return; }
	}
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(0);
	Firing_Spikes *fd = _model.firing_spikes();
	ofs << "# " << fd->filename() << "\n";
	ofs << "# " << fd->duration() << " cycles (" << fd->const_start_time() << " to " << fd->max_time() << ") at ";
	ofs.precision(1);
	ofs << (1.0f / fd->timescale()) << " ms/cycle\n";
	ofs << "# Averaging every " << step << " cycles from " << start << " to " << stop << "\n";
	size32_t n_intervals = n / step;
	ofs << "# <type> <id> <x> <y> <z>";
	for (size32_t i = 0; i < n_intervals; i++) {
		ofs << " <avg:" << (start + i * step) << "-" << (start + (i + 1) * step - 1) << ">";
	}
	ofs << "\n";
	ofs.precision(2);
	size32_t n_somas = _model.num_somas();
	size_t n_selected = _state.num_selected();
	std::map<size32_t, std::vector<float>> averages;
	fd->start_time(start);
	for (size32_t i = 0; i < n; i++) {
		for (size32_t index = 0; index < n_somas; index++) {
			const Soma *s = _model.soma(index);
			if (n_selected > 0 && !_state.is_selected(s)) { continue; }
			const Soma_Type *t = _model.type(s->type_index());
			if (t->display_state() == Soma_Type::DISABLED) { continue; }
			if (!fd->active(index)) { continue; }
			float hz = fd->hertz(index);
			if (hz == 0.0f) { continue; }
			averages[index].resize(n);
			averages[index][i] = hz;
		}
		fd->step_time();
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / n);
			Fl::check();
			if (p->canceled()) { return; }
		}
	}
	if (p) {
		denom = averages.size() / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Averaging frequencies...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return; }
	}
	size_t n_written = 0;
	for (std::map<size32_t, std::vector<float>>::const_iterator it = averages.begin(); it != averages.end(); ++it) {
		size32_t index = it->first;
		const std::vector<float> &freqs = it->second;
		const Soma *s = _model.soma(index);
		const coord_t *c = s->coords();
		ofs << (size32_t)s->type_index() << " " << s->id() << " " << c[0] << " " << c[1] << " " << c[2];
		for (size32_t i = 0; i < n_intervals; i++) {
			float avg = 0.0f;
			for (size32_t j = 0; j < step; j++) {
				avg += freqs[i*step+j];
			}
			avg /= step;
			ofs << " " << avg;
		}
		ofs << "\n";
		n_written++;
		if (p && !(n_written % denom)) {
			p->progress((float)n_written / n);
			Fl::check();
			if (p->canceled()) { return; }
		}
	}
	if (p) {
		p->progress(1.0f);
		Fl::check();
		if (p->canceled()) { return; }
	}
}

void Model_Area::redraw() {
	Fl_Gl_Window::redraw();
	if (_overview_area) { _overview_area->refresh(); }
}

void Model_Area::draw() {
	_fps.start();
	if (!_initialized) {
#ifdef __APPLE__
		if (!context_valid()) { return; } // temporary fix for some OpenGL crashes
#endif
		refresh_gl();
		_initialized = true;
	}
	if (!valid()) {
		refresh_projection(DRAWING);
		valid(1);
	}
	refresh_view();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_draw(" ", 1); // fix for erratic FLTK font drawing <http://www.fltk.org/newsgroups.php?gfltk.opengl+v:17>
	if (_opened && _draw_opts.display() == Draw_Options::STATIC_MODEL) {
		draw_static_model();
		draw_selected();
	}
	else if (_model.has_firing_spikes() && _draw_opts.display() == Draw_Options::FIRING_SPIKES) {
		const Firing_Spikes *fd = _model.const_firing_spikes();
		draw_firing_spikes();
		draw_selected(fd);
		draw_scale(fd, "Hz:");
	}
	else if (_model.has_voltages() && _draw_opts.display() == Draw_Options::VOLTAGES) {
		const Voltages *v = _model.const_voltages();
		draw_voltages();
		draw_selected(v);
		draw_scale(v, "mV:");
	}
	else if (_model.has_weights() && _draw_opts.display() == Draw_Options::WEIGHTS) {
		const Weights *w = _model.const_weights();
		draw_weights();
		draw_selected(w);
		draw_scale(w, "mV:", 2);
	}
	draw_bulletin();
	draw_clip_rect();
	draw_rotation_guide();
	draw_axes();
	_fps.stop();
	draw_fps();
}

void Model_Area::draw_static_model() const {
	if (_draw_opts.only_show_selected()) { return; }
	glPointSize(3.0f);
	size32_t n = _model.num_somas();
	if (_draw_opts.allow_letters()) {
		// Draw somas as letters colored by type
		gl_font(SOMA_FONT, Soma::soma_letter_size());
		for (size32_t i = 0; i < n; i++) {
			const Soma *s = _model.soma(i);
			const Soma_Type *t = _model.type(s->type_index());
			if (!t->visible()) { continue; }
			glColor3fv(t->color()->rgb());
			s->draw_letter(t);
		}
	}
	else {
		// Draw somas as small dots colored by type
		glBegin(GL_POINTS);
		for (size32_t i = 0; i < n; i++) {
			const Soma *s = _model.soma(i);
			const Soma_Type *t = _model.type(s->type_index());
			if (!t->visible()) { continue; }
			glColor3fv(t->color()->rgb());
			s->draw();
		}
		glEnd();
	}
}

void Model_Area::draw_inactive() const {
	if (!_draw_opts.show_inactive_somas()) { return; }
	// Draw inactive somas as tiny gray dots
	glPointSize(1.0f);
	size32_t n = _model.num_somas();
	glColor3fv(_draw_opts.invert_background() ? Sim_Data::INVERT_INACTIVE_SOMA_COLOR : Sim_Data::INACTIVE_SOMA_COLOR);
	glBegin(GL_POINTS);
	for (size32_t i = 0; i < n; i++) {
		const Soma *s = _model.soma(i);
		const Soma_Type *t = _model.type(s->type_index());
		if (!t->visible()) { continue; }
		glVertex3cv(s->coords());
	}
	glEnd();
}

void Model_Area::draw_firing_spikes() const {
	if (_draw_opts.only_show_selected()) { return; }
	draw_inactive();
	const Firing_Spikes *fd = _model.const_firing_spikes();
	float cv[3];
	size32_t n = fd->num_somas();
	if (_draw_opts.display_value_for_somas()) {
		// Draw active somas as Hertz values colored by firing frequency (highlighted if firing)
		std::ostringstream ss;
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(0);
		for (size32_t index = 0; index < n; index++) {
			const Soma *s = _model.soma(index);
			const Soma_Type *t = _model.type(s->type_index());
			if (!fd->active(index) || _state.is_selected(s) || !t->visible()) { continue; }
			if (t->display_state() == Soma_Type::LETTER) {
				fd->bright_color(index, t, cv, _draw_opts.invert_background());
				glColor3fv(cv);
				ss.str("");
				ss << fd->hertz(index);
				s->draw_firing_value(ss.str(), fd->firing_or_suppressing(index));
			}
			else {
				fd->color(index, t, cv, _draw_opts.invert_background());
				glColor3fv(cv);
				s->draw_firing(fd->firing_or_suppressing(index));
			}
		}
	}
	else if (_draw_opts.allow_letters()) {
		// Draw active somas as letters colored by firing frequency (highlighted if firing)
		for (size32_t index = 0; index < n; index++) {
			const Soma *s = _model.soma(index);
			const Soma_Type *t = _model.type(s->type_index());
			if (!fd->active(index) || _state.is_selected(s) || !t->visible()) { continue; }
			fd->color(index, t, cv, _draw_opts.invert_background());
			glColor3fv(cv);
			s->draw_firing_letter(t, fd->firing_or_suppressing(index));
		}
	}
	else {
		// Draw active somas as large dots colored by firing frequency (highlighted if firing)
		for (size32_t index = 0; index < n; index++) {
			const Soma *s = _model.soma(index);
			const Soma_Type *t = _model.type(s->type_index());
			if (!fd->active(index) || _state.is_selected(s) || !t->visible()) { continue; }
			fd->color(index, t, cv, _draw_opts.invert_background());
			glColor3fv(cv);
			s->draw_firing(fd->firing_or_suppressing(index));
		}
	}
}

void Model_Area::draw_voltages() const {
	if (_draw_opts.only_show_selected()) { return; }
	draw_inactive();
	const Firing_Spikes *fd = _model.const_firing_spikes();
	const Voltages *vt = _model.const_voltages();
	float cv[3];
	size32_t n = vt->num_active_somas();
	if (_draw_opts.display_value_for_somas()) {
		// Draw active somas as mV values colored by voltage (highlighted if firing)
		std::ostringstream ss;
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(0);
		for (size32_t i = 0; i < n; i++) {
			size32_t index = vt->active_soma_index(i);
			const Soma *s = _model.soma(index);
			const Soma_Type *t = _model.type(s->type_index());
			if (_state.is_selected(s) || !t->visible()) { continue; }
			vt->color(index, t, cv, _draw_opts.invert_background());
			glColor3fv(cv);
			if (t->display_state() == Soma_Type::LETTER) {
				ss.str("");
				ss << vt->voltage(index);
				s->draw_firing_value(ss.str(), fd->firing(index));
			}
			else {
				s->draw_firing(fd->firing(index));
			}
		}
	}
	else if (_draw_opts.allow_letters()) {
		// Draw active somas as letters colored by voltage (highlighted if firing)
		for (size32_t i = 0; i < n; i++) {
			size32_t index = vt->active_soma_index(i);
			const Soma *s = _model.soma(index);
			const Soma_Type *t = _model.type(s->type_index());
			if (_state.is_selected(s) || !t->visible()) { continue; }
			vt->color(index, t, cv, _draw_opts.invert_background());
			glColor3fv(cv);
			s->draw_firing_letter(t, fd->firing(index));
		}
	}
	else {
		// Draw active somas as large dots colored by voltage (highlighted if firing)
		for (size32_t i = 0; i < n; i++) {
			size32_t index = vt->active_soma_index(i);
			const Soma *s = _model.soma(index);
			const Soma_Type *t = _model.type(s->type_index());
			if (_state.is_selected(s) || !t->visible()) { continue; }
			vt->color(index, t, cv, _draw_opts.invert_background());
			glColor3fv(cv);
			s->draw_firing(fd->firing(index));
		}
	}
}

void Model_Area::draw_weights() const {
	if (_draw_opts.only_show_selected()) { return; }
	draw_inactive();
	const Weights *wt = _model.const_weights();
	const weights_instance_t &wcs = wt->weight_changes();
	if (_draw_opts.axon_conns() || _draw_opts.den_conns()) {
		const Firing_Spikes *fd = _model.const_firing_spikes();
		if (_draw_opts.allow_letters()) {
			// Draw somas of active synapses as letters colored by type (highlighted if firing)
			for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
				size32_t y_index = ys->first;
				const Synapse *y = _model.synapse(y_index);
				// Draw axonal soma
				size32_t a_index = y->axon_soma_index();
				const Soma *a = _model.soma(a_index);
				const Soma_Type *t = _model.type(a->type_index());
				if (!_state.is_selected(a) && t->visible()) {
					glColor3fv(t->color()->rgb());
					a->draw_firing_letter(t, fd->firing(a_index));
				}
				// Draw dendritic soma
				size32_t d_index = y->den_soma_index();
				const Soma *d = _model.soma(d_index);
				const Soma_Type *u = _model.type(d->type_index());
				if (!_state.is_selected(d) && u->visible()) {
					glColor3fv(u->color()->rgb());
					d->draw_firing_letter(u, fd->firing(d_index));
				}
			}
		}
		else {
			// Draw somas of active synapses as large dots colored by type (highlighted if firing)
			for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
				size32_t y_index = ys->first;
				const Synapse *y = _model.synapse(y_index);
				// Draw axonal soma
				size32_t a_index = y->axon_soma_index();
				const Soma *a = _model.soma(a_index);
				const Soma_Type *t = _model.type(a->type_index());
				if (!_state.is_selected(a) && t->visible()) {
					glColor3fv(t->color()->rgb());
					a->draw_firing(fd->firing(a_index));
				}
				// Draw dendritic soma
				size32_t d_index = y->den_soma_index();
				const Soma *d = _model.soma(d_index);
				const Soma_Type *u = _model.type(d->type_index());
				if (!_state.is_selected(d) && u->visible()) {
					glColor3fv(u->color()->rgb());
					d->draw_firing(fd->firing(d_index));
				}
			}
		}
	}
	if (_draw_opts.axon_conns() || _draw_opts.den_conns() || _draw_opts.syn_dots()) {
		// Draw active synapses colored by weight
		const float *bgcv = _draw_opts.invert_background() ? INVERT_BACKGROUND_COLOR : BACKGROUND_COLOR;
		for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
			size32_t y_index = ys->first;
			const Synapse *y = _model.synapse(y_index);
			bool y_marked = _state.is_marked(y, y_index);
			if (_draw_opts.only_show_marked() && !y_marked) { continue; }
			// Get axonal soma
			size32_t a_index = y->axon_soma_index();
			const Soma *a = _model.soma(a_index);
			const Soma_Type *t = _model.type(a->type_index());
			if (!t->visible()) { continue; }
			// Get dendritic soma
			size32_t d_index = y->den_soma_index();
			const Soma *d = _model.soma(d_index);
			const Soma_Type *u = _model.type(d->type_index());
			if (!u->visible()) { continue; }
			// Draw synapse
			float cv[3];
			wt->synapse_color(y_index, cv, _draw_opts.weights_color_after());
			glColor3fv(cv);
			bool conn_unsel = _draw_opts.only_conn_selected() && (!_state.is_selected(a) || !_state.is_selected(d));
			if ((_draw_opts.axon_conns() || _draw_opts.den_conns()) && !conn_unsel) {
				y->draw_conn(a, d, _draw_opts.to_axon(), _draw_opts.to_via(), _draw_opts.to_synapse(), _draw_opts.to_den());
			}
			if (_draw_opts.syn_dots()) {
				if (y_marked) {
					y->draw_marked(cv, bgcv);
				}
				else {
					glPointSize(5.0f);
					glBegin(GL_POINTS);
					y->draw();
					glEnd();
				}
			}
		}
	}
}

void Model_Area::draw_selected() const {
	bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
	bool only_enable_clipped = _state.clipped() && _draw_opts.only_enable_clipped();
	if (only_show_clipped) { Clip_Volume::disable(); }
	const Clip_Volume &clip_volume = _state.const_clip_volume();
	const float *bgcv = _draw_opts.invert_background() ? INVERT_BACKGROUND_COLOR : BACKGROUND_COLOR;
	size_t n = _state.num_selected();
	size32_t ny = _model.num_synapses();
	glPointSize(3.0f);
	// Draw selected somas with their synapses and neuritic fields
	gl_font(SOMA_FONT, Soma::soma_letter_size());
	for (size_t i = 0; i < n; i++) {
		const Soma *s = _state.selected(i);
		const Soma_Type *t = _model.type(s->type_index());
		const Synapse *y = NULL;
		if (_draw_opts.axon_conns()) {
			// Draw axonal connections
			for (size32_t a_index = s->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
				y = _model.synapse(a_index);
				if (_draw_opts.only_show_marked() && !_state.is_marked(y, a_index)) { continue; }
				const Soma *d = _model.soma(y->den_soma_index());
				if (_draw_opts.only_conn_selected() && !_state.is_selected(d)) { continue; }
				const Soma_Type *u = _model.type(d->type_index());
				bool outside = !clip_volume.contains(d->coords());
				if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
				if (u->display_state() == Soma_Type::HIDDEN || (only_show_clipped && outside) ||
					_draw_opts.only_show_selected()) {
					// Draw dendritic soma as a letter or dot colored by type
					glColor3fv(u->color()->rgb());
					if (_draw_opts.allow_letters()) {
						d->draw_letter(u);
					}
					else {
						glBegin(GL_POINTS);
						d->draw();
						glEnd();
					}
				}
				// Draw synapse colored by axonal soma type
				glColor3fv(t->color()->rgb());
				y->draw_conn(s, d, _draw_opts.to_axon(), _draw_opts.to_via(), _draw_opts.to_synapse(), _draw_opts.to_den());
			}
		}
		if (_draw_opts.den_conns()) {
			// Draw dendritic connections
			for (size32_t d_index = s->first_den_syn_index(); d_index < ny; d_index = y->next_den_syn_index()) {
				y = _model.synapse(d_index);
				if (_draw_opts.only_show_marked() && !_state.is_marked(y, d_index)) { continue; }
				const Soma *a = _model.soma(y->axon_soma_index());
				if (_draw_opts.only_conn_selected() && !_state.is_selected(a)) { continue; }
				const Soma_Type *u = _model.type(a->type_index());
				bool outside = !clip_volume.contains(a->coords());
				if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
				if (u->display_state() == Soma_Type::HIDDEN || (only_show_clipped && outside) ||
					_draw_opts.only_show_selected()) {
					// Draw axonal soma as a letter or dot colored by type
					glColor3fv(u->color()->rgb());
					if (_draw_opts.allow_letters()) {
						a->draw_letter(u);
					}
					else {
						glBegin(GL_POINTS);
						a->draw();
						glEnd();
					}
				}
				// Draw synapse colored by axonal soma type
				glColor3fv(u->color()->rgb());
				y->draw_conn(a, s, _draw_opts.to_axon(), _draw_opts.to_via(), _draw_opts.to_synapse(), _draw_opts.to_den());
			}
		}
		if (_draw_opts.neur_fields() && (s->num_axon_fields() || s->num_den_fields())) {
			// Draw neuritic fields
			if (_draw_opts.conn_fields()) {
				// Draw connected fields for axonal synapses
				for (size32_t a_index = s->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
					y = _model.synapse(a_index);
					if (_draw_opts.only_show_marked() && !_state.is_marked(y, a_index)) { continue; }
					const Soma *o = _model.soma(y->den_soma_index());
					if (_draw_opts.only_conn_selected() && !_state.is_selected(o)) { continue; }
					const Soma_Type *u = _model.type(o->type_index());
					bool outside = !clip_volume.contains(o->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					o->draw_fields(true, _model);
				}
				// Draw connected fields for dendritic synapses
				for (size32_t d_index = s->first_den_syn_index(); d_index < ny; d_index = y->next_den_syn_index()) {
					y = _model.synapse(d_index);
					if (_draw_opts.only_show_marked() && !_state.is_marked(y, d_index)) { continue; }
					const Soma *o = _model.soma(y->axon_soma_index());
					if (_draw_opts.only_conn_selected() && !_state.is_selected(o)) { continue; }
					const Soma_Type *u = _model.type(o->type_index());
					bool outside = !clip_volume.contains(o->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					o->draw_fields(true, _model);
				}
			}
			// Draw fields as boxes
			s->draw_fields(false, _model);
		}
		if (_draw_opts.syn_dots() && !_draw_opts.only_show_marked()) {
			// Draw synapse dots
			glPointSize(5.0f);
			glBegin(GL_POINTS);
			// Draw axonal synapses as large red dots
			glColor3fv(AXON_SYN_COLOR);
			for (size32_t a_index = s->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
				y = _model.synapse(a_index);
				if (_state.is_marked(y, a_index)) { continue; }
				const coord_t *c = y->coords();
				if (only_enable_clipped && !clip_volume.contains(c)) { continue; }
				const Soma *d = _model.soma(y->den_soma_index());
				if (_draw_opts.only_conn_selected() && !_state.is_selected(d)) { continue; }
				const Soma_Type *u = _model.type(d->type_index());
				bool outside = !clip_volume.contains(d->coords());
				if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
				y->draw();
			}
			// Draw dendritic synapses as large purple dots
			glColor3fv(DEN_SYN_COLOR);
			for (size32_t d_index = s->first_den_syn_index(); d_index < ny; d_index = y->next_den_syn_index()) {
				y = _model.synapse(d_index);
				if (_state.is_marked(y, d_index)) { continue; }
				const coord_t *c = y->coords();
				if (only_enable_clipped && !clip_volume.contains(c)) { continue; }
				const Soma *a = _model.soma(y->axon_soma_index());
				if (_draw_opts.only_conn_selected() && !_state.is_selected(a)) { continue; }
				const Soma_Type *u = _model.type(a->type_index());
				bool outside = !clip_volume.contains(a->coords());
				if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
				y->draw();
			}
			glEnd();
		}
		// Draw selected soma as circled dot colored by type
		const float *cv = t->color()->rgb();
		s->draw_circled(cv, bgcv);
	}
	for (marked_syns_t::const_iterator it = _state.begin_marked_synapses(); it != _state.end_marked_synapses(); ++it) {
		const Synapse *y = it->first;
		if ((_draw_opts.axon_conns() || _draw_opts.den_conns()) && !_draw_opts.only_conn_selected()) {
			const Soma *a = _model.soma(y->axon_soma_index());
			const Soma_Type *t = _model.type(a->type_index());
			bool a_outside = !clip_volume.contains(a->coords());
			const Soma *d = _model.soma(y->den_soma_index());
			const Soma_Type *u = _model.type(d->type_index());
			bool d_outside = !clip_volume.contains(d->coords());
			if (t->display_state() == Soma_Type::HIDDEN || (only_show_clipped && a_outside) ||
				_draw_opts.only_show_selected()) {
				// Draw axonal soma as a letter or dot colored by type
				glColor3fv(t->color()->rgb());
				if (_draw_opts.allow_letters()) {
					a->draw_letter(t);
				}
				else {
					glPointSize(3.0f);
					glBegin(GL_POINTS);
					a->draw();
					glEnd();
				}
			}
			if (u->display_state() == Soma_Type::HIDDEN || (only_show_clipped && d_outside) ||
				_draw_opts.only_show_selected()) {
				// Draw dendritic soma as a letter or dot colored by type
				glColor3fv(u->color()->rgb());
				if (_draw_opts.allow_letters()) {
					d->draw_letter(u);
				}
				else {
					glPointSize(3.0f);
					glBegin(GL_POINTS);
					d->draw();
					glEnd();
				}
			}
			// Draw synapse colored by axonal soma type
			glColor3fv(t->color()->rgb());
			y->draw_conn(a, d, _draw_opts.to_axon(), _draw_opts.to_via(), _draw_opts.to_synapse(), _draw_opts.to_den());
		}
		if (_draw_opts.syn_dots()) {
			// Draw marked synapses as circled orange dots
			y->draw_marked(MARKED_SYN_COLOR, bgcv);
		}
	}
	if (_draw_opts.gap_junctions()) {
		// Draw gap junctions involving selected somas as dotted lines with dots at the junctions
		size32_t ng = _model.num_gap_junctions();
		glPointSize(3.0f);
		glColor3fv(GAP_JUNCTION_COLOR);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(1, 0xAAAA); // dotted lines - 1010101010101010
		for (size32_t i = 0; i < ng; i++) {
			const Gap_Junction *g = _model.gap_junction(i);
			const Soma *s1 = _model.soma(g->soma1_index());
			const Soma_Type *t1 = _model.type(s1->type_index());
			const Soma *s2 = _model.soma(g->soma2_index());
			const Soma_Type *t2 = _model.type(s2->type_index());
			if (t1->display_state() == Soma_Type::DISABLED || t2->display_state() == Soma_Type::DISABLED) { continue; }
			if (_draw_opts.only_conn_selected() ? _state.is_selected(s1) && _state.is_selected(s2) :
				_state.is_selected(s1) || _state.is_selected(s2)) {
				g->draw(s1, s2);
			}
		}
		glDisable(GL_LINE_STIPPLE);
		glPointSize(1.0f);
	}
	if (only_show_clipped) { Clip_Volume::enable(); }
}

void Model_Area::draw_selected(const Sim_Data *sd) const {
	bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
	bool only_enable_clipped = _state.clipped() && _draw_opts.only_enable_clipped();
	if (only_show_clipped) { Clip_Volume::disable(); }
	const Clip_Volume &clip_volume = _state.const_clip_volume();
	const float *bgcv = _draw_opts.invert_background() ? INVERT_BACKGROUND_COLOR : BACKGROUND_COLOR;
	double model_view[16], projection[16];
	int viewport[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, model_view);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);
	const Firing_Spikes *fd = _model.const_firing_spikes();
	size_t n = _state.num_selected();
	size32_t ny = _model.num_synapses();
	// Draw selected somas as circled letters colored by the active set (highlighted if firing)
	for (size_t i = 0; i < n; i++) {
		const Soma *s = _state.selected(i);
		size32_t index = _state.selected_index(i);
		const Soma_Type *t = _model.type(s->type_index());
		const Synapse *y = NULL;
		if (_draw_opts.neur_fields() && (s->num_axon_fields() || s->num_den_fields())) {
			// Draw neuritic fields
			if (_draw_opts.conn_fields()) {
				// Draw connected fields for axonal synapses
				for (size32_t a_index = s->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
					y = _model.synapse(a_index);
					if (_draw_opts.only_show_marked() && !_state.is_marked(y, a_index)) { continue; }
					const Soma *o = _model.soma(y->den_soma_index());
					if (_draw_opts.only_conn_selected() && !_state.is_selected(o)) { continue; }
					const Soma_Type *u = _model.type(o->type_index());
					bool outside = !clip_volume.contains(o->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					o->draw_fields(true, _model);
				}
				// Draw connected fields for dendritic synapses
				for (size32_t d_index = s->first_den_syn_index(); d_index < ny; d_index = y->next_den_syn_index()) {
					y = _model.synapse(d_index);
					if (_draw_opts.only_show_marked() && !_state.is_marked(y, d_index)) { continue; }
					const Soma *o = _model.soma(y->axon_soma_index());
					if (_draw_opts.only_conn_selected() && !_state.is_selected(o)) { continue; }
					const Soma_Type *u = _model.type(o->type_index());
					bool outside = !clip_volume.contains(o->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					o->draw_fields(true, _model);
				}
			}
			// Draw fields as boxes
			s->draw_fields(false, _model);
		}
		bool firing = fd->firing(index);
		float cv[3];
		sd->color(index, t, cv, _draw_opts.invert_background());
		s->draw_circled_firing(t, cv, bgcv, firing, model_view, projection, viewport);
	}
	const Weights *wt = _model.const_weights();
	if (sd == wt && _draw_opts.only_show_selected() && (_draw_opts.axon_conns() || _draw_opts.den_conns() || _draw_opts.syn_dots())) {
		const weights_instance_t &wcs = wt->weight_changes();
		float cv[3];
		// Draw selected somas' synapses colored by weight
		for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
			size32_t y_index = ys->first;
			const Synapse *y = _model.synapse(y_index);
			bool y_marked = _state.is_marked(y, y_index);
			// Get axonal soma
			size32_t a_index = y->axon_soma_index();
			const Soma *a = _model.soma(a_index);
			const Soma_Type *t = _model.type(a->type_index());
			if (t->display_state() == Soma_Type::DISABLED) { continue; }
			bool a_sel = _state.is_selected(a);
			// Get dendritic soma
			size32_t d_index = y->den_soma_index();
			const Soma *d = _model.soma(d_index);
			const Soma_Type *u = _model.type(d->type_index());
			if (u->display_state() == Soma_Type::DISABLED) { continue; }
			bool d_sel = _state.is_selected(d);
			if (((!a_sel && !d_sel) || _draw_opts.only_show_marked()) && !y_marked) { continue; }
			// Draw synapse
			if (_draw_opts.axon_conns() || _draw_opts.den_conns()) {
				wt->synapse_color(y_index, cv, _draw_opts.weights_color_after());
				glColor3fv(cv);
				y->draw_conn(a, d, _draw_opts.to_axon(), _draw_opts.to_via(), _draw_opts.to_synapse(), _draw_opts.to_den());
				if (y_marked) {
					y->draw_marked(cv, bgcv);
				}
				else {
					glPointSize(5.0f);
					glBegin(GL_POINTS);
					y->draw();
					glEnd();
				}
				if (!a_sel) {
					glColor3fv(t->color()->rgb());
					if (_draw_opts.allow_letters()) {
						a->draw_firing_letter(t, fd->firing(a_index));
					}
					else {
						a->draw_firing(fd->firing(a_index));
					}
				}
				if (!d_sel) {
					glColor3fv(u->color()->rgb());
					if (_draw_opts.allow_letters()) {
						d->draw_firing_letter(u, fd->firing(d_index));
					}
					else {
						d->draw_firing(fd->firing(d_index));
					}
				}
			}
			else if (_draw_opts.syn_dots()) {
				wt->synapse_color(y_index, cv, _draw_opts.weights_color_after());
				if (y_marked) {
					y->draw_marked(cv, bgcv);
				}
				else {
					glColor3fv(cv);
					glPointSize(5.0f);
					glBegin(GL_POINTS);
					y->draw();
					glEnd();
				}
			}
		}
	}
	if (only_show_clipped) { Clip_Volume::enable(); }
}

void Model_Area::draw_scale(const Sim_Data *sd, const char *l, std::streamsize p) const {
	if (!_draw_opts.show_color_scale()) { return; }
	// Push projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, w(), h(), 0.0);
	// Push model view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	// Disable depth test and clip planes
	glDisable(GL_DEPTH_TEST);
	Clip_Volume::disable();
	// Draw color scale
	int xc = w() - 16, yc = h() - 16;
#ifdef LARGE_INTERFACE
	int sw = 24, shi = 24;
#else
	int sw = 16, shi = 16;
#endif
	sd->color_map()->draw(xc, yc, sw, shi * 10);
	// Draw ticks
	glColor3f(0.5f, 0.5f, 0.5f);
	glBegin(GL_LINES);
	for (int dsh = 0; dsh <= 10; dsh++) {
		glVertex2i(xc-sw, yc-shi*dsh+(dsh==10));
		glVertex2i(xc-sw-3, yc-shi*dsh+(dsh==10));
	}
	glEnd();
	// Draw labels
#ifdef LARGE_INTERFACE
	int cw = 9, ch = 5, ld = 6;
	gl_font(FL_HELVETICA, 16);
#else
	int cw = 7, ch = 3, ld = 5;
	gl_font(FL_HELVETICA, 12);
#endif
	glColor3fv(_draw_opts.invert_background() ? BACKGROUND_COLOR : INVERT_BACKGROUND_COLOR);
	std::ostringstream ss;
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(p);
	for (int dsh = 0; dsh <= 10; dsh++) {
		ss.str("");
		float q = sd->quantity(dsh * 0.1f);
		ss << q;
		int tll = (int)ss.str().length();
		float tlw = (float)tll;
		if (q < 0.0f) { tlw -= 0.5f; } // half-width "-"
		if (p > 0) { tlw -= 0.5f; } // half-width "."
		glRasterPos2i((int)(xc-sw-ld-cw*tlw), yc+ch-shi*dsh);
		gl_draw(ss.str().c_str(), tll);
	}
	// Draw heading
	int ll = (int)strlen(l);
	glRasterPos2i(xc-sw-ld-cw*ll, yc+ch-shi*11);
	gl_draw(l, ll);
	// Re-enable depth test
	glEnable(GL_DEPTH_TEST);
	// Pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void Model_Area::draw_bulletin() const {
	if (!_draw_opts.show_bulletin()) { return; }
	// Push projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, w(), h(), 0.0);
	// Push model view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	// Disable depth test and clip planes
	glDisable(GL_DEPTH_TEST);
	Clip_Volume::disable();
	// Prepare bulletin
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	// Add lines in reverse order
	size_t nl = 0;
	size_t ns = _state.num_selected();
	if (ns) {
		for (size_t i = ns; i > 0; i--) {
			const Soma *s = _state.selected(i - 1);
			const Soma_Type *t = _model.type(s->type_index());
			ss << t->letter() << " #" << s->id() << "\n";
			nl++;
		}
		ss << ns << " selected:\n";
		nl++;
	}
	if (_draw_opts.display() != Draw_Options::STATIC_MODEL) {
		ss << "Cycle " << (_model.has_firing_spikes() ? _model.const_firing_spikes()->time() : 0) << "\n";
		nl++;
	}
	// Draw bulletin
	glColor3fv(_draw_opts.invert_background() ? BACKGROUND_COLOR : INVERT_BACKGROUND_COLOR);
#ifdef LARGE_INTERFACE
	gl_font(FL_HELVETICA, 16);
#else
	gl_font(FL_HELVETICA, 12);
#endif
	gl_draw(ss.str().c_str(), 2, 2, 200, (int)nl * fl_height() + fl_height() / 2, FL_ALIGN_TOP_LEFT);
	// Re-enable depth test
	glEnable(GL_DEPTH_TEST);
	// Pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void Model_Area::draw_clip_rect() const {
	if (!_dragging || action() != CLIP) { return; }
	// Push projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	if (_draw_opts.left_handed()) {
		gluOrtho2D(w(), 0.0, 0.0, h());
	}
	else {
		gluOrtho2D(0.0, w(), 0.0, h());
	}
	// Push model view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	// Disable depth test and clip planes
	glDisable(GL_DEPTH_TEST);
	Clip_Volume::disable();
	// Draw selection rectangle
	glColor3fv(_draw_opts.invert_background() ? BACKGROUND_COLOR : INVERT_BACKGROUND_COLOR);
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1, 0xCCCC); // dashed lines - 1100110011001100
	glBegin(GL_LINE_LOOP);
	glVertex2i(_click_coords[0], _click_coords[1]);
	glVertex2i(_click_coords[0], _drag_coords[1]);
	glVertex2i(_drag_coords[0], _drag_coords[1]);
	glVertex2i(_drag_coords[0], _click_coords[1]);
	glEnd();
	glDisable(GL_LINE_STIPPLE);
	// Re-enable depth test
	glEnable(GL_DEPTH_TEST);
	// Pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

static void wireCylinder(double radius, int slices, int stacks) {
	GLUquadricObj *quadric = gluNewQuadric();
	gluQuadricDrawStyle(quadric, GLU_LINE);
	glPushMatrix();
	glTranslated(0.0, 0.0, -radius);
	gluCylinder(quadric, radius, radius, 2.0 * radius, slices, stacks);
	glPushMatrix();
	glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
	gluDisk(quadric, 0.0, radius, slices, stacks);
	glPopMatrix();
	glTranslated(0.0, 0.0, 2.0 * radius);
	gluDisk(quadric, 0.0, radius, slices, stacks);
	glPopMatrix();
	gluDeleteQuadric(quadric);
}

void Model_Area::draw_rotation_guide() {
	if (!_dragging || action() != ROTATE || !_draw_opts.show_rotation_guide()) { return; }
	// Refresh projection and model view matrices
	refresh_projection(DRAWING);
	refresh_view();
	// Disable clip planes
	Clip_Volume::disable();
	const coord_t *p = _state.pivot();
	glTranslatef((float)p[0], (float)p[1], (float)p[2]);
	// Draw bounding sphere
	if (_draw_opts.invert_background()) {
		glColor4f(BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2], 0.6f);
	}
	else {
		glColor4f(INVERT_BACKGROUND_COLOR[0], INVERT_BACKGROUND_COLOR[1], INVERT_BACKGROUND_COLOR[2], 0.6f);
	}
	double r = ROTATION_GUIDE_SCALE * (double)_state.max_range();
	if (!_scale_rotation) { r *= _state.zoom(); }
	switch (_rotation_mode) {
	case ARCBALL_2D:
	case ARCBALL_3D:
		r *= SQRT_3;
		glutWireSphere(r, ROTATION_GUIDE_DETAIL * 2, ROTATION_GUIDE_DETAIL * 2);
		break;
	case AXIS_X:
		r *= SQRT_2;
		glPushMatrix();
		glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		wireCylinder(r, ROTATION_GUIDE_DETAIL * 2, ROTATION_GUIDE_DETAIL);
		glPopMatrix();
		break;
	case AXIS_Y:
		r *= SQRT_2;
		glPushMatrix();
		glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
		wireCylinder(r, ROTATION_GUIDE_DETAIL * 2, ROTATION_GUIDE_DETAIL);
		glPopMatrix();
		break;
	case AXIS_Z:
		r *= SQRT_2;
		wireCylinder(r, ROTATION_GUIDE_DETAIL * 2, ROTATION_GUIDE_DETAIL);
		break;
	}
	// Draw axes
	r *= 1.1;
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	glColor3fv(X_AXIS_COLOR);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(r, 0.0, 0.0);
	glColor3fv(Y_AXIS_COLOR);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, r, 0.0);
	glColor3fv(Z_AXIS_COLOR);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, 0.0, r);
	glEnd();
	glLineWidth(1.0f);
	glTranslatef(-(float)p[0], -(float)p[1], -(float)p[2]);
}

void Model_Area::draw_axes() const {
	if (!_opened || !_draw_opts.show_axes()) { return; }
	// Change viewport
	glViewport(0, 0, 64, 64);
	// Push projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(30.0, 1.0, 0.0, 1.0);
	if (_draw_opts.left_handed()) {
		glScaled(-1.0, 1.0, 1.0); // mirror image
	}
	// Push model view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(0.0, 0.0, FOCAL_LENGTH, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); // look down the z-axis; y-axis points up
	glMultMatrixd(_state.rotate());
	// Disable depth test and clip planes
	glDisable(GL_DEPTH_TEST);
	Clip_Volume::disable();
	// Draw axes
	glLineWidth(2.0f);
	gl_font(FL_HELVETICA, 12);
	char l;
	glColor3fv(X_AXIS_COLOR);
	glBegin(GL_LINES);
	glVertex3i(0, 0, 0);
	glVertex3i(1, 0, 0);
	glEnd();
	if (_draw_opts.show_axis_labels()) {
		l = 'X';
		glRasterPos3d(1.1, 0.0, 0.0);
		gl_draw(&l, 1);
	}
	glColor3fv(Y_AXIS_COLOR);
	glBegin(GL_LINES);
	glVertex3i(0, 0, 0);
	glVertex3i(0, 1, 0);
	glEnd();
	if (_draw_opts.show_axis_labels()) {
		l = 'Y';
		glRasterPos3d(0.0, 1.1, 0.0);
		gl_draw(&l, 1);
	}
	glColor3fv(Z_AXIS_COLOR);
	glBegin(GL_LINES);
	glVertex3i(0, 0, 0);
	glVertex3i(0, 0, 1);
	glEnd();
	if (_draw_opts.show_axis_labels()) {
		l = 'Z';
		glRasterPos3d(0.0, 0.0, 1.1);
		gl_draw(&l, 1);
	}
	glLineWidth(1.0f);
	// Re-enable depth test
	glEnable(GL_DEPTH_TEST);
	// Pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	// Restore viewport
	glViewport(0, 0, w(), h());
}

void Model_Area::draw_fps() const {
	if (!_draw_opts.show_fps()) { return; }
	// Push projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, w(), h(), 0.0);
	// Push model view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	// Disable depth test and clip planes
	glDisable(GL_DEPTH_TEST);
	Clip_Volume::disable();
	// Prepare FPS
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	size_t fps = _fps.fps();
	if (fps > 0) {
		ss << fps << " FPS";
	}
	else {
		ss << _fps.spf() << " SPF";
	}
	// Draw FPS
	glColor3fv(_draw_opts.invert_background() ? BACKGROUND_COLOR : INVERT_BACKGROUND_COLOR);
#ifdef LARGE_INTERFACE
	gl_font(FL_HELVETICA, 16);
#else
	gl_font(FL_HELVETICA, 12);
#endif
	gl_draw(ss.str().c_str(), 2, 2, w() - 4, fl_height() + fl_height() / 2, FL_ALIGN_TOP_RIGHT);
	// Re-enable depth test
	glEnable(GL_DEPTH_TEST);
	// Pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

int Model_Area::handle(int event) {
	if (_dnd_receiver) {
		switch (event) {
		case FL_DND_ENTER:
		case FL_DND_LEAVE:
		case FL_DND_DRAG:
		case FL_DND_RELEASE:
			return 1;
		case FL_PASTE:
			return _dnd_receiver->handle(event);
		}
	}
	if (!_model.empty()) {
		switch (event) {
		case FL_ENTER:
		case FL_LEAVE:
		case FL_FOCUS:
			return handle_focus(event);
		case FL_PUSH:
		case FL_RELEASE:
			return handle_click(event);
		case FL_DRAG:
			return handle_drag(event);
		}
	}
	return Fl_Gl_Window::handle(event);
}

int Model_Area::handle_focus(int event) const {
	switch (event) {
	case FL_ENTER:
		refresh_cursor();
		return 1;
	case FL_LEAVE:
		fl_cursor(FL_CURSOR_DEFAULT);
		return 1;
	case FL_FOCUS:
		return 1;
	}
	return 0;
}

int Model_Area::handle_click(int event) {
	switch (event) {
	case FL_PUSH:
		Fl::focus(this);
		_click_coords[0] = Fl::event_x() - x(); _click_coords[1] = Fl::event_y() - y();
		if (_draw_opts.left_handed()) {
			_click_coords[0] = w() - _click_coords[0] - 1;
		}
		_click_coords[1] = h() - _click_coords[1] - 1;
		_dragging = true;
		switch (type()) {
		case SELECT:
		case MARK:
			return 1;
		case CLIP:
			_drag_coords[0] = _click_coords[0]; _drag_coords[1] = _click_coords[1];
			refresh();
			return 1;
		case PAN:
		case ROTATE:
		case ZOOM:
			remember(_state);
			_prev_state = _state;
			refresh();
			return 1;
		}
		break;
	case FL_RELEASE:
		_drag_coords[0] = Fl::event_x() - x(); _drag_coords[1] = Fl::event_y() - y();
		if (_draw_opts.left_handed()) {
			_drag_coords[0] = w() - _drag_coords[0] - 1;
		}
		_drag_coords[1] = h() - _drag_coords[1] - 1;
		_dragging = false;
		switch (type()) {
		case SELECT:
			if (Fl::event_is_click()) { select(); }
			else { refresh(); }
			return 1;
		case CLIP:
			if (!Fl::event_is_click()) { clip(); }
			else { refresh(); }
			return 1;
		case PAN:
		case ROTATE:
		case ZOOM:
			refresh();
			return 1;
		case MARK:
			if (Fl::event_is_click()) { mark(); }
			else { refresh(); }
			return 1;
		}
		break;
	}
	return 0;
}

int Model_Area::handle_drag(int) {
	_drag_coords[0] = Fl::event_x() - x(); _drag_coords[1] = Fl::event_y() - y();
	if (_draw_opts.left_handed()) {
		_drag_coords[0] = w() - _drag_coords[0] - 1;
	}
	_drag_coords[1] = h() - _drag_coords[1] - 1;
	switch (type()) {
	case SELECT:
	case MARK:
		return 1;
	case CLIP:
		refresh();
		return 1;
	case PAN:
		pan();
		return 1;
	case ROTATE:
		rotate();
		return 1;
	case ZOOM:
		zoom();
		return 1;
	}
	return 0;
}

void Model_Area::select() {
	make_current();
	refresh_projection(SELECTING);
	refresh_view();
	// Find closest hit, if any
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // corresponds to index 0, not used for any soma
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPointSize(SELECT_TOLERANCE);
	glBegin(GL_POINTS);
	if (_model.has_weights() && _draw_opts.display() == Draw_Options::WEIGHTS) {
		const Weights *wt = _model.const_weights();
		const weights_instance_t &wcs = wt->weight_changes();
		if (!_draw_opts.only_show_selected()) {
			if (_draw_opts.show_inactive_somas()) {
				// Imitate drawing the inactive somas
				size32_t n = _model.num_somas();
				for (size32_t index = 0; index < n; index++) {
					const Soma *s = _model.soma(index);
					const Soma_Type *t = _model.type(s->type_index());
					if (!t->visible()) { continue; }
					s->draw_for_selection(index);
				}
			}
			if (_draw_opts.axon_conns() || _draw_opts.den_conns()) {
				// Imitate drawing the active somas
				for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
					size32_t y_index = ys->first;
					const Synapse *y = _model.synapse(y_index);
					size32_t a_index = y->axon_soma_index();
					const Soma *a = _model.soma(a_index);
					const Soma_Type *t = _model.type(a->type_index());
					if (!_state.is_selected(a) && t->visible()) {
						a->draw_for_selection(a_index);
					}
					size32_t d_index = y->den_soma_index();
					const Soma *d = _model.soma(d_index);
					const Soma_Type *u = _model.type(d->type_index());
					if (!_state.is_selected(d) && u->visible()) {
						d->draw_for_selection(d_index);
					}
				}
			}
		}
		else if (_draw_opts.axon_conns() || _draw_opts.den_conns() || _draw_opts.syn_dots()) {
			// Imitate drawing the active somas
			for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
				size32_t y_index = ys->first;
				const Synapse *y = _model.synapse(y_index);
				size32_t a_index = y->axon_soma_index();
				const Soma *a = _model.soma(a_index);
				const Soma_Type *t = _model.type(a->type_index());
				if (t->display_state() == Soma_Type::DISABLED) { continue; }
				size32_t d_index = y->den_soma_index();
				const Soma *d = _model.soma(d_index);
				const Soma_Type *u = _model.type(d->type_index());
				if (u->display_state() == Soma_Type::DISABLED) { continue; }
				if (_draw_opts.axon_conns() && _state.is_selected(a) && !_state.is_selected(d)) {
					d->draw_for_selection(d_index);
				}
				else if (_draw_opts.den_conns() && _state.is_selected(d) && !_state.is_selected(a)) {
					a->draw_for_selection(a_index);
				}
			}
		}
		// Imitate drawing the selected somas
		bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
		if (only_show_clipped) { Clip_Volume::disable(); }
		size_t n = _state.num_selected();
		for (size_t i = 0; i < n; i++) {
			const Soma *s = _state.selected(i);
			size32_t index = _state.selected_index(i);
			s->draw_for_selection(index);
		}
		if (only_show_clipped) { Clip_Volume::enable(); }
	}
	else if (_draw_opts.display() != Draw_Options::STATIC_MODEL) {
		const Sim_Data *sd = active_sim_data();
		// Imitate drawing the inactive somas or just the active set
		if (!_draw_opts.only_show_selected()) {
			size32_t n = _model.num_somas();
			if (_draw_opts.show_inactive_somas()) {
				for (size32_t i = 0; i < n; i++) {
					const Soma *s = _model.soma(i);
					const Soma_Type *t = _model.type(s->type_index());
					if (!t->visible()) { continue; }
					s->draw_for_selection(i);
				}
			}
			else {
				for (size32_t i = 0; i < n; i++) {
					const Soma *s = _model.soma(i);
					const Soma_Type *t = _model.type(s->type_index());
					if (!t->visible() || !sd->active(i)) { continue; }
					s->draw_for_selection(i);
				}
			}
		}
		// Imitate drawing the selected somas
		bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
		if (only_show_clipped) { Clip_Volume::disable(); }
		size_t n_sel = _state.num_selected();
		for (size_t i = 0; i < n_sel; i++) {
			const Soma *sel_s = _state.selected(i);
			size32_t sel_index = _state.selected_index(i);
			sel_s->draw_for_selection(sel_index);
		}
		if (only_show_clipped) { Clip_Volume::enable(); }
	}
	else {
		// Imitate drawing the static model
		if (!_draw_opts.only_show_selected()) {
			size32_t n = _model.num_somas();
			for (size32_t i = 0; i < n; i++) {
				const Soma *s = _model.soma(i);
				const Soma_Type *t = _model.type(s->type_index());
				if (!t->visible()) { continue; }
				s->draw_for_selection(i);
			}
		}
		// Imitate drawing the selected somas
		bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
		bool only_enable_clipped = _state.clipped() && _draw_opts.only_enable_clipped();
		if (only_show_clipped) { Clip_Volume::disable(); }
		const Clip_Volume &clip_volume = _state.const_clip_volume();
		size32_t ny = _model.num_synapses();
		size_t n_sel = _state.num_selected();
		for (size_t i = 0; i < n_sel; i++) {
			const Soma *sel = _state.selected(i);
			const Synapse *y = NULL;
			if (_draw_opts.axon_conns()) {
				for (size32_t a_index = sel->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
					y = _model.synapse(a_index);
					if (_draw_opts.only_show_marked() && !_state.is_marked(y, a_index)) { continue; }
					size32_t d_index = y->den_soma_index();
					const Soma *d = _model.soma(d_index);
					const Soma_Type *u = _model.type(d->type_index());
					bool outside = !clip_volume.contains(d->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					if (u->display_state() == Soma_Type::HIDDEN || (only_show_clipped && outside) ||
						_draw_opts.only_show_selected()) {
						d->draw_for_selection(d_index);
					}
				}
			}
			if (_draw_opts.den_conns()) {
				for (size32_t d_index = sel->first_den_syn_index(); d_index < ny; d_index = y->next_den_syn_index()) {
					y = _model.synapse(d_index);
					if (_draw_opts.only_show_marked() && !_state.is_marked(y, d_index)) { continue; }
					size32_t a_index = y->axon_soma_index();
					const Soma *a = _model.soma(a_index);
					const Soma_Type *u = _model.type(a->type_index());
					bool outside = !clip_volume.contains(a->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					if (u->display_state() == Soma_Type::HIDDEN || (only_show_clipped && outside) ||
						_draw_opts.only_show_selected()) {
						a->draw_for_selection(a_index);
					}
				}
			}
			size32_t sel_index = _state.selected_index(i);
			sel->draw_for_selection(sel_index);
		}
		// Imitate drawing the somas of marked synapses
		for (marked_syns_t::const_iterator it = _state.begin_marked_synapses(); it != _state.end_marked_synapses(); ++it) {
			const Synapse *y = it->first;
			if ((_draw_opts.axon_conns() || _draw_opts.den_conns()) && !_draw_opts.only_conn_selected()) {
				size32_t a_index = y->axon_soma_index();
				const Soma *a = _model.soma(a_index);
				const Soma_Type *t = _model.type(a->type_index());
				bool a_outside = !clip_volume.contains(a->coords());
				size32_t d_index = y->den_soma_index();
				const Soma *d = _model.soma(d_index);
				const Soma_Type *u = _model.type(d->type_index());
				bool d_outside = !clip_volume.contains(d->coords());
				if (t->display_state() == Soma_Type::HIDDEN || (only_show_clipped && a_outside) ||
					(_draw_opts.only_show_selected() && !_state.is_selected(a) && !_draw_opts.axon_conns())) {
					a->draw_for_selection(a_index);
				}
				if (u->display_state() == Soma_Type::HIDDEN || (only_show_clipped && d_outside) ||
					(_draw_opts.only_show_selected() && !_state.is_selected(d) && !_draw_opts.den_conns())) {
					d->draw_for_selection(d_index);
				}
			}
		}
		if (only_show_clipped) { Clip_Volume::enable(); }
	}
	glEnd();
#ifdef _DEBUG
	Image *image = new(std::nothrow) Image("viz_select_buffer.png", (size_t)w(), (size_t)h());
	if (image) {
		image->write(Image::PNG);
		image->close();
		delete image;
	}
#endif
	GLubyte px[3];
	glReadPixels(_click_coords[0], _click_coords[1], 1, 1, GL_RGB, GL_UNSIGNED_BYTE, px);
	size32_t sel_index = ((size32_t)px[0] << 16) | ((size32_t)px[1] << 8) | (size32_t)px[2];
	bool hit = sel_index-- > 0;
	const Soma *sel = hit ? _model.soma(sel_index) : NULL;
	// Ctrl+click or right-click doesn't select, but shows detailed information
	if (Fl::event_ctrl() || Fl::event_button() == FL_RIGHT_MOUSE) {
		refresh();
		Viz_Window *vw = static_cast<Viz_Window *>(parent());
		if (sel) {
			vw->summary_dialog(sel, sel_index);
		}
		else {
			vw->summary_dialog();
		}
		refresh();
		return;
	}
	remember(_state);
	_prev_state = _state;
	// Shift+click or CapsLock+click allows deselection and multiple selection
	if (Fl::event_state(FL_SHIFT | FL_CAPS_LOCK)) {
		if (!hit) {
			refresh();
			return;
		}
	}
	// Modify selection
	else {
		_state.deselect_all();
		if (!hit) {
			refresh_selected();
			refresh();
			return;
		}
	}
	if (_state.is_selected(sel)) {
		_state.deselect(sel);
		refresh_selected();
	}
	else if (_state.select(sel, sel_index)) {
		refresh_selected();
	}
	refresh();
}

void Model_Area::clip() {
	make_current();
	refresh_projection(CLIPPING);
	refresh_view();
	Clip_Volume &v = _state.clip_volume();
	Clip_Volume pv, tv;
	pv.copy(v);
	v.acquire();
	Bounds b;
	size_t hits = 0;
	size32_t n = _model.num_somas();
	for (size32_t i = 0; i < n; i++) {
		const Soma *s = _model.soma(i);
		const Soma_Type *t = _model.type(s->type_index());
		if (!t->visible()) { continue; }
		const coord_t *c = s->coords();
		if (!v.contains(c)) { continue; }
		hits++;
		b.update(c);
	}
	if (hits >= 2) {
		tv.copy(v);
		v.copy(pv);
		remember(_state);
		_prev_state = _state;
		v.copy(tv);
		_state.reclip(b);
	}
	else {
		v.copy(pv);
	}
	refresh();
}

void Model_Area::pan() {
	double d[2];
	d[0] = (double)(_drag_coords[0] - _click_coords[0]) / w();
	d[1] = (double)(_drag_coords[1] - _click_coords[1]) / h();
	if (w() > h()) { d[0] *= (double)w() / h(); }
	else { d[1] *= (double)h() / w(); }
	_state.pan(_prev_state.pan_x() + PAN_SCALE * _state.zoom() * d[0],
		_prev_state.pan_y() + PAN_SCALE * _state.zoom() * d[1]);
	refresh();
}

void Model_Area::rotate() {
	double r, p1[2], p2[2];
	double dm[16], m[16];
	double dv[2], av[2];
	double dp, theta;
	const double *pr = NULL;
	r = 1.0;
	if (_scale_rotation) { r /= _state.zoom(); }
	p1[0] = 2.0 * _click_coords[0] / w() - 1.0; p1[1] = 2.0 * _click_coords[1] / h() - 1.0;
	p2[0] = 2.0 * _drag_coords[0] / w() - 1.0; p2[1] = 2.0 * _drag_coords[1] / h() - 1.0;
	if (w() > h()) { p1[0] *= (double)w() / h(); p2[0] *= (double)w() / h(); }
	else { p1[1] *= (double)h() / w(); p2[1] *= (double)h() / w(); }
	switch (_rotation_mode) {
	case ARCBALL_2D:
		p2[0] -= p1[0]; p2[1] -= p1[1];
		p1[0] = p1[1] = 0.0;
		arcball_matrix(r, p1, p2, dm);
		matrix_mul(_prev_state.rotate(), dm, m);
		break;
	case ARCBALL_3D:
		arcball_matrix(r, p1, p2, dm);
		matrix_mul(_prev_state.rotate(), dm, m);
		break;
	case AXIS_X:
		dv[0] = p2[0] - p1[0]; dv[1] = p2[1] - p1[1];
		pr = _prev_state.rotate();
		av[0] = pr[0]; av[1] = pr[1]; // rotated x-axis
		if (vector2_length(av) > ROTATE_AXIS_TOLERANCE) {
			dp = vector2_length(dv) / TWO_SQRT_2 * sin(vector2_angle(av, dv));
			theta = dp * TWO_PI;
		}
		else {
			theta = vector2_angle(p2, p1);
			if (pr[2] < 0.0) { theta *= -1.0; }
		}
		rotate_x_matrix(theta, dm);
		matrix_mul(dm, _prev_state.rotate(), m);
		break;
	case AXIS_Y:
		dv[0] = p2[0] - p1[0]; dv[1] = p2[1] - p1[1];
		pr = _prev_state.rotate();
		av[0] = pr[4]; av[1] = pr[5]; // rotated y-axis
		if (vector2_length(av) > ROTATE_AXIS_TOLERANCE) {
			dp = vector2_length(dv) / TWO_SQRT_2 * sin(vector2_angle(av, dv));
			theta = dp * TWO_PI;
		}
		else {
			theta = vector2_angle(p2, p1);
			if (pr[6] < 0.0) { theta *= -1.0; }
		}
		rotate_y_matrix(theta, dm);
		matrix_mul(dm, _prev_state.rotate(), m);
		break;
	case AXIS_Z:
		dv[0] = p2[0] - p1[0]; dv[1] = p2[1] - p1[1];
		pr = _prev_state.rotate();
		av[0] = pr[8]; av[1] = pr[9]; // rotated z-axis
		if (vector2_length(av) > ROTATE_AXIS_TOLERANCE) {
			dp = vector2_length(dv) / TWO_SQRT_2 * sin(vector2_angle(av, dv));
			theta = dp * TWO_PI;
		}
		else {
			theta = vector2_angle(p2, p1);
			if (pr[10] < 0.0) { theta *= -1.0; }
		}
		rotate_z_matrix(theta, dm);
		matrix_mul(dm, _prev_state.rotate(), m);
		break;
	}
	_state.rotate(m);
	refresh();
}

void Model_Area::zoom() {
	double f = (double)(_drag_coords[1] - _click_coords[1]) / h();
	if (!_invert_zoom) { f *= -1; }
	_state.zoom(_prev_state.zoom() * pow(ZOOM_SCALE, f));
	refresh();
}

void Model_Area::mark() {
	make_current();
	refresh_projection(SELECTING);
	refresh_view();
	// Find closest hit, if any
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // corresponds to index 0, not used for any synapse
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPointSize(SELECT_TOLERANCE);
	glBegin(GL_POINTS);
	size32_t y_i = 0;
	if (_model.has_weights() && _draw_opts.display() == Draw_Options::WEIGHTS) {
		const Weights *wt = _model.const_weights();
		const weights_instance_t &wcs = wt->weight_changes();
		if (!_draw_opts.only_show_selected() && _draw_opts.syn_dots()) {
			// Imitate drawing the active synapses
			for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
				size32_t y_index = ys->first;
				const Synapse *y = _model.synapse(y_index);
				if (_draw_opts.only_show_marked() && !_state.is_marked(y, y_index)) { continue; }
				size32_t a_index = y->axon_soma_index();
				const Soma *a = _model.soma(a_index);
				const Soma_Type *t = _model.type(a->type_index());
				if (!t->visible()) { continue; }
				size32_t d_index = y->den_soma_index();
				const Soma *d = _model.soma(d_index);
				const Soma_Type *u = _model.type(d->type_index());
				if (!u->visible()) { continue; }
				y->draw_for_selection(y_i++);
			}
		}
		else if (_draw_opts.only_show_selected() && (_draw_opts.axon_conns() || _draw_opts.den_conns() || _draw_opts.syn_dots())) {
			bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
			if (only_show_clipped) { Clip_Volume::disable(); }
			// Imitate drawing the selected somas' synapses
			for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
				size32_t y_index = ys->first;
				const Synapse *y = _model.synapse(y_index);
				size32_t a_index = y->axon_soma_index();
				const Soma *a = _model.soma(a_index);
				const Soma_Type *t = _model.type(a->type_index());
				if (t->display_state() == Soma_Type::DISABLED) { continue; }
				bool a_sel = _state.is_selected(a);
				size32_t d_index = y->den_soma_index();
				const Soma *d = _model.soma(d_index);
				const Soma_Type *u = _model.type(d->type_index());
				if (u->display_state() == Soma_Type::DISABLED) { continue; }
				bool d_sel = _state.is_selected(d);
				if (((!a_sel && !d_sel) || _draw_opts.only_show_marked()) && !_state.is_marked(y, y_index)) { continue; }
				if (_draw_opts.axon_conns() || _draw_opts.den_conns() || _draw_opts.syn_dots()) {
					y->draw_for_selection(y_i++);
				}
			}
			if (only_show_clipped) { Clip_Volume::enable(); }
		}
	}
	else if (_draw_opts.display() == Draw_Options::STATIC_MODEL && _draw_opts.syn_dots()) {
		bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
		bool only_enable_clipped = _state.clipped() && _draw_opts.only_enable_clipped();
		if (only_show_clipped) { Clip_Volume::disable(); }
		const Clip_Volume &clip_volume = _state.const_clip_volume();
		size_t n = _state.num_selected();
		size32_t ny = _model.num_synapses();
		// Imitate drawing the selected somas' synapses
		for (size_t i = 0; i < n && !_draw_opts.only_show_marked(); i++) {
			const Soma *s = _state.selected(i);
			const Synapse *y = NULL;
			if (!s->num_axon_fields() && !s->num_den_fields()) { continue; }
			// Imitate drawing the axonal synapses
			for (size32_t a_index = s->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
				y = _model.synapse(a_index);
				if (_state.is_marked(y, a_index)) { continue; }
				const coord_t *c = y->coords();
				if (only_enable_clipped && !clip_volume.contains(c)) { continue; }
				const Soma *d = _model.soma(y->den_soma_index());
				if (_draw_opts.only_conn_selected() && !_state.is_selected(d)) { continue; }
				const Soma_Type *u = _model.type(d->type_index());
				bool outside = !clip_volume.contains(d->coords());
				if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
				y->draw_for_selection(y_i++);
			}
			// Imitate drawing the dendritic synapses
			for (size32_t d_index = s->first_den_syn_index(); d_index < ny; d_index = y->next_den_syn_index()) {
				y = _model.synapse(d_index);
				if (_state.is_marked(y, d_index)) { continue; }
				const coord_t *c = y->coords();
				if (only_enable_clipped && !clip_volume.contains(c)) { continue; }
				const Soma *a = _model.soma(y->axon_soma_index());
				if (_draw_opts.only_conn_selected() && !_state.is_selected(a)) { continue; }
				const Soma_Type *u = _model.type(a->type_index());
				bool outside = !clip_volume.contains(a->coords());
				if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
				y->draw_for_selection(y_i++);
			}
		}
		// Imitate drawing the marked synapses
		for (marked_syns_t::const_iterator it = _state.begin_marked_synapses(); it != _state.end_marked_synapses(); ++it) {
			const Synapse *y = it->first;
			y->draw_for_selection(y_i++);
		}
		if (only_show_clipped) { Clip_Volume::enable(); }
	}
	glEnd();
#ifdef _DEBUG
	Image *image = new(std::nothrow) Image("viz_select_synapse_buffer.png", (size_t)w(), (size_t)h());
	if (image) {
		image->write(Image::PNG);
		image->close();
		delete image;
	}
#endif
	GLubyte px[3];
	glReadPixels(_click_coords[0], _click_coords[1], 1, 1, GL_RGB, GL_UNSIGNED_BYTE, px);
	size32_t sel_i = ((size32_t)px[0] << 16) | ((size32_t)px[1] << 8) | (size32_t)px[2];
	const Synapse *sel = NULL;
	bool hit = sel_i-- > 0;
	if (hit) {
		size32_t search_i = 0;
		if (_model.has_weights() && _draw_opts.display() == Draw_Options::WEIGHTS) {
			const Weights *wt = _model.const_weights();
			const weights_instance_t &wcs = wt->weight_changes();
			if (!_draw_opts.only_show_selected() && _draw_opts.syn_dots()) {
				// Search the active synapses
				for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
					size32_t y_index = ys->first;
					const Synapse *y = _model.synapse(y_index);
					if (_draw_opts.only_show_marked() && !_state.is_marked(y, y_index)) { continue; }
					size32_t a_index = y->axon_soma_index();
					const Soma *a = _model.soma(a_index);
					const Soma_Type *t = _model.type(a->type_index());
					if (!t->visible()) { continue; }
					size32_t d_index = y->den_soma_index();
					const Soma *d = _model.soma(d_index);
					const Soma_Type *u = _model.type(d->type_index());
					if (!u->visible()) { continue; }
					if (search_i++ == sel_i) { sel = y; break; }
				}
			}
			else if (_draw_opts.only_show_selected() && (_draw_opts.axon_conns() || _draw_opts.den_conns() || _draw_opts.syn_dots())) {
				bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
				if (only_show_clipped) { Clip_Volume::disable(); }
				// Search the selected somas' synapses
				for (weights_instance_t::const_iterator ys = wcs.begin(); ys != wcs.end(); ++ys) {
					size32_t y_index = ys->first;
					const Synapse *y = _model.synapse(y_index);
					size32_t a_index = y->axon_soma_index();
					const Soma *a = _model.soma(a_index);
					const Soma_Type *t = _model.type(a->type_index());
					if (t->display_state() == Soma_Type::DISABLED) { continue; }
					bool a_sel = _state.is_selected(a);
					size32_t d_index = y->den_soma_index();
					const Soma *d = _model.soma(d_index);
					const Soma_Type *u = _model.type(d->type_index());
					if (u->display_state() == Soma_Type::DISABLED) { continue; }
					bool d_sel = _state.is_selected(d);
					if (((!a_sel && !d_sel) || _draw_opts.only_show_marked()) && !_state.is_marked(y, y_index)) { continue; }
					if (_draw_opts.axon_conns() || _draw_opts.den_conns() || _draw_opts.syn_dots()) {
						if (search_i++ == sel_i) { sel = y; break; }
					}
				}
				if (only_show_clipped) { Clip_Volume::enable(); }
			}
		}
		else if (_draw_opts.display() == Draw_Options::STATIC_MODEL && _draw_opts.syn_dots()) {
			bool only_show_clipped = _state.clipped() && _draw_opts.only_show_clipped();
			bool only_enable_clipped = _state.clipped() && _draw_opts.only_enable_clipped();
			if (only_show_clipped) { Clip_Volume::disable(); }
			const Clip_Volume &clip_volume = _state.const_clip_volume();
			size_t n = _state.num_selected();
			size32_t ny = _model.num_synapses();
			// Search the selected somas' synapses
			for (size_t i = 0; i < n && !_draw_opts.only_show_marked(); i++) {
				const Soma *s = _state.selected(i);
				const Synapse *y = NULL;
				if (!s->num_axon_fields() && !s->num_den_fields()) { continue; }
				// Search the axonal synapses
				for (size32_t a_index = s->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
					y = _model.synapse(a_index);
					if (_state.is_marked(y, a_index) || _draw_opts.only_show_marked()) { continue; }
					const coord_t *c = y->coords();
					if (only_enable_clipped && !clip_volume.contains(c)) { continue; }
					const Soma *d = _model.soma(y->den_soma_index());
					const Soma_Type *u = _model.type(d->type_index());
					bool outside = !clip_volume.contains(d->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					if (search_i++ == sel_i) { sel = y; break; }
				}
				if (sel) { break; }
				// Search the dendritic synapses
				for (size32_t d_index = s->first_den_syn_index(); d_index < ny; d_index = y->next_den_syn_index()) {
					// False positive "C28182: Dereferencing a copy of a null pointer" error with Visual Studio 2013 code analysis
					y = _model.synapse(d_index);
					if (_state.is_marked(y, d_index) || _draw_opts.only_show_marked()) { continue; }
					const coord_t *c = y->coords();
					if (only_enable_clipped && !clip_volume.contains(c)) { continue; }
					const Soma *a = _model.soma(y->axon_soma_index());
					const Soma_Type *u = _model.type(a->type_index());
					bool outside = !clip_volume.contains(a->coords());
					if (u->display_state() == Soma_Type::DISABLED || (only_enable_clipped && outside)) { continue; }
					if (search_i++ == sel_i) { sel = y; break; }
				}
				if (sel) { break; }
			}
			for (marked_syns_t::const_iterator it = _state.begin_marked_synapses(); it != _state.end_marked_synapses(); ++it) {
				const Synapse *y = it->first;
				if (search_i++ == sel_i) { sel = y; break; }
			}
			if (only_show_clipped) { Clip_Volume::enable(); }
		}
	}
	hit = sel != NULL;
	size32_t sel_index = hit ? _model.synapse_index(sel) : NULL_INDEX;
	// Ctrl+click or right-click doesn't mark, but shows detailed information
	if (Fl::event_ctrl() || Fl::event_button() == FL_RIGHT_MOUSE) {
		refresh();
		Viz_Window *vw = static_cast<Viz_Window *>(parent());
		if (sel) {
			vw->summary_dialog(sel, sel_index);
		}
		else {
			vw->summary_dialog();
		}
		refresh();
		return;
	}
	remember(_state);
	_prev_state = _state;
	// Shift+click or CapsLock+click allows unmarking and multiple markings
	if (Fl::event_state(FL_SHIFT | FL_CAPS_LOCK)) {
		if (!hit) {
			refresh();
			return;
		}
	}
	// Modify selection
	else {
		_state.unmark_all();
		if (!hit) {
			refresh_selected();
			refresh();
			return;
		}
	}
	if (_state.is_marked(sel, sel_index)) {
		_state.unmark(sel, sel_index);
		refresh_selected();
	}
	else if (_state.mark(sel, sel_index)) {
		refresh_selected();
	}
	refresh();
}
