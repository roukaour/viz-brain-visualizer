#include <cstdlib>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#pragma warning(push, 0)
#include <FL/Fl.H>
#pragma warning(pop)

#include "from-file.h"
#include "bounds.h"
#include "color.h"
#include "clip-volume.h"
#include "soma-type.h"
#include "soma.h"
#include "neuritic-field.h"
#include "synapse.h"
#include "gap-junction.h"
#include "input-parser.h"
#include "binary-parser.h"
#include "firing-spikes.h"
#include "voltages.h"
#include "weights.h"
#include "progress-dialog.h"
#include "brain-model.h"

Brain_Model::Brain_Model() : From_File(), _num_types(0), _types(NULL), _num_somas(0), _somas(NULL), _num_fields(0),
	_fields(NULL), _num_synapses(0), _synapses(NULL), _num_gap_junctions(0), _gap_junctions(NULL), _bounds(),
	_firing_spikes(NULL), _voltages(NULL), _weights(NULL) {}

Brain_Model::~Brain_Model() {
	clear();
}

size32_t Brain_Model::soma_index(size32_t id) const {
	size32_t low = 0, high = _num_somas;
	while (low < high) {
		size32_t mid = low + (high - low) / 2;
		Soma &s = _somas[mid];
		size32_t s_id = s.id();
		if (s_id == id) {
			return mid;
		}
		else if (s_id < id) {
			low = mid + 1;
		}
		else {
			high = mid;
		}
	}
	return NULL_INDEX;
}

size32_t Brain_Model::synapse_index(const Synapse *y) const {
	size32_t low = 0, high = _num_synapses;
	while (low < high) {
		size32_t mid = low + (high - low) / 2;
		Synapse &sy = _synapses[mid];
		if (&sy == y) {
			return mid;
		}
		else if (&sy < y) {
			low = mid + 1;
		}
		else {
			high = mid;
		}
	}
	return NULL_INDEX;
}

size32_t Brain_Model::num_gap_junctions(size32_t index) const {
	size32_t n = 0;
	for (size32_t i = 0; i < _num_gap_junctions; i++) {
		Gap_Junction &gj = _gap_junctions[i];
		if (gj.soma1_index() == index || gj.soma2_index() == index) { n++; }
	}
	return n;
}

size32_t Brain_Model::start_time(size32_t t) {
	size32_t time = t;
	if (_firing_spikes) {
		time = _firing_spikes->start_time(t);
		if (_voltages) { _voltages->start_time(t); }
		if (_weights) { _weights->start_time(t); }
	}
	return time;
}

size32_t Brain_Model::step_time(void) {
	size32_t time = 0;
	if (_firing_spikes) {
		time = _firing_spikes->step_time();
		if (_voltages) { _voltages->step_time(); }
		if (_weights) { _weights->step_time(); }
	}
	return time;
}

void Brain_Model::clear() {
	_filename = "";
	_filesize = 0;
	_num_types = 0;
	delete [] _types; _types = NULL;
	_num_somas = 0;
	delete [] _somas; _somas = NULL;
	_num_synapses = 0;
	delete [] _synapses; _synapses = NULL;
	_num_gap_junctions = 0;
	delete [] _gap_junctions; _gap_junctions = NULL;
	_bounds.reset();
	firing_spikes(NULL);
	voltages(NULL);
	weights(NULL);
}

Read_Status Brain_Model::read_from(Input_Parser &ip, Progress_Dialog *p) {
	size_t denom = 1;
	// Get the file name and size
	_filename = ip.filename();
	_filesize = ip.filesize();
	// Get the file format version
	size8_t version = 1;
	if (ip.peek() == 'v') {
		ip.get_char();
		version = ip.get_size8();
	}
	if (version < 1 || version > 2) { return BAD_VERSION; }
	// Get the number of types
	_num_types = ip.get_size8();
	if (!_num_types) { return NO_TYPES; }
	// Prepare to show type-parsing progress
	if (p) {
		denom = _num_types / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing types...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of types
	delete [] _types;
	_types = new(std::nothrow) Soma_Type[_num_types];
	if (_types == NULL) { return NO_MEMORY; }
	// Get each type
	for (size8_t i = 0; i < _num_types; i++) {
		Soma_Type &t = _types[i];
		t.read_from(ip);
		if (ip.done()) { return END_OF_FILE; }
		if (t.letter() < 'A' || t.letter() > 'Z') { return BAD_TYPE_LETTER; }
		// Update progress
		if (p && !((size_t)(i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_types);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	// Get the number of somas
	_num_somas = ip.get_size32();
	if (!_num_somas) { return NO_SOMAS; }
	// Get the number of neuritic fields
	if (version >= 2) {
		_num_fields = ip.get_size32();
	}
	else {
		// Prepare to show field-counting progress
		if (p) {
			denom = _num_somas / Progress_Dialog::PROGRESS_STEPS;
			if (!denom) { denom = 1; }
			p->message("Counting neuritic fields...");
			p->progress(0.0f);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
		// Count each neuritic field
		ip.save_place();
		Soma s;
		for (size32_t i = 0; i < _num_somas; i++) {
			s.read_from(ip, 0);
			if (ip.done()) { return END_OF_FILE; }
			size8_t nf = s.num_axon_fields() + s.num_den_fields();
			_num_fields += nf;
			Neuritic_Field f;
			for (size8_t j = 0; j < nf; j++) {
				f.read_from(ip);
			}
			// Update progress
			if (p && !((i + 1) % denom)) {
				p->progress((float)(i + 1) / _num_somas);
				Fl::check();
				if (p->canceled()) { return CANCELED; }
			}
		}
		ip.restore_place();
	}
	// Prepare to show soma-parsing progress
	if (p) {
		denom = _num_somas / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing somas...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of somas
	delete [] _somas;
	_somas = new(std::nothrow) Soma[_num_somas];
	if (_somas == NULL) { return NO_MEMORY; }
	// Initialize the array of neuritic fields
	delete [] _fields;
	_fields = _num_fields > 0 ? new(std::nothrow) Neuritic_Field[_num_fields] : NULL;
	if (_num_fields > 0 && _fields == NULL) { return NO_MEMORY; }
	// Initialize the coordinate bounds
	Bounds b;
	// Get each soma
	size32_t next_field_index = 0;
	for (size32_t i = 0; i < _num_somas; i++) {
		Soma &s = _somas[i];
		s.read_from(ip, next_field_index);
		if (ip.done()) { return END_OF_FILE; }
		// Get each axonal and dendritic neuritic field
		size8_t n = s.num_axon_fields() + s.num_den_fields();
		for (size8_t j = 0; j < n; j++) {
			_fields[next_field_index++].read_from(ip);
		}
#ifdef SHORT_COORDS
		// Keep track of coordinate bounds
		if (ip.overflow()) { return SIGN_OVERFLOW; }
#endif
		b.update(s.coords());
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_somas);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	bound(b);
	// Get the number of synapses
	_num_synapses = (size32_t)ip.get_size64();
	// Prepare to show synapse-parsing progress
	if (p) {
		denom = _num_synapses / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing synapses...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of synapses
	delete [] _synapses;
	_synapses = _num_synapses > 0 ? new(std::nothrow) Synapse[_num_synapses] : NULL;
	if (_num_synapses > 0 && _synapses == NULL) { return NO_MEMORY; }
	// Get each synapse
	for (size32_t i = 0; i < _num_synapses; i++) {
		Synapse &y = _synapses[i];
		y.read_from(ip, *this);
		if (ip.done() && i < _num_synapses - 1) { return END_OF_FILE; }
#ifdef SHORT_COORDS
		if (ip.overflow()) { return SIGN_OVERFLOW; }
#endif
		// Maintain the axonal soma's linked list of synapses
		size32_t a_index = y.axon_soma_index();
		if (a_index >= _num_somas) { return BAD_SYNAPSE_SOMA_ID; }
		soma(a_index)->first_axon_syn(&y, i);
		// Maintain the dendritic soma's linked list of synapses
		size32_t d_index = y.den_soma_index();
		if (d_index >= _num_somas) { return BAD_SYNAPSE_SOMA_ID; }
		soma(d_index)->first_den_syn(&y, i);
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_synapses);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	// Get the number of gap junctions (optional for backwards compatibility)
	_num_gap_junctions = ip.done() ? 0 : ip.get_size32();
	// Prepare to show gap junction-parsing progress
	if (p) {
		denom = _num_gap_junctions / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing gap junctions...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of gap junctions
	delete [] _gap_junctions;
	_gap_junctions = _num_gap_junctions > 0 ? new(std::nothrow) Gap_Junction[_num_gap_junctions] : NULL;
	if (_num_gap_junctions > 0 && _gap_junctions == NULL) { return NO_MEMORY; }
	// Get each gap junction
	for (size32_t i = 0; i < _num_gap_junctions; i++) {
		Gap_Junction &g = _gap_junctions[i];
		g.read_from(ip, *this);
		if (ip.done() && i < _num_gap_junctions - 1) { return END_OF_FILE; }
#ifdef SHORT_COORDS
		if (ip.overflow()) { return SIGN_OVERFLOW; }
#endif
		// Maintain the joined somas' counts of gap junctions
		size32_t s1_index = g.soma1_index();
		if (s1_index >= _num_somas) { return BAD_GAP_JUNCTION_SOMA_ID; }
		size32_t s2_index = g.soma2_index();
		if (s2_index >= _num_somas) { return BAD_GAP_JUNCTION_SOMA_ID; }
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_somas);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	// Update progress
	if (p) {
		p->progress(1.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	return SUCCESS;
}

Read_Status Brain_Model::read_from(Binary_Parser &bp, Progress_Dialog *p) {
	size_t denom = 1;
	if (p) {
		p->canceled(false);
	}
	// Get the file name and size
	_filename = bp.filename();
	_filesize = bp.filesize();
	// Get the file signature
	char sig[5];
	bp.get_chars(5, sig);
	if (sig[0] != '\a' || sig[1] != 'R' || sig[2] != 'J' || sig[3] != 'V' || sig[4] != '\xF7') {
		return BAD_SIGNATURE;
	}
	// Get the file format version
	size8_t version = bp.get_size8();
	if (version < 1 || version > 2) { return BAD_VERSION; }
	// Get the comment
	bp.get_string();
	// Get the number of types
	_num_types = bp.get_size8();
	if (!_num_types) { return NO_TYPES; }
	// Prepare to show type-parsing progress
	if (p) {
		denom = _num_types / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing types...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of types
	delete [] _types;
	_types = new(std::nothrow) Soma_Type[_num_types];
	if (_types == NULL) { return NO_MEMORY; }
	// Get each type
	for (size8_t i = 0; i < _num_types; i++) {
		Soma_Type &t = _types[i];
		t.read_from(bp);
		if (bp.done()) { return END_OF_FILE; }
		if (t.letter() < 'A' || t.letter() > 'Z') { return BAD_TYPE_LETTER; }
		// Update progress
		if (p && !((size_t)(i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_types);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	// Get the number of somas
	_num_somas = bp.get_size32();
	if (!_num_somas) { return NO_SOMAS; }
	// Get the number of neuritic fields
	if (version >= 2) {
		_num_fields = bp.get_size32();
	}
	else {
		_num_fields = 0;
		// Prepare to show field-counting progress
		if (p) {
			denom = _num_somas / Progress_Dialog::PROGRESS_STEPS;
			if (!denom) { denom = 1; }
			p->message("Counting neuritic fields...");
			p->progress(0.0f);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
		// Count each neuritic field
		bp.save_place();
		Soma s;
		for (size32_t i = 0; i < _num_somas; i++) {
			s.read_from(bp, 0);
			if (bp.done()) { return END_OF_FILE; }
			size8_t nf = s.num_axon_fields() + s.num_den_fields();
			_num_fields += nf;
			Neuritic_Field f;
			for (size8_t j = 0; j < nf; j++) {
				f.read_from(bp);
			}
			// Update progress
			if (p && !((i + 1) % denom)) {
				p->progress((float)(i + 1) / _num_somas);
				Fl::check();
				if (p->canceled()) { return CANCELED; }
			}
		}
		bp.restore_place();
	}
	// Prepare to show soma-parsing progress
	if (p) {
		denom = _num_somas / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing somas...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of somas
	delete [] _somas;
	_somas = new(std::nothrow) Soma[_num_somas];
	if (_somas == NULL) { return NO_MEMORY; }
	// Initialize the array of neuritic fields
	delete [] _fields;
	_fields = _num_fields > 0 ? new(std::nothrow) Neuritic_Field[_num_fields] : NULL;
	if (_num_fields > 0 && _fields == NULL) { return NO_MEMORY; }
	// Initialize the coordinate bounds
	Bounds b;
	// Get each soma
	size32_t next_field_index = 0;
	for (size32_t i = 0; i < _num_somas; i++) {
		Soma &s = _somas[i];
		s.read_from(bp, next_field_index);
		if (bp.done()) { return END_OF_FILE; }
		// Get each axonal and dendritic neuritic field
		size8_t n = s.num_axon_fields() + s.num_den_fields();
		for (size8_t j = 0; j < n; j++) {
			_fields[next_field_index++].read_from(bp);
		}
#ifdef SHORT_COORDS
		// Keep track of coordinate bounds
		if (bp.overflow()) { return SIGN_OVERFLOW; }
#endif
		b.update(s.coords());
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_somas);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	bound(b);
	// Get the number of synapses
	size64_t num_synapses = bp.get_size64();
	if (num_synapses > std::numeric_limits<size32_t>::max()) { return WRONG_NUM_SYNAPSES; }
	_num_synapses = (size32_t)num_synapses;
	// Prepare to show synapse-parsing progress
	if (p) {
		denom = _num_synapses / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing synapses...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of synapses
	delete [] _synapses;
	_synapses = _num_synapses > 0 ? new(std::nothrow) Synapse[_num_synapses] : NULL;
	if (_num_synapses > 0 && _synapses == NULL) { return NO_MEMORY; }
	// Get each synapse
	for (size32_t i = 0; i < _num_synapses; i++) {
		Synapse &y = _synapses[i];
		y.read_from(bp, *this);
		if (bp.done()) { return END_OF_FILE; }
#ifdef SHORT_COORDS
		if (bp.overflow()) { return SIGN_OVERFLOW; }
#endif
		// Maintain the axonal soma's linked list of synapses
		size32_t a_index = y.axon_soma_index();
		if (a_index >= _num_somas) { return BAD_SYNAPSE_SOMA_ID; }
		soma(a_index)->first_axon_syn(&y, i);
		// Maintain the dendritic soma's linked list of synapses
		size32_t d_index = y.den_soma_index();
		if (d_index >= _num_somas) { return BAD_SYNAPSE_SOMA_ID; }
		soma(d_index)->first_den_syn(&y, i);
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_synapses);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	// Get the number of gap junctions
	_num_gap_junctions = bp.get_size32();
	// Prepare to show gap junction-parsing progress
	if (p) {
		denom = _num_gap_junctions / Progress_Dialog::PROGRESS_STEPS;
		if (!denom) { denom = 1; }
		p->message("Parsing gap junctions...");
		p->progress(0.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	// Initialize the array of gap junctions
	delete [] _gap_junctions;
	_gap_junctions = _num_gap_junctions > 0 ? new(std::nothrow) Gap_Junction[_num_gap_junctions] : NULL;
	if (_num_gap_junctions > 0 && _gap_junctions == NULL) { return NO_MEMORY; }
	// Get each gap junction
	for (size32_t i = 0; i < _num_gap_junctions; i++) {
		Gap_Junction &g = _gap_junctions[i];
		g.read_from(bp, *this);
		if (bp.done() && i < _num_gap_junctions - 1) { return END_OF_FILE; }
#ifdef SHORT_COORDS
		if (bp.overflow()) { return SIGN_OVERFLOW; }
#endif
		// Maintain the joined somas' counts of gap junctions
		size32_t s1_index = g.soma1_index();
		if (s1_index >= _num_somas) { return BAD_GAP_JUNCTION_SOMA_ID; }
		size32_t s2_index = g.soma2_index();
		if (s2_index >= _num_somas) { return BAD_GAP_JUNCTION_SOMA_ID; }
		// Update progress
		if (p && !((i + 1) % denom)) {
			p->progress((float)(i + 1) / _num_somas);
			Fl::check();
			if (p->canceled()) { return CANCELED; }
		}
	}
	// Update progress
	if (p) {
		p->progress(1.0f);
		Fl::check();
		if (p->canceled()) { return CANCELED; }
	}
	return SUCCESS;
}

static const std::string whitespace(" \f\n\r\t\v");

static void trim(std::string &s, const std::string &t = whitespace) {
	std::string::size_type p = s.find_first_not_of(t);
	s.erase(0, p);
	p = s.find_last_not_of(t);
	s.erase(p + 1);
}

Read_Status Brain_Model::read_config_from(std::ifstream &ifs) const {
	std::map<char, const char *> type_names;
	std::map<char, const Color *> type_colors;
	std::map<char, Soma_Type::Display_State> type_states;
	bool failed = true;
#define BM_CONFIGURE_TYPE(l, n, c, d) (type_names[(l)] = (n), type_colors[(l)] = (c), type_states[(l)] = (d))
	// Parse the configuration file
	while (ifs.good()) {
		// A soma type configuration line is formatted as:
		//     letter:name:color:display_state
		std::string line;
		std::getline(ifs, line);
		std::istringstream lss(line);
		std::string letter_s, name_s, color_s, display_s;
		failed = false;
		// Get the type letter
		std::getline(lss, letter_s, CONFIG_SEPARATOR);
		trim(letter_s);
		if (letter_s.empty()) { continue; }
		char letter = letter_s[0];
		if (letter == CONFIG_COMMENT) { continue; }
		if (letter_s.length() > 1) {
			failed = true;
			break;
		}
		// Get the type name
		std::getline(lss, name_s, CONFIG_SEPARATOR);
		trim(name_s);
		if (name_s.empty()) { name_s = "Unknown"; }
		const char *name = name_s.c_str();
		// Get the type color
		std::getline(lss, color_s, CONFIG_SEPARATOR);
		trim(color_s);
		const Color *color = Color::color(color_s.c_str());
		// Get the type display state
		std::getline(lss, display_s, CONFIG_COMMENT);
		trim(display_s);
		Soma_Type::Display_State display_state = Soma_Type::LETTER;
		if (display_s == "letter") {
			display_state = Soma_Type::LETTER;
		}
		else if (display_s == "dot") {
			display_state = Soma_Type::DOT;
		}
		else if (display_s == "hidden") {
			display_state = Soma_Type::HIDDEN;
		}
		else if (display_s == "disabled") {
			display_state = Soma_Type::DISABLED;
		}
		else {
			display_state = Soma_Type::LETTER;
		}
		// Store the parsed configuration data
		BM_CONFIGURE_TYPE(letter, strdup(name), color, display_state);
	}
	if (failed) {
		type_names.clear();
		type_colors.clear();
		type_states.clear();
		// Use the default configuration data
		BM_CONFIGURE_TYPE('P', "Purkinje", Color::color("light blue"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('N', "Granule", Color::color("blue"), Soma_Type::DOT);
		BM_CONFIGURE_TYPE('G', "Golgi", Color::color("yellow-green"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('B', "Basket +x", Color::color("green"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('A', "Basket -x", Color::color("pink"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('S', "Outer stellate +x", Color::color("red"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('T', "Outer stellate -x", Color::color("tan"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('I', "Outer stellate A", Color::color("orange"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('C', "Climbing fiber", Color::color("lavender"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('M', "Mossy fiber", Color::color("purple"), Soma_Type::LETTER);
		BM_CONFIGURE_TYPE('R', "Rosette tip", Color::color("yellow"), Soma_Type::DOT);
		BM_CONFIGURE_TYPE('D', "Dentate nucleus", Color::color("maroon"), Soma_Type::LETTER);
	}
#undef BM_CONFIGURE_TYPE
	// Apply the configuration data to the soma types
	for (size8_t i = 0; i < _num_types; i++) {
		Soma_Type *t = type(i);
		char letter = t->letter();
		t->name(type_names.count(letter) ? type_names[letter] : "Unknown");
		t->orig_color(type_colors.count(letter) ? type_colors[letter] : Color::UNKNOWN_COLOR);
		t->orig_display_state(type_states.count(letter) ? type_states[letter] : Soma_Type::LETTER);
	}
	return failed ? FAILURE : SUCCESS;
}

void Brain_Model::write_config_to(std::ofstream &ofs) const {
	ofs << "# Valid colors are:\n";
	ofs << "# red, maroon, pink, magenta, orange, tan, brown, yellow, goldenrod,\n";
	ofs << "# olive, yellow-green, green, lime, cyan, turquoise, blue, sky blue,\n";
	ofs << "# light blue, indigo, violet, purple, lavender, white, gray\n";
	ofs << "\n";
	ofs << "# Valid display states are:\n";
	ofs << "# letter, dot, hidden, disabled\n";
	ofs << "\n";
	static const char *display_states[4] = {"letter", "dot", "hidden", "disabled"};
	for (size8_t i = 0; i < _num_types; i++) {
		Soma_Type *t = type(i);
		const Color *c = t->color();
		if (!c) { c = Color::UNKNOWN_COLOR; }
		const char *color = c->name();
		const char *display_state = display_states[t->display_state()];
		ofs << t->letter() << CONFIG_SEPARATOR << t->orig_name() << CONFIG_SEPARATOR << color << CONFIG_SEPARATOR << display_state << "\n";
	}
}
