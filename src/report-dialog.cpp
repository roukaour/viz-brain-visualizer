#include "report-dialog.h"

#pragma warning(push, 0)
#include <FL/Enumerations.H>
#include <FL/Fl_Double_Window.H>
#pragma warning(pop)

#include "os-themes.h"
#include "widgets.h"
#include "report-dialog.h"

Report_Dialog::Report_Dialog(const char *t) : _title(t), _canceled(false), _dialog(NULL), _body(NULL),
	_report_bounding_box(NULL), _report_relations(NULL), _ok_button(NULL), _cancel_button(NULL) {}

Report_Dialog::~Report_Dialog() {
	delete _dialog;
	delete _body;
	delete _report_bounding_box;
	delete _report_relations;
	delete _ok_button;
	delete _cancel_button;
}

void Report_Dialog::initialize() {
	if (_dialog) { return; }
	Fl_Group *prev_current = Fl_Group::current();
	Fl_Group::current(NULL);
	// Populate dialog
	_dialog = new Fl_Double_Window(0, 0, 0, 0, _title);
	_body = new Label(0, 0, 0, 0, "When reporting selected somas:");
	_report_bounding_box = new OS_Check_Button(0, 0, 0, 0, "Report other somas within bounding box");
	_report_relations = new OS_Check_Button(0, 0, 0, 0, "Tell BOSS to report parents and children");
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
	_body->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
	_report_relations->set();
	_ok_button->tooltip("OK (" ENTER_KEY_NAME ")");
	_ok_button->callback((Fl_Callback *)close_cb, this);
	_cancel_button->shortcut(FL_Escape);
	_cancel_button->tooltip("Cancel (Esc)");
	_cancel_button->callback((Fl_Callback *)cancel_cb, this);
	Fl_Group::current(prev_current);
}

void Report_Dialog::refresh() {
	_canceled = false;
	_dialog->copy_label(_title);
	// Refresh widget positions and sizes
	int dy = 10;
#ifdef LARGE_INTERFACE
	int w = 390;
	int ww = w - 20;
	_body->resize(10, dy, ww, 27);
	dy += _body->h();
	_report_bounding_box->resize(10, dy, ww, 27);
	dy += _report_bounding_box->h();
	_report_relations->resize(10, dy, ww, 27);
	dy += _report_relations->h() + 10;
#ifdef _WIN32
	_ok_button->resize(w-224, dy, 100, 28);
	_cancel_button->resize(w-110, dy, 100, 28);
#else
	_cancel_button->resize(w-224, dy, 100, 28);
	_ok_button->resize(w-110, dy, 100, 28);
#endif
	dy += _ok_button->h() + 10;
#else
	int w = 320;
	int ww = w - 20;
	_body->resize(10, dy, ww, 21);
	dy += _body->h();
	_report_bounding_box->resize(10, dy, ww, 21);
	dy += _report_bounding_box->h();
	_report_relations->resize(10, dy, ww, 21);
	dy += _report_relations->h() + 10;
#ifdef _WIN32
	_ok_button->resize(w-184, dy, 80, 22);
	_cancel_button->resize(w-90, dy, 80, 22);
#else
	_cancel_button->resize(w-184, dy, 80, 22);
	_ok_button->resize(w-90, dy, 80, 22);
#endif
	dy += _cancel_button->h() + 10;
#endif
	_dialog->size_range(w, dy, w, dy);
	_dialog->size(w, dy);
	_dialog->redraw();
}

void Report_Dialog::show(const Fl_Widget *p) {
	initialize();
	refresh();
	int x = p->x() + (p->w() - _dialog->w()) / 2;
	int y = p->y() + (p->h() - _dialog->h()) / 2;
	_dialog->position(x, y);
	_ok_button->take_focus();
	_dialog->show();
	while (_dialog->shown()) { Fl::wait(); }
}

void Report_Dialog::close_cb(Fl_Widget *, Report_Dialog *rd) {
	rd->_dialog->hide();
}

void Report_Dialog::cancel_cb(Fl_Widget *, Report_Dialog *rd) {
	rd->_canceled = true;
	rd->_dialog->hide();
}
