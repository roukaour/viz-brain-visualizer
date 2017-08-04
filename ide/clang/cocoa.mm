#import <Cocoa/Cocoa.h>

#include "cocoa.h"

void setWindowTransparency(Fl_Window *w, double alpha) {
	[fl_xid(w) setAlphaValue:alpha];
}
