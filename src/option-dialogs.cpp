#pragma warning(push, 0)
#include <FL/Enumerations.H>
#include <FL/Fl_Double_Window.H>
#pragma warning(pop)

#include "os-themes.h"
#include "widgets.h"
#include "algebra.h"
#include "firing-spikes.h"
#include "option-dialogs.h"

Option_Dialog::Option_Dialog(const char *t) : _title(t), _canceled(false), _dialog(NULL), _content(NULL),
	_ok_button(NULL), _cancel_button(NULL) {}

Option_Dialog::~Option_Dialog() {
	delete _dialog;
	delete _content;
	delete _ok_button;
	delete _cancel_button;
}

void Option_Dialog::initialize() {
	if (_dialog) { return; }
	Fl_Group *prev_current = Fl_Group::current();
	Fl_Group::current(NULL);
	// Populate dialog
	_dialog = new Fl_Double_Window(0, 0, 0, 0, _title);
	_content = new Fl_Group(0, 0, 0, 0);
	_content->begin();
	initialize_content();
	_content->end();
	_dialog->begin();
#ifdef _WIN32
	_ok_button = new Default_Button(0, 0, 0, 0, "OK");
	_cancel_button = new OS_Button(0, 0, 0, 0, "Cancel");
#else
	_cancel_button = new OS_Button(0, 0, 0, 0, "Cancel");
	_ok_button = new Default_Button(0, 0, 0, 0, "OK");
#endif
	_dialog->end();
	// Initialize dialog
	_dialog->resizable(NULL);
	_dialog->callback((Fl_Callback *)cancel_cb, this);
	_dialog->set_modal();
	// Initialize dialog's children
	_ok_button->tooltip("OK (" ENTER_KEY_NAME ")");
	_ok_button->callback((Fl_Callback *)close_cb, this);
	_cancel_button->shortcut(FL_Escape);
	_cancel_button->tooltip("Cancel (Esc)");
	_cancel_button->callback((Fl_Callback *)cancel_cb, this);
	Fl_Group::current(prev_current);
}

void Option_Dialog::refresh() {
	_canceled = false;
	_dialog->copy_label(_title);
	// Refresh widget positions and sizes
	int dy = 10;
#ifdef LARGE_INTERFACE
	int w = 390;
#else
	int w = 320;
#endif
	dy += refresh_content(w - 20, dy) + 16;
#ifdef LARGE_INTERFACE
#ifdef _WIN32
	_ok_button->resize(w-224, dy, 100, 28);
	_cancel_button->resize(w-110, dy, 100, 28);
#else
	_cancel_button->resize(w-224, dy, 100, 28);
	_ok_button->resize(w-110, dy, 100, 28);
#endif
#else
#ifdef _WIN32
	_ok_button->resize(w-184, dy, 80, 22);
	_cancel_button->resize(w-90, dy, 80, 22);
#else
	_cancel_button->resize(w-184, dy, 80, 22);
	_ok_button->resize(w-90, dy, 80, 22);
#endif
#endif
	dy += _cancel_button->h() + 10;
	_dialog->size_range(w, dy, w, dy);
	_dialog->size(w, dy);
	_dialog->redraw();
}

void Option_Dialog::show(const Fl_Widget *p) {
	initialize();
	refresh();
	int x = p->x() + (p->w() - _dialog->w()) / 2;
	int y = p->y() + (p->h() - _dialog->h()) / 2;
	_dialog->position(x, y);
	_ok_button->take_focus();
	_dialog->show();
	while (_dialog->shown()) { Fl::wait(); }
}

void Option_Dialog::close_cb(Fl_Widget *, Option_Dialog *od) {
	od->_dialog->hide();
}

void Option_Dialog::cancel_cb(Fl_Widget *, Option_Dialog *od) {
	od->_canceled = true;
	od->_dialog->hide();
}

Report_Somas_Dialog::Report_Somas_Dialog(const char *t) : Option_Dialog(t), _body(NULL), _report_bounding_box(NULL),
	_report_relations(NULL) {}

Report_Somas_Dialog::~Report_Somas_Dialog() {
	delete _body;
	delete _report_bounding_box;
	delete _report_relations;
}

void Report_Somas_Dialog::initialize_content() {
	// Populate content group
	_body = new Label(0, 0, 0, 0, "When reporting selected somas:");
	_report_bounding_box = new OS_Check_Button(0, 0, 0, 0, "Report other somas within bounding box");
	_report_relations = new OS_Check_Button(0, 0, 0, 0, "Tell BOSS to report parents and children");
	// Initialize content group's children
	_body->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
	_report_relations->set();
}

int Report_Somas_Dialog::refresh_content(int ww, int dy) {
#ifdef LARGE_INTERFACE
	int txt_h = 27;
#else
	int txt_h = 21;
#endif
	int ch = txt_h * 3;
	_content->resize(10, dy, ww, ch);
	_body->resize(10, dy, ww, txt_h);
	dy += _body->h();
	_report_bounding_box->resize(10, dy, ww, txt_h);
	dy += _report_bounding_box->h();
	_report_relations->resize(10, dy, ww, txt_h);
	return ch;
}

Report_Averages_Dialog::Report_Averages_Dialog(const char *t) : Option_Dialog(t), _start_spinner(NULL),
	_stop_spinner(NULL), _step_spinner(NULL), _num_cycles(0) {}

Report_Averages_Dialog::~Report_Averages_Dialog() {
	delete _start_spinner;
	delete _stop_spinner;
	delete _step_spinner;
}

void Report_Averages_Dialog::initialize_content() {
	// Populate content group
	_start_spinner = new OS_Spinner(0, 0, 0, 0, "From:");
	_stop_spinner = new OS_Spinner(0, 0, 0, 0, "To:");
	_step_spinner = new OS_Spinner(0, 0, 0, 0, "Average interval:");
	// Initialize content group's children
	_start_spinner->type(FL_INT_INPUT);
	_start_spinner->range(0.0, (double)(std::numeric_limits<size32_t>::max() - 1));
	_start_spinner->step(1.0);
	_start_spinner->value(0.0);
	_stop_spinner->type(FL_INT_INPUT);
	_stop_spinner->range(0.0, (double)(std::numeric_limits<size32_t>::max() - 1));
	_stop_spinner->step(1.0);
	_stop_spinner->value(0.0);
	_step_spinner->type(FL_INT_INPUT);
	_step_spinner->range(1.0, (double)(std::numeric_limits<size32_t>::max() - 1));
	_step_spinner->step(1.0);
	_step_spinner->value(1.0);
}

static int text_width(const char *l, int pad = 0) {
	int lw = 0, lh = 0;
	fl_measure(l, lw, lh, 0);
	return lw + 2 * pad;
}

int Report_Averages_Dialog::refresh_content(int ww, int dy) {
	_start_spinner->range(0.0, _num_cycles - 1);
	_start_spinner->value(0.0);
	_stop_spinner->range(0.0, _num_cycles - 1);
	_stop_spinner->value(_num_cycles - 1);
	_step_spinner->range(1.0, _num_cycles);
	_step_spinner->value(MAX(_num_cycles / 100, 1.0));
#ifdef LARGE_INTERFACE
	int wgt_h = 28;
#else
	int wgt_h = 22;
#endif
	int ch = wgt_h * 2 + 5;
	_content->resize(10, dy, ww, ch);
	int wgt_w = text_width("10000", 2) + wgt_h;
	int txt_w = text_width("From:", 3);
	_start_spinner->resize(10+txt_w, dy, wgt_w, wgt_h);
	txt_w = text_width("To:", 3);
	_stop_spinner->resize(_start_spinner->x()+_start_spinner->w()+10+txt_w, dy, wgt_w, wgt_h);
	dy += _stop_spinner->h() + 5;
	txt_w = text_width("Average interval:", 3);
	_step_spinner->resize(10+txt_w, dy, wgt_w, wgt_h);
	return ch;
}
