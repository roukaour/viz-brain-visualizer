#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#pragma warning(pop)

#include "coords.h"
#include "input-parser.h"
#include "binary-parser.h"
#include "neuritic-field.h"

Neuritic_Field::Neuritic_Field() {
	_min[0] = _min[1] = _min[2] = ZERO_COORD;
	_max[0] = _max[1] = _max[2] = ZERO_COORD;
}

void Neuritic_Field::draw() const {
	glBegin(GL_LINE_LOOP);
	glVertex3c(_min[0], _min[1], _min[2]);
	glVertex3c(_min[0], _max[1], _min[2]);
	glVertex3c(_max[0], _max[1], _min[2]);
	glVertex3c(_max[0], _min[1], _min[2]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3c(_min[0], _min[1], _max[2]);
	glVertex3c(_min[0], _max[1], _max[2]);
	glVertex3c(_max[0], _max[1], _max[2]);
	glVertex3c(_max[0], _min[1], _max[2]);
	glEnd();
	glBegin(GL_LINES);
	glVertex3c(_min[0], _min[1], _min[2]);
	glVertex3c(_min[0], _min[1], _max[2]);
	glVertex3c(_min[0], _max[1], _min[2]);
	glVertex3c(_min[0], _max[1], _max[2]);
	glVertex3c(_max[0], _max[1], _min[2]);
	glVertex3c(_max[0], _max[1], _max[2]);
	glVertex3c(_max[0], _min[1], _min[2]);
	glVertex3c(_max[0], _min[1], _max[2]);
	glEnd();
}

void Neuritic_Field::read_from(Input_Parser &ip) {
	// A line defining a neuritic field is formatted:
	//     x_min x_max y_min y_max z_min z_max
	_min[0] = ip.get_coord(); _max[0] = ip.get_coord();
	_min[1] = ip.get_coord(); _max[1] = ip.get_coord();
	_min[2] = ip.get_coord(); _max[2] = ip.get_coord();
}

void Neuritic_Field::read_from(Binary_Parser &bp) {
	// A sequence defining a neuritic field is formatted:
	//     x_min:i32 x_max:i32 y_min:i32 y_max:i32 z_min:i32 z_max:i32
	_min[0] = bp.get_coord(); _max[0] = bp.get_coord();
	_min[1] = bp.get_coord(); _max[1] = bp.get_coord();
	_min[2] = bp.get_coord(); _max[2] = bp.get_coord();
}
