#ifndef COLOR_H
#define COLOR_H

#include <vector>

#pragma warning(push, 0)
#include <FL/Enumerations.H>
#pragma warning(pop)

#include "utils.h"

class Color {
private:
	static std::vector<const Color *> colors;
	static bool initialized_colors;
	static bool initialize_colors(void);
public:
	static const Color *UNKNOWN_COLOR;
public:
	static inline size8_t num_colors(void) { return (size8_t)colors.size(); }
	static const Color *color(size8_t index);
	static const Color *color(const char *name);
	static void fl2gl(Fl_Color flc, float rgb[3]);
	static Fl_Color gl2fl(const float rgb[3]);
private:
	const char *_name;
	float _rgb[3];
	size8_t _index;
private:
	Color(const char *name, float r, float g, float b);
	Color(const char *name, int r, int g, int b);
public:
	inline size8_t index(void) const { return _index; }
	inline const char *name(void) const { return _name; }
	inline const float *rgb(void) const { return _rgb; }
	inline void rgb(float *cv) const { cv[0] = _rgb[0]; cv[1] = _rgb[1]; cv[2] = _rgb[2]; }
	Fl_Color as_fl_color(void) const;
};

#endif
