#ifndef COLOR_MAPS_H
#define COLOR_MAPS_H

#include <vector>

#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#pragma warning(pop)

typedef std::vector<float> color_stop_t;

class Color_Map {
public:
	virtual ~Color_Map() {}
	virtual void map(float s, float *cv) const = 0;
	inline void map_glColor(float s) const { float cv[3]; map(s, cv); glColor3fv(cv); }
	virtual void draw(int x, int y, int w, int h) const = 0;
};

class Gradient_Map : public Color_Map {
protected:
	std::vector<color_stop_t> _color_stops;
public:
	Gradient_Map() : Color_Map(), _color_stops() {}
	virtual ~Gradient_Map() {}
	void initialize(const float (*cs)[3], size_t n);
	virtual void map(float s, float *cv) const;
	virtual void draw(int x, int y, int w, int h) const;
};

class Grayscale_Map : public Gradient_Map {
public:
	Grayscale_Map();
};

class Rainbow_Map : public Gradient_Map {
public:
	Rainbow_Map();
};

class Thermal_Map : public Gradient_Map {
public:
	Thermal_Map();
};

class Opposed_Map : public Gradient_Map {
public:
	Opposed_Map();
};

class CubeHelix_Map : public Color_Map {
private:
	float _start_hue;  // 0 = blue, 1 = red, 2 = green, 3 = blue, ...
	float _rotations;  // rotations (typically -1.5 to 1.5; e.g. -1.0 is one blue->green->red cycle)
	float _saturation; // 0 = grayscale, 1 = fully saturated
	float _gamma;      // gamma correction
public:
	CubeHelix_Map(float h = 0.0f, float r = -1.0f, float s = 1.0f, float g = 1.0f);
	virtual ~CubeHelix_Map() {}
	virtual void map(float s, float *cv) const;
	virtual void draw(int x, int y, int w, int h) const;
};

#endif
