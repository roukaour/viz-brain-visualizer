#include <cstdlib>

#include "color.h"
#include "input-parser.h"
#include "binary-parser.h"
#include "soma-type.h"

Soma_Type::Soma_Type(char l) : _letter(l), _orig_name(), _name(), _orig_color(NULL), _color(NULL), _orig_state(LETTER),
	_state(LETTER) {}

void Soma_Type::read_from(Input_Parser &ip) {
	// A line defining a soma type is formatted as:
	//     type_id letter
	ip.get_size8(); // type index; TODO: remove these from the file format
	_letter = ip.get_char();
}

void Soma_Type::read_from(Binary_Parser &bp) {
	// A line defining a soma type is formatted as:
	//     letter:uc
	_letter = bp.get_char();
}
