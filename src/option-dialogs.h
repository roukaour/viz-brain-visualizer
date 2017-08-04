#ifndef OPTION_DIALOGS_H
#define OPTION_DIALOGS_H

#include "utils.h"
#include "widgets.h"
#include "firing-spikes.h"

class Fl_Double_Window;
class Fl_Box;
class Fl_Widget;

class Option_Dialog {
protected:
	const char *_title;
	bool _canceled;
	Fl_Double_Window *_dialog;
	Fl_Group *_content;
	Default_Button *_ok_button;
	OS_Button *_cancel_button;
public:
	Option_Dialog(const char *t = NULL);
	virtual ~Option_Dialog();
	inline bool canceled(void) const { return _canceled; }
	inline void canceled(bool c) { _canceled = c; }
protected:
	void initialize(void);
	void refresh(void);
	virtual void initialize_content(void) = 0;
	virtual int refresh_content(int ww, int dy) = 0;
public:
	void show(const Fl_Widget *p);
private:
	static void close_cb(Fl_Widget *, Option_Dialog *od);
	static void cancel_cb(Fl_Widget *, Option_Dialog *od);
};

class Report_Somas_Dialog : public Option_Dialog {
private:
	Label *_body;
	OS_Check_Button *_report_bounding_box, *_report_relations;
public:
	Report_Somas_Dialog(const char *t);
	~Report_Somas_Dialog();
	inline bool report_bounding_box(void) const { return _report_bounding_box->value() != 0; }
	inline bool report_relations(void) const { return _report_relations->value() != 0; }
protected:
	void initialize_content(void);
	int refresh_content(int ww, int dy);
};

class Report_Averages_Dialog : public Option_Dialog {
private:
	OS_Spinner *_start_spinner, *_stop_spinner, *_step_spinner;
	size32_t _num_cycles;
public:
	Report_Averages_Dialog(const char *t);
	~Report_Averages_Dialog();
	inline void limit_spinners(const Firing_Spikes *fs) { _num_cycles = fs->num_cycles(); }
	inline size32_t start_time(void) const { return (size32_t)_start_spinner->value(); }
	inline size32_t stop_time(void) const { return (size32_t)_stop_spinner->value(); }
	inline size32_t step_time(void) const { return (size32_t)_step_spinner->value(); }
protected:
	void initialize_content(void);
	int refresh_content(int ww, int dy);
};

#endif
