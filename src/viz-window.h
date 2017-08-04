#ifndef VIZ_WINDOW_H
#define VIZ_WINDOW_H

#pragma warning(push, 0)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#pragma warning(pop)

#include "utils.h"

#ifdef _DEBUG
#define PROGRAM_NAME "Brain Visualizer [DEBUG]"
#else
#define PROGRAM_NAME "Brain Visualizer"
#endif

#define DEFAULT_CONFIG_FILE "viz.cfg"

struct Fl_Menu_Item;
class Fl_Box;
class Fl_Menu_;
class Fl_Wizard;
class Fl_Native_File_Chooser;

class Soma;
class Synapse;
class Model_Area;
class Overview_Area;
class Sidebar;
class Toolbar;
class Panel;
class Toolbar_Button;
class Toolbar_Toggle_Button;
class Toolbar_Radio_Button;
class Toolbar_Dropdown_Button;
class DnD_Receiver;
class Label;
class Spacer;
class OS_Button;
class OS_Repeat_Button;
class OS_Check_Button;
class OS_Radio_Button;
class Link_Button;
class Expander_Collapser;
class Toggle_Switch;
class OS_Spinner;
class Default_Spinner;
class OS_Slider;
class Default_Slider;
class Default_Value_Slider;
class Dropdown;
class Control_Group;
class OS_Tabs;
class OS_Tab;
class Status_Bar_Field;
class Help_Window;
class Modal_Dialog;
class Summary_Dialog;
class Progress_Dialog;
class Waiting_Dialog;
class Report_Somas_Dialog;
class Report_Averages_Dialog;

class Viz_Window : public Fl_Double_Window {
private:
	static const double FIRING_SPEED_TIMEOUTS[10];
private:
	DnD_Receiver *_dnd_receiver;
	Fl_Sys_Menu_Bar *_menu_bar;
	Toolbar *_toolbar, *_status_bar;
	Toolbar_Button *_open_model_tb, *_open_and_load_tb, *_export_image_tb, *_load_firing_spikes_tb, *_load_voltages_tb,
		*_load_weights_tb, *_load_prunings_tb, *_undo_tb, *_redo_tb, *_repeat_tb, *_copy_tb, *_paste_tb, *_reset_tb,
		*_snap_tb, *_reset_settings_tb;
	Toolbar_Toggle_Button *_axon_conns_tb, *_den_conns_tb, *_gap_junctions_tb, *_neur_fields_tb, *_conn_fields_tb,
		*_syn_dots_tb, *_to_axon_tb, *_to_via_tb, *_to_synapse_tb, *_to_den_tb, *_allow_letters_tb,
		*_only_show_selected_tb, *_only_conn_selected_tb, *_only_show_marked_tb, *_only_show_clipped_tb,
		*_orthographic_tb;
	Toolbar_Radio_Button *_select_tb, *_clip_tb, *_rotate_tb, *_pan_tb, *_zoom_tb, *_mark_tb;
	Toolbar_Dropdown_Button *_rotate_mode_tb;
	Fl_Menu_Item *_classic_theme_mi, *_aero_theme_mi, *_metro_theme_mi, *_aqua_theme_mi, *_greybird_theme_mi,
		*_metal_theme_mi, *_blue_theme_mi, *_dark_theme_mi, *_overview_area_mi, *_large_icon_mi, *_axon_conns_mi,
		*_den_conns_mi, *_gap_junctions_mi, *_neur_fields_mi, *_syn_dots_mi, *_conn_fields_mi, *_to_axon_mi,
		*_to_via_mi, *_to_synapse_mi, *_to_den_mi, *_allow_letters_mi, *_only_show_selected_mi, *_only_conn_selected_mi,
		*_only_show_marked_mi, *_only_show_clipped_mi, *_orthographic_mi, *_display_static_model_mi,
		*_display_firing_spikes_mi, *_display_voltages_mi, *_display_weights_mi, *_display_value_for_somas_mi,
		*_show_inactive_somas_mi, *_show_color_scale_mi, *_select_mi, *_clip_mi, *_rotate_mi, *_pan_mi, *_zoom_mi,
		*_mark_mi, *_2d_arcball_mi, *_3d_arcball_mi, *_x_axis_mi, *_y_axis_mi, *_z_axis_mi, *_2d_arcball_dd_mi,
		*_3d_arcball_dd_mi, *_x_axis_dd_mi, *_y_axis_dd_mi, *_z_axis_dd_mi;
	Model_Area *_model_area;
	Sidebar *_sidebar;
	Expander_Collapser *_overview_heading;
	Overview_Area *_overview_area;
	OS_Slider *_overview_zoom;
	Spacer *_sidebar_spacer;
	OS_Tabs *_sidebar_tabs;
	OS_Tab *_config_tab, *_selection_tab, *_marking_tab;
	Dropdown *_type_choice;
	Control_Group *_type_group;
	Default_Value_Slider *_soma_letter_size_slider;
	OS_Radio_Button *_type_is_letter, *_type_is_dot, *_type_is_hidden, *_type_is_disabled;
	Link_Button *_all_letter, *_all_dot, *_all_hidden, *_all_disabled;
	Dropdown *_type_color_choice;
	Fl_Box *_type_color_swatch;
	OS_Button *_load_config, *_save_config, *_reset_config;
	OS_Spinner *_select_id_spinner;
	OS_Button *_select_id, *_deselect_id;
	Dropdown *_select_type_choice;
	OS_Spinner *_syn_count_spinner;
	Toggle_Switch *_syn_kind;
	OS_Button *_select_syn_count, *_report_syn_count;
	OS_Spinner *_axon_id_spinner, *_den_id_spinner, *_limit_paths_spinner;
	OS_Check_Button *_include_disabled;
	OS_Button *_mark_conn_paths, *_select_conn_paths, *_report_conn_paths;
	OS_Button *_fetch_select_id, *_fetch_axon_id, *_fetch_den_id;
	Status_Bar_Field *_soma_count, *_synapse_count, *_marked_count, *_selected_count, *_selected_type,
		*_selected_coords, *_selected_hertz, *_selected_voltage;
	Toolbar_Button *_report_synapses, *_report_marked, *_report_selected, *_prev_selected, *_next_selected,
		*_deselect_shown, *_pivot_shown, *_summary;
	Panel *_simulation_bar;
	OS_Check_Button *_display_value_for_somas, *_show_inactive_somas, *_show_color_scale;
	Label *_display_heading;
	OS_Radio_Button *_display_static_model, *_display_firing_spikes, *_display_voltages, *_display_weights;
	Label *_display_choice_heading;
	Fl_Wizard *_display_wizard;
	Fl_Group *_display_static_group, *_display_firing_group, *_display_voltages_group, *_display_weights_group;
	Label *_static_size;
	Label *_firing_cycle_length;
	OS_Button *_firing_report_current, *_firing_report_average, *_firing_select_top;
	OS_Spinner *_firing_select_top_spinner;
	Label *_voltages_count;
	OS_Button *_voltages_graph_selected;
	OS_Repeat_Button *_weights_prev_change, *_weights_next_change, *_weights_prev_selected, *_weights_next_selected;
	Toggle_Switch *_weights_color_after;
	Default_Spinner *_weights_scale_center_spinner, *_weights_scale_spread_spinner;
	Default_Slider *_weights_scale_center_slider, *_weights_scale_spread_slider;
	OS_Spinner *_firing_speed_spinner, *_firing_time_spinner;
	OS_Button *_play_pause_firing;
	OS_Repeat_Button *_step_firing;
	OS_Slider *_firing_time_slider;
	Fl_Native_File_Chooser *_model_chooser, *_firing_chooser, *_voltages_chooser, *_weights_chooser, *_prunings_chooser,
		*_image_chooser, *_animation_chooser, *_state_load_chooser, *_state_save_chooser, *_config_load_chooser,
		*_config_save_chooser, *_text_report_chooser;
	Help_Window *_help_window;
	Modal_Dialog *_about_dialog, *_success_dialog, *_warning_dialog, *_error_dialog;
	Progress_Dialog *_progress_dialog;
	Waiting_Dialog *_waiting_dialog;
	Report_Somas_Dialog *_report_somas_dialog;
	Report_Averages_Dialog *_report_averages_dialog;
	Summary_Dialog *_summary_dialog;
	size_t _shown_selected;
	bool _playing;
	int _wx, _wy, _ww, _wh;
#if defined(__LINUX__)
	Pixmap _icon_pixmap, _icon_mask;
#endif
public:
	Viz_Window(int x, int y, int w, int h, const char *l = NULL);
#ifdef __APPLE__
	using Fl_Double_Window::show; // fix for "warning: 'Viz_Window::show' hides overloaded virtual function" in Mac OS X
#endif
	void show(void);
	void refresh_model_file(void);
	void refresh_static_model(void);
	void refresh_firing_spikes(void);
	void refresh_voltages(void);
	void refresh_weights(void);
	void refresh_selected(bool show_last = true);
	void refresh_selected_sim_data(void);
	void summary_dialog(void);
	void summary_dialog(const Soma *s, size32_t index);
	void summary_dialog(const Synapse *y, size32_t index);
	bool open_model(const char *filename);
	bool open_and_load_all(const char *filename);
	bool load_firing_spikes(const char *filename, bool warn = false);
	bool load_voltages(const char *filename, bool warn = false);
	bool load_weights(const char *filename, bool warn = false);
	bool load_prunings(const char *filename, bool warn = false);
	void overview_area(bool show);
private:
	void refresh_config(void);
	static void drag_and_drop_cb(DnD_Receiver *dndr, Viz_Window *vw);
	static void open_model_cb(Fl_Widget *w, Viz_Window *vw);
	static void close_model_cb(Fl_Widget *w, Viz_Window *vw);
	static void open_and_load_all_cb(Fl_Widget *w, Viz_Window *vw);
	static void load_firing_spikes_cb(Fl_Widget *w, Viz_Window *vw);
	static void unload_firing_spikes_cb(Fl_Widget *w, Viz_Window *vw);
	static void load_voltages_cb(Fl_Widget *w, Viz_Window *vw);
	static void unload_voltages_cb(Fl_Widget *w, Viz_Window *vw);
	static void load_weights_cb(Fl_Widget *w, Viz_Window *vw);
	static void load_prunings_cb(Fl_Widget *w, Viz_Window *vw);
	static void unload_weights_cb(Fl_Widget *w, Viz_Window *vw);
	static void export_image_cb(Fl_Widget *w, Viz_Window *vw);
	static void export_animation_cb(Fl_Widget *w, Viz_Window *vw);
	static void exit_cb(Fl_Widget *w, void *v);
	static void undo_cb(Fl_Widget *w, Viz_Window *vw);
	static void redo_cb(Fl_Widget *w, Viz_Window *vw);
	static void repeat_cb(Fl_Widget *w, Viz_Window *vw);
	static void copy_state_cb(Fl_Widget *w, Viz_Window *vw);
	static void paste_state_cb(Fl_Widget *w, Viz_Window *vw);
	static void load_state_cb(Fl_Widget *w, Viz_Window *vw);
	static void save_state_cb(Fl_Widget *w, Viz_Window *vw);
	static void reset_state_cb(Fl_Widget *w, Viz_Window *vw);
	static void load_config_cb(Fl_Widget *w, Viz_Window *vw);
	static void save_config_cb(Fl_Widget *w, Viz_Window *vw);
	static void reset_config_cb(Fl_Widget *w, Viz_Window *vw);
	static void toolbar_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void sidebar_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void status_bar_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void large_icon_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void show_axes_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void show_axis_labels_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void show_bulletin_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void show_fps_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void invert_background_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void transparent_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void full_screen_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void axon_conns_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void axon_conns_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void den_conns_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void den_conns_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void gap_junctions_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void gap_junctions_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void neur_fields_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void neur_fields_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void conn_fields_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void conn_fields_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void syn_dots_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void syn_dots_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void to_axon_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void to_axon_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void to_via_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void to_via_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void to_synapse_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void to_synapse_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void to_den_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void to_den_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void allow_letters_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void allow_letters_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void only_show_selected_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void only_show_selected_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void only_conn_selected_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void only_conn_selected_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void only_show_marked_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void only_show_marked_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void only_show_clipped_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void only_show_clipped_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void only_enable_clipped_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void orthographic_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void orthographic_tb_cb(Toolbar_Toggle_Button *tb, Viz_Window *vw);
	static void left_handed_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void select_cb(Fl_Widget *w, Viz_Window *vw);
	static void select_tb_cb(Toolbar_Radio_Button *tb, Viz_Window *vw);
	static void clip_cb(Fl_Widget *w, Viz_Window *vw);
	static void clip_tb_cb(Toolbar_Radio_Button *tb, Viz_Window *vw);
	static void rotate_cb(Fl_Widget *w, Viz_Window *vw);
	static void rotate_tb_cb(Toolbar_Radio_Button *tb, Viz_Window *vw);
	static void pan_cb(Fl_Widget *w, Viz_Window *vw);
	static void pan_tb_cb(Toolbar_Radio_Button *tb, Viz_Window *vw);
	static void zoom_cb(Fl_Widget *w, Viz_Window *vw);
	static void zoom_tb_cb(Toolbar_Radio_Button *tb, Viz_Window *vw);
	static void mark_cb(Fl_Widget *w, Viz_Window *vw);
	static void mark_tb_cb(Toolbar_Radio_Button *tb, Viz_Window *vw);
	static void snap_cb(Fl_Widget *w, Viz_Window *vw);
	static void reset_settings_cb(Fl_Widget *w, Viz_Window *vw);
	static void rotate_2d_arcball_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void rotate_3d_arcball_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void rotate_x_axis_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void rotate_y_axis_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void rotate_z_axis_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void show_rotation_guide_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void scale_rotation_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void invert_zoom_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void help_cb(Fl_Widget *w, Viz_Window *vw);
	static void about_cb(Fl_Widget *w, Viz_Window *vw);
	static void classic_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void aero_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void metro_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void aqua_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void greybird_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void metal_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void blue_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void dark_theme_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void overview_area_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void overview_area_sb_cb(Expander_Collapser *w, Viz_Window *vw);
	static void overview_zoom_cb(Fl_Widget *w, Viz_Window *vw);
	static void soma_letter_size_cb(Default_Value_Slider *s, Viz_Window *vw);
	static void type_get_cb(Fl_Widget *w, Viz_Window *vw);
	static void type_set_cb(Fl_Widget *w, Viz_Window *vw);
	static void all_types_use_letter(Fl_Widget *w, Viz_Window *vw);
	static void all_types_use_dot(Fl_Widget *w, Viz_Window *vw);
	static void all_types_hidden(Fl_Widget *w, Viz_Window *vw);
	static void all_types_disabled(Fl_Widget *w, Viz_Window *vw);
	static void select_id_cb(Fl_Widget *w, Viz_Window *vw);
	static void deselect_id_cb(Fl_Widget *w, Viz_Window *vw);
	static void select_syn_count_cb(Fl_Widget *w, Viz_Window *vw);
	static void report_syn_count_cb(Fl_Widget *w, Viz_Window *vw);
	static void mark_conn_paths_cb(Fl_Widget *w, Viz_Window *vw);
	static void select_conn_paths_cb(Fl_Widget *w, Viz_Window *vw);
	static void report_conn_paths_cb(Fl_Widget *w, Viz_Window *vw);
	static void fetch_select_id_cb(Fl_Widget *w, Viz_Window *vw);
	static void fetch_axon_id_cb(Fl_Widget *w, Viz_Window *vw);
	static void fetch_den_id_cb(Fl_Widget *w, Viz_Window *vw);
	static void report_synapses_cb(Fl_Widget *w, Viz_Window *vw);
	static void report_marked_cb(Fl_Widget *w, Viz_Window *vw);
	static void report_selected_cb(Fl_Widget *w, Viz_Window *vw);
	static void prev_selected_cb(Fl_Widget *w, Viz_Window *vw);
	static void next_selected_cb(Fl_Widget *w, Viz_Window *vw);
	static void deselect_shown_cb(Fl_Widget *w, Viz_Window *vw);
	static void pivot_shown_cb(Fl_Widget *w, Viz_Window *vw);
	static void summary_cb(Fl_Widget *w, Viz_Window *vw);
	static void set_simulation_display_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void set_simulation_display_tb_cb(OS_Radio_Button *b, Viz_Window *vw);
	static void display_value_for_somas_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void display_value_for_somas_tb_cb(OS_Check_Button *b, Viz_Window *vw);
	static void show_inactive_somas_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void show_inactive_somas_tb_cb(OS_Check_Button *b, Viz_Window *vw);
	static void show_color_scale_cb(Fl_Menu_ *m, Viz_Window *vw);
	static void show_color_scale_tb_cb(OS_Check_Button *b, Viz_Window *vw);
	static void play_pause_firing_cb(Fl_Widget *w, Viz_Window *vw);
	static void step_firing_cb(Fl_Widget *w, Viz_Window *vw);
	static void firing_start_time_cb(Fl_Widget *w, Viz_Window *vw);
	static void do_step_firing_time_cb(Viz_Window *vw);
	static void firing_report_current_cb(Fl_Widget *w, Viz_Window *vw);
	static void firing_report_average_cb(Fl_Widget *w, Viz_Window *vw);
	static void firing_select_top_cb(Fl_Widget *w, Viz_Window *vw);
	static void voltages_graph_selected_cb(Fl_Widget *w, Viz_Window *vw);
	static void weights_color_after_cb(Toggle_Switch *w, Viz_Window *vw);
	static void weights_prev_change_cb(Fl_Widget *w, Viz_Window *vw);
	static void weights_next_change_cb(Fl_Widget *w, Viz_Window *vw);
	static void weights_prev_selected_cb(Fl_Widget *w, Viz_Window *vw);
	static void weights_next_selected_cb(Fl_Widget *w, Viz_Window *vw);
	static void weights_scale_center_cb(Fl_Widget *w, Viz_Window *vw);
	static void weights_scale_spread_cb(Fl_Widget *w, Viz_Window *vw);
};

#endif
