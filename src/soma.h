#ifndef SOMA_H
#define SOMA_H

#include "utils.h"
#include "coords.h"
#include "neuritic-field.h"

#define SOMA_FONT FL_HELVETICA
#define SOMA_FIRING_FONT FL_HELVETICA_BOLD_ITALIC

class Brain_Model;
class Synapse;
class Soma_Type;
class Input_Parser;
class Binary_Parser;

class Soma {
private:
	static const float AXON_COLOR[3], DENDRITE_COLOR[3], CONN_AXON_COLOR[3], CONN_DENDRITE_COLOR[3];
	static int _soma_font_size;
public:
	inline static int soma_letter_size(void) { return _soma_font_size; }
	inline static void soma_letter_size(int s) { _soma_font_size = s; }
private:
	size32_t _first_axon_syn_index, _first_den_syn_index;
	size32_t _num_axon_syns, _num_den_syns;
	size32_t _first_axon_field_index, _first_den_field_index;
	size32_t _id;
	coord_t _coords[3];
	size8_t _type_index, _num_axon_fields, _num_den_fields;
public:
	Soma();
	~Soma();
private:
	void initialize(void);
public:
	inline size32_t id(void) const { return _id; }
	inline size8_t type_index(void) const { return _type_index; }
	inline const coord_t *coords(void) const { return _coords; }
	inline size8_t num_axon_fields(void) const { return _num_axon_fields; }
	inline size32_t first_axon_field_index(void) const { return _first_axon_field_index; }
	inline size8_t num_den_fields(void) const { return _num_den_fields; }
	inline size32_t first_den_field_index(void) const { return _first_den_field_index; }
	inline size32_t num_axon_syns(void) const { return _num_axon_syns; }
	inline size32_t first_axon_syn_index(void) const { return _first_axon_syn_index; }
	void first_axon_syn(Synapse *ay, size32_t ai);
	inline size32_t num_den_syns(void) const { return _num_den_syns; }
	inline size32_t first_den_syn_index(void) const { return _first_den_syn_index; }
	void first_den_syn(Synapse *dy, size32_t di);
	void draw(void) const;
	void draw_letter(const Soma_Type *t) const;
	void draw_firing(bool firing) const;
	void draw_firing_letter(const Soma_Type *t, bool firing) const;
	void draw_firing_value(std::string s, bool firing) const;
	void draw_circled(const float *cv, const float *bgcv) const;
	void draw_circled_firing(const Soma_Type *t, const float *cv, const float *bgcv, bool firing,
		const double model_view[16], const double projection[16], const int viewport[4]) const;
	void draw_fields(bool conn, const Brain_Model &bm) const;
	void draw_for_selection(size32_t i) const;
	void read_from(Input_Parser &ip, size32_t next_field_index);
	void read_from(Binary_Parser &bp, size32_t next_field_index);
};

#endif
