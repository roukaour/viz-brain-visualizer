#include <cstdlib>
#include <string>

#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#pragma warning(pop)

#include "coords.h"
#include "model-area.h"
#include "soma-type.h"
#include "synapse.h"
#include "draw-options.h"
#include "utils.h"
#include "input-parser.h"
#include "binary-parser.h"
#include "soma.h"

const float Soma::AXON_COLOR[3] = {0.0f, 0.5f, 1.0f}; // blue
const float Soma::DENDRITE_COLOR[3] = {0.0f, 1.0f, 0.0f}; // green
const float Soma::CONN_AXON_COLOR[3] = {0.0f, 0.3125f, 0.625f}; // dark blue
const float Soma::CONN_DENDRITE_COLOR[3] = {0.0f, 0.5f, 0.0f}; // dark green

int Soma::_soma_font_size = 12;

Soma::Soma() : _first_axon_syn_index(NULL_INDEX), _first_den_syn_index(NULL_INDEX), _num_axon_syns(0), _num_den_syns(0),
	_first_axon_field_index(0), _first_den_field_index(0), _id(0), _coords(), _type_index(0), _num_axon_fields(0),
	_num_den_fields(0) {}

Soma::~Soma() {
	_first_axon_field_index = _first_den_field_index = NULL_INDEX;
	_num_axon_fields = _num_den_fields = 0;
	_first_axon_syn_index = _first_den_syn_index = NULL_INDEX;
	_num_axon_syns = _num_den_syns = 0;
}

void Soma::initialize() {
	_first_axon_syn_index = _first_den_syn_index = NULL_INDEX;
	_num_axon_syns = _num_den_syns = 0;
}

void Soma::first_axon_syn(Synapse *ay, size32_t a_index) {
	if (ay == NULL || a_index == NULL_INDEX) {
		_first_axon_syn_index = NULL_INDEX;
		_num_axon_syns = 0;
	}
	else {
		ay->next_axon_syn_index(_first_axon_syn_index);
		_first_axon_syn_index = a_index;
		_num_axon_syns++;
	}
}

void Soma::first_den_syn(Synapse *dy, size32_t d_index) {
	if (dy == NULL || d_index == NULL_INDEX) {
		_first_den_syn_index = NULL_INDEX;
		_num_den_syns = 0;
	}
	else {
		dy->next_den_syn_index(_first_den_syn_index);
		_first_den_syn_index = d_index;
		_num_den_syns++;
	}
}

void Soma::draw() const {
	glVertex3cv(_coords);
}

void Soma::draw_letter(const Soma_Type *t) const {
	if (t->display_state() == Soma_Type::LETTER) {
		char l = t->letter();
		glRasterPos3cv(_coords);
		gl_draw(&l, 1);
	}
	else {
		glBegin(GL_POINTS);
		glVertex3cv(_coords);
		glEnd();
	}
}

void Soma::draw_firing(bool firing) const {
	glPointSize(firing ? 5.0f : 3.0f);
	glBegin(GL_POINTS);
	glVertex3cv(coords());
	glEnd();
}

void Soma::draw_firing_letter(const Soma_Type *t, bool firing) const {
	if (t->display_state() == Soma_Type::LETTER) {
		char l = t->letter();
		gl_font(firing ? SOMA_FIRING_FONT : SOMA_FONT, _soma_font_size);
		glRasterPos3cv(coords());
		gl_draw(&l, 1);
	}
	else {
		glPointSize(firing ? 5.0f : 3.0f);
		glBegin(GL_POINTS);
		glVertex3cv(coords());
		glEnd();
	}
}

void Soma::draw_firing_value(std::string s, bool firing) const {
	gl_font(firing ? SOMA_FIRING_FONT : SOMA_FONT, _soma_font_size);
	glRasterPos3cv(coords());
	gl_draw(s.c_str(), (int)s.length());
}

void Soma::draw_circled(const float *cv, const float *bgcv) const {
	glColor3fv(cv);
	glPointSize(13.0f);
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glDisable(GL_DEPTH_TEST);
	glColor3fv(bgcv);
	glPointSize(9.0f);
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glColor3fv(cv);
	glPointSize(3.0f);
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glEnable(GL_DEPTH_TEST);
}

void Soma::draw_circled_firing(const Soma_Type *t, const float *cv, const float *bgcv, bool firing,
	const double model_view[16], const double projection[16], const int viewport[4]) const {
		glColor3fv(cv);
		glPointSize(17.0f);
		glBegin(GL_POINTS);
		glVertex3cv(_coords);
		glEnd();
		glDisable(GL_DEPTH_TEST);
		glColor3fv(bgcv);
		glPointSize(13.0f);
		glBegin(GL_POINTS);
		glVertex3cv(_coords);
		glEnd();
		// Position letter in center of circle
		char l = t->letter();
		double wc[3], sc[3];
		gluProject(_coords[0], _coords[1], _coords[2], model_view, projection, viewport, &wc[0], &wc[1], &wc[2]);
		wc[0] -= 4;
		wc[1] -= 4;
		gluUnProject(wc[0], wc[1], wc[2], model_view, projection, viewport, &sc[0], &sc[1], &sc[2]);
		glColor3fv(cv);
		gl_font(firing ? SOMA_FIRING_FONT : SOMA_FONT, _soma_font_size);
		glRasterPos3dv(sc);
		gl_draw(&l, 1);
		glEnable(GL_DEPTH_TEST);
}

void Soma::draw_fields(bool conn, const Brain_Model &bm) const {
	glColor3fv(conn ? CONN_AXON_COLOR : AXON_COLOR);
	for (size32_t i = 0; i < _num_axon_fields; i++) {
		bm.field(_first_axon_field_index + i)->draw();
	}
	glColor3fv(conn ? CONN_DENDRITE_COLOR : DENDRITE_COLOR);
	for (size32_t i = 0; i < _num_den_fields; i++) {
		bm.field(_first_den_field_index + i)->draw();
	}
}

void Soma::draw_for_selection(size32_t i) const {
	i++;
	GLubyte r = (i >> 16) & 0xFF;
	GLubyte g = (i >> 8) & 0xFF;
	GLubyte b = i & 0xFF;
	glColor3ub(r, g, b);
	glVertex3cv(_coords);
}

void Soma::read_from(Input_Parser &ip, size32_t next_field_index) {
	// A line defining a soma is formatted as:
	//     type_index soma_id x y z num_axon_fields num_den_fields
	// followed by (num_axon_fields + num_den_fields) lines defining neuritic fields.
	_type_index = ip.get_size8();
	_id = ip.get_size32();
	_coords[0] = ip.get_coord(); _coords[1] = ip.get_coord(); _coords[2] = ip.get_coord();
	_num_axon_fields = ip.get_size8();
	_num_den_fields = ip.get_size8();
	_first_axon_field_index = next_field_index;
	_first_den_field_index = next_field_index + _num_axon_fields;
	initialize();
}

void Soma::read_from(Binary_Parser &bp, size32_t next_field_index) {
	// A sequence defining a soma is formatted as:
	//     type_index:uv soma_id:uv x:sv y:sv z:sv num_axon_fields:uv num_den_fields:uv
	// followed by (num_axon_fields + num_den_fields) sequences defining neuritic fields.
	_type_index = bp.get_size8();
	_id = bp.get_size32();
	_coords[0] = bp.get_coord(); _coords[1] = bp.get_coord(); _coords[2] = bp.get_coord();
	_num_axon_fields = bp.get_size8();
	_num_den_fields = bp.get_size8();
	_first_axon_field_index = next_field_index;
	_first_den_field_index = next_field_index + _num_axon_fields;
	initialize();
}
