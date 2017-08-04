#include <iostream>
#include <cmath>

#include "algebra.h"

double vector2_length(const double v[2]) {
	return sqrt(v[0] * v[0] + v[1] * v[1]);
}

double vector2_dot(const double a[2], const double b[2]) {
	return a[0] * b[0] + a[1] * b[1];
}

double vector2_cross(const double a[2], const double b[2]) {
	return a[0] * b[1] - a[1] * b[0];
}

double vector2_angle(const double a[2], const double b[2]) {
	return atan2(vector2_cross(a, b), vector2_dot(a, b));
}

double vector_length(const double v[3]) {
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

double vector_distance(const double a[3], const double b[3]) {
	double d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
	return vector_length(d);
}

double vector_dot(const double a[3], const double b[3]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void vector_cross(const double a[3], const double b[3], double c[3]) {
	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];
}

void identity_matrix(double m[16]) {
	m[0] = m[5] = m[10] = m[15] = 1.0;
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = m[12] = m[13] = m[14] = 0.0;
}

// Return the z-coordinate for a point p projected onto a sphere of radius r centered at the origin.
static void project_to_sphere(double r, const double p[2], double v[3]) {
	double d = vector2_length(p);
	v[0] = p[0]; v[1] = p[1];
	v[2] = d < r * HALF_SQRT_2 ? sqrt(r * r - d * d) : r * r / d / 2.0;
}

// Rotate point p1 to p2, as projected on a sphere of radius 1, and store the corresponding 4x4 rotation matrix in m.
// Adapted from trackball.c by Gavin Bell, included with the GLUT 3.7 example programs.
// <http://www.opengl.org/resources/libraries/glut/glut_downloads.php>
void arcball_matrix(double r, const double p1[2], const double p2[2], double m[16]) {
	double q[4];
	if (fabs(p1[0] - p2[0]) < EPSILON && fabs(p1[1] - p2[1]) < EPSILON) {
		q[0] = q[1] = q[2] = 0.0; q[3] = 1.0;
	}
	else {
		double v1[3], v2[3], a[3];
		project_to_sphere(r, p1, v1);
		project_to_sphere(r, p2, v2);
		vector_cross(v2, v1, a);
		double t = vector_distance(v1, v2) / (2.0 * r);
		if (t > 1.0) { t = 1.0; }
		if (t < -1.0) { t = -1.0; }
		double phi = 4.0 * asin(t);
		t = sin(phi / 2.0) / vector_length(a);
		q[0] = a[0] * t; q[1] = a[1] * t; q[2] = a[2] * t; q[3] = cos(phi / 2.0);
	}
	quat_to_matrix(q, m);
}

void rotate_x_matrix(double theta, double m[16]) {
	double s = sin(theta), c = cos(theta);
	m[0]  = 1.0; m[1]  = 0.0; m[2]  = 0.0; m[3]  = 0.0;
	m[4]  = 0.0; m[5]  = c;   m[6]  = -s;  m[7]  = 0.0;
	m[8]  = 0.0; m[9]  = s;   m[10] = c;   m[11] = 0.0;
	m[12] = 0.0; m[13] = 0.0; m[14] = 0.0; m[15] = 1.0;
}

void rotate_y_matrix(double theta, double m[16]) {
	double s = sin(theta), c = cos(theta);
	m[0]  = c;   m[1]  = 0.0; m[2]  = s;   m[3]  = 0.0;
	m[4]  = 0.0; m[5]  = 1.0; m[6]  = 0.0; m[7]  = 0.0;
	m[8]  = -s;  m[9]  = 0.0; m[10] = c;   m[11] = 0.0;
	m[12] = 0.0; m[13] = 0.0; m[14] = 0.0; m[15] = 1.0;
}

void rotate_z_matrix(double theta, double m[16]) {
	double s = sin(theta), c = cos(theta);
	m[0]  = c;   m[1]  = -s;  m[2]  = 0.0; m[3]  = 0.0;
	m[4]  = s;   m[5]  = c;   m[6]  = 0.0; m[7]  = 0.0;
	m[8]  = 0.0; m[9]  = 0.0; m[10] = 1.0; m[11] = 0.0;
	m[12] = 0.0; m[13] = 0.0; m[14] = 0.0; m[15] = 1.0;
}

void matrix_mul(const double m1[16], const double m2[16], double m[16]) {
	m[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12];
	m[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13];
	m[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
	m[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];
	m[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12];
	m[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13];
	m[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
	m[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];
	m[8] = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12];
	m[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13];
	m[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
	m[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];
	m[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12];
	m[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13];
	m[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
	m[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];
}

void matrix_transpose(const double m[16], double t[16]) {
	t[0] = m[0]; t[1] = m[4]; t[2] = m[8]; t[3] = m[12];
	t[4] = m[1]; t[5] = m[5]; t[6] = m[9]; t[7] = m[13];
	t[8] = m[2]; t[9] = m[6]; t[10] = m[10]; t[11] = m[14];
	t[12] = m[3]; t[13] = m[7]; t[14] = m[11]; t[15] = m[15];
}

void matrix_to_quat(const double m[16], double q[4]) {
	double trace = m[0] + m[5] + m[10] + 1.0;
	if (trace > 0.0) {
		double s = 0.5 / sqrt(trace);
		q[0] = s * (m[9] - m[6]);
		q[1] = s * (m[2] - m[8]);
		q[2] = s * (m[4] - m[1]);
		q[3] = 0.25 / s;
	}
	else if (m[0] >= m[5] && m[0] >= m[10]) {
		double s = 2.0 * sqrt(1.0 + m[0] - m[5] - m[10]);
		q[0] = 0.5 / s;
		q[1] = (m[1] + m[4]) / s;
		q[2] = (m[2] + m[8]) / s;
		q[3] = (m[6] + m[9]) / s;
	}
	else if (m[5] >= m[0] && m[5] >= m[10]) {
		double s = 2.0 * sqrt(1.0 + m[5] - m[0] - m[10]);
		q[0] = (m[1] + m[4]) / s;
		q[1] = 0.5 / s;
		q[2] = (m[6] + m[9]) / s;
		q[3] = (m[2] + m[8]) / s;
	}
	else { // m[10] >= m[0] && m[10] >= m[5]
		double s = 2.0 * sqrt(1.0 + m[10] - m[0] - m[5]);
		q[0] = (m[2] + m[8]) / s;
		q[1] = (m[6] + m[9]) / s;
		q[2] = 0.5 / s;
		q[3] = (m[1] + m[4]) / s;
	}
}

void quat_to_matrix(const double q[4], double m[16]) {
	double q00, q01, q02, q03, q11, q12, q13, q22, q23;
	q00 = q[0] * q[0]; q01 = q[0] * q[1]; q02 = q[0] * q[2]; q03 = q[0] * q[3];
	q11 = q[1] * q[1]; q12 = q[1] * q[2]; q13 = q[1] * q[3];
	q22 = q[2] * q[2]; q23 = q[2] * q[3];
	m[0] = 1.0 - 2.0 * (q11 + q22); m[1] = 2.0 * (q01 - q23); m[2] = 2.0 * (q02 + q13); m[3] = 0.0;
	m[4] = 2.0 * (q01 + q23); m[5] = 1.0 - 2.0 * (q22 + q00); m[6] = 2.0 * (q12 - q03); m[7] = 0.0;
	m[8] = 2.0 * (q02 - q13); m[9] = 2.0 * (q12 + q03); m[10] = 1.0 - 2.0 * (q11 + q00); m[11] = 0.0;
	m[12] = m[13] = m[14] = 0.0; m[15] = 1.0;
}

void normalize_quat(double q[4]) {
	double d = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	if (fabs(d) > EPSILON) { q[0] /= d; q[1] /= d; q[2] /= d; q[3] /= d; }
}

void matrix_to_axis_angle(const double m[16], double aa[4]) {
	aa[0] = m[9] - m[6]; aa[1] = m[2] - m[8]; aa[2] = m[4] - m[1];
	aa[3] = acos((m[0] + m[5] + m[10] - 1.0) / 2.0);
}

void axis_angle_to_matrix(const double aa[4], double m[16]) {
	double x = aa[0], y = aa[1], z = aa[2], s = sin(aa[3]), c = cos(aa[3]);
	m[0] = c + x * x * (1.0 - c); m[1] = x * y * (1.0 - c) - z * s; m[2] = x * z * (1.0 - c) + y * s; m[3] = 0.0;
	m[4] = y * x * (1.0 - c) + z * s; m[5] = c + y * y * (1.0 - c); m[6] = y * z * (1.0 - c) - x * s; m[7] = 0.0;
	m[8] = z * x * (1.0 - c) - y * s; m[9] = z * y * (1.0 - c) + x * s; m[10] = c + z * z * (1.0 - c); m[11] = 0.0;
	m[12] = m[13] = m[14] = 0.0; m[15] = 1.0;
}

void normalize_axis_angle(double aa[4]) {
	double d = vector_length(aa);
	if (fabs(d) > EPSILON) { aa[0] /= d; aa[1] /= d; aa[2] /= d; }
	else { aa[0] = 1.0; aa[1] = aa[2] = aa[3] = 0.0; }
	if (aa[3] < 0.0) { aa[3] += TWO_PI; }
}
