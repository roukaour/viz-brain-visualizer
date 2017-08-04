#ifndef OVERVIEW_AREA_H
#define OVERVIEW_AREA_H

#pragma warning(push, 0)
#include <FL/Fl_Gl_Window.H>
#pragma warning(pop)

class Model_Area;

class Overview_Area : public Fl_Gl_Window {
private:
	static const double FOV_Y;
	static const double NEAR_PLANE, FAR_PLANE;
	static const double FOCAL_LENGTH;
private:
	const Model_Area *_model_area;
	DnD_Receiver *_dnd_receiver;
	double _zoom;
	bool _initialized;
private:
	static void refresh_gl(void);
private:
	void refresh_view(void);
	void refresh_projection(void);
public:
	Overview_Area(int x, int y, int w, int h, const char *l = NULL);
	inline void model_area(const Model_Area *m) { _model_area = m; }
	inline void dnd_receiver(DnD_Receiver *dndr) { _dnd_receiver = dndr; }
	inline double zoom(void) const { return _zoom; }
	inline void zoom(double z) { _zoom = z; }
	void refresh(void);
protected:
	void draw(void);
	int handle(int event);
};

#endif
