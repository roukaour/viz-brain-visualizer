#include <cmath>

#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "algebra.h"
#include "color-maps.h"

void Gradient_Map::initialize(const float (*cs)[3], size_t n) {
	_color_stops.resize(n);
	for (size_t i = 0; i < n; i++) {
		_color_stops[i].push_back(cs[i][0]);
		_color_stops[i].push_back(cs[i][1]);
		_color_stops[i].push_back(cs[i][2]);
	}
}

void Gradient_Map::map(float s, float *cv) const {
	size_t n = _color_stops.size() - 1;
	float ss = MAX(s * n, 0.0f);
	double g;
	float t = (float)modf(ss, &g);
	size_t p = (size_t)MAX(g, 0.0);
	if (p >= n) {
		const color_stop_t &c = _color_stops[n];
		cv[0] = c[0]; cv[1] = c[1]; cv[2] = c[2];
	}
	else {
		const color_stop_t &c1 = _color_stops[p];
		const color_stop_t &c2 = _color_stops[p + 1];
		cv[0] = c2[0] * t + c1[0] * (1.0f - t);
		cv[1] = c2[1] * t + c1[1] * (1.0f - t);
		cv[2] = c2[2] * t + c1[2] * (1.0f - t);
	}
}

void Gradient_Map::draw(int x, int y, int w, int h) const {
	int n = (int)_color_stops.size() - 1;
	float dh = (float)h / n;
	glBegin(GL_QUADS);
	map_glColor(0.0f);
	for (int i = 1; i <= n; i++) {
		int yi = (int)(y - dh * i + dh);
		glVertex2i(x - w, yi);
		glVertex2i(x, yi);
		map_glColor((float)i / n);
		yi = (int)(y - dh * i);
		glVertex2i(x, yi);
		glVertex2i(x - w, yi);
	}
	glEnd();
}

Grayscale_Map::Grayscale_Map() : Gradient_Map() {
	const float stops[2][3] = {
		{0.0f, 0.0f, 0.0f}, // black, #000000
		{1.0f, 1.0f, 1.0f}  // white, #FFFFFF
	};
	initialize(stops, 2);
}

Rainbow_Map::Rainbow_Map() : Gradient_Map() {
	const float stops[11][3] = {
		{0.5f, 0.0f, 0.5f},    // purple, #800080
		{0.375f, 0.0f, 0.75f}, // blue-purple, #6000C0
		{0.0f, 0.0f, 1.0f},    // blue, #0000FF
		{0.0f, 0.5f, 1.0f},    // blue-green, #0080FF
		{0.0f, 1.0f, 1.0f},    // cyan, #00FFFF
		{0.0f, 1.0f, 0.0f},    // green, #00FF00
		{0.5f, 1.0f, 0.0f},    // yellow-green, #80FF00
		{1.0f, 1.0f, 0.0f},    // yellow, #FFFF00
		{1.0f, 1.0f, 1.0f},    // white, #FFFFFF
		{1.0f, 0.5f, 0.0f},    // orange, #FF8000
		{1.0f, 0.0f, 0.0f}     // red, #FF0000
	};
	initialize(stops, 11);
}

Thermal_Map::Thermal_Map() : Gradient_Map() {
	const float stops[6][3] = {
		{0.0f, 0.0f, 0.625f},      // navy blue, #0000A0
		{0.375f, 0.0f, 0.625f},    // deep purple, #6000A0
		{0.625f, 0.0f, 0.375f},    // purple, #A00060
		{0.9375f, 0.125f, 0.125f}, // red-orange, #E02020
		{1.0f, 0.9375f, 0.0f},     // yellow, #FFF000
		{1.0f, 1.0f, 1.0f}         // white, #FFFFFF
	};
	initialize(stops, 6);
}

Opposed_Map::Opposed_Map() : Gradient_Map() {
	const float stops[7][3] = {
		{0.0f, 0.0f, 1.0f},        // blue, #0000FF
		{0.0625f, 0.75f, 0.9375f}, // teal, #10C0F0
		{0.125f, 0.9375f, 0.0f},   // green, #20F000
		{1.0f, 1.0f, 1.0f},        // white, #FFFFFF
		{1.0f, 0.9375f, 0.125f},   // yellow, #FFF020
		{1.0f, 0.5f, 0.0625f},     // orange, #FF8010
		{1.0f, 0.0f, 0.0f}         // red, #FF0000
	};
	initialize(stops, 7);
}

// Adapted from "A colour scheme for the display of astronomical intensity images" (D. A. Green, 2011)
CubeHelix_Map::CubeHelix_Map(float h, float r, float s, float g) : Color_Map(), _start_hue(h), _rotations(r),
	_saturation(s), _gamma(g) {}

void CubeHelix_Map::map(float s, float *cv) const {
	float angle = (float)TWO_PI * (_start_hue / 3.0f + 1.0f + _rotations * s);
	float n = sin(angle), c = cos(angle);
	s *= _gamma;
	float a = _saturation * s * (1.0f - s) / 2.0f;
	cv[0] = s + a * (-0.14861f * c + 1.78277f * n);
	cv[1] = s + a * (-0.29227f * c - 0.90649f * n);
	cv[2] = s + a * (1.97294f * c);
}

void CubeHelix_Map::draw(int x, int y, int w, int h) const {
	glBegin(GL_LINES);
	for (int i = 0; i < h; i++) {
		float s = (float)i / (h - 1);
		map_glColor(s);
		glVertex2i(x - w, y - i);
		glVertex2i(x, y - i);
	}
	glEnd();
}
