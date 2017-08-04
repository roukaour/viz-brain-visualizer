#ifndef REPORT_DIALOG_H
#define REPORT_DIALOG_H

#include "utils.h"
#include "widgets.h"

class Fl_Double_Window;
class Fl_Box;
class Fl_Widget;

class Report_Dialog {
private:
	const char *_title;
	bool _canceled;
	Fl_Double_Window *_dialog;
	Label *_body;
	OS_Check_Button *_report_bounding_box, *_report_relations;
	Default_Button *_ok_button;
	OS_Button *_cancel_button;
public:
	Report_Dialog(const char *t = NULL);
	~Report_Dialog();
	inline bool canceled(void) const { return _canceled; }
	inline void canceled(bool c) { _canceled = c; }
	inline bool report_bounding_box(void) const { return _report_bounding_box->value() != 0; }
	inline bool report_relations(void) const { return _report_relations->value() != 0; }
private:
	void initialize(void);
	void refresh(void);
public:
	void show(const Fl_Widget *p);
private:
	static void close_cb(Fl_Widget *, Report_Dialog *rd);
	static void cancel_cb(Fl_Widget *, Report_Dialog *rd);
};

#endif
