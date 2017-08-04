#ifndef SYNAPSE_H
#define SYNAPSE_H

#include "coords.h"

class Soma;
class Input_Parser;
class Binary_Parser;
class Brain_Model;

class Synapse {
private:
	size32_t _axon_soma_index, _den_soma_index;
	size32_t _next_axon_syn_index, _next_den_syn_index;
	coord_t _coords[3], _via_coords[3];
public:
	Synapse();
	inline size32_t axon_soma_index(void) const { return _axon_soma_index; }
	inline size32_t den_soma_index(void) const { return _den_soma_index; }
	inline size32_t next_axon_syn_index(void) const { return _next_axon_syn_index; }
	inline void next_axon_syn_index(size32_t a_index) { _next_axon_syn_index = a_index; }
	inline size32_t next_den_syn_index(void) const { return _next_den_syn_index; }
	inline void next_den_syn_index(size32_t d_index) { _next_den_syn_index = d_index; }
	inline const coord_t *coords(void) const { return _coords; }
	inline const coord_t *via_coords(void) const { return _via_coords; }
	inline bool has_via(void) const { return _coords[0] != _via_coords[0] || _coords[1] != _via_coords[1] ||
		_coords[2] != _via_coords[2]; }
	void draw(void) const;
	void draw_marked(const float *cv, const float *bgcv) const;
	void draw_conn(const Soma *a, const Soma *d, bool to_axon, bool to_via, bool to_syn, bool to_den) const;
	void draw_for_selection(size32_t i) const;
	void read_from(Input_Parser &ip, const Brain_Model &bm);
	void read_from(Binary_Parser &bp, const Brain_Model &bm);
};

#endif
