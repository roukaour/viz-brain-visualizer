#ifndef MODEL_STATE_H
#define MODEL_STATE_H

#include <fstream>
#include <unordered_set>

#include "utils.h"
#include "clip-volume.h"

class Soma;
class Input_Parser;
class Brain_Model;

struct marked_syn_hash {
	inline size_t operator()(const std::pair<const Synapse *, size32_t> &v) const {
		return (size_t)v.first ^ (size_t)v.second;
	}
};

typedef std::pair<const Synapse *, size32_t> marked_syn_t;
typedef std::unordered_set<marked_syn_t, marked_syn_hash> marked_syns_t;

class Model_State {
public:
	static const double MIN_ZOOM;
	static const double MAX_ZOOM;
	static const size_t MAX_SELECTED = 1000;
private:
	double _rotate_matrix[16];
	double _pan_vector[2];
	double _zoom_factor;
	coord_t _pivot[3];
	Clip_Volume _clip_volume;
	bool _clipped;
	Bounds _bounds;
	size_t _num_selected;
	const Soma *_selected[MAX_SELECTED];
	size32_t _selected_indices[MAX_SELECTED];
	marked_syns_t _marked_syns;
public:
	Model_State();
	inline const double *rotate(void) const { return _rotate_matrix; }
	void rotate(const double m[16]);
	inline const double *pan(void) const { return _pan_vector; }
	inline void pan(const double v[2]) { _pan_vector[0] = v[0]; _pan_vector[1] = v[1]; }
	inline void pan(double x, double y) { _pan_vector[0] = x; _pan_vector[1] = y; }
	inline double pan_x(void) const { return _pan_vector[0]; }
	inline void pan_x(double x) { _pan_vector[0] = x; }
	inline double pan_y(void) const { return _pan_vector[1]; }
	inline void pan_y(double y) { _pan_vector[1] = y; }
	inline double zoom(void) const { return _zoom_factor; }
	void zoom(double z);
	inline const coord_t *pivot(void) const { return _pivot; }
	inline void pivot(const coord_t *p) { _pivot[0] = p[0]; _pivot[1] = p[1]; _pivot[2] = p[2]; }
	inline bool clipped(void) const { return _clipped; }
	inline void clipped(bool c) { _clipped = c; }
	inline Clip_Volume &clip_volume(void) { return _clip_volume; }
	inline const Clip_Volume &const_clip_volume(void) const { return _clip_volume; }
	void reclip(Bounds &b);
	inline const coord_t *center(void) const { return _bounds.center(); }
	inline const coord_t *range(void) const { return _bounds.range(); }
	inline coord_t max_range(void) const { return _bounds.max_range(); }
	inline void bound(const Bounds &b) { _bounds = b; pivot(center()); }
	inline size_t num_selected(void) const { return _num_selected; }
	inline const Soma *selected(size_t i) const { return _selected[i]; }
	inline size32_t selected_index(size_t i) const { return _selected_indices[i]; }
	bool is_selected(const Soma *s) const;
	bool select(const Soma *s, size32_t index);
	bool deselect(const Soma *s);
	inline bool reselect(const Soma *s, size32_t index) { deselect(s); return select(s, index); }
	inline void deselect_all(void) { _num_selected = 0; }
	inline size_t num_marked(void) const { return _marked_syns.size(); }
	inline marked_syns_t::const_iterator begin_marked_synapses(void) const { return _marked_syns.begin(); }
	inline marked_syns_t::const_iterator end_marked_synapses(void) const { return _marked_syns.end(); }
	bool is_marked(const Synapse *y, size32_t index) const;
	bool mark(const Synapse *y, size32_t index);
	bool unmark(const Synapse *y, size32_t index);
	inline void unmark_all(void) { _marked_syns.clear(); }
	void reset(void);
	Read_Status read_from(Input_Parser &ip, const Brain_Model &bm);
	void write_to(std::ofstream &ofs) const;
};

#endif
