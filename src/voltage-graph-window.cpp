#include <sstream>
#include <fstream>
#include <locale>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "zoom-in.xpm"
#include "zoom-out.xpm"

#include "algebra.h"
#include "os-themes.h"
#include "image.h"
#include "voltage-graph.h"
#include "voltage-graph-window.h"
#include "widgets.h"
#include "modal-dialog.h"

Fl_Pixmap Voltage_Graph_Window::ZOOM_IN_ICON(ZOOM_IN_XPM);
Fl_Pixmap Voltage_Graph_Window::ZOOM_OUT_ICON(ZOOM_OUT_XPM);

Voltage_Graph_Window::Voltage_Graph_Window(int x, int y, int w, int h, const char *t) : _dx(x), _dy(y), _width(w),
	_height(h), _title(t), _soma_id(0), _soma_index(0), _voltages(NULL), _graph(NULL), _zoom_heading(NULL),
	_zoom_out_button(NULL), _zoom_in_button(NULL), _mv_min_spinner(NULL), _mv_max_spinner(NULL), _range_units(NULL),
	_export_image_button(NULL), _detect_peaks_button(NULL), _ok_button(NULL), _window(NULL), _image_chooser(NULL),
	_report_peaks_chooser(NULL), _error_dialog(NULL), _success_dialog(NULL) {}

Voltage_Graph_Window::~Voltage_Graph_Window() {
	delete _window;
	delete _image_chooser;
	delete _report_peaks_chooser;
	delete _error_dialog;
	delete _success_dialog;
}

static int text_width(const char *l, int pad = 0) {
	int lw = 0, lh = 0;
	fl_measure(l, lw, lh, 0);
	return lw + 2 * pad;
}

static const char *EN_DASH = "\xE2\x80\x93"; // UTF-8 encoding of U+2013 "EN DASH"

void Voltage_Graph_Window::initialize() {
	if (_window) { return; }
	Fl_Group *prev_current = Fl_Group::current();
	Fl_Group::current(NULL);
	// Populate window
	_window = new Fl_Double_Window(_dx, _dy, _width, _height, _title.c_str());
#ifdef LARGE_INTERFACE
	int ok_w = 100, btn_w = 124, wgt_h = 28, pad = 4;
#else
	int ok_w = 80, btn_w = 100, wgt_h = 22, pad = 0;
#endif
	int wx = 10, wy = _height - wgt_h - 10;
	int wgt_w = text_width("Zoom:");
	_zoom_heading = new Label(wx, wy, wgt_w, wgt_h, "Zoom:");
	wx += _zoom_heading->w();
	_zoom_out_button = new OS_Repeat_Button(wx, wy, wgt_h, wgt_h);
	wx += _zoom_out_button->w() + 5;
	_zoom_in_button = new OS_Repeat_Button(wx, wy, wgt_h, wgt_h);
	wx += _zoom_in_button->w() + 5;
	Spacer *zoom_range_spacer = new Spacer(wx, wy, 2, wgt_h);
	wx += zoom_range_spacer->w() + 5;
	wgt_w = text_width("Range:", 1);
	_mv_min_spinner = new Default_Spinner(wx + wgt_w, wy, text_width("-999") + wgt_h, wgt_h, "Range:");
	wx += wgt_w + _mv_min_spinner->w();
	wgt_w = text_width(EN_DASH, 3);
	_mv_max_spinner = new Default_Spinner(wx + wgt_w, wy, text_width("-999") + wgt_h, wgt_h, EN_DASH);
	wx += wgt_w + _mv_max_spinner->w();
	_range_units = new Label(wx, wy, text_width("mV", 3), wgt_h, "mV");
	wx += _range_units->w() + 5;
	int wrx = _width - 10;
	_graph = new Voltage_Graph(10, 10, _width-20, wy-20);
	_ok_button = new Default_Button(wrx-ok_w, wy, ok_w, wgt_h, "OK");
	wrx -= _ok_button->w() + 20 + pad;
	_detect_peaks_button = new OS_Button(wrx-btn_w, wy, btn_w, wgt_h, "Detect Peaks");
	wrx -= _detect_peaks_button->w() + 10 + pad;
	_export_image_button = new OS_Button(wrx-btn_w, wy, btn_w, wgt_h, "Export Image");
	wrx -= _export_image_button->w() + pad;
	Fl_Box *spacer = new Fl_Box(wx, _graph->y(), wrx-wx, _graph->h());
	_window->end();
	// Initialize window
	_window->resizable(spacer);
	_window->callback((Fl_Callback *)close_cb, this);
	// Initialize window's children
	_graph->soma_id(_soma_id);
	_zoom_heading->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	_zoom_out_button->shortcut(FL_COMMAND + '-');
	_zoom_out_button->tooltip("Zoom Out (" COMMAND_KEY_PLUS "-)");
	_zoom_out_button->callback((Fl_Callback *)zoom_out_cb, this);
	_zoom_out_button->image(ZOOM_OUT_ICON);
	_zoom_in_button->shortcut(FL_COMMAND + FL_SHIFT + '=');
	_zoom_in_button->tooltip("Zoom In (" COMMAND_KEY_PLUS "+)");
	_zoom_in_button->callback((Fl_Callback *)zoom_in_cb, this);
	_zoom_in_button->image(ZOOM_IN_ICON);
	_mv_min_spinner->type(FL_INT_INPUT);
	_mv_min_spinner->step(1.0);
	_mv_min_spinner->callback((Fl_Callback *)range_cb, this);
	_mv_max_spinner->type(FL_INT_INPUT);
	_mv_max_spinner->step(1.0);
	_mv_max_spinner->callback((Fl_Callback *)range_cb, this);
	_export_image_button->shortcut(FL_COMMAND + 'p');
	_export_image_button->tooltip("Export Image (" COMMAND_KEY_PLUS "P)");
	_export_image_button->callback((Fl_Callback *)export_image_cb, this);
	_detect_peaks_button->shortcut(FL_COMMAND + 'k');
	_detect_peaks_button->tooltip("Detect Peaks (" COMMAND_KEY_PLUS "K)");
	_detect_peaks_button->callback((Fl_Callback *)detect_peaks_cb, this);
	_ok_button->tooltip("OK (" ENTER_KEY_NAME ")");
	_ok_button->callback((Fl_Callback *)close_cb, this);
	// Initialize other windows and dialogs
	_image_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	_image_chooser->title("Export Image");
	_image_chooser->filter(Image::FILE_CHOOSER_FILTER);
	_image_chooser->preset_file("viz_voltage_graph.png");
	_report_peaks_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	_report_peaks_chooser->title("Report Peaks");
	_report_peaks_chooser->filter("Text File\t*.txt\n");
	_report_peaks_chooser->preset_file("viz_voltage_peaks.txt");
	_error_dialog = new Modal_Dialog(_window, "Error", Modal_Dialog::ERROR_ICON);
	_success_dialog = new Modal_Dialog(_window, "Success", Modal_Dialog::SUCCESS_ICON);
	Fl_Group::current(prev_current);
}

void Voltage_Graph_Window::refresh() {
	_window->label(_title.c_str());
	if (!_voltages) { return; }
	Voltage_Chart *vc = _graph->scroll()->chart();
	vc->data_from(_voltages, _soma_index);
	_mv_min_spinner->range(MIN(-999.0f, vc->min()), vc->min());
	_mv_min_spinner->default_value(vc->min());
	_mv_max_spinner->range(vc->max(), MAX(999.0f, vc->max()));
	_mv_max_spinner->default_value(vc->max());
}

void Voltage_Graph_Window::show(const Fl_Widget *p) {
	initialize();
	refresh();
	Fl_Window *prev_grab = Fl::grab();
	_window->position(p->x() + _dx, p->y() + _dy);
	Fl::grab(NULL);
	_ok_button->take_focus();
	_window->show();
	while (_window->shown()) { Fl::wait(); }
	Fl::grab(prev_grab);
}

void Voltage_Graph_Window::zoom_out_cb(Fl_Widget *, Voltage_Graph_Window *vgw) {
	vgw->_graph->scroll()->zoom_out();
}

void Voltage_Graph_Window::zoom_in_cb(Fl_Widget *, Voltage_Graph_Window *vgw) {
	vgw->_graph->scroll()->zoom_in();
}

void Voltage_Graph_Window::range_cb(Fl_Widget *, Voltage_Graph_Window *vgw) {
	float min = (float)vgw->_mv_min_spinner->value();
	float max = (float)vgw->_mv_max_spinner->value();
	vgw->_graph->scroll()->chart()->range(min, max);
	vgw->_graph->redraw();
}

void Voltage_Graph_Window::export_image_cb(Fl_Widget *, Voltage_Graph_Window *vgw) {
	int status = vgw->_image_chooser->show();
	if (status == 1) { return; }
	// Get name of chosen file
	const char *filename = vgw->_image_chooser->filename();
	const char *basename = fl_filename_name(filename);
	// Operation failed
	if (status == -1) {
		std::string msg = "Could not export ";
		msg = msg + basename + "!\n\n" + vgw->_image_chooser->errmsg();
		vgw->_error_dialog->message(strdup(msg.c_str()));
		vgw->_error_dialog->show(vgw->_window);
		return;
	}
	// Write image
	Image::Format fmt = (Image::Format)vgw->_image_chooser->filter_value();
	int err = vgw->_graph->write_image(filename, fmt);
	if (err) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vgw->_error_dialog->message(strdup(msg.c_str()));
		vgw->_error_dialog->show(vgw->_window);
	}
	else {
		std::string msg = "Exported ";
		msg = msg + basename + "!";
		vgw->_success_dialog->message(strdup(msg.c_str()));
		vgw->_success_dialog->show(vgw->_window);
	}
}

void Voltage_Graph_Window::detect_peaks_cb(Fl_Widget *, Voltage_Graph_Window *vgw) {
	int status = vgw->_report_peaks_chooser->show();
	if (status == 1) { return; }
	// Get name of chosen file
	const char *filename = vgw->_report_peaks_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	ofs.setf(std::ios::fixed, std::ios::floatfield);
	ofs.precision(2);
	// Operation failed
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vgw->_error_dialog->message(strdup(msg.c_str()));
		vgw->_error_dialog->show(vgw->_window);
		return;
	}
	// Detect peaks
	ofs << "# " << vgw->_title << "\n";
	ofs << "# <cycle id> <voltage> <firing state>\n";
	Voltage_Chart *vc = vgw->_graph->scroll()->chart();
	size_t n = vc->num_points();
	std::vector<float> &vs = vc->voltages();
	firings_instance_t &fs = vc->firings();
	size_t n_peaks = 0;
	for (size32_t t = 1; t < n - 1; t++) {
		float v0 = vs[t-1], v1 = vs[t], v2 = vs[t+1];
		if (v1 > v0 && v1 > v2) {
			Firing_State s = fs[t];
			fs[t] = (Firing_State)(s | PEAK);
			ofs << t << " " << v1 << " " << s << "\n";
			n_peaks++;
		}
	}
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss << "Found " << n_peaks << " peak" << (n_peaks != 1 ? "s" : "") << "!\n";
	ss << "Saved report to " << basename << "!";
	vgw->_success_dialog->message(strdup(ss.str().c_str()));
	vgw->_success_dialog->show(vgw->_window);
	vgw->_graph->redraw();
}

void Voltage_Graph_Window::close_cb(Fl_Widget *, Voltage_Graph_Window *vgw) { vgw->_window->hide(); }
