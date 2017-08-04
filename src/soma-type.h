#ifndef SOMA_TYPE_H
#define SOMA_TYPE_H

#include <string>

class Color;
class Input_Parser;
class Binary_Parser;

class Soma_Type {
public:
	enum Display_State { LETTER, DOT, HIDDEN, DISABLED };
private:
	char _letter;
	std::string _orig_name, _name;
	const Color *_orig_color, *_color;
	Display_State _orig_state, _state;
public:
	Soma_Type(char l = '?');
	inline char letter(void) const { return _letter; }
	inline const std::string &name(void) const { return _name; }
	inline const std::string &orig_name(void) const { return _orig_name; }
	inline void name(const char *s) { _orig_name = s; _name = _orig_name + " (" + _letter + ")"; }
	inline const Color *color(void) const { return _color; }
	inline void color(const Color *c) { _color = c; }
	inline void orig_color(const Color *c) { _orig_color = _color = c; }
	inline Display_State display_state(void) const { return _state; }
	inline void display_state(Display_State d) { _state = d; }
	inline void orig_display_state(Display_State d) { _orig_state = _state = d; }
	inline bool visible(void) const { return _state == LETTER || _state == DOT; }
	inline void reset(void) { _state = _orig_state; _color = _orig_color; }
	void read_from(Input_Parser &ip);
	void read_from(Binary_Parser &bp);
};

#endif
