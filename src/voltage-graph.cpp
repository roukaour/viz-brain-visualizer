#include <cstdio>
#include <cerrno>
#include <sstream>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "os-themes.h"
#include "algebra.h"
#include "sim-data.h"
#include "voltages.h"
#include "image.h"
#include "soma.h"
#include "voltage-graph.h"

Voltage_Tooltip::Voltage_Tooltip(int w, int h, const char *l) : Fl_Menu_Window(w, h, l), _tooltip() {
	_tooltip.imbue(std::locale(""));
	_tooltip.setf(std::ios::fixed, std::ios::floatfield);
	set_override();
	end();
}

void Voltage_Tooltip::draw() {
	draw_box(FL_BORDER_BOX, 0, 0, w(), h(), Fl_Tooltip::color());
	fl_color(Fl_Tooltip::textcolor());
	fl_font(Fl_Tooltip::font(), Fl_Tooltip::size());
	int tx = Fl_Tooltip::margin_width(), ty = Fl_Tooltip::margin_height();
	int tw = w() - Fl_Tooltip::margin_width() * 2, th = h() - Fl_Tooltip::margin_height() * 2;
	fl_draw(_tooltip.str().c_str(), tx, ty, tw, th, Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_WRAP));
}

void Voltage_Tooltip::tooltip(size32_t t, float v, Firing_State s) {
	_tooltip.str("");
	_tooltip << "Cycle " << t << ": " << v << " mV";
	if (s & SUPPRESSED) { _tooltip << "\nSuppression"; }
	if (s & BINARY) { _tooltip << "\nBinary firing"; }
	if (s & FORCED) { _tooltip << "\nForced firing"; }
	if (s & NORMAL) { _tooltip << "\nNatural firing"; }
	if (s & PEAK) { _tooltip << "\nPeak detected"; }
	fl_font(Fl_Tooltip::font(), Fl_Tooltip::size());
	int tw = Fl_Tooltip::wrap_width(), th = 0;
	fl_measure(_tooltip.str().c_str(), tw, th, FL_ALIGN_LEFT | FL_ALIGN_WRAP | FL_ALIGN_INSIDE);
	tw += Fl_Tooltip::margin_width() * 2;
	th += Fl_Tooltip::margin_height() * 2;
	size(tw, th);
	redraw();
}

const Fl_Color Voltage_Chart::SUPPRESSED_COLOR = fl_rgb_color(0x00, 0xC0, 0x00); // green
const Fl_Color Voltage_Chart::BINARY_COLOR = fl_rgb_color(0xFF, 0x80, 0x00); // orange
const Fl_Color Voltage_Chart::FORCED_COLOR = fl_rgb_color(0xC0, 0x00, 0xFF); // purple
const Fl_Color Voltage_Chart::NATURAL_COLOR = FL_RED;
const Fl_Color Voltage_Chart::PEAK_COLOR = fl_rgb_color(0x00, 0xA0, 0xFF); // sky blue

Voltage_Chart::Voltage_Chart(int x, int y, int w, int h, const char *l) : Fl_Widget(x, y, w, h, l),
	_voltages(), _firings(), _min(-0.5f), _max(0.5f), _range(1.0f), _margin_bottom(), _tooltip(NULL) {
	fl_font(OS_FONT, OS_FONT_SIZE);
	_margin_bottom = fl_height() + 1;
	labeltype(FL_NO_LABEL);
	box(FL_NO_BOX);
	align(FL_ALIGN_BOTTOM);
	Fl_Group *group = Fl_Group::current();
	_tooltip = new Voltage_Tooltip(1, 1);
	_tooltip->hide();
	Fl_Group::current(group);
}

Voltage_Chart::~Voltage_Chart() {
	_voltages.clear();
	_firings.clear();
	delete _tooltip;
}

void Voltage_Chart::data_from(const Voltages *v, size32_t index) {
	size32_t i = v->active_soma_relative_index(index);
	size32_t n = v->num_cycles();
	if (i >= v->num_active_somas()) {
		_voltages.clear();
		_firings.clear();
		_min = 0.0f; _max = 1.0f; _range = _max - _min;
		return;
	}
	_voltages.resize(n);
	_min = (std::numeric_limits<float>::max)();
	_max = -(std::numeric_limits<float>::max)();
	for (size32_t t = 0; t < n; t++) {
		float x = v->voltage_relative(i, t);
		_voltages[t] = x;
		if (x < _min) { _min = x; }
		if (x > _max) { _max = x; }
	}
	const firings_instance_t &fs = v->firings_relative(i);
	_firings.clear();
	for (firings_instance_t::const_iterator it = fs.begin(); it != fs.end(); ++it) {
		if (it->second != NOTHING) {
			_firings[it->first] = it->second;
		}
	}
	_min = floor(_min);
	_max = ceil(_max);
	if (_min == _max) { _max++; }
	_range = _max - _min;
}

static int round_int(double v) { return (int)floor(v + 0.5); }

static void draw_text(const char *s, int x, int y, int w = 0) {
	if (w == 0) { w = (int)ceil(fl_width(s)); }
	fl_draw(s, x, y, w, OS_FONT_SIZE, FL_ALIGN_TOP_RIGHT);
}

static float nice_ceil(float x) {
	float ms[15] = { 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 25.0f, 50.0f,
		100.0f, 200.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 2500.0f, 5000.0f };
	for (int i = 0; i < 15; i++) {
		if (x <= ms[i]) { return ms[i]; }
	}
	return ceil(x / 10000.0f) * 10000.0f;
}

void Voltage_Chart::draw() {
	draw_box();
	draw_label();
	// Measure quantities for drawing
	fl_font(OS_FONT, OS_FONT_SIZE);
	int gh = h() - _margin_bottom;
	int ay = y() + gh;
	// Draw graph background
	fl_color(FL_WHITE);
	fl_rectf(x(), y(), w(), gh);
	// Draw time axis
	fl_color(FL_FOREGROUND_COLOR);
	fl_xyline(x(), ay, x()+w());
	// Draw time axis tick labels and marks
	Fl_Color gray = fl_rgb_color(0xCF, 0xCF, 0xCF);
	std::ostringstream ss;
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(0);
	size_t n = num_points();
	int lw = round_int(fl_width("999999")) + 8;
	int xly = ay + 1;
	float dt = nice_ceil((float)n * lw / w());
	int xlx0 = x(), xlx1 = x() + w();
	for (float t = dt; t < (float)n; t += dt) {
		float pt = t / (float)(n - 1);
		int xlx = (int)(xlx1 * pt + xlx0 * (1.0f - pt));
		ss.str("");
		ss << t;
		fl_color(FL_FOREGROUND_COLOR);
		draw_text(ss.str().c_str(), xlx - round_int(fl_width(ss.str().c_str()) / 2.0), xly);
		if (t > 0.0f) {
			fl_color(gray);
			fl_yxline(xlx, y(), ay-1);
		}
	}
	// Draw voltage axis tick marks
	int lh = fl_height();
	float dv = nice_ceil(_range * lh * 2 / gh);
	float v0 = ceil(_min / dv) * dv;
	int yly0 = ay, yly1 = y();
	fl_color(gray);
	for (float v = v0; v <= _max; v += dv) {
		float pv = (v - _min) / _range;
		int yly = (int)(yly1 * pv + yly0 * (1.0f - pv));
		if (yly < yly0) {
			fl_xyline(x()+1, yly, x()+w());
		}
	}
	// Draw voltage curve
	double bw = (double)w() / n;
	fl_color(FL_BLUE);
	for (size_t i = 0; i < n; i++) {
		int x0 = x() + round_int((i - 0.5) * bw);
		int x1 = x() + round_int((i + 0.5) * bw);
		int yy1 = y() - round_int(gh * (_voltages[i] - _max) / _range);
		int yy0 = i > 0 ? y() - round_int(gh * (_voltages[i - 1] - _max) / _range) : yy1;
		if (i) { fl_line(x0, yy0, x1, yy1); }
	}
	// Draw firing dots
	for (firings_instance_t::const_iterator it = _firings.begin(); it != _firings.end(); ++it) {
		size32_t i = it->first;
		size32_t fs = it->second;
		fl_color((fs & PEAK) ? PEAK_COLOR :
			(fs & SUPPRESSED) ? SUPPRESSED_COLOR :
			(fs & BINARY) ? BINARY_COLOR :
			(fs & FORCED) ? FORCED_COLOR :
			NATURAL_COLOR);
		int fx = x() + round_int((i + 0.5) * bw);
		int fy = round_int(y() - gh * (_voltages[i] - _max) / _range);
		fl_rectf(fx-2, fy-2, 4, 4);
	}
}

bool Voltage_Chart::values_for_tooltip(int cx, int cy, size32_t &t, float &v, Firing_State &s) const {
	int gh = h() - _margin_bottom;
	float tp = (float)(cx - x()) / w(), vp = (float)(cy - y()) / gh;
	if (tp <= 0.0f || tp >= 1.0f || vp <= 0.0f || vp >= 1.0f) { return false; }
	t = (size32_t)(tp * num_points());
	v = _max * (1.0f - vp) + _min * vp;
	firings_instance_t::const_iterator it = _firings.find(t);
	s = it == _firings.end() ? NOTHING : it->second;
	return true;
}

int Voltage_Chart::handle(int event) {
	int cx, cy;
	size32_t t;
	float v;
	Firing_State s;
	switch (event) {
	case FL_PUSH:
		_tooltip->position(Fl::event_x_root(), Fl::event_y_root() + Voltage_Tooltip::CURSOR_OFFSET);
		cx = Fl::event_x(); cy = Fl::event_y();
		if (values_for_tooltip(cx, cy, t, v, s)) {
			_tooltip->tooltip(t, v, s);
		}
		_tooltip->show();
		return 1;
	case FL_DRAG:
		_tooltip->position(Fl::event_x_root(), Fl::event_y_root() + Voltage_Tooltip::CURSOR_OFFSET);
		cx = Fl::event_x(); cy = Fl::event_y();
		if (values_for_tooltip(cx, cy, t, v, s)) {
			_tooltip->tooltip(t, v, s);
		}
		return 1;
	case FL_RELEASE:
	case FL_HIDE:
	case FL_LEAVE:
		_tooltip->hide();
		return 1;
	}
	return Fl_Widget::handle(event);
}

Voltage_Scroll::Voltage_Scroll(int x, int y, int w, int h, const char *l) : OS_Scroll(x, y, w, h, l),
	_chart(NULL), _zoom(1) {
	type(Fl_Scroll::HORIZONTAL_ALWAYS);
	_chart = new Voltage_Chart(x, y, w, h, l);
	end();
}

void Voltage_Scroll::zoom_in(int dx) {
	int z = zoom();
	if (z < (int)ceil(_chart->num_points() * 10.0 / w())) { z++; }
	zoom(z, dx);
}

void Voltage_Scroll::zoom_out(int dx) {
	int z = zoom();
	if (z > 1) { z--; }
	zoom(z, dx);
}

void Voltage_Scroll::zoom(int z, int dx) {
	float ratio = (float)z / _zoom;
	_zoom = z;
	_chart->size(_zoom * w(), h() - Fl::scrollbar_size());
	dx = MAX(0, MIN(w(), dx));
	int sx = MAX(0, MIN(_chart->w() - w(), (int)((xposition() + dx) * ratio - dx)));
	scroll_to(sx, yposition());
	redraw();
}

int Voltage_Scroll::handle(int event) {
	switch (event) {
	case FL_MOUSEWHEEL:
		int dx = MAX(0, MIN(w(), Fl::event_x() - x()));
		if (Fl::event_dy() > 0) { zoom_out(dx); }
		else if (Fl::event_dy() < 0) { zoom_in(dx); }
		return 1;
	}
	return Fl_Scroll::handle(event);
}

Voltage_Graph::Voltage_Graph(int x, int y, int w, int h, const char *l) : Fl_Group(x, y, w, h, l),
	_scroll(NULL), _soma_id(), _margin_left(), _margin_right(), _margin_top() {
	fl_font(OS_FONT, OS_FONT_SIZE);
	_margin_left = round_int(fl_width("-999")) + 4;
	_margin_right = round_int(fl_width("t")) + 5;
	_margin_top = fl_height() + fl_descent() + 2;
	_scroll = new Voltage_Scroll(x+_margin_left, y+_margin_top, w-_margin_left-_margin_right, h-_margin_top);
	labeltype(FL_NO_LABEL);
	box(FL_FLAT_BOX);
	resizable(_scroll);
	align(FL_ALIGN_BOTTOM);
	end();
}

Voltage_Graph::~Voltage_Graph() {
	delete _scroll;
}

int Voltage_Graph::write_image(const char *f, Image::Format m) {
	window()->make_current();
	Image *image = new(std::nothrow) Image(f, (size_t)w(), (size_t)h(), this);
	if (image == NULL) { return ENOMEM; }
	image->write(m);
	int r = image->error();
	image->close();
	delete image;
	return r;
}

void Voltage_Graph::draw() {
	if (damage() & ~FL_DAMAGE_CHILD) {
		draw_box();
		draw_label();
	}
	// Measure quantities for drawing
	fl_font(OS_FONT, OS_FONT_SIZE);
	int lh = fl_height();
	int gx = x() + _margin_left, gy = y() + _margin_top;
	int gw = _scroll->w(), gh = _scroll->h() - _scroll->chart()->margin_bottom() - Fl::scrollbar_size();
	fl_color(FL_FOREGROUND_COLOR);
	// Draw title
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(0);
	ss << "Soma #" << _soma_id;
	size_t n_fired = _scroll->const_chart()->num_firings();
	if (!n_fired) {
		ss << ": No firing spikes";
	}
	else {
		ss << ": " << n_fired << " firing spikes";
	}
	fl_draw(ss.str().c_str(), gx, y(), gw, OS_FONT_SIZE, FL_ALIGN_TOP);
	// Draw voltage axis
	fl_yxline(gx-1, gy, gy+gh);
	// Draw axis labels
	draw_text("mV", gx-(int)fl_width("mV")/2, y());
	draw_text("t", gx+gw+5, gy+gh-lh/2);
	// Draw voltage axis tick labels
	const Voltage_Chart *vc = _scroll->const_chart();
	float dv = nice_ceil(vc->range() * lh * 2 / gh);
	float v0 = ceil(vc->min() / dv) * dv;
	int yly0 = gy + gh, yly1 = gy;
	for (float v = v0; v <= vc->max(); v += dv) {
		float pv = (v - vc->min()) / vc->range();
		int yly = (int)(yly1 * pv + yly0 * (1.0f - pv));
		ss.str("");
		ss << v;
		draw_text(ss.str().c_str(), x(), yly-lh/2, _margin_left-4);
	}
	draw_children();
}
