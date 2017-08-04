#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#pragma warning(pop)

#include "coords.h"
#include "utils.h"
#include "input-parser.h"
#include "binary-parser.h"
#include "brain-model.h"
#include "gap-junction.h"

Gap_Junction::Gap_Junction() : _soma1_index(NULL_INDEX), _soma2_index(NULL_INDEX), _coords() {}

Gap_Junction::~Gap_Junction() {
	_soma1_index = _soma2_index = NULL_INDEX;
}

void Gap_Junction::draw(const Soma *s1, const Soma *s2) const {
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3cv(s1->coords());
	glVertex3cv(_coords);
	glVertex3cv(s2->coords());
	glEnd();
}

void Gap_Junction::read_from(Input_Parser &ip, const Brain_Model &bm) {
	// A line defining a synapse is formatted as:
	//     soma1_id soma2_id x y z
	_soma1_index = bm.soma_index(ip.get_size32());
	_soma2_index = bm.soma_index(ip.get_size32());
	_coords[0] = ip.get_coord(); _coords[1] = ip.get_coord(); _coords[2] = ip.get_coord();
}

void Gap_Junction::read_from(Binary_Parser &bp, const Brain_Model &bm) {
	// A sequence defining a synapse is formatted as:
	//     soma1_id:uv soma2_id:uv x:sv y:sv z:sv
	_soma1_index = bm.soma_index(bp.get_size32());
	_soma2_index = bm.soma_index(bp.get_size32());
	_coords[0] = bp.get_coord(); _coords[1] = bp.get_coord(); _coords[2] = bp.get_coord();
}
