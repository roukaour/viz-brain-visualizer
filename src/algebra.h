#ifndef ALGEBRA_H
#define ALGEBRA_H

#include <cmath>

// Visual Studio 2012 and below do not define log1p or expm1
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#define log1p(x) log(1.0f + (x))
#define expm1(x) (exp(x) - 1.0f)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define EPSILON 1.192092896e-7f

#define PI          3.141592653589793238463
#define TWO_PI      6.283185307179586476925
#define HALF_PI     1.570796326794896619231
#define QUARTER_PI  0.785398163397448309616
#define SQRT_2      1.414213562373095048801
#define TWO_SQRT_2  2.828427124746190097603
#define HALF_SQRT_2 0.707106781186547524401
#define SQRT_3      1.732050807568877293527

double vector2_length(const double v[2]);
double vector2_dot(const double a[2], const double b[2]);
double vector2_cross(const double a[2], const double b[2]);
double vector2_angle(const double a[2], const double b[2]);

double vector_length(const double v[3]);
double vector_distance(const double a[3], const double b[3]);
double vector_dot(const double a[3], const double b[3]);
void vector_cross(const double a[3], const double b[3], double c[3]);

void identity_matrix(double m[16]);
void arcball_matrix(double r, const double p1[2], const double p2[2], double m[16]);
void rotate_x_matrix(double theta, double m[16]);
void rotate_y_matrix(double theta, double m[16]);
void rotate_z_matrix(double theta, double m[16]);

void matrix_mul(const double m1[16], const double m2[16], double m[16]);
void matrix_transpose(const double m[16], double t[16]);
void matrix_to_quat(const double m[16], double q[4]);

void quat_to_matrix(const double q[4], double m[16]);
void normalize_quat(double q[4]);

void matrix_to_axis_angle(const double m[16], double aa[4]);
void axis_angle_to_matrix(const double aa[4], double m[16]);
void normalize_axis_angle(double aa[4]);

#endif
