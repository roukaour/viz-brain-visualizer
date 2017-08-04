#ifndef VOLTAGE_GRAPH_H
#define VOLTAGE_GRAPH_H

#include <vector>
#include <map>

#pragma warning(push, 0)
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#pragma warning(pop)

#include "utils.h"
#include "widgets.h"
#include "voltages.h"

class Voltage_Tooltip : public Fl_Menu_Window {
public:
	static const size_t MAX_TOOLTIP_LENGTH = 255;
	static const int CURSOR_OFFSET = 20;
private:
	std::ostringstream _tooltip;
public:
	Voltage_Tooltip(int w, int h, const char *l = NULL);
	void tooltip(size32_t t, float v, Firing_State s);
protected:
	void draw(void);
};

class Voltage_Chart : public Fl_Widget {
private:
	static const Fl_Color SUPPRESSED_COLOR, BINARY_COLOR, FORCED_COLOR, NATURAL_COLOR, PEAK_COLOR;
private:
	std::vector<float> _voltages;
	firings_instance_t _firings;
	float _min, _max, _range;
	int _margin_bottom;
	Voltage_Tooltip *_tooltip;
public:
	Voltage_Chart(int x, int y, int w, int h, const char *l = NULL);
	~Voltage_Chart();
	inline size_t num_points(void) const { return _voltages.size(); }
	inline std::vector<float> &voltages(void) { return _voltages; }
	inline size_t num_firings(void) const { return _firings.size(); }
	inline firings_instance_t &firings(void) { return _firings; }
	inline float min(void) const { return _min; }
	inline float max(void) const { return _max; }
	inline float range(void) const { return _range; }
	inline void range(float min, float max) { _min = min; _max = max; _range = _max - _min; }
	inline int margin_bottom(void) const { return _margin_bottom; }
	void data_from(const Voltages *v, size32_t index);
protected:
	void draw(void);
	int handle(int event);
private:
	bool values_for_tooltip(int cx, int cy, size32_t &t, float &v, Firing_State &s) const;
};

class Voltage_Scroll : public OS_Scroll {
	Voltage_Chart *_chart;
	int _zoom;
public:
	Voltage_Scroll(int x, int y, int w, int h, const char *l = NULL);
	inline Voltage_Chart *chart(void) { return _chart; }
	inline const Voltage_Chart *const_chart(void) const { return _chart; }
	inline int zoom(void) const { return _zoom; }
	void zoom_in(int dx = 0);
	void zoom_out(int dx = 0);
	void zoom(int z, int dx = 0);
	inline void resize(int x, int y, int w, int h) { Fl_Scroll::resize(x, y, w, h); zoom(_zoom); }
	int handle(int event);
};

class Voltage_Graph : public Fl_Group {
private:
	Voltage_Scroll *_scroll;
	size32_t _soma_id;
	int _margin_left, _margin_right, _margin_top;
public:
	Voltage_Graph(int x, int y, int w, int h, const char *l = NULL);
	~Voltage_Graph();
	inline Voltage_Scroll *scroll(void) { return _scroll; }
	inline const Voltage_Scroll *const_scroll(void) const { return _scroll; }
	inline size32_t soma_id(void) const { return _soma_id; }
	inline void soma_id(size32_t id) { _soma_id = id; }
	int write_image(const char *f, Image::Format m);
protected:
	void draw(void);
};

#endif
