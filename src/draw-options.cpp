#include "draw-options.h"

Draw_Options::Draw_Options() : _axon_conns(true), _den_conns(false), _gap_junctions(true), _neur_fields(true),
	_conn_fields(false), _syn_dots(true), _to_axon(true), _to_via(true), _to_synapse(true), _to_den(true),
	_allow_letters(false), _only_show_selected(false), _only_conn_selected(false), _only_show_marked(false),
	_only_show_clipped(true), _only_enable_clipped(false), _orthographic(false), _left_handed(false),
	_show_rotation_guide(true), _show_axes(true), _show_axis_labels(true), _show_bulletin(false), _show_fps(false),
	_invert_background(false), _display(STATIC_MODEL), _display_value_for_somas(false), _show_inactive_somas(true),
	_show_color_scale(true), _weights_color_after(true) {}
