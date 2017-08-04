#ifndef GAP_JUNCTION_H
#define GAP_JUNCTION_H

#include "utils.h"
#include "coords.h"

class Input_Parser;
class Binary_Parser;
class Brain_Model;

class Gap_Junction {
private:
	size32_t _soma1_index, _soma2_index;
	coord_t _coords[3];
public:
	Gap_Junction();
	~Gap_Junction();
private:
	void initialize(void);
public:
	inline size32_t soma1_index(void) const { return _soma1_index; }
	inline size32_t soma2_index(void) const { return _soma2_index; }
	inline const coord_t *coords(void) const { return _coords; }
	void draw(const Soma *s1, const Soma *s2) const;
	void read_from(Input_Parser &ip, const Brain_Model &bm);
	void read_from(Binary_Parser &bp, const Brain_Model &bm);
};

#endif
