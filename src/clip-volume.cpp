#include <iostream>

#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#pragma warning(pop)

#include "coords.h"
#include "algebra.h"
#include "clip-volume.h"

void Clip_Volume::enable() {
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
}

void Clip_Volume::disable() {
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
}

Clip_Volume::Clip_Volume() {
	reset();
}

void Clip_Volume::specify() const {
	glClipPlane(GL_CLIP_PLANE0, _top);
	glClipPlane(GL_CLIP_PLANE1, _right);
	glClipPlane(GL_CLIP_PLANE2, _bottom);
	glClipPlane(GL_CLIP_PLANE3, _left);
}

// Derive clipping planes from model view and projection matrices.
// Based on "Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix" by Gil Gribb and Klaus Hartmann.
// <http://graphics.cs.ucf.edu/cap4720/fall2008/plane_extraction.pdf>
void Clip_Volume::acquire() {
	double model_view[16], projection[16], m[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, model_view);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	matrix_mul(model_view, projection, m);
	_top[0] = m[3] - m[1]; _top[1] = m[7] - m[5]; _top[2] = m[11] - m[9]; _top[3] = m[15] - m[13];
	_right[0] = m[3] - m[0]; _right[1] = m[7] - m[4]; _right[2] = m[11] - m[8]; _right[3] = m[15] - m[12];
	_bottom[0] = m[3] + m[1]; _bottom[1] = m[7] + m[5]; _bottom[2] = m[11] + m[9]; _bottom[3] = m[15] + m[13];
	_left[0] = m[3] + m[0]; _left[1] = m[7] + m[4]; _left[2] = m[11] + m[8]; _left[3] = m[15] + m[12];
}

void Clip_Volume::copy(const Clip_Volume &v) {
	_top[0] = v._top[0]; _top[1] = v._top[1]; _top[2] = v._top[2]; _top[3] = v._top[3];
	_right[0] = v._right[0]; _right[1] = v._right[1]; _right[2] = v._right[2]; _right[3] = v._right[3];
	_bottom[0] = v._bottom[0]; _bottom[1] = v._bottom[1]; _bottom[2] = v._bottom[2]; _bottom[3] = v._bottom[3];
	_left[0] = v._left[0]; _left[1] = v._left[1]; _left[2] = v._left[2]; _left[3] = v._left[3];
}

void Clip_Volume::reset() {
	_top[0] = _right[0] = _bottom[0] = _left[0] = 0.0;
	_top[1] = _right[1] = _bottom[1] = _left[1] = 0.0;
	_top[2] = _right[2] = _bottom[2] = _left[2] = 0.0;
	_top[3] = _right[3] = _bottom[3] = _left[3] = 0.0;
}

bool Clip_Volume::contains(const coord_t c[3]) const {
	return c[0] * _top[0] + c[1] * _top[1] + c[2] * _top[2] + _top[3] >= 0.0 &&
		c[0] * _right[0] + c[1] * _right[1] + c[2] * _right[2] + _right[3] >= 0.0 &&
		c[0] * _bottom[0] + c[1] * _bottom[1] + c[2] * _bottom[2] + _bottom[3] >= 0.0 &&
		c[0] * _left[0] + c[1] * _left[1] + c[2] * _left[2] + _left[3] >= 0.0;
}
