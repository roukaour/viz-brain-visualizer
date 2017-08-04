#include <vector>
#include <cstring>
#include <limits>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "color.h"

std::vector<const Color *> Color::colors;

const Color *Color::UNKNOWN_COLOR = NULL;

// C++ lacks Java's static initialization blocks, so Color::colors must be initialized by setting this flag
bool Color::initialized_colors = Color::initialize_colors();

bool Color::initialize_colors() {
	new Color("red", 0xE0, 0x20, 0x20);
	new Color("maroon", 0x80, 0x10, 0x10);
	new Color("pink", 0xFF, 0xA0, 0xA0);
	new Color("magenta", 0xFF, 0x40, 0x60);
	new Color("orange", 0xFF, 0x80, 0x00);
	new Color("tan", 0xFF, 0xB0, 0x40);
	new Color("brown", 0x80, 0x60, 0x30);
	new Color("yellow", 0xFF, 0xFF, 0x90);
	new Color("goldenrod", 0xD0, 0xA0, 0x20);
	new Color("olive", 0x70, 0x90, 0x20);
	new Color("yellow-green", 0x90, 0xE0, 0x40);
	new Color("green", 0x30, 0xA0, 0x30);
	new Color("lime", 0x40, 0xFF, 0x40);
	new Color("cyan", 0x80, 0xFF, 0xD0);
	new Color("turquoise", 0x20, 0xB0, 0x80);
	new Color("blue", 0x20, 0x80, 0xB0);
	new Color("sky blue", 0x60, 0xB0, 0xFF);
	new Color("light blue", 0xA0, 0xD0, 0xE0);
	new Color("indigo", 0x60, 0x60, 0xD0);
	new Color("violet", 0xB0, 0x50, 0xB0);
	new Color("purple", 0x70, 0x40, 0xA0);
	new Color("lavender", 0xFF, 0x80, 0xFF);
	new Color("white", 0xFF, 0xFF, 0xFF);
	UNKNOWN_COLOR = new Color("gray", 0x90, 0x90, 0x90);
	return true;
}

const Color *Color::color(size8_t index) {
	size_t n = colors.size();
	for (size_t i = 0; i < n; i++) {
		const Color *c = colors.at(i);
		if (c->_index == index) { return c; }
	}
	return UNKNOWN_COLOR;
}

const Color *Color::color(const char *name) {
	size_t n = colors.size();
	for (size_t i = 0; i < n; i++) {
		const Color *c = colors.at(i);
		if (!strcmp(c->_name, name)) { return c; }
	}
	return UNKNOWN_COLOR;
}

void Color::fl2gl(Fl_Color flc, float rgb[3]) {
	uchar r, g, b;
	Fl::get_color(flc, r, g, b);
	rgb[0] = (float)r / 0xFF; rgb[1] = (float)g / 0xFF; rgb[2] = (float)b / 0xFF;
}

Fl_Color Color::gl2fl(const float rgb[3]) {
	uchar m = (std::numeric_limits<uchar>::max)();
	uchar r = (uchar)(rgb[0] * m);
	uchar g = (uchar)(rgb[1] * m);
	uchar b = (uchar)(rgb[2] * m);
	return fl_rgb_color(r, g, b);
}

Color::Color(const char *name, float r, float g, float b) : _name(name), _rgb(), _index((size8_t)colors.size()) {
	_rgb[0] = r; _rgb[1] = g; _rgb[2] = b;
	colors.push_back(this);
}

Color::Color(const char *name, int r, int g, int b) : _name(name), _rgb(), _index((size8_t)colors.size()) {
	float m = (float)(std::numeric_limits<uchar>::max)();
	_rgb[0] = r / m; _rgb[1] = g / m; _rgb[2] = b / m;
	colors.push_back(this);
}

Fl_Color Color::as_fl_color() const {
	uchar m = (std::numeric_limits<uchar>::max)();
	uchar r = (uchar)(_rgb[0] * m);
	uchar g = (uchar)(_rgb[1] * m);
	uchar b = (uchar)(_rgb[2] * m);
	fl_color(r, g, b);
	return fl_color();
}
