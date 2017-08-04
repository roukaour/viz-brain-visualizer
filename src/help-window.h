#ifndef HELP_WINDOW_H
#define HELP_WINDOW_H

#include <string>

class Fl_Double_Window;
class Fl_Box;
class HTML_View;
class Default_Button;

class Help_Window {
private:
	int _dx, _dy, _width, _height;
	std::string _title;
	std::string _file;
	Fl_Double_Window *_window;
	HTML_View *_body;
	Default_Button *_ok_button;
	Fl_Box *_spacer;
public:
	Help_Window(int x, int y, int w, int h, const char *t = NULL);
	~Help_Window();
	inline void title(std::string t) { _title = t; }
	inline void file(std::string f) { _file = f; }
private:
	void initialize(void);
	void refresh(void);
public:
	void show(const Fl_Widget *p);
private:
	static void close_cb(Fl_Widget *w, Help_Window *hw);
};

#endif
