#ifndef VOLTAGE_GRAPH_WINDOW_H
#define VOLTAGE_GRAPH_WINDOW_H

#include <string>

#pragma warning(push, 0)
#include <FL/Fl_Pixmap.H>
#pragma warning(pop)

#include "utils.h"

class Label;
class Default_Spinner;
class OS_Button;
class Default_Button;
class OS_Repeat_Button;
class Fl_Double_Window;
class Fl_Native_File_Chooser;

class Voltage_Graph;
class Voltages;
class Modal_Dialog;

class Voltage_Graph_Window {
public:
	static Fl_Pixmap ZOOM_IN_ICON, ZOOM_OUT_ICON;
private:
	int _dx, _dy, _width, _height;
	std::string _title;
	size32_t _soma_id, _soma_index;
	const Voltages *_voltages;
	Voltage_Graph *_graph;
	Label *_zoom_heading;
	OS_Repeat_Button *_zoom_out_button, *_zoom_in_button;
	Default_Spinner *_mv_min_spinner, *_mv_max_spinner;
	Label *_range_units;
	OS_Button *_export_image_button, *_detect_peaks_button;
	Default_Button *_ok_button;
	Fl_Double_Window *_window;
	Fl_Native_File_Chooser *_image_chooser, *_report_peaks_chooser;
	Modal_Dialog *_error_dialog, *_success_dialog;
public:
	Voltage_Graph_Window(int x, int y, int w, int h, const char *t = NULL);
	~Voltage_Graph_Window();
	inline void title(std::string t) { _title = t; }
	inline void soma(size32_t id, size32_t index) { _soma_id = id; _soma_index = index; }
	inline void voltages(const Voltages *v) { _voltages = v; }
private:
	void initialize(void);
	void refresh(void);
public:
	void show(const Fl_Widget *p);
private:
	static void zoom_out_cb(Fl_Widget *w, Voltage_Graph_Window *vgw);
	static void zoom_in_cb(Fl_Widget *w, Voltage_Graph_Window *vgw);
	static void range_cb(Fl_Widget *w, Voltage_Graph_Window *vgw);
	static void export_image_cb(Fl_Widget *w, Voltage_Graph_Window *vgw);
	static void detect_peaks_cb(Fl_Widget *w, Voltage_Graph_Window *vgw);
	static void close_cb(Fl_Widget *w, Voltage_Graph_Window *vgw);
};

#endif
