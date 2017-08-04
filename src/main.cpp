#include <iostream>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#pragma warning(pop)

#include "os-themes.h"
#include "viz-window.h"

#ifdef _WIN32

#include <shlobj.h>
#include <tchar.h>

#define MAKE_WSTR_HELPER(x) L ## x
#define MAKE_WSTR(x) MAKE_WSTR_HELPER(x)

#endif

#ifdef __APPLE__

Viz_Window *viz_window = NULL;

void open_dragged_cb(const char *filename) {
	if (viz_window == NULL) { return; }
	viz_window->open_and_load_all(filename);
}

#endif

static void use_theme(int &argc, char **&argv) {
	if (argc < 2) {
		OS::use_native_theme();
		return;
	}
	if (!strcmp(argv[1], "--classic")) {
		OS::use_classic_theme();
	}
	else if (!strcmp(argv[1], "--aero")) {
		OS::use_aero_theme();
	}
	else if (!strcmp(argv[1], "--metro")) {
		OS::use_metro_theme();
	}
	else if (!strcmp(argv[1], "--aqua")) {
		OS::use_aqua_theme();
	}
	else if (!strcmp(argv[1], "--greybird")) {
		OS::use_greybird_theme();
	}
	else if (!strcmp(argv[1], "--metal")) {
		OS::use_metal_theme();
	}
	else if (!strcmp(argv[1], "--blue")) {
		OS::use_blue_theme();
	}
	else if (!strcmp(argv[1], "--dark")) {
		OS::use_dark_theme();
	}
	else {
		OS::use_native_theme();
		return;
	}
	argv[1] = argv[0];
	argv++;
	argc--;
}

int main(int argc, char **argv) {
	std::ios::sync_with_stdio(false);
#ifdef _WIN32
	SetCurrentProcessExplicitAppUserModelID(MAKE_WSTR(PROGRAM_NAME));
	// Fix for Ctrl+Shift+0 keyboard shortcut being used as input method editor switch
	// <https://support.microsoft.com/en-us/kb/967893>
	HKEY key;
	LONG status = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Toggle"), 0, KEY_ALL_ACCESS, &key);
	if (status == ERROR_SUCCESS && key != NULL) {
		status = RegSetValueEx(key, _T("Hotkey"), 0, REG_SZ, (LPBYTE)"3\0", 2);
		RegCloseKey(key);
	}
#endif
#ifdef __APPLE__
	setenv("LANG", "en_US.UTF-8", 1);
#endif
	use_theme(argc, argv);
#ifdef __APPLE__
	fl_open_callback(open_dragged_cb);
	viz_window = new Viz_Window(0, 0, 1600, 1200); // allow space for components to lay out
#else
	Viz_Window *viz_window = new Viz_Window(0, 0, 1600, 1200); // allow space for components to lay out
#endif
	Fl::visual(FL_DOUBLE | FL_INDEX);
	viz_window->resize(75, 75, Fl::w() - 150, Fl::h() - 150);
	viz_window->show();
	if (argc > 1) {
		viz_window->open_and_load_all(argv[1]);
	}
#ifdef __APPLE__
	int r = Fl::run();
	delete viz_window;
	viz_window = NULL;
	return r;
#else
	return Fl::run();
#endif
}
