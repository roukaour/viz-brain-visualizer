#ifndef INFO_DIALOG_H
#define INFO_DIALOG_H

#include <string>

#pragma warning(push, 0)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Scroll.H>
#pragma warning(pop)

#include "utils.h"

class Brain_Model;
class Soma;
class Clip_Volume;

class Summary_Dialog {
private:
	std::string _title;
	const Brain_Model *_model;
	int _min_w, _max_w;
	Fl_Double_Window *_dialog;
	Fl_Scroll *_body_scroll;
	Label *_body;
	OS_Button *_copy_button;
	Default_Button *_ok_button;
public:
	Summary_Dialog(std::string t);
	~Summary_Dialog();
private:
	void initialize(void);
	void refresh_body(const Soma *s, size32_t index, const Clip_Volume *v);
	void refresh_body(const Synapse *y, size32_t index);
	void refresh_body(const Clip_Volume *v);
	void refresh(void);
	void show(const Fl_Widget *p);
public:
	inline void title(std::string t) { _title = t; }
	inline void model(const Brain_Model *m) { _model = m; }
	inline void width_range(int min_w, int max_w) { _min_w = min_w; _max_w = max_w; }
	void show(const Fl_Widget *p, const Soma *s, size32_t index, const Clip_Volume *v);
	void show(const Fl_Widget *p, const Synapse *y, size32_t index);
	void show(const Fl_Widget *p, const Clip_Volume *v);
private:
	static void copy_cb(Fl_Widget *, Summary_Dialog *sd);
	static void close_cb(Fl_Widget *, Summary_Dialog *sd);
};

#endif
