#ifndef MODEL_AREA_H
#define MODEL_AREA_H

#include <deque>

#pragma warning(push, 0)
#include <FL/gl.h>
#include <FL/glu.h>
#include <FL/Fl_Gl_Window.H>
#pragma warning(pop)

#include "brain-model.h"
#include "model-state.h"
#include "draw-options.h"
#include "image.h"
#include "fps.h"

class Overview_Area;
class DnD_Receiver;
class Sim_Data;
class Input_Parser;
class Binary_Parser;
class Progress_Dialog;
class Waiting_Dialog;

typedef std::pair<size32_t, size32_t> soma_syn_t;

class Model_Area : public Fl_Gl_Window {
private:
	enum Mode { DRAWING, SELECTING, CLIPPING };
	static const float AXON_SYN_COLOR[3], DEN_SYN_COLOR[3], MARKED_SYN_COLOR[3];
	static const float GAP_JUNCTION_COLOR[3];
	static const float X_AXIS_COLOR[3], Y_AXIS_COLOR[3], Z_AXIS_COLOR[3];
	static const double FOV_Y;
	static const double NEAR_PLANE, FAR_PLANE;
	static const double FOCAL_LENGTH;
	static const double PAN_SCALE, ZOOM_SCALE;
	static const double ROTATION_GUIDE_SCALE;
	static const int ROTATION_GUIDE_DETAIL = 8;
	static const size_t MAX_HISTORY = 50;
	static const float SELECT_TOLERANCE;
	static const double ROTATE_AXIS_TOLERANCE;
public:
	enum Action { SELECT, CLIP, ROTATE, PAN, ZOOM, MARK };
	enum Rotation_Mode { ARCBALL_2D, ARCBALL_3D, AXIS_X, AXIS_Y, AXIS_Z };
	static const float BACKGROUND_COLOR[3], INVERT_BACKGROUND_COLOR[3];
private:
	Brain_Model _model;
	Overview_Area *_overview_area;
	DnD_Receiver *_dnd_receiver;
	Model_State _state, _prev_state, _saved_state;
	std::deque<Model_State> _history, _future;
	Draw_Options _draw_opts;
	FPS _fps;
	bool _opened, _initialized, _dragging;
	int _click_coords[2], _drag_coords[2];
	Rotation_Mode _rotation_mode;
	bool _scale_rotation, _invert_zoom;
public:
	Model_Area(int x, int y, int w, int h, const char *l = NULL);
	inline Action action(void) const { return (Action)type(); }
	void action(Action a);
	inline Brain_Model &model(void) { return _model; }
	inline const Brain_Model &const_model(void) const { return _model; }
	inline void overview_area(Overview_Area *o) { _overview_area = o; }
	inline void dnd_receiver(DnD_Receiver *dndr) { _dnd_receiver = dndr; }
	inline Model_State &state(void) { return _state; }
	inline const Model_State &const_state(void) const { return _state; }
	inline Draw_Options &draw_options(void) { return _draw_opts; }
	inline const Draw_Options &const_draw_options(void) const { return _draw_opts; }
	inline bool opened(void) const { return _opened; }
	inline Rotation_Mode rotation_mode(void) const { return (Rotation_Mode)_rotation_mode; }
	inline void rotation_mode(Rotation_Mode r) { _rotation_mode = r; }
	inline bool scale_rotation(void) const { return _scale_rotation; }
	inline void scale_rotation(bool s) { _scale_rotation = s; }
	inline bool invert_zoom(void) const { return _invert_zoom; }
	inline void invert_zoom(bool z) { _invert_zoom = z; }
	inline void resize(int x, int y, int w, int h) { Fl_Gl_Window::resize(x, y, w, h); if (shown()) { refresh(); } }
	void refresh(void);
	void clear(void);
	void prepare(void);
	void undo(void);
	void redo(void);
	void copy(void);
	void paste(void);
	void reset(void);
	void repeat_action(void);
	void snap_to_axes(void);
	void pivot_ith(size_t i);
	void select_top(int n);
	void select_id(size32_t id);
	void deselect_id(size32_t id);
	void deselect_ith(size_t i);
	size32_t select_syn_count(size8_t t_index, size_t y_count, bool count_den, Progress_Dialog *p = NULL);
	size32_t report_syn_count(std::ofstream &ofs, size8_t t_index, size_t y_count, bool count_den,
		Progress_Dialog *p = NULL);
	size_t mark_conn_paths(size32_t a_id, size32_t d_id, size_t limit, bool include_disabled, Waiting_Dialog *w = NULL);
	size_t select_conn_paths(size32_t a_id, size32_t d_id, size_t limit, bool include_disabled,
		Waiting_Dialog *w = NULL);
	size_t report_conn_paths(std::ofstream &ofs, size32_t a_id, size32_t d_id, size_t limit, bool include_disabled,
		Waiting_Dialog *w = NULL);
	Read_Status read_model_from(Input_Parser &ip, Progress_Dialog *p = NULL);
	Read_Status read_model_from(Binary_Parser &bp, Progress_Dialog *p = NULL);
	Read_Status read_state_from(Input_Parser &ip);
	int write_image(const char *f, Image::Format m);
	void write_selected_somas_to(std::ofstream &ofs, bool bounding_box, bool relations) const;
	void write_selected_synapses_to(std::ofstream &ofs) const;
	void write_marked_synapses_to(std::ofstream &ofs) const;
	void write_current_frequencies_to(std::ofstream &ofs) const;
	void write_average_frequencies_to(std::ofstream &ofs, size32_t start, size32_t stop, size32_t step,
		Progress_Dialog *p = NULL);
protected:
	void redraw(void);
	void draw(void);
	int handle(int event);
private:
	static void refresh_gl(void);
private:
	const Sim_Data *active_sim_data(void) const;
	bool mark_conn_paths(size32_t a_index, size32_t d_index, size_t limit, bool include_disabled,
		std::deque<size32_t> &path_syns, size_t &n_paths, std::deque<soma_syn_t> &stack, Waiting_Dialog *w = NULL);
	bool select_conn_paths(size32_t a_index, size32_t d_index, size_t limit, bool include_disabled,
		std::deque<size32_t> &path_somas, size_t &n_paths, std::deque<size32_t> &stack, Waiting_Dialog *w = NULL);
	bool report_conn_paths(std::ofstream &ofs, size32_t a_index, size32_t d_index, size_t limit, bool include_disabled,
		size_t &n_paths, std::deque<soma_syn_t> &stack, Waiting_Dialog *w = NULL);
	void refresh_selected(void) const;
	void refresh_cursor(void) const;
	void refresh_view(void);
	void refresh_projection(Mode mode);
	void prepare_for_model(void);
	void remember(const Model_State &s);
	void draw_static_model(void) const;
	void draw_inactive(void) const;
	void draw_firing_spikes(void) const;
	void draw_voltages(void) const;
	void draw_weights(void) const;
	void draw_selected(void) const;
	void draw_selected(const Sim_Data *sd) const;
	void draw_scale(const Sim_Data *sd, const char *l, std::streamsize p = 0) const;
	void draw_bulletin(void) const;
	void draw_clip_rect(void) const;
	void draw_rotation_guide(void);
	void draw_axes(void) const;
	void draw_fps(void) const;
	int handle_focus(int event) const;
	int handle_click(int event);
	int handle_drag(int event);
	void select(void);
	void clip(void);
	void rotate(void);
	void pan(void);
	void zoom(void);
	void mark(void);
};

#endif
