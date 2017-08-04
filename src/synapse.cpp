#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#pragma warning(pop)

#include "coords.h"
#include "utils.h"
#include "input-parser.h"
#include "binary-parser.h"
#include "soma.h"
#include "brain-model.h"
#include "synapse.h"

Synapse::Synapse() : _axon_soma_index(NULL_INDEX), _den_soma_index(NULL_INDEX), _next_axon_syn_index(NULL_INDEX),
	_next_den_syn_index(NULL_INDEX), _coords(), _via_coords() {}

void Synapse::draw() const {
	glVertex3cv(_coords);
}

void Synapse::draw_marked(const float *cv, const float *bgcv) const {
	glColor3fv(cv);
	glPointSize(8.0f);
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glDisable(GL_DEPTH_TEST);
	glColor3fv(bgcv);
	glPointSize(6.0f);
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glColor3fv(cv);
	glPointSize(4.0f);
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glColor3fv(bgcv);
	glPointSize(2.0f);
	glBegin(GL_POINTS);
	glVertex3cv(_coords);
	glEnd();
	glEnable(GL_DEPTH_TEST);
}

void Synapse::draw_conn(const Soma *a, const Soma *d, bool to_axon, bool to_via, bool to_syn, bool to_den) const {
	glBegin(GL_LINE_STRIP);
	if (to_axon) { glVertex3cv(a->coords()); }
	if (to_via) { glVertex3cv(_via_coords); }
	if (to_syn) { glVertex3cv(_coords); }
	if (to_den) { glVertex3cv(d->coords()); }
	glEnd();
}

void Synapse::draw_for_selection(size32_t i) const {
	i++;
	GLubyte r = (i >> 16) & 0xFF;
	GLubyte g = (i >> 8) & 0xFF;
	GLubyte b = i & 0xFF;
	glColor3ub(r, g, b);
	glVertex3cv(_coords);
}

void Synapse::read_from(Input_Parser &ip, const Brain_Model &bm) {
	// A line defining a synapse is formatted as:
	//     synapse_index 'v' axon_id den_id via_x via_y via_z x y z
	// Or for a synapse without a via point, as:
	//     synapse_index axon_id den_id x y z
	ip.get_size64(); // synapse index; TODO: remove these from the file format
	if (ip.peek() == 'v') {
		ip.get_char();
		_axon_soma_index = bm.soma_index(ip.get_size32());
		_den_soma_index = bm.soma_index(ip.get_size32());
		_via_coords[0] = ip.get_coord(); _via_coords[1] = ip.get_coord(); _via_coords[2] = ip.get_coord();
		_coords[0] = ip.get_coord(); _coords[1] = ip.get_coord(); _coords[2] = ip.get_coord();
	}
	else {
		_axon_soma_index = bm.soma_index(ip.get_size32());
		_den_soma_index = bm.soma_index(ip.get_size32());
		_coords[0] = ip.get_coord(); _coords[1] = ip.get_coord(); _coords[2] = ip.get_coord();
		_via_coords[0] = _coords[0]; _via_coords[1] = _coords[1]; _via_coords[2] = _coords[2];
	}
}

void Synapse::read_from(Binary_Parser &bp, const Brain_Model &bm) {
	// A sequence defining a synapse is formatted as:
	//     synapse_index:uv 1:uv axon_id:uv den_id:uv via_x:sv via_y:sv via_z:sv x:sv y:sv z:sv
	// Or for a synapse without a via point, as:
	//     synapse_index:uv 0:uv axon_id:uv den_id:uv x:sv y:sv z:sv
	bp.get_unsigned(); // synapse index; TODO: remove these from the file format
	bool has_via = bp.get_bool();
	if (has_via) {
		_axon_soma_index = bm.soma_index(bp.get_size32());
		_den_soma_index = bm.soma_index(bp.get_size32());
		_via_coords[0] = bp.get_coord(); _via_coords[1] = bp.get_coord(); _via_coords[2] = bp.get_coord();
		_coords[0] = bp.get_coord(); _coords[1] = bp.get_coord(); _coords[2] = bp.get_coord();
	}
	else {
		_axon_soma_index = bm.soma_index(bp.get_size32());
		_den_soma_index = bm.soma_index(bp.get_size32());
		_coords[0] = bp.get_coord(); _coords[1] = bp.get_coord(); _coords[2] = bp.get_coord();
		_via_coords[0] = _coords[0]; _via_coords[1] = _coords[1]; _via_coords[2] = _coords[2];
	}
}
