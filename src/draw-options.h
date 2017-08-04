#ifndef DRAW_OPTIONS_H
#define DRAW_OPTIONS_H

class Draw_Options {
public:
	enum Display { STATIC_MODEL, FIRING_SPIKES, VOLTAGES, WEIGHTS };
private:
	bool _axon_conns, _den_conns, _gap_junctions, _neur_fields, _conn_fields, _syn_dots;
	bool _to_axon,  _to_via, _to_synapse,_to_den;
	bool _allow_letters, _only_show_selected, _only_conn_selected, _only_show_marked;
	bool _only_show_clipped, _only_enable_clipped;
	bool _orthographic, _left_handed;
	bool _show_rotation_guide, _show_axes, _show_axis_labels, _show_bulletin, _show_fps, _invert_background;
	Display _display;
	bool _display_value_for_somas, _show_inactive_somas, _show_color_scale;
	bool _weights_color_after;
public:
	Draw_Options();
	inline bool axon_conns(void) const { return _axon_conns; }
	inline void axon_conns(bool b) { _axon_conns = b; }
	inline bool den_conns(void) const { return _den_conns; }
	inline void den_conns(bool b) { _den_conns = b; }
	inline bool gap_junctions(void) const { return _gap_junctions; }
	inline void gap_junctions(bool b) { _gap_junctions = b; }
	inline bool neur_fields(void) const { return _neur_fields; }
	inline void neur_fields(bool b) { _neur_fields = b; }
	inline bool conn_fields(void) const { return _conn_fields; }
	inline void conn_fields(bool b) { _conn_fields = b; }
	inline bool syn_dots(void) const { return _syn_dots; }
	inline void syn_dots(bool b) { _syn_dots = b; }
	inline bool to_axon(void) const { return _to_axon; }
	inline void to_axon(bool b) { _to_axon = b; }
	inline bool to_via(void) const { return _to_via; }
	inline void to_via(bool b) { _to_via = b; }
	inline bool to_synapse(void) const { return _to_synapse; }
	inline void to_synapse(bool b) { _to_synapse = b; }
	inline bool to_den(void) const { return _to_den; }
	inline void to_den(bool b) { _to_den = b; }
	inline bool allow_letters(void) const { return _allow_letters; }
	inline void allow_letters(bool b) { _allow_letters = b; }
	inline bool only_show_selected(void) const { return _only_show_selected; }
	inline void only_show_selected(bool b) { _only_show_selected = b; }
	inline bool only_conn_selected(void) const { return _only_conn_selected; }
	inline void only_conn_selected(bool b) { _only_conn_selected = b; }
	inline bool only_show_marked(void) const { return _only_show_marked; }
	inline void only_show_marked(bool b) { _only_show_marked = b; }
	inline bool only_show_clipped(void) const { return _only_show_clipped; }
	inline void only_show_clipped(bool b) { _only_show_clipped = b; }
	inline bool only_enable_clipped(void) const { return _only_enable_clipped; }
	inline void only_enable_clipped(bool b) { _only_enable_clipped = b; }
	inline bool orthographic(void) const { return _orthographic; }
	inline void orthographic(bool b) { _orthographic = b; }
	inline bool left_handed(void) const { return _left_handed; }
	inline void left_handed(bool b) { _left_handed = b; }
	inline bool show_rotation_guide(void) const { return _show_rotation_guide; }
	inline void show_rotation_guide(bool b) { _show_rotation_guide = b; }
	inline bool show_axes(void) const { return _show_axes; }
	inline void show_axes(bool b) { _show_axes = b; }
	inline bool show_axis_labels(void) const { return _show_axis_labels; }
	inline void show_axis_labels(bool b) { _show_axis_labels = b; }
	inline bool show_bulletin(void) const { return _show_bulletin; }
	inline void show_bulletin(bool b) { _show_bulletin = b; }
	inline bool show_fps(void) const { return _show_fps; }
	inline void show_fps(bool b) { _show_fps = b; }
	inline bool invert_background(void) const { return _invert_background; }
	inline void invert_background(bool b) { _invert_background = b; }
	inline Display display(void) const { return _display; }
	inline void display(Display d) { _display = d; }
	inline bool display_value_for_somas(void) const { return _display_value_for_somas; }
	inline void display_value_for_somas(bool b) { _display_value_for_somas = b; }
	inline bool show_inactive_somas(void) const { return _show_inactive_somas; }
	inline void show_inactive_somas(bool b) { _show_inactive_somas = b; }
	inline bool show_color_scale(void) const { return _show_color_scale; }
	inline void show_color_scale(bool b) { _show_color_scale = b; }
	inline bool weights_color_after(void) const { return _weights_color_after; }
	inline void weights_color_after(bool b) { _weights_color_after = b; }
};

#endif
