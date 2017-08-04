#include <cmath>

#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "coords.h"
#include "bounds.h"
#include "clip-volume.h"
#include "brain-model.h"
#include "model-state.h"
#include "model-area.h"
#include "algebra.h"
#include "color.h"
#include "os-themes.h"
#include "widgets.h"
#include "overview-area.h"

const double Overview_Area::FOV_Y = 30.0;

const double Overview_Area::NEAR_PLANE = 0.25;
const double Overview_Area::FAR_PLANE = 120.0;

const double Overview_Area::FOCAL_LENGTH = 5.0;

Overview_Area::Overview_Area(int x, int y, int w, int h, const char *l) : Fl_Gl_Window(x, y, w, h, l),
	_model_area(NULL), _dnd_receiver(NULL), _zoom(0.0), _initialized(false) {
	mode(FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);
	labelfont(OS_FONT);
	labelsize(OS_FONT_SIZE);
	align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	resizable(NULL);
	end();
}

void Overview_Area::refresh() {
	invalidate();
	if (visible()) { flush(); }
}

void Overview_Area::refresh_gl() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static double interpolate(double a, double b, double t) {
	t = sin(t * HALF_PI); // sinusoidal interpolation from 0 to 1
	return a * t + b * (1.0 - t);
}

void Overview_Area::refresh_projection() {
	if (!_model_area) { return; }
	const Draw_Options &draw_opts = _model_area->const_draw_options();
	float glcv[3];
	const float *bgcv;
	if (active()) {
		bgcv = draw_opts.invert_background() ? _model_area->INVERT_BACKGROUND_COLOR : _model_area->BACKGROUND_COLOR;
	}
	else {
		Color::fl2gl(FL_DARK3, glcv);
		bgcv = glcv;
	}
	glClearColor(bgcv[0], bgcv[1], bgcv[2], 1.0f);
	glClearDepth(1.0);
	glViewport(0, 0, w(), h());
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	const Model_State &state = _model_area->const_state();
	const Bounds &base = _model_area->const_model().bounds();
	double aspect = (double)w() / h();
	double r = interpolate((double)state.max_range(), (double)base.max_range(), _zoom);
	gluPerspective(FOV_Y, aspect, NEAR_PLANE * r, FAR_PLANE * r);
	if (draw_opts.left_handed()) {
		glScaled(-1.0, 1.0, 1.0); // mirror image
	}
}

void Overview_Area::refresh_view() {
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (!_model_area) { return; }
	const Model_State &state = _model_area->const_state();
	const Bounds &base = _model_area->const_model().bounds();
	const coord_t *state_c = state.center();
	const coord_t *base_c = base.center();
	double c[3];
	c[0] = interpolate((double)state_c[0], (double)base_c[0], _zoom);
	c[1] = interpolate((double)state_c[1], (double)base_c[1], _zoom);
	c[2] = interpolate((double)state_c[2], (double)base_c[2], _zoom);
	double r = interpolate((double)state.max_range(), (double)base.max_range(), _zoom);
	gluLookAt(c[0], c[1], c[2] + FOCAL_LENGTH * r, c[0], c[1], c[2], 0.0, 1.0, 0.0); // eye, look, up
	glTranslatef((float)c[0], (float)c[1], (float)c[2]);
	glMultMatrixd(state.rotate());
	glTranslatef((float)-c[0], (float)-c[1], (float)-c[2]);
}

void Overview_Area::draw() {
	if (!_initialized) {
#ifdef __APPLE__
		if (!context_valid()) { return; } // fix some OpenGL crashes
#endif
		refresh_gl();
		_initialized = true;
	}
	if (!valid()) {
		refresh_projection();
		valid(1);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (!_model_area || !active() || !_model_area->opened()) { return; }
	Clip_Volume::disable();
	refresh_view();
	const Brain_Model &m = _model_area->const_model();
	glPointSize(2.0f);
	glBegin(GL_POINTS);
	size32_t n = m.num_somas();
	for (size32_t i = 0; i < n; i++) {
		const Soma *s = m.soma(i);
		const Soma_Type *t = m.type(s->type_index());
		glColor3fv(t->color()->rgb());
		s->draw();
	}
	glEnd();
}

int Overview_Area::handle(int event) {
	if (_dnd_receiver) {
		switch (event) {
		case FL_DND_ENTER:
		case FL_DND_RELEASE:
		case FL_DND_LEAVE:
		case FL_DND_DRAG:
			return 1;
		case FL_PASTE:
			return _dnd_receiver->handle(event);
		}
	}
	return Fl_Gl_Window::handle(event);
}
