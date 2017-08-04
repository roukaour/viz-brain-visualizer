#ifndef NEURITIC_FIELD_H
#define NEURITIC_FIELD_H

#include "coords.h"

class Input_Parser;
class Binary_Parser;

class Neuritic_Field {
private:
	coord_t _min[3], _max[3];
public:
	Neuritic_Field();
	void draw(void) const;
	void read_from(Input_Parser &ip);
	void read_from(Binary_Parser &bp);
};

#endif
