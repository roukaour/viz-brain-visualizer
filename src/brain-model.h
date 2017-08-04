#ifndef BRAIN_MODEL_H
#define BRAIN_MODEL_H

#include "from-file.h"
#include "bounds.h"
#include "soma-type.h"
#include "soma.h"
#include "synapse.h"
#include "gap-junction.h"
#include "firing-spikes.h"
#include "voltages.h"
#include "weights.h"

#define CONFIG_SEPARATOR ':'
#define CONFIG_COMMENT '#'

class Progress_Dialog;
class Input_Parser;
class Binary_Parser;

class Brain_Model : public From_File {
private:
	size8_t _num_types;
	Soma_Type *_types;
	size32_t _num_somas;
	Soma *_somas;
	size32_t _num_fields;
	Neuritic_Field *_fields;
	size32_t _num_synapses;
	Synapse *_synapses;
	size32_t _num_gap_junctions;
	Gap_Junction *_gap_junctions;
	Bounds _bounds;
	Firing_Spikes *_firing_spikes;
	Voltages *_voltages;
	Weights *_weights;
public:
	Brain_Model();
	virtual ~Brain_Model();
	inline size8_t num_types(void) const { return _num_types; }
	inline Soma_Type *type(size8_t index) const { return &_types[index]; }
	inline size32_t num_somas(void) const { return _num_somas; }
	size32_t soma_index(size32_t id) const;
	inline Soma *soma(size32_t index) const { return &_somas[index]; }
	inline size32_t num_fields(void) const { return _num_fields; }
	inline const Neuritic_Field *field(size32_t index) const { return &_fields[index]; }
	inline size32_t num_synapses(void) const { return _num_synapses; }
	size32_t synapse_index(const Synapse *y) const;
	inline Synapse *synapse(size32_t index) const { return &_synapses[index]; }
	inline size32_t num_gap_junctions(void) const { return _num_gap_junctions; }
	size32_t num_gap_junctions(size32_t index) const;
	inline Gap_Junction *gap_junction(size32_t index) const { return &_gap_junctions[index]; }
	const Bounds &bounds(void) const { return _bounds; }
	inline void bound(Bounds b) { b.recenter(); _bounds = b; }
	inline Firing_Spikes *firing_spikes(void) { return _firing_spikes; }
	inline const Firing_Spikes *const_firing_spikes(void) const { return _firing_spikes; }
	inline bool has_firing_spikes(void) const { return _firing_spikes != NULL; }
	inline void firing_spikes(Firing_Spikes *fd) { delete _firing_spikes; _firing_spikes = fd; }
	inline Voltages *voltages(void) { return _voltages; }
	const Voltages *const_voltages(void) const { return _voltages; }
	inline bool has_voltages(void) const { return _voltages != NULL; }
	inline void voltages(Voltages *v) { delete _voltages; _voltages = v; }
	inline Weights *weights(void) { return _weights; }
	inline const Weights *const_weights(void) const { return _weights; }
	inline bool has_weights(void) const { return _weights != NULL; }
	inline void weights(Weights *w) { delete _weights; _weights = w; }
	size32_t start_time(size32_t t);
	size32_t step_time(void);
	inline bool empty(void) const { return !_num_somas; }
	void clear(void);
	Read_Status read_from(Input_Parser &ip, Progress_Dialog *p = NULL);
	Read_Status read_from(Binary_Parser &bp, Progress_Dialog *p = NULL);
	Read_Status read_config_from(std::ifstream &ifs) const;
	void write_config_to(std::ofstream &ofs) const;
};

#endif
