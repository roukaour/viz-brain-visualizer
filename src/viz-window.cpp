#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ios>
#include <iomanip>
#include <locale>
#include <algorithm>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Wizard.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Tooltip.H>
#include <FL/filename.H>
#pragma warning(pop)

#include "utils.h"
#include "os-themes.h"
#include "coords.h"
#include "viz-icons.h"
#include "version.h"
#include "image.h"
#include "color.h"
#include "widgets.h"
#include "overview-area.h"
#include "model-area.h"
#include "help-window.h"
#include "modal-dialog.h"
#include "option-dialogs.h"
#include "summary-dialog.h"
#include "progress-dialog.h"
#include "waiting-dialog.h"
#include "voltage-graph-window.h"
#include "input-parser.h"
#include "binary-parser.h"
#include "firing-spikes.h"
#include "voltages.h"
#include "weights.h"
#include "viz-window.h"

#ifdef _WIN32
#include "resource.h"
#endif

#ifdef __APPLE__
#include "cocoa.h"
#endif

#ifdef __LINUX__
#include <X11/xpm.h>
#include "viz-16.xpm"
#endif

// Hz: {1, 2, 5, 10, 20, 25, 33, 50, 67, 100}
const double Viz_Window::FIRING_SPEED_TIMEOUTS[10] = {1.0, 0.5, 0.2, 0.1, 0.05, 0.04, 0.03, 0.02, 0.015, 0.01};

static int text_width(const char *l, int pad = 0) {
	int lw = 0, lh = 0;
	fl_measure(l, lw, lh, 0);
	return lw + 2 * pad;
}

Viz_Window::Viz_Window(int x, int y, int w, int h, const char *) : Fl_Double_Window(x, y, w, h, PROGRAM_NAME),
	_shown_selected(0), _playing(false), _wx(x), _wy(y), _ww(w), _wh(h)
#ifdef __LINUX__
	, _icon_pixmap(), _icon_mask()
#endif
{
#define VW_MENU_STYLE FL_NORMAL_LABEL, OS_FONT, OS_FONT_SIZE, FL_FOREGROUND_COLOR
#ifdef __APPLE__
#define VWP
#else
#define VWP "     "
#endif
	// Populate window
	int wx = 0, wy = 0, ww = w, wh = h;
#ifdef LARGE_INTERFACE
	int bar_h = 29;
	int btn_pad = 15;
	int sidebar_w = 280;
	int tab_h = OS_FONT + 10;
#else
	int bar_h = 23;
	int btn_pad = 12;
	int sidebar_w = 230;
	int tab_h = OS_FONT + 4;
#endif
	int wgt_h = bar_h - 1, txt_h = bar_h - 2;
	int tbarsmall_h = 26, tbarlarge_h = 34;
	int tbtnsmall_h = tbarsmall_h - 2, tbtnlarge_h = tbarlarge_h - 2;
	int wgt_w = 0, txt_w = 0;
	// Populate menu bar
#ifdef __APPLE__
	Fl_Mac_App_Menu::about = "About " PROGRAM_NAME;
	Fl_Mac_App_Menu::hide = "Hide " PROGRAM_NAME;
	Fl_Mac_App_Menu::quit = "Quit " PROGRAM_NAME;
	_menu_bar = new Fl_Sys_Menu_Bar(wx, wy, ww, 0);
#else
	_menu_bar = new Fl_Sys_Menu_Bar(wx, wy, ww, txt_h);
#endif
	wy += _menu_bar->h();
	wh -= _menu_bar->h();
	// Populate toolbar
	_toolbar = new Toolbar(wx, wy, ww, tbarsmall_h);
	_open_model_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_open_and_load_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_load_firing_spikes_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_load_voltages_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_load_weights_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_load_prunings_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_export_image_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_undo_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_redo_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_repeat_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_copy_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_paste_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_reset_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_axon_conns_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_den_conns_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_gap_junctions_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_neur_fields_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_conn_fields_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_syn_dots_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_to_axon_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_to_via_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_to_synapse_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_to_den_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_allow_letters_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_only_show_selected_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_only_conn_selected_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_only_show_marked_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_only_show_clipped_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_orthographic_tb = new Toolbar_Toggle_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_select_tb = new Toolbar_Radio_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_clip_tb = new Toolbar_Radio_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_rotate_tb = new Toolbar_Radio_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_rotate_mode_tb = new Toolbar_Dropdown_Button(0, 0, 9, tbtnsmall_h);
	_pan_tb = new Toolbar_Radio_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_zoom_tb = new Toolbar_Radio_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_mark_tb = new Toolbar_Radio_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_snap_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	new Fl_Box(0, 0, 2, 0); new Spacer(0, 0, 2, 0); new Fl_Box(0, 0, 2, 0);
	_reset_settings_tb = new Toolbar_Button(0, 0, tbtnsmall_h, tbtnsmall_h);
	_toolbar->end();
	wy += _toolbar->h();
	wh -= _toolbar->h();
	begin();
	// Populate status bar
	_status_bar = new Toolbar(wx, h-bar_h, ww, bar_h);
	_soma_count = new Status_Bar_Field(0, 0, text_width("999,999,999 somas", 3), 0, "No somas");
	new Spacer(0, 0, 2, 0);
	_synapse_count = new Status_Bar_Field(0, 0, text_width("99,999,999,999 synapses", 3), 0, "No synapses");
	_report_synapses = new Toolbar_Button(0, 0, txt_h, txt_h);
	new Spacer(0, 0, 2, 0);
	_marked_count = new Status_Bar_Field(0, 0, text_width("99,999,999,999 marked", 3), 0, "None marked");
	_report_marked = new Toolbar_Button(0, 0, txt_h, txt_h);
	new Spacer(0, 0, 2, 0);
	_selected_count = new Status_Bar_Field(0, 0, text_width("None selected", 3), 0, "None selected");
	_report_selected = new Toolbar_Button(0, 0, txt_h, txt_h);
	_prev_selected = new Toolbar_Button(0, 0, txt_h, txt_h);
	_next_selected = new Toolbar_Button(0, 0, txt_h, txt_h);
	_deselect_shown = new Toolbar_Button(0, 0, txt_h, txt_h);
	_pivot_shown = new Toolbar_Button(0, 0, txt_h, txt_h);
	_summary = new Toolbar_Button(0, 0, txt_h, txt_h);
	new Spacer(0, 0, 2, 0);
	_selected_type = new Status_Bar_Field(0, 0, text_width("Outer stellate +x (S) #999,999,999", 3), 0, "");
	new Spacer(0, 0, 2, 0);
	_selected_coords = new Status_Bar_Field(0, 0, text_width("(-999999, -999999, -999999)", 3), 0, "");
	new Spacer(0, 0, 2, 0);
	_selected_hertz = new Status_Bar_Field(0, 0, text_width("9,999 Hz", 3), 0, "");
	new Spacer(0, 0, 2, 0);
	_selected_voltage = new Status_Bar_Field(0, 0, text_width("9,999.999 mV", 3), 0, "");
	_status_bar->end();
	wh -= _status_bar->h();
	begin();
	// Populate sidebar
	_sidebar = new Sidebar(wx, wy, sidebar_w, wh);
	int sx = _sidebar->x() + 5, sy = _sidebar->y() + 5, sw = _sidebar->w() - 10;
	_overview_heading = new Expander_Collapser(sx, sy, sw, txt_h, "Overview area:");
	sy += _overview_heading->h() + 5;
	_overview_area = new Overview_Area(sx, sy, sw, sw);
	sy += _overview_area->h() + 5;
	txt_w = text_width("Zoom:", 3);
	_overview_zoom = new OS_Slider(sx+txt_w, sy, sw-txt_w, wgt_h, "Zoom:");
	sy += _overview_zoom->h() + 10;
	_sidebar_spacer = new Spacer(sx, sy, sw, 2);
	sy += _sidebar_spacer->h() + 10;
	int th = MAX(MAX(55+wgt_h*4+txt_h*4, 59+wgt_h*6+txt_h*2), 39+wgt_h*4+txt_h*2);
	_sidebar_tabs = new OS_Tabs(sx, sy, sw, th+tab_h);
	sy += tab_h;
	_config_tab = new OS_Tab(sx, sy, sw, th, "Configure");
	_config_tab->end();
	_selection_tab = new OS_Tab(sx, sy, sw, th, "Select");
	_selection_tab->end();
	_marking_tab = new OS_Tab(sx, sy, sw, th, "Mark");
	_marking_tab->end();
	// Populate sidebar's configuration tab
	_config_tab->begin();
	int stx = sx + 7, sty = sy + 7, stw = sw - 14;
	txt_w = text_width("Soma letter size:", 3);
	_soma_letter_size_slider = new Default_Value_Slider(stx+txt_w, sty, stw-txt_w, wgt_h, text_width("16", 3), "Soma letter size:");
	sty += _soma_letter_size_slider->h() + 5;
	txt_w = text_width("Type:", 3);
	_type_choice = new Dropdown(stx+txt_w, sty, stw-txt_w, wgt_h, "Type:");
	sty += _type_choice->h() + 10;
	// Populate sidebar's configuration tab's type group
	_type_group = new Control_Group(stx, sty, stw, 16+wgt_h+txt_h*4);
	int tgx = _type_group->x() + 7, tgy = _type_group->y() + 7, tgw = _type_group->w() - 14;
	txt_w = text_width("Color:", 3);
	_type_color_choice = new Dropdown(tgx+txt_w, tgy, tgw-txt_w-wgt_h-5, wgt_h, "Color:");
	_type_color_swatch = new Fl_Box(tgx+tgw-wgt_h, tgy, wgt_h, wgt_h);
	tgy += _type_color_choice->h() + 2;
	wgt_w = txt_h - 4;
	_type_is_letter = new OS_Radio_Button(tgx, tgy, tgw-wgt_w-5, txt_h, "Letters for somas");
	_all_letter = new Link_Button(tgx+tgw-wgt_w, tgy+2, wgt_w, wgt_w);
	tgy += _type_is_letter->h();
	_type_is_dot = new OS_Radio_Button(tgx, tgy, tgw-wgt_w-5, txt_h, "Dots for somas");
	_all_dot = new Link_Button(tgx+tgw-wgt_w, tgy+2, wgt_w, wgt_w);
	tgy += _type_is_dot->h();
	_type_is_hidden = new OS_Radio_Button(tgx, tgy, tgw-wgt_w-5, txt_h, "Hide somas");
	_all_hidden = new Link_Button(tgx+tgw-wgt_w, tgy+2, wgt_w, wgt_w);
	tgy += _type_is_hidden->h();
	_type_is_disabled = new OS_Radio_Button(tgx, tgy, tgw-wgt_w-5, txt_h, "Disable somas");
	_all_disabled = new Link_Button(tgx+tgw-wgt_w, tgy+2, wgt_w, wgt_w);
	_type_group->end();
	sty += _type_group->h() + 10;
	_config_tab->begin();
	wgt_w = (stw - 20) / 3;
	_load_config = new OS_Button(stx, sty, wgt_w, wgt_h, "Load");
	stx += _load_config->w() + 10;
	_save_config = new OS_Button(stx, sty, stw-20-wgt_w*2, wgt_h, "Save");
	stx += _save_config->w() + 10;
	_reset_config = new OS_Button(stx, sty, wgt_w, wgt_h, "Reset");
	_config_tab->end();
	// Populate sidebar's selection tab
	_selection_tab->begin();
	stx = sx + 7, sty = sy + 7, stw = sw - 14;
	Label *heading1 = new Label(stx, sty, stw, txt_h, "Select by ID:");
	sty += heading1->h() + 3;
	txt_w = text_width("Soma ID:", 3);
	wgt_w = MIN(text_width("4294967295", 2) + wgt_h, stw-txt_w-wgt_h-5);
	_select_id_spinner = new OS_Spinner(stx+txt_w, sty, wgt_w, wgt_h, "Soma ID:");
	_fetch_select_id = new OS_Button(_select_id_spinner->x()+_select_id_spinner->w()+5, sty, wgt_h, wgt_h);
	sty += _select_id_spinner->h() + 5;
	wgt_w = text_width("Select", btn_pad);
	_select_id = new OS_Button(stx, sty, wgt_w, wgt_h, "Select");
	wgt_w = text_width("Deselect", btn_pad);
	_deselect_id = new OS_Button(_select_id->x()+_select_id->w()+10, sty, wgt_w, wgt_h, "Deselect");
	sty += _select_id->h() + 10;
	Spacer *hspacer2 = new Spacer(stx, sty, stw, 2);
	sty += hspacer2->h() + 7;
	Label *heading2 = new Label(stx, sty, stw, txt_h, "Select by synapse count:");
	sty += heading2->h() + 3;
	txt_w = text_width("Type:", 3);
	_select_type_choice = new Dropdown(stx+txt_w, sty, stw-txt_w, wgt_h, "Type:");
	sty += _select_type_choice->h() + 5;
	txt_w = text_width("Synapse count:", 3);
	wgt_w = MIN(text_width("10000", 2) + wgt_h, stw-txt_w-wgt_h-5);
	_syn_count_spinner = new OS_Spinner(stx+txt_w, sty, wgt_w, wgt_h, "Synapse count:");
	sty += _syn_count_spinner->h() + 5;
	txt_w = text_width("Synapse kind:", 3);
	wgt_w = MIN(MAX(text_width("Axonal", 3), text_width("Dendritic", 3)) + wgt_h, stw-txt_w-wgt_h-5);
	_syn_kind = new Toggle_Switch(stx+txt_w, sty, wgt_w, wgt_h, "Synapse kind:");
	sty += _syn_kind->h() + 5;
	wgt_w = text_width("Select", btn_pad);
	_select_syn_count = new OS_Button(stx, sty, wgt_w, wgt_h, "Select");
	wgt_w = text_width("Report", btn_pad);
	_report_syn_count = new OS_Button(_select_syn_count->x()+_select_syn_count->w()+10, sty, wgt_w, wgt_h, "Report");
	_selection_tab->end();
	// Populate sidebar's marking tab
	_marking_tab->begin();
	stx = sx + 7, sty = sy + 7, stw = sw - 14;
	Label *heading3 = new Label(stx, sty, stw, txt_h, "Connecting synapse paths:");
	sty += heading3->h() + 5;
	txt_w = MAX(text_width("Axonal ID:", 3), text_width("Dendritic ID:", 3));
	wgt_w = MIN(text_width("4294967295", 2) + wgt_h, stw-txt_w-wgt_h-5);
	_axon_id_spinner = new OS_Spinner(stx+txt_w, sty, wgt_w, wgt_h, "Axonal ID:");
	_fetch_axon_id = new OS_Button(_axon_id_spinner->x()+_axon_id_spinner->w()+5, sty, wgt_h, wgt_h);
	sty += _axon_id_spinner->h() + 5;
	txt_w = MAX(text_width("Axonal ID:", 3), text_width("Dendritic ID:", 3));
	wgt_w = MIN(text_width("4294967295", 2) + wgt_h, stw-txt_w-wgt_h-5);
	_den_id_spinner = new OS_Spinner(stx+txt_w, sty, wgt_w, wgt_h, "Dendritic ID:");
	_fetch_den_id = new OS_Button(_axon_id_spinner->x()+_axon_id_spinner->w()+5, sty, wgt_h, wgt_h);
	sty += _den_id_spinner->h() + 5;
	txt_w = text_width("Limit path count:", 3);
	wgt_w = MIN(text_width("10000", 2) + wgt_h, stw-txt_w-wgt_h-5);
	_limit_paths_spinner = new OS_Spinner(stx+txt_w, sty, wgt_w, wgt_h, "Limit path count:");
	sty += _limit_paths_spinner->h() + 5;
	_include_disabled = new OS_Check_Button(stx, sty, stw, txt_h, "Include disabled somas");
	sty += _include_disabled->h() + 5;
	wgt_w = (stw - 20) / 3;
	_mark_conn_paths = new OS_Button(stx, sty, wgt_w, wgt_h, "Mark");
	stx += _load_config->w() + 10;
	_select_conn_paths = new OS_Button(stx, sty, stw-20-wgt_w*2, wgt_h, "Select");
	stx += _save_config->w() + 10;
	_report_conn_paths = new OS_Button(stx, sty, wgt_w, wgt_h, "Report");
	_marking_tab->end();
	_sidebar->end();
	wx += _sidebar->w();
	ww -= _sidebar->w();
	begin();
	// Populate model area
	_model_area = new Model_Area(wx, wy, ww, wh);
	end();
	// Populate simulation bar
	_simulation_bar = new Panel(wx, _model_area->y() + _model_area->h(), ww, wgt_h*3+14);
	int fx = _simulation_bar->x() + 5, fy = _simulation_bar->y() + 5, fw = _simulation_bar->w() - 10;
	// Populate simulation bar's first row
	Fl_Group *row1 = new Fl_Group(fx, fy, fw, wgt_h);
	wgt_w = text_width("Show color scale") + txt_h;
	_show_color_scale = new OS_Check_Button(fx+fw-wgt_w, fy, wgt_w, txt_h, "Show color scale");
	fw -= _show_color_scale->w();
	wgt_w = text_width("Show inactive somas") + txt_h;
	_show_inactive_somas = new OS_Check_Button(fx+fw-wgt_w, fy, wgt_w, txt_h, "Show inactive somas");
	fw -= _show_inactive_somas->w();
	wgt_w = text_width("Display value for somas") + txt_h;
	_display_value_for_somas = new OS_Check_Button(fx+fw-wgt_w, fy, wgt_w, txt_h, "Display value for somas");
	fw -= _display_value_for_somas->w();
	wgt_w = text_width("Display:", 3);
	_display_heading = new Label(fx, fy, wgt_w, txt_h, "Display:");
	fx += _display_heading->w();
	fw -= _display_heading->w();
	wgt_w = text_width("Static model") + txt_h;
	_display_static_model = new OS_Radio_Button(fx, fy, wgt_w, txt_h, "Static model");
	fx += _display_static_model->w();
	fw -= _display_static_model->w();
	wgt_w = text_width("Firing spikes") + txt_h;
	_display_firing_spikes = new OS_Radio_Button(fx, fy, wgt_w, txt_h, "Firing spikes");
	fx += _display_firing_spikes->w();
	fw -= _display_firing_spikes->w();
	wgt_w = text_width("Voltages") + txt_h;
	_display_voltages = new OS_Radio_Button(fx, fy, wgt_w, txt_h, "Voltages");
	fx += _display_voltages->w();
	fw -= _display_voltages->w();
	wgt_w = text_width("Weights") + txt_h;
	_display_weights = new OS_Radio_Button(fx, fy, wgt_w, txt_h, "Weights");
	fx += _display_weights->w();
	fw -= _display_weights->w();
	Fl_Box *row1_spacer = new Fl_Box(fx+1, fy, 0, txt_h);
	row1->end();
	row1->clip_children(1);
	row1->resizable(row1_spacer);
	fx = _simulation_bar->x() + 5;
	fy += row1->h() + 2;
	fw = _simulation_bar->w() - 10;
	_simulation_bar->begin();
	// Populate simulation bar's second row
	Fl_Group *row2 = new Fl_Group(fx, fy, fw, wgt_h);
	_display_choice_heading = new Label(fx, fy, fw, txt_h, "File: None");
	_display_wizard = new Fl_Wizard(fx, fy, fw, wgt_h);
	// Populate simulation bar's second row's static model display controls
	_display_static_group = new Fl_Group(fx, fy, fw, wgt_h);
	int gw = fw;
	wgt_w = text_width("999,999X x 999,999Y x 999,999Z");
	_static_size = new Label(fx+gw-wgt_w, fy, wgt_w, txt_h, "0Xx0Yx0Z");
	gw -= _static_size->w() + 1;
	Fl_Box *display_static_spacer = new Fl_Box(fx+gw-1, fy, 1, 1);
	_display_static_group->end();
	_display_static_group->resizable(display_static_spacer);
	_display_wizard->begin();
	// Populate simulation bar's second row's firing spikes display controls
	_display_firing_group = new Fl_Group(fx, fy, fw, wgt_h);
	gw = fw;
	wgt_w = text_width("999", 2) + wgt_h;
	_firing_select_top_spinner = new OS_Spinner(fx+gw-wgt_w, fy, wgt_w, wgt_h);
	gw -= _firing_select_top_spinner->w() + 5;
	wgt_w = text_width("Select Most Frequent:", btn_pad);
	_firing_select_top = new OS_Button(fx+gw-wgt_w, fy, wgt_w, wgt_h, "Select Most Frequent:");
	gw -= _firing_select_top->w() + 5;
	Spacer *firing_spikes_spacer1 = new Spacer(fx+gw-2, fy, 2, wgt_h);
	gw -= firing_spikes_spacer1->w() + 5;
	wgt_w = text_width("Average", btn_pad);
	_firing_report_average = new OS_Button(fx+gw-wgt_w, fy, wgt_w, wgt_h, "Average");
	gw -= _firing_report_average->w() + 5;
	wgt_w = text_width("Current", btn_pad);
	_firing_report_current = new OS_Button(fx+gw-wgt_w, fy, wgt_w, wgt_h, "Current");
	gw -= _firing_report_current->w() + 1;
	wgt_w = text_width("Report frequencies:", 3);
	Label *report_frequencies_label = new Label(fx+gw-wgt_w, fy, wgt_w, txt_h, "Report frequencies:");
	gw -= report_frequencies_label->w() + 1;
	Spacer *firing_spikes_spacer2 = new Spacer(fx+gw-2, fy, 2, wgt_h);
	gw -= firing_spikes_spacer2->w() + 5;
	wgt_w = text_width("999,999 cycles; 99.9 ms/cycle");
	_firing_cycle_length = new Label(fx+gw-wgt_w, fy, wgt_w, txt_h, "No cycles");
	gw -= _firing_cycle_length->w() + 1;
	Fl_Box *display_firing_spacer = new Fl_Box(fx+gw-1, fy, 1, 1);
	_display_firing_group->end();
	_display_firing_group->resizable(display_firing_spacer);
	_display_wizard->begin();
	// Populate simulation bar's second row's voltages display controls
	_display_voltages_group = new Fl_Group(fx, fy, fw, wgt_h);
	gw = fw;
	wgt_w = text_width("Graph Selected Voltage", btn_pad);
	_voltages_graph_selected = new OS_Button(fx+gw-wgt_w, fy, wgt_w, wgt_h, "Graph Selected Voltage");
	gw -= _voltages_graph_selected->w() + 5;
	Spacer *voltages_spacer = new Spacer(fx+gw-2, fy, 2, wgt_h);
	gw -= voltages_spacer->w() + 5;
	wgt_w = text_width("9,999 recorded somas");
	_voltages_count = new Label(fx+gw-wgt_w, fy, wgt_w, txt_h, "No recorded somas");
	gw -= _voltages_count->w() + 1;
	Fl_Box *display_voltages_spacer = new Fl_Box(fx+gw-1, fy, 1, 1);
	_display_voltages_group->end();
	_display_voltages_group->resizable(display_voltages_spacer);
	_display_wizard->begin();
	// Populate simulation bar's second row's weights display controls
	_display_weights_group = new Fl_Group(fx, fy, fw, wgt_h);
	gw = fw;
	wgt_w = wgt_h * 3;
	_weights_scale_spread_slider = new Default_Slider(fx+gw-wgt_w, fy, wgt_w, wgt_h);
	gw -= _weights_scale_spread_slider->w() + 5;
	wgt_w = text_width("-9.9", 2) + wgt_h;
	_weights_scale_spread_spinner = new Default_Spinner(fx+gw-wgt_w, fy, wgt_w, wgt_h, "Spread:");
	gw -= _weights_scale_spread_spinner->w() + text_width("Spread:", 1) + 5;
	wgt_w = wgt_h * 3;
	_weights_scale_center_slider = new Default_Slider(fx+gw-wgt_w, fy, wgt_w, wgt_h);
	gw -= _weights_scale_spread_slider->w() + 5;
	wgt_w = text_width("-9.9", 2) + wgt_h;
	_weights_scale_center_spinner = new Default_Spinner(fx+gw-wgt_w, fy, wgt_w, wgt_h, "Center:");
	gw -= _weights_scale_center_spinner->w() + text_width("Center:", 1) + 5;
	Spacer *weights_spacer1 = new Spacer(fx+gw-2, fy, 2, wgt_h);
	gw -= weights_spacer1->w() + 5;
	wgt_w = MAX(text_width("Pre-change", 3), text_width("Post-change", 3)) + wgt_h;
	_weights_color_after = new Toggle_Switch(fx+gw-wgt_w, fy, wgt_w, wgt_h, "Color:");
	gw -= _weights_color_after->w() + text_width("Color:", 1) + 5;
	Spacer *weights_spacer2 = new Spacer(fx+gw-2, fy, 2, wgt_h);
	gw -= weights_spacer2->w() + 5;
	_weights_next_selected = new OS_Repeat_Button(fx+gw-wgt_h, fy, wgt_h, wgt_h);
	gw -= _weights_next_selected->w() + 5;
	_weights_prev_selected = new OS_Repeat_Button(fx+gw-wgt_h, fy, wgt_h, wgt_h);
	gw -= _weights_prev_selected->w() + 5;
	_weights_next_change = new OS_Repeat_Button(fx+gw-wgt_h, fy, wgt_h, wgt_h);
	gw -= _weights_next_change->w() + 5;
	_weights_prev_change = new OS_Repeat_Button(fx+gw-wgt_h, fy, wgt_h, wgt_h);
	gw -= _weights_prev_change->w() + 5;
	Fl_Box *display_weights_spacer = new Fl_Box(fx+gw-1, fy, 1, 1);
	_display_weights_group->end();
	_display_weights_group->resizable(display_weights_spacer);
	_display_wizard->end();
	row2->clip_children(1);
	row2->resizable(_display_wizard);
	fx = _simulation_bar->x() + 5;
	fy += row2->h() + 2;
	fw = _simulation_bar->w() - 10;
	_simulation_bar->begin();
	// Populate simulation bar's third row
	wgt_w = text_width("Speed:", 3);
	_firing_speed_spinner = new OS_Spinner(fx+wgt_w, fy, text_width("999")+wgt_h, wgt_h, "Speed:");
	fx += wgt_w + _firing_speed_spinner->w() + 5;
	fw -= wgt_w + _firing_speed_spinner->w() + 5;
	_play_pause_firing = new OS_Button(fx, fy, wgt_h, wgt_h);
	fx += _play_pause_firing->w() + 5;
	fw -= _play_pause_firing->w() + 5;
	_step_firing = new OS_Repeat_Button(fx, fy, wgt_h, wgt_h);
	fx += _step_firing->w() + 5;
	fw -= _step_firing->w() + 5;
	Spacer *row3_spacer2 = new Spacer(fx, fy, 2, wgt_h);
	fx += row3_spacer2->w() + 5;
	fw -= row3_spacer2->w() + 5;
	wgt_w = text_width("Cycle:", 1);
	_firing_time_spinner = new OS_Spinner(fx+wgt_w, fy, text_width("999999")+wgt_h, wgt_h, "Cycle:");
	fx += wgt_w + _firing_time_spinner->w() + 5;
	fw -= wgt_w + _firing_time_spinner->w() + 5;
	_firing_time_slider = new OS_Slider(fx, fy, fw, wgt_h);
	_simulation_bar->end();
	// Initialize drag-and-drop receiver
	begin();
	_dnd_receiver = new DnD_Receiver(0, 0, 0, 0);
	end();
	_dnd_receiver->callback((Fl_Callback *)drag_and_drop_cb);
	_dnd_receiver->user_data(this);
	// Keep model area above drag-and-drop receiver so enter/leave events work properly
	remove(_model_area);
	add(_model_area);
	// Initialize other windows and dialogs
	_model_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_firing_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_voltages_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_weights_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_prunings_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_image_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	_animation_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY);
	_state_load_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_state_save_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	_config_load_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_config_save_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	_text_report_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	_help_window = new Help_Window(24, 24, 640, 480, PROGRAM_NAME " Manual");
	_about_dialog = new Modal_Dialog(this, "About " PROGRAM_NAME, Modal_Dialog::PROGRAM_ICON);
	_success_dialog = new Modal_Dialog(this, "Success", Modal_Dialog::SUCCESS_ICON);
	_warning_dialog = new Modal_Dialog(this, "Warning", Modal_Dialog::WARNING_ICON);
	_error_dialog = new Modal_Dialog(this, "Error", Modal_Dialog::ERROR_ICON);
	_progress_dialog = new Progress_Dialog(this, "Progress...");
	_waiting_dialog = new Waiting_Dialog(this, "Progress...");
	_report_somas_dialog = new Report_Somas_Dialog("Report Selected Somas");
	_report_averages_dialog = new Report_Averages_Dialog("Report Average Frequencies");
	_summary_dialog = new Summary_Dialog("Summary");
	// Initialize window
	resizable(_model_area);
#ifdef LARGE_INTERFACE
	size_range(511, 372);
#else
	size_range(466, 326);
#endif
	callback(exit_cb);
#if defined(_WIN32)
	icon((const void *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON1)));
#elif defined(__LINUX__)
	fl_open_display();
	XpmCreatePixmapFromData(fl_display, DefaultRootWindow(fl_display), (char **)&VIZ_16_XPM, &_icon_pixmap,
		&_icon_mask, NULL);
	icon((const void *)_icon_pixmap);
#endif
	// Initialize menu bar
	_menu_bar->box(OS_PANEL_THIN_UP_BOX);
	_menu_bar->down_box(FL_FLAT_BOX);
	const Draw_Options &opts = _model_area->draw_options();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers" // Disable "missing-field-initializers" warning due to {} literal
#pragma GCC diagnostic ignored "-Wconversion-null" // Disable "conversion-null" warning
	Fl_Menu_Item menu_items[] = {
		// label, shortcut, callback, data, flags, labeltype, font, size, color
		{"&File", 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
			{"&Open Model..." VWP, FL_COMMAND + 'o', (Fl_Callback *)open_model_cb, this, 0, VW_MENU_STYLE},
			{"&Close Model" VWP, FL_COMMAND + 'w', (Fl_Callback *)close_model_cb, this, 0, VW_MENU_STYLE},
			{"Ope&n and Load All..." VWP, FL_COMMAND + FL_SHIFT + 'o', (Fl_Callback *)open_and_load_all_cb, this,
				FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Load &Firing Spikes..." VWP, FL_COMMAND + 'f', (Fl_Callback *)load_firing_spikes_cb, this, 0, VW_MENU_STYLE},
			{"&Unload Firing Spikes" VWP, 0, (Fl_Callback *)unload_firing_spikes_cb, this, FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Load &Voltages..." VWP, FL_COMMAND + 't', (Fl_Callback *)load_voltages_cb, this, 0, VW_MENU_STYLE},
			{"Unloa&d Voltages" VWP, 0, (Fl_Callback *)unload_voltages_cb, this, FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Load &Weights..." VWP, FL_COMMAND + 'i', (Fl_Callback *)load_weights_cb, this, 0, VW_MENU_STYLE},
			{"Load &Prunings..." VWP, FL_COMMAND + 'u', (Fl_Callback *)load_prunings_cb, this, 0, VW_MENU_STYLE},
			{"Unload Wei&ghts" VWP, 0, (Fl_Callback *)unload_weights_cb, this, FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"&Report Selected Somas..." VWP, FL_SHIFT + '8', (Fl_Callback *)report_selected_cb, this, 0, VW_MENU_STYLE},
			{"Report Selected S&ynapses..." VWP, FL_SHIFT + '7', (Fl_Callback *)report_synapses_cb, this, 0, VW_MENU_STYLE},
			{"Report Mar&ked Synapses..." VWP, FL_SHIFT + '6', (Fl_Callback *)report_marked_cb, this, FL_MENU_DIVIDER,
				VW_MENU_STYLE},
			{"&Export Image..." VWP, FL_COMMAND + 'p', (Fl_Callback *)export_image_cb, this, 0, VW_MENU_STYLE},
#ifdef __APPLE__
			{"Export Ani&mation..." VWP, FL_COMMAND + FL_SHIFT + 'p', (Fl_Callback *)export_animation_cb, this, 0, VW_MENU_STYLE},
#else
			{"Export Ani&mation..." VWP, FL_COMMAND + FL_SHIFT + 'p', (Fl_Callback *)export_animation_cb, this,
				FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"E&xit" VWP, FL_ALT + FL_F + 4, (Fl_Callback *)exit_cb, this, 0, VW_MENU_STYLE},
#endif
			{},
		{"&Edit", 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
			{"&Undo" VWP, FL_COMMAND + 'z', (Fl_Callback *)undo_cb, this, 0, VW_MENU_STYLE},
			{"&Redo" VWP, FL_COMMAND + 'y', (Fl_Callback *)redo_cb, this, 0, VW_MENU_STYLE},
			{"Repea&t" VWP, FL_COMMAND + 'r', (Fl_Callback *)repeat_cb, this, FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"&Copy State" VWP, FL_COMMAND + 'c', (Fl_Callback *)copy_state_cb, this, 0, VW_MENU_STYLE},
			{"&Paste State" VWP, FL_COMMAND + 'v', (Fl_Callback *)paste_state_cb, this, 0, VW_MENU_STYLE},
			{"&Load State..." VWP, FL_F + 7, (Fl_Callback *)load_state_cb, this, 0, VW_MENU_STYLE},
			{"&Save State..." VWP, FL_F + 6, (Fl_Callback *)save_state_cb, this, 0, VW_MENU_STYLE},
			{"Rese&t State" VWP, FL_COMMAND + 'n', (Fl_Callback *)reset_state_cb, this, FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Loa&d Config..." VWP, FL_COMMAND + FL_F + 7, (Fl_Callback *)load_config_cb, this, 0, VW_MENU_STYLE},
			{"Sa&ve Config..." VWP, FL_COMMAND + FL_F + 6, (Fl_Callback *)save_config_cb, this, 0, VW_MENU_STYLE},
			{"Res&et Config..." VWP, FL_COMMAND + FL_SHIFT + 'n', (Fl_Callback *)reset_config_cb, this, 0, VW_MENU_STYLE},
			{},
		{"&View", 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
			{"T&heme" VWP, 0, NULL, NULL, FL_SUBMENU | FL_MENU_DIVIDER, VW_MENU_STYLE},
				{"&Classic (Windows 2000)", NULL, (Fl_Callback *)classic_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::CLASSIC ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&Aero (Windows 7)", NULL, (Fl_Callback *)aero_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::AERO ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&Metro (Windows 8)", NULL, (Fl_Callback *)metro_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::METRO ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"A&qua (Mac OS X Lion)", NULL, (Fl_Callback *)aqua_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::AQUA ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&Greybird (Linux GTK+)", NULL, (Fl_Callback *)greybird_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::GREYBIRD ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"Me&tal (Java Swing)", NULL, (Fl_Callback *)metal_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::METAL ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&Blue (Windows Calculator)", NULL, (Fl_Callback *)blue_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::BLUE ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&Dark (Adobe Photoshop CS6)", NULL, (Fl_Callback *)dark_theme_cb, this, FL_MENU_RADIO |
					(OS::current_theme() == OS::BLUE ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{},
			{"&Overview Area" VWP, FL_F + 3, (Fl_Callback *)overview_area_cb, this, FL_MENU_TOGGLE | FL_MENU_VALUE,
				VW_MENU_STYLE},
			{"&Toolbar" VWP, FL_COMMAND + '\\', (Fl_Callback *)toolbar_cb, this, FL_MENU_TOGGLE | FL_MENU_VALUE,
				VW_MENU_STYLE},
			{"&Sidebar" VWP, FL_F + 4, (Fl_Callback *)sidebar_cb, this, FL_MENU_TOGGLE | FL_MENU_VALUE, VW_MENU_STYLE},
			{"St&atus Bar" VWP, FL_COMMAND + '/', (Fl_Callback *)status_bar_cb, this,
				FL_MENU_TOGGLE | FL_MENU_VALUE | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"&Large Icons" VWP, FL_COMMAND + FL_SHIFT + '=', (Fl_Callback *)large_icon_cb, this,
				FL_MENU_TOGGLE | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Show 3D A&xes" VWP, 'k', (Fl_Callback *)show_axes_cb, this, FL_MENU_TOGGLE |
				(opts.show_axes() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Sho&w Axis Labels" VWP, 'j', (Fl_Callback *)show_axis_labels_cb, this, FL_MENU_TOGGLE |
				(opts.show_axis_labels() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Show &Bulletin" VWP, FL_SHIFT + '`', (Fl_Callback *)show_bulletin_cb, this, FL_MENU_TOGGLE |
				(opts.show_bulletin() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Show F&PS" VWP, FL_SHIFT + '3', (Fl_Callback *)show_fps_cb, this, FL_MENU_TOGGLE |
				(opts.show_fps() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Invert Background" VWP, FL_COMMAND + FL_SHIFT + 'i', (Fl_Callback *)invert_background_cb, this,
				FL_MENU_TOGGLE | (opts.invert_background() ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Su&mmary..." VWP, FL_SHIFT + '/', (Fl_Callback *)summary_cb, this, FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"T&ransparent" VWP, FL_COMMAND + FL_SHIFT + '0', (Fl_Callback *)transparent_cb, this, FL_MENU_TOGGLE,
				VW_MENU_STYLE},
			{"&Full Screen" VWP,
#ifdef __APPLE__
				// F11 toggles all open windows in Mac OS X
				FL_COMMAND + FL_SHIFT + 'f',
#else
				FL_F + 11,
#endif
				(Fl_Callback *)full_screen_cb, this, FL_MENU_TOGGLE, VW_MENU_STYLE},
			{},
		{"&Model", 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
			{"&Axonal Connections" VWP, 'a', (Fl_Callback *)axon_conns_cb, this, FL_MENU_TOGGLE |
				(opts.axon_conns() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Dendritic Connections" VWP, 'd', (Fl_Callback *)den_conns_cb, this, FL_MENU_TOGGLE |
				(opts.den_conns() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Gap Junctions" VWP, 'g', (Fl_Callback *)gap_junctions_cb, this, FL_MENU_TOGGLE |
				(opts.gap_junctions() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Neuritic &Fields" VWP, 'f', (Fl_Callback *)neur_fields_cb, this, FL_MENU_TOGGLE |
				(opts.neur_fields() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Connected Fields" VWP, 's', (Fl_Callback *)conn_fields_cb, this, FL_MENU_TOGGLE |
				(opts.conn_fields() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Sy&napse Dots" VWP, 'n', (Fl_Callback *)syn_dots_cb, this, FL_MENU_TOGGLE |
				(opts.syn_dots() ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"To A&xonal Soma" VWP, 'x', (Fl_Callback *)to_axon_cb, this, FL_MENU_TOGGLE |
				(opts.to_axon() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"To &Via Point" VWP, 'v', (Fl_Callback *)to_via_cb, this, FL_MENU_TOGGLE |
				(opts.to_via() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"To &Synapse" VWP, 'y', (Fl_Callback *)to_synapse_cb, this, FL_MENU_TOGGLE |
				(opts.to_synapse() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"To &Dendritic Soma" VWP, 'i', (Fl_Callback *)to_den_cb, this, FL_MENU_TOGGLE |
				(opts.to_den() ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Allow &Letters" VWP, 'l', (Fl_Callback *)allow_letters_cb, this, FL_MENU_TOGGLE |
				(opts.allow_letters() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Only Show S&elected" VWP, 'e', (Fl_Callback *)only_show_selected_cb, this, FL_MENU_TOGGLE |
				(opts.only_show_selected() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Only Connec&t Selected" VWP, 'r', (Fl_Callback *)only_conn_selected_cb, this, FL_MENU_TOGGLE |
				(opts.only_conn_selected() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Only Show &Marked" VWP, 'm', (Fl_Callback *)only_show_marked_cb, this, FL_MENU_TOGGLE |
				(opts.only_show_marked() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Only Show Cli&pped" VWP, 'u', (Fl_Callback *)only_show_clipped_cb, this, FL_MENU_TOGGLE |
				(opts.only_show_clipped() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Only Enable Cl&ipped" VWP, 'c', (Fl_Callback *)only_enable_clipped_cb, this, FL_MENU_TOGGLE |
				(opts.only_enable_clipped() ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Ort&hographic Projection" VWP, 'h', (Fl_Callback *)orthographic_cb, this, FL_MENU_TOGGLE |
				(opts.orthographic() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Left-Handed Coo&rdinates" VWP, 't', (Fl_Callback *)left_handed_cb, this, FL_MENU_TOGGLE |
				(opts.left_handed() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{},
		{"&Simulation", 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
			{"&Static Model" VWP, FL_ALT + 's', (Fl_Callback *)set_simulation_display_cb, this, FL_MENU_RADIO |
				(opts.display() == Draw_Options::STATIC_MODEL ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Firing Spikes" VWP, FL_ALT + 'f', (Fl_Callback *)set_simulation_display_cb, this, FL_MENU_RADIO |
				(opts.display() == Draw_Options::FIRING_SPIKES ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Voltages" VWP, FL_ALT + 'v', (Fl_Callback *)set_simulation_display_cb, this, FL_MENU_RADIO |
				(opts.display() == Draw_Options::VOLTAGES ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Weights" VWP, FL_ALT + 'w', (Fl_Callback *)set_simulation_display_cb, this, FL_MENU_RADIO |
				(opts.display() == Draw_Options::WEIGHTS ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"&Display Value For Somas" VWP, FL_SHIFT + '4', (Fl_Callback *)display_value_for_somas_cb, this, FL_MENU_TOGGLE |
				(opts.display_value_for_somas() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Show &Inactive Somas" VWP, FL_SHIFT + '1', (Fl_Callback *)show_inactive_somas_cb, this, FL_MENU_TOGGLE |
				(opts.show_inactive_somas() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Show &Color Scale" VWP, FL_SHIFT + '2', (Fl_Callback *)show_color_scale_cb, this, FL_MENU_TOGGLE |
				(opts.show_color_scale() ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"&Play/Pause" VWP, '.', (Fl_Callback *)play_pause_firing_cb, this, 0, VW_MENU_STYLE},
			{"S&tep" VWP, FL_SHIFT + '.', (Fl_Callback *)step_firing_cb, this, 0, VW_MENU_STYLE},
			{},
		{"&Tools", 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
			{"&Select Soma" VWP, '1', (Fl_Callback *)select_cb, this, FL_MENU_RADIO |
				(_model_area->action() == Model_Area::SELECT ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Clip" VWP, '2', (Fl_Callback *)clip_cb, this, FL_MENU_RADIO |
				(_model_area->action() == Model_Area::CLIP ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Rotate" VWP, '3', (Fl_Callback *)rotate_cb, this, FL_MENU_RADIO |
				(_model_area->action() == Model_Area::ROTATE ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Pan" VWP, '4', (Fl_Callback *)pan_cb, this, FL_MENU_RADIO |
				(_model_area->action() == Model_Area::PAN ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Zoom" VWP, '5', (Fl_Callback *)zoom_cb, this, FL_MENU_RADIO |
				(_model_area->action() == Model_Area::ZOOM ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"&Mark Synapse" VWP, '6', (Fl_Callback *)mark_cb, this, FL_MENU_RADIO |
				(_model_area->action() == Model_Area::MARK ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Snap to A&xes" VWP, FL_COMMAND + 'x', (Fl_Callback *)snap_cb, this, 0, VW_MENU_STYLE},
			{"Pi&vot Point" VWP, '\'', (Fl_Callback *)pivot_shown_cb, this, FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"Ro&tation Mode" VWP, 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
				{"2D &Arcball" VWP, FL_ALT + 'a', (Fl_Callback *)rotate_2d_arcball_cb, this, FL_MENU_RADIO |
					(_model_area->rotation_mode() == Model_Area::ARCBALL_2D ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"3D Arc&ball" VWP, FL_ALT + 'b', (Fl_Callback *)rotate_3d_arcball_cb, this, FL_MENU_RADIO |
					(_model_area->rotation_mode() == Model_Area::ARCBALL_3D ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&X-Axis" VWP, FL_ALT + 'x', (Fl_Callback *)rotate_x_axis_cb, this, FL_MENU_RADIO |
					(_model_area->rotation_mode() == Model_Area::AXIS_X ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&Y-Axis" VWP, FL_ALT + 'y', (Fl_Callback *)rotate_y_axis_cb, this, FL_MENU_RADIO |
					(_model_area->rotation_mode() == Model_Area::AXIS_Y ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{"&Z-Axis" VWP, FL_ALT + 'z', (Fl_Callback *)rotate_z_axis_cb, this, FL_MENU_RADIO |
					(_model_area->rotation_mode() == Model_Area::AXIS_Z ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
				{},
			{"Show Rotation &Guide" VWP, FL_COMMAND + 'g', (Fl_Callback *)show_rotation_guide_cb, this, FL_MENU_TOGGLE |
				(opts.show_rotation_guide() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"Sca&le Rotation" VWP, FL_COMMAND + FL_SHIFT + '\'', (Fl_Callback *)scale_rotation_cb, this, FL_MENU_TOGGLE |
				(_model_area->scale_rotation() ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
			{"In&vert Zoom" VWP, FL_COMMAND + FL_SHIFT + '-', (Fl_Callback *)invert_zoom_cb, this, FL_MENU_TOGGLE |
				(_model_area->invert_zoom() ? FL_MENU_VALUE : 0) | FL_MENU_DIVIDER, VW_MENU_STYLE},
			{"R&eset Settings" VWP, FL_COMMAND + '0', (Fl_Callback *)reset_settings_cb, this, 0, VW_MENU_STYLE},
			{},
		{"&Help", 0, NULL, NULL, FL_SUBMENU, VW_MENU_STYLE},
#ifdef __APPLE__
			{PROGRAM_NAME " &Help" VWP, FL_F + 1, (Fl_Callback *)help_cb, this, 0, VW_MENU_STYLE},
#else
			{"&Help" VWP, FL_F + 1, (Fl_Callback *)help_cb, this, 0, VW_MENU_STYLE},
			{"&About" VWP, FL_COMMAND + 'a', (Fl_Callback *)about_cb, this, 0, VW_MENU_STYLE},
#endif
			{},
		{}
	};
#pragma GCC diagnostic pop
#ifdef __APPLE__
	// Fix for menu items not working in Mac OS X
	Fl_Menu_Item *menu_items_copy = static_cast<Fl_Menu_Item *>(malloc(sizeof(menu_items)));
	memcpy(menu_items_copy, menu_items, sizeof(menu_items));
	_menu_bar->menu(menu_items_copy);
	// Initialize Mac OS X application menu
	fl_mac_set_about((Fl_Callback *)about_cb, this);
#else
	_menu_bar->copy(menu_items);
#endif
	// Initialize menu bar items
#define VW_FIND_MENU_ITEM_CB(C) (const_cast<Fl_Menu_Item *>(_menu_bar->find_item((Fl_Callback *)(C))))
#define VW_FIND_MENU_ITEM_NAME(N) (const_cast<Fl_Menu_Item *>(_menu_bar->find_item((N))))
	_classic_theme_mi = VW_FIND_MENU_ITEM_CB(classic_theme_cb);
	_aero_theme_mi = VW_FIND_MENU_ITEM_CB(aero_theme_cb);
	_metro_theme_mi = VW_FIND_MENU_ITEM_CB(metro_theme_cb);
	_aqua_theme_mi = VW_FIND_MENU_ITEM_CB(aqua_theme_cb);
	_greybird_theme_mi = VW_FIND_MENU_ITEM_CB(greybird_theme_cb);
	_metal_theme_mi = VW_FIND_MENU_ITEM_CB(metal_theme_cb);
	_blue_theme_mi = VW_FIND_MENU_ITEM_CB(blue_theme_cb);
	_dark_theme_mi = VW_FIND_MENU_ITEM_CB(dark_theme_cb);
	_overview_area_mi = VW_FIND_MENU_ITEM_CB(overview_area_cb);
	_large_icon_mi = VW_FIND_MENU_ITEM_CB(large_icon_cb);
	_axon_conns_mi = VW_FIND_MENU_ITEM_CB(axon_conns_cb);
	_den_conns_mi = VW_FIND_MENU_ITEM_CB(den_conns_cb);
	_gap_junctions_mi = VW_FIND_MENU_ITEM_CB(gap_junctions_cb);
	_neur_fields_mi = VW_FIND_MENU_ITEM_CB(neur_fields_cb);
	_syn_dots_mi = VW_FIND_MENU_ITEM_CB(syn_dots_cb);
	_conn_fields_mi = VW_FIND_MENU_ITEM_CB(conn_fields_cb);
	_to_axon_mi = VW_FIND_MENU_ITEM_CB(to_axon_cb);
	_to_via_mi = VW_FIND_MENU_ITEM_CB(to_via_cb);
	_to_synapse_mi = VW_FIND_MENU_ITEM_CB(to_synapse_cb);
	_to_den_mi = VW_FIND_MENU_ITEM_CB(to_den_cb);
	_allow_letters_mi = VW_FIND_MENU_ITEM_CB(allow_letters_cb);
	_only_show_selected_mi = VW_FIND_MENU_ITEM_CB(only_show_selected_cb);
	_only_conn_selected_mi = VW_FIND_MENU_ITEM_CB(only_conn_selected_cb);
	_only_show_marked_mi = VW_FIND_MENU_ITEM_CB(only_show_marked_cb);
	_only_show_clipped_mi = VW_FIND_MENU_ITEM_CB(only_show_clipped_cb);
	_orthographic_mi = VW_FIND_MENU_ITEM_CB(orthographic_cb);
	_display_static_model_mi = VW_FIND_MENU_ITEM_NAME("&Simulation/&Static Model" VWP);
	_display_firing_spikes_mi = VW_FIND_MENU_ITEM_NAME("&Simulation/&Firing Spikes" VWP);
	_display_voltages_mi = VW_FIND_MENU_ITEM_NAME("&Simulation/&Voltages" VWP);
	_display_weights_mi = VW_FIND_MENU_ITEM_NAME("&Simulation/&Weights" VWP);
	_display_value_for_somas_mi = VW_FIND_MENU_ITEM_CB(display_value_for_somas_cb);
	_show_inactive_somas_mi = VW_FIND_MENU_ITEM_CB(show_inactive_somas_cb);
	_show_color_scale_mi = VW_FIND_MENU_ITEM_CB(show_color_scale_cb);
	_select_mi = VW_FIND_MENU_ITEM_CB(select_cb);
	_clip_mi = VW_FIND_MENU_ITEM_CB(clip_cb);
	_rotate_mi = VW_FIND_MENU_ITEM_CB(rotate_cb);
	_pan_mi = VW_FIND_MENU_ITEM_CB(pan_cb);
	_zoom_mi = VW_FIND_MENU_ITEM_CB(zoom_cb);
	_mark_mi = VW_FIND_MENU_ITEM_CB(mark_cb);
	_2d_arcball_mi = VW_FIND_MENU_ITEM_CB(rotate_2d_arcball_cb);
	_3d_arcball_mi = VW_FIND_MENU_ITEM_CB(rotate_3d_arcball_cb);
	_x_axis_mi = VW_FIND_MENU_ITEM_CB(rotate_x_axis_cb);
	_y_axis_mi = VW_FIND_MENU_ITEM_CB(rotate_y_axis_cb);
	_z_axis_mi = VW_FIND_MENU_ITEM_CB(rotate_z_axis_cb);
#undef VW_FIND_MENU_ITEM_CB
#undef VW_FIND_MENU_ITEM_NAME
	// Initialize toolbar
	_toolbar->alt_h(tbarlarge_h);
	// Initialize toolbar buttons
	_open_model_tb->tooltip("Open Model... (" COMMAND_KEY_PLUS "O)");
	_open_model_tb->callback((Fl_Callback *)open_model_cb, this);
	_open_model_tb->image(OPEN_MODEL_SMALL_ICON);
	_open_model_tb->alt_image(OPEN_MODEL_LARGE_ICON);
	_open_model_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_open_model_tb->take_focus();
	_open_and_load_tb->tooltip("Open and Load All... (" COMMAND_SHIFT_KEYS_PLUS "O)");
	_open_and_load_tb->callback((Fl_Callback *)open_and_load_all_cb, this);
	_open_and_load_tb->image(OPEN_AND_LOAD_SMALL_ICON);
	_open_and_load_tb->alt_image(OPEN_AND_LOAD_LARGE_ICON);
	_open_and_load_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_load_firing_spikes_tb->tooltip("Load Firing Spikes... (" COMMAND_KEY_PLUS "F)");
	_load_firing_spikes_tb->callback((Fl_Callback *)load_firing_spikes_cb, this);
	_load_firing_spikes_tb->image(LOAD_FIRING_SPIKES_SMALL_ICON);
	_load_firing_spikes_tb->deimage(LOAD_FIRING_SPIKES_DISABLED_SMALL_ICON);
	_load_firing_spikes_tb->alt_image(LOAD_FIRING_SPIKES_LARGE_ICON);
	_load_firing_spikes_tb->alt_deimage(LOAD_FIRING_SPIKES_DISABLED_LARGE_ICON);
	_load_firing_spikes_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_load_voltages_tb->tooltip("Load Voltages... (" COMMAND_KEY_PLUS "T)");
	_load_voltages_tb->callback((Fl_Callback *)load_voltages_cb, this);
	_load_voltages_tb->image(LOAD_VOLTAGES_SMALL_ICON);
	_load_voltages_tb->deimage(LOAD_VOLTAGES_DISABLED_SMALL_ICON);
	_load_voltages_tb->alt_image(LOAD_VOLTAGES_LARGE_ICON);
	_load_voltages_tb->alt_deimage(LOAD_VOLTAGES_DISABLED_LARGE_ICON);
	_load_voltages_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_load_weights_tb->tooltip("Load Weights... (" COMMAND_KEY_PLUS "I)");
	_load_weights_tb->callback((Fl_Callback *)load_weights_cb, this);
	_load_weights_tb->image(LOAD_WEIGHTS_SMALL_ICON);
	_load_weights_tb->deimage(LOAD_WEIGHTS_DISABLED_SMALL_ICON);
	_load_weights_tb->alt_image(LOAD_WEIGHTS_LARGE_ICON);
	_load_weights_tb->alt_deimage(LOAD_WEIGHTS_DISABLED_LARGE_ICON);
	_load_weights_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_load_prunings_tb->tooltip("Load Prunings... (" COMMAND_KEY_PLUS "U)");
	_load_prunings_tb->callback((Fl_Callback *)load_prunings_cb, this);
	_load_prunings_tb->image(LOAD_PRUNINGS_SMALL_ICON);
	_load_prunings_tb->deimage(LOAD_PRUNINGS_DISABLED_SMALL_ICON);
	_load_prunings_tb->alt_image(LOAD_PRUNINGS_LARGE_ICON);
	_load_prunings_tb->alt_deimage(LOAD_PRUNINGS_DISABLED_LARGE_ICON);
	_load_prunings_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_export_image_tb->tooltip("Export Image... (" COMMAND_KEY_PLUS "P)");
	_export_image_tb->callback((Fl_Callback *)export_image_cb, this);
	_export_image_tb->image(EXPORT_IMAGE_SMALL_ICON);
	_export_image_tb->deimage(EXPORT_IMAGE_DISABLED_SMALL_ICON);
	_export_image_tb->alt_image(EXPORT_IMAGE_LARGE_ICON);
	_export_image_tb->alt_deimage(EXPORT_IMAGE_DISABLED_LARGE_ICON);
	_export_image_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_undo_tb->tooltip("Undo (" COMMAND_KEY_PLUS "Z)");
	_undo_tb->callback((Fl_Callback *)undo_cb, this);
	_undo_tb->image(UNDO_SMALL_ICON);
	_undo_tb->deimage(UNDO_DISABLED_SMALL_ICON);
	_undo_tb->alt_image(UNDO_LARGE_ICON);
	_undo_tb->alt_deimage(UNDO_DISABLED_LARGE_ICON);
	_undo_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_redo_tb->tooltip("Redo (" COMMAND_KEY_PLUS "Y)");
	_redo_tb->callback((Fl_Callback *)redo_cb, this);
	_redo_tb->image(REDO_SMALL_ICON);
	_redo_tb->deimage(REDO_DISABLED_SMALL_ICON);
	_redo_tb->alt_image(REDO_LARGE_ICON);
	_redo_tb->alt_deimage(REDO_DISABLED_LARGE_ICON);
	_redo_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_repeat_tb->tooltip("Repeat (" COMMAND_KEY_PLUS "R)");
	_repeat_tb->callback((Fl_Callback *)repeat_cb, this);
	_repeat_tb->image(REPEAT_SMALL_ICON);
	_repeat_tb->deimage(REPEAT_DISABLED_SMALL_ICON);
	_repeat_tb->alt_image(REPEAT_LARGE_ICON);
	_repeat_tb->alt_deimage(REPEAT_DISABLED_LARGE_ICON);
	_repeat_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_copy_tb->tooltip("Copy State (" COMMAND_KEY_PLUS "C)");
	_copy_tb->callback((Fl_Callback *)copy_state_cb, this);
	_copy_tb->image(COPY_SMALL_ICON);
	_copy_tb->deimage(COPY_DISABLED_SMALL_ICON);
	_copy_tb->alt_image(COPY_LARGE_ICON);
	_copy_tb->alt_deimage(COPY_DISABLED_LARGE_ICON);
	_copy_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_paste_tb->tooltip("Paste State (" COMMAND_KEY_PLUS "V)");
	_paste_tb->callback((Fl_Callback *)paste_state_cb, this);
	_paste_tb->image(PASTE_SMALL_ICON);
	_paste_tb->deimage(PASTE_DISABLED_SMALL_ICON);
	_paste_tb->alt_image(PASTE_LARGE_ICON);
	_paste_tb->alt_deimage(PASTE_DISABLED_LARGE_ICON);
	_paste_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_reset_tb->tooltip("Reset State (" COMMAND_KEY_PLUS "N)");
	_reset_tb->callback((Fl_Callback *)reset_state_cb, this);
	_reset_tb->image(RESET_SMALL_ICON);
	_reset_tb->deimage(RESET_DISABLED_SMALL_ICON);
	_reset_tb->alt_image(RESET_LARGE_ICON);
	_reset_tb->alt_deimage(RESET_DISABLED_LARGE_ICON);
	_reset_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_axon_conns_tb->tooltip("Axonal Connections (A)");
	_axon_conns_tb->callback((Fl_Callback *)axon_conns_tb_cb, this);
	_axon_conns_tb->value(opts.axon_conns() ? 1 : 0);
	_axon_conns_tb->image(AXON_CONNS_SMALL_ICON);
	_axon_conns_tb->alt_image(AXON_CONNS_LARGE_ICON);
	_axon_conns_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_den_conns_tb->tooltip("Dendritic Connections (D)");
	_den_conns_tb->callback((Fl_Callback *)den_conns_tb_cb, this);
	_den_conns_tb->value(opts.den_conns() ? 1 : 0);
	_den_conns_tb->image(DEN_CONNS_SMALL_ICON);
	_den_conns_tb->alt_image(DEN_CONNS_LARGE_ICON);
	_den_conns_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_gap_junctions_tb->tooltip("Gap Junctions (G)");
	_gap_junctions_tb->callback((Fl_Callback *)gap_junctions_tb_cb, this);
	_gap_junctions_tb->value(opts.gap_junctions() ? 1 : 0);
	_gap_junctions_tb->image(GAP_JUNCTIONS_SMALL_ICON);
	_gap_junctions_tb->alt_image(GAP_JUNCTIONS_LARGE_ICON);
	_gap_junctions_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_neur_fields_tb->tooltip("Neuritic Fields (F)");
	_neur_fields_tb->callback((Fl_Callback *)neur_fields_tb_cb, this);
	_neur_fields_tb->value(opts.neur_fields() ? 1 : 0);
	_neur_fields_tb->image(NEURITIC_FIELDS_SMALL_ICON);
	_neur_fields_tb->alt_image(NEURITIC_FIELDS_LARGE_ICON);
	_neur_fields_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_syn_dots_tb->tooltip("Synapse Dots (N)");
	_syn_dots_tb->callback((Fl_Callback *)syn_dots_tb_cb, this);
	_syn_dots_tb->value(opts.syn_dots() ? 1 : 0);
	_syn_dots_tb->image(SYN_DOTS_SMALL_ICON);
	_syn_dots_tb->alt_image(SYN_DOTS_LARGE_ICON);
	_syn_dots_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_conn_fields_tb->tooltip("Connected Fields (S)");
	_conn_fields_tb->callback((Fl_Callback *)conn_fields_tb_cb, this);
	_conn_fields_tb->value(opts.conn_fields() ? 1 : 0);
	_conn_fields_tb->image(CONN_FIELDS_SMALL_ICON);
	_conn_fields_tb->alt_image(CONN_FIELDS_LARGE_ICON);
	_conn_fields_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_to_axon_tb->tooltip("To Axonal Soma (X)");
	_to_axon_tb->callback((Fl_Callback *)to_axon_tb_cb, this);
	_to_axon_tb->value(opts.to_axon() ? 1 : 0);
	_to_axon_tb->image(TO_AXON_SMALL_ICON);
	_to_axon_tb->alt_image(TO_AXON_LARGE_ICON);
	_to_axon_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_to_via_tb->tooltip("To Via Point (V)");
	_to_via_tb->callback((Fl_Callback *)to_via_tb_cb, this);
	_to_via_tb->value(opts.to_via() ? 1 : 0);
	_to_via_tb->image(TO_VIA_SMALL_ICON);
	_to_via_tb->alt_image(TO_VIA_LARGE_ICON);
	_to_via_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_to_synapse_tb->tooltip("To Synapse (Y)");
	_to_synapse_tb->callback((Fl_Callback *)to_synapse_tb_cb, this);
	_to_synapse_tb->value(opts.to_synapse() ? 1 : 0);
	_to_synapse_tb->image(TO_SYNAPSE_SMALL_ICON);
	_to_synapse_tb->alt_image(TO_SYNAPSE_LARGE_ICON);
	_to_synapse_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_to_den_tb->tooltip("To Dendritic Soma (I)");
	_to_den_tb->callback((Fl_Callback *)to_den_tb_cb, this);
	_to_den_tb->value(opts.to_den() ? 1 : 0);
	_to_den_tb->image(TO_DEN_SMALL_ICON);
	_to_den_tb->alt_image(TO_DEN_LARGE_ICON);
	_to_den_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_allow_letters_tb->tooltip("Allow Letters (L)");
	_allow_letters_tb->callback((Fl_Callback *)allow_letters_tb_cb, this);
	_allow_letters_tb->value(opts.allow_letters() ? 1 : 0);
	_allow_letters_tb->image(ALLOW_LETTERS_SMALL_ICON);
	_allow_letters_tb->alt_image(ALLOW_LETTERS_LARGE_ICON);
	_allow_letters_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_only_show_selected_tb->tooltip("Only Show Selected (E)");
	_only_show_selected_tb->callback((Fl_Callback *)only_show_selected_tb_cb, this);
	_only_show_selected_tb->value(opts.only_show_selected() ? 1 : 0);
	_only_show_selected_tb->image(SHOW_SELECTED_SMALL_ICON);
	_only_show_selected_tb->alt_image(SHOW_SELECTED_LARGE_ICON);
	_only_show_selected_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_only_conn_selected_tb->tooltip("Only Connect Selected (R)");
	_only_conn_selected_tb->callback((Fl_Callback *)only_conn_selected_tb_cb, this);
	_only_conn_selected_tb->value(opts.only_conn_selected() ? 1 : 0);
	_only_conn_selected_tb->image(CONN_SELECTED_SMALL_ICON);
	_only_conn_selected_tb->alt_image(CONN_SELECTED_LARGE_ICON);
	_only_conn_selected_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_only_show_marked_tb->tooltip("Only Show Marked (M)");
	_only_show_marked_tb->callback((Fl_Callback *)only_show_marked_tb_cb, this);
	_only_show_marked_tb->value(opts.only_show_marked() ? 1 : 0);
	_only_show_marked_tb->image(SHOW_MARKED_SMALL_ICON);
	_only_show_marked_tb->alt_image(SHOW_MARKED_LARGE_ICON);
	_only_show_marked_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_only_show_clipped_tb->tooltip("Only Show Clipped (U)");
	_only_show_clipped_tb->callback((Fl_Callback *)only_show_clipped_tb_cb, this);
	_only_show_clipped_tb->value(opts.only_show_clipped() ? 1 : 0);
	_only_show_clipped_tb->image(SHOW_CLIPPED_SMALL_ICON);
	_only_show_clipped_tb->alt_image(SHOW_CLIPPED_LARGE_ICON);
	_only_show_clipped_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_orthographic_tb->tooltip("Orthographic Projection (H)");
	_orthographic_tb->callback((Fl_Callback *)orthographic_tb_cb, this);
	_orthographic_tb->value(opts.orthographic() ? 1 : 0);
	_orthographic_tb->image(ORTHOGRAPHIC_SMALL_ICON);
	_orthographic_tb->alt_image(ORTHOGRAPHIC_LARGE_ICON);
	_orthographic_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_select_tb->tooltip("Select Soma (1)");
	_select_tb->callback((Fl_Callback *)select_tb_cb, this);
	if (_model_area->action() == Model_Area::SELECT) { _select_tb->setonly(); }
	_select_tb->image(SELECT_SMALL_ICON);
	_select_tb->alt_image(SELECT_LARGE_ICON);
	_select_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_clip_tb->tooltip("Clip (2)");
	_clip_tb->callback((Fl_Callback *)clip_tb_cb, this);
	if (_model_area->action() == Model_Area::CLIP) { _clip_tb->setonly(); }
	_clip_tb->image(CLIP_SMALL_ICON);
	_clip_tb->alt_image(CLIP_LARGE_ICON);
	_clip_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_rotate_tb->tooltip("Rotate (3)");
	_rotate_tb->callback((Fl_Callback *)rotate_tb_cb, this);
	if (_model_area->action() == Model_Area::ROTATE) { _rotate_tb->setonly(); }
	_rotate_tb->image(ROTATE_SMALL_ICON);
	_rotate_tb->alt_image(ROTATE_LARGE_ICON);
	_rotate_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_rotate_tb->overlay_image(ROTATE_3D_SMALL_ICON);
	_rotate_tb->alt_overlay_image(ROTATE_3D_LARGE_ICON);
	_rotate_mode_tb->tooltip("Rotation Mode");
	_rotate_mode_tb->image(DROPDOWN_SMALL_ICON);
	_rotate_mode_tb->alt_image(DROPDOWN_LARGE_ICON);
	_rotate_mode_tb->alt_size(13, tbtnlarge_h);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	Fl_Menu_Item rotate_mode_dropdown_items[] = {
		{"2D &Arcball (" ALT_KEY_PLUS "A)", 0, (Fl_Callback *)rotate_2d_arcball_cb, this, FL_MENU_RADIO |
			(_model_area->rotation_mode() == Model_Area::ARCBALL_2D ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
		{"3D Arc&ball (" ALT_KEY_PLUS "B)", 0, (Fl_Callback *)rotate_3d_arcball_cb, this, FL_MENU_RADIO |
			(_model_area->rotation_mode() == Model_Area::ARCBALL_3D ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
		{"&X-Axis (" ALT_KEY_PLUS "X)", 0, (Fl_Callback *)rotate_x_axis_cb, this, FL_MENU_RADIO |
			(_model_area->rotation_mode() == Model_Area::AXIS_X ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
		{"&Y-Axis (" ALT_KEY_PLUS "Y)", 0, (Fl_Callback *)rotate_y_axis_cb, this, FL_MENU_RADIO |
			(_model_area->rotation_mode() == Model_Area::AXIS_Y ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
		{"&Z-Axis (" ALT_KEY_PLUS "Z)", 0, (Fl_Callback *)rotate_z_axis_cb, this, FL_MENU_RADIO |
			(_model_area->rotation_mode() == Model_Area::AXIS_Z ? FL_MENU_VALUE : 0), VW_MENU_STYLE},
		{}
	};
#pragma GCC diagnostic pop
	_rotate_mode_tb->copy(rotate_mode_dropdown_items);
#define VWR_FIND_MENU_ITEM_CB(C) (const_cast<Fl_Menu_Item *>(_rotate_mode_tb->find_item((Fl_Callback *)(C))))
	_2d_arcball_dd_mi = VWR_FIND_MENU_ITEM_CB(rotate_2d_arcball_cb);
	_3d_arcball_dd_mi = VWR_FIND_MENU_ITEM_CB(rotate_3d_arcball_cb);
	_x_axis_dd_mi = VWR_FIND_MENU_ITEM_CB(rotate_x_axis_cb);
	_y_axis_dd_mi = VWR_FIND_MENU_ITEM_CB(rotate_y_axis_cb);
	_z_axis_dd_mi = VWR_FIND_MENU_ITEM_CB(rotate_z_axis_cb);
#undef VWR_FIND_MENU_ITEM_CB
	_pan_tb->tooltip("Pan (4)");
	_pan_tb->callback((Fl_Callback *)pan_tb_cb, this);
	if (_model_area->action() == Model_Area::PAN) { _pan_tb->setonly(); }
	_pan_tb->image(PAN_SMALL_ICON);
	_pan_tb->alt_image(PAN_LARGE_ICON);
	_pan_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_zoom_tb->tooltip("Zoom (5)");
	_zoom_tb->callback((Fl_Callback *)zoom_tb_cb, this);
	if (_model_area->action() == Model_Area::ZOOM) { _zoom_tb->setonly(); }
	_zoom_tb->image(ZOOM_SMALL_ICON);
	_zoom_tb->alt_image(ZOOM_LARGE_ICON);
	_zoom_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_mark_tb->tooltip("Mark Synapse (6)");
	_mark_tb->callback((Fl_Callback *)mark_tb_cb, this);
	if (_model_area->action() == Model_Area::MARK) { _mark_tb->setonly(); }
	_mark_tb->image(MARK_SMALL_ICON);
	_mark_tb->alt_image(MARK_LARGE_ICON);
	_mark_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_snap_tb->tooltip("Snap to Axes (" COMMAND_KEY_PLUS "X)");
	_snap_tb->callback((Fl_Callback *)snap_cb, this);
	_snap_tb->image(SNAP_SMALL_ICON);
	_snap_tb->deimage(SNAP_DISABLED_SMALL_ICON);
	_snap_tb->alt_image(SNAP_LARGE_ICON);
	_snap_tb->alt_deimage(SNAP_DISABLED_LARGE_ICON);
	_snap_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	_reset_settings_tb->tooltip("Reset Settings (" COMMAND_KEY_PLUS "0)");
	_reset_settings_tb->callback((Fl_Callback *)reset_settings_cb, this);
	_reset_settings_tb->image(RESET_SETTINGS_SMALL_ICON);
	_reset_settings_tb->alt_image(RESET_SETTINGS_LARGE_ICON);
	_reset_settings_tb->alt_size(tbtnlarge_h, tbtnlarge_h);
	// Initialize sidebar
	_sidebar->clip_children(1);
	// Initialize sidebar's overview-related children
	_overview_heading->value(1);
	_overview_heading->callback((Fl_Callback *)overview_area_sb_cb, this);
	_overview_area->model_area(_model_area);
	_overview_area->dnd_receiver(_dnd_receiver);
	_overview_zoom->bounds(0.0, 1.0);
	_overview_zoom->value(_overview_area->zoom());
	_overview_zoom->callback((Fl_Callback *)overview_zoom_cb, this);
	// Initialize sidebar's configuration-related children
	_soma_letter_size_slider->range(8.0, 16.0);
	_soma_letter_size_slider->step(2.0);
	_soma_letter_size_slider->value(12.0);
	_soma_letter_size_slider->default_value(12.0);
	_soma_letter_size_slider->callback((Fl_Callback *)soma_letter_size_cb, this);
	_type_choice->callback((Fl_Callback *)type_get_cb, this);
	_type_is_letter->callback((Fl_Callback *)type_set_cb, this);
	_all_letter->image(STAR_ICON);
	_all_letter->deimage(STAR_DISABLED_ICON);
	_all_letter->tooltip("Letters For All Types");
	_all_letter->callback((Fl_Callback *)all_types_use_letter, this);
	_type_is_dot->callback((Fl_Callback *)type_set_cb, this);
	_all_dot->image(STAR_ICON);
	_all_dot->deimage(STAR_DISABLED_ICON);
	_all_dot->tooltip("Dots For All Types");
	_all_dot->callback((Fl_Callback *)all_types_use_dot, this);
	_type_is_hidden->callback((Fl_Callback *)type_set_cb, this);
	_all_hidden->image(STAR_ICON);
	_all_hidden->deimage(STAR_DISABLED_ICON);
	_all_hidden->tooltip("Hide All Types");
	_all_hidden->callback((Fl_Callback *)all_types_hidden, this);
	_type_is_disabled->callback((Fl_Callback *)type_set_cb, this);
	_all_disabled->image(STAR_ICON);
	_all_disabled->deimage(STAR_DISABLED_ICON);
	_all_disabled->tooltip("Disable All Types");
	_all_disabled->callback((Fl_Callback *)all_types_disabled, this);
	_type_color_choice->callback((Fl_Callback *)type_set_cb, this);
	size8_t num_colors = Color::num_colors();
	for (size8_t id = 0; id < num_colors; id++) {
		_type_color_choice->add(Color::color(id)->name(), 0, (Fl_Callback *)NULL, NULL, 0);
	}
	_type_color_swatch->box(OS_SWATCH_BOX);
	_load_config->tooltip("Load Config (F7)");
	_load_config->callback((Fl_Callback *)load_config_cb, this);
	_save_config->tooltip("Save Config (F6)");
	_save_config->callback((Fl_Callback *)save_config_cb, this);
	_reset_config->tooltip("Reset Config (" COMMAND_SHIFT_KEYS_PLUS "N)");
	_reset_config->callback((Fl_Callback *)reset_config_cb, this);
	// Initialize sidebar's selection-related children
	_select_id_spinner->type(FL_INT_INPUT);
	_select_id_spinner->range(0.0, (double)(std::numeric_limits<size32_t>::max() - 1));
	_select_id_spinner->step(1.0);
	_select_id_spinner->value(0.0);
	_fetch_select_id->image(REPORT_ICON);
	_fetch_select_id->deimage(REPORT_DISABLED_ICON);
	_fetch_select_id->tooltip("Fetch From Selected Soma");
	_fetch_select_id->callback((Fl_Callback *)fetch_select_id_cb, this);
	_select_id->callback((Fl_Callback *)select_id_cb, this);
	_deselect_id->callback((Fl_Callback *)deselect_id_cb, this);
	_syn_count_spinner->type(FL_INT_INPUT);
	_syn_count_spinner->range(0.0, (double)std::numeric_limits<size32_t>::max());
	_syn_count_spinner->step(1.0);
	_syn_count_spinner->value(0.0);
	_syn_kind->off_text("Axonal");
	_syn_kind->on_text("Dendritic");
	_select_syn_count->callback((Fl_Callback *)select_syn_count_cb, this);
	_report_syn_count->callback((Fl_Callback *)report_syn_count_cb, this);
	// Initialize sidebar's marking-related children
	_axon_id_spinner->type(FL_INT_INPUT);
	_axon_id_spinner->range(0.0, (double)(std::numeric_limits<size32_t>::max() - 1));
	_axon_id_spinner->step(1.0);
	_axon_id_spinner->value(0.0);
	_fetch_axon_id->image(REPORT_ICON);
	_fetch_axon_id->deimage(REPORT_DISABLED_ICON);
	_fetch_axon_id->tooltip("Fetch From Selected Soma");
	_fetch_axon_id->callback((Fl_Callback *)fetch_axon_id_cb, this);
	_den_id_spinner->type(FL_INT_INPUT);
	_den_id_spinner->range(0.0, (double)(std::numeric_limits<size32_t>::max() - 1));
	_den_id_spinner->step(1.0);
	_den_id_spinner->value(0.0);
	_fetch_den_id->image(REPORT_ICON);
	_fetch_den_id->deimage(REPORT_DISABLED_ICON);
	_fetch_den_id->tooltip("Fetch From Selected Soma");
	_fetch_den_id->callback((Fl_Callback *)fetch_den_id_cb, this);
	_limit_paths_spinner->type(FL_INT_INPUT);
	_limit_paths_spinner->range(0.0, (double)std::numeric_limits<size_t>::max());
	_limit_paths_spinner->step(1.0);
	_limit_paths_spinner->value(0.0);
	_select_conn_paths->callback((Fl_Callback *)select_conn_paths_cb, this);
	_mark_conn_paths->callback((Fl_Callback *)mark_conn_paths_cb, this);
	_report_conn_paths->callback((Fl_Callback *)report_conn_paths_cb, this);
	// Initialize model area
	_model_area->overview_area(_overview_area);
	_model_area->dnd_receiver(_dnd_receiver);
	// Initialize status bar buttons
	_report_synapses->image(REPORT_ICON);
	_report_synapses->deimage(REPORT_DISABLED_ICON);
	_report_synapses->tooltip("Report Selected Synapses (&)");
	_report_synapses->callback((Fl_Callback *)report_synapses_cb, this);
	_report_marked->image(REPORT_ICON);
	_report_marked->deimage(REPORT_DISABLED_ICON);
	_report_marked->tooltip("Report Marked Synapses (^)");
	_report_marked->callback((Fl_Callback *)report_marked_cb, this);
	_report_selected->image(REPORT_ICON);
	_report_selected->deimage(REPORT_DISABLED_ICON);
	_report_selected->tooltip("Report Selected Somas (*)");
	_report_selected->callback((Fl_Callback *)report_selected_cb, this);
	_prev_selected->image(PREVIOUS_ICON);
	_prev_selected->deimage(PREVIOUS_DISABLED_ICON);
	_prev_selected->tooltip("Previous ([)");
	_prev_selected->shortcut('[');
	_prev_selected->callback((Fl_Callback *)prev_selected_cb, this);
	_next_selected->image(NEXT_ICON);
	_next_selected->deimage(NEXT_DISABLED_ICON);
	_next_selected->tooltip("Next (])");
	_next_selected->shortcut(']');
	_next_selected->callback((Fl_Callback *)next_selected_cb, this);
	_deselect_shown->image(DESELECT_ICON);
	_deselect_shown->deimage(DESELECT_DISABLED_ICON);
	_deselect_shown->tooltip("Deselect (\\)");
	_deselect_shown->shortcut('\\');
	_deselect_shown->callback((Fl_Callback *)deselect_shown_cb, this);
	_pivot_shown->image(PIVOT_ICON);
	_pivot_shown->deimage(PIVOT_DISABLED_ICON);
	_pivot_shown->tooltip("Pivot Point (')");
	_pivot_shown->callback((Fl_Callback *)pivot_shown_cb, this);
	_summary->image(SUMMARY_ICON);
	_summary->deimage(SUMMARY_DISABLED_ICON);
	_summary->tooltip("Summary (?)");
	_summary->callback((Fl_Callback *)summary_cb, this);
	// Initialize simulation bar
	_simulation_bar->resizable(_firing_time_slider);
	// Initialize simulation bar's first row
	_display_value_for_somas->value(opts.display_value_for_somas() ? 1 : 0);
	_display_value_for_somas->tooltip("Display value for somas ($)");
	_display_value_for_somas->callback((Fl_Callback *)display_value_for_somas_tb_cb, this);
	_show_inactive_somas->value(opts.show_inactive_somas() ? 1 : 0);
	_show_inactive_somas->tooltip("Show inactive somas (!)");
	_show_inactive_somas->callback((Fl_Callback *)show_inactive_somas_tb_cb, this);
	_show_color_scale->value(opts.show_color_scale() ? 1 : 0);
	_show_color_scale->tooltip("Show color scale (@@)"); // In FLTK, "@@" escapes a literal "@"
	_show_color_scale->callback((Fl_Callback *)show_color_scale_tb_cb, this);
	_display_static_model->value(opts.display() == Draw_Options::STATIC_MODEL ? 1 : 0);
	_display_static_model->tooltip("Static model (" ALT_KEY_PLUS "S)");
	_display_static_model->callback((Fl_Callback *)set_simulation_display_tb_cb, this);
	_display_firing_spikes->value(opts.display() == Draw_Options::FIRING_SPIKES ? 1 : 0);
	_display_firing_spikes->tooltip("Firing spikes (" ALT_KEY_PLUS "F)");
	_display_firing_spikes->callback((Fl_Callback *)set_simulation_display_tb_cb, this);
	_display_voltages->value(opts.display() == Draw_Options::VOLTAGES ? 1 : 0);
	_display_voltages->tooltip("Voltages (" ALT_KEY_PLUS "V)");
	_display_voltages->callback((Fl_Callback *)set_simulation_display_tb_cb, this);
	_display_weights->value(opts.display() == Draw_Options::WEIGHTS ? 1 : 0);
	_display_weights->tooltip("Weights (" ALT_KEY_PLUS "W)");
	_display_weights->callback((Fl_Callback *)set_simulation_display_tb_cb, this);
	// Initialize simulation bar's second row
	_display_choice_heading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE); // no clipping
	_display_wizard->labelfont(OS_FONT);
	_display_wizard->labelsize(OS_FONT_SIZE);
	_display_wizard->box(FL_NO_BOX);
	_display_wizard->clip_children(0);
	_display_static_group->box(FL_NO_BOX);
	_display_firing_group->box(FL_NO_BOX);
	_display_voltages_group->box(FL_NO_BOX);
	_display_weights_group->box(FL_NO_BOX);
	// Initialize simulation bar's second row's wizard widgets
	_static_size->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP); // right-justified
	_firing_cycle_length->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP); // right-justified
	_firing_report_current->callback((Fl_Callback *)firing_report_current_cb, this);
	_firing_report_average->callback((Fl_Callback *)firing_report_average_cb, this);
	_firing_select_top->callback((Fl_Callback *)firing_select_top_cb, this);
	_firing_select_top_spinner->type(FL_INT_INPUT);
	_firing_select_top_spinner->range(1.0, (double)Model_State::MAX_SELECTED);
	_firing_select_top_spinner->step(1.0);
	_firing_select_top_spinner->value(5.0);
	_voltages_count->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP); // right-justified
	_voltages_graph_selected->callback((Fl_Callback *)voltages_graph_selected_cb, this);
	_weights_prev_change->image(PREVIOUS_CHANGE_ICON);
	_weights_prev_change->tooltip("Previous Weight Change");
	_weights_prev_change->callback((Fl_Callback *)weights_prev_change_cb, this);
	_weights_next_change->image(NEXT_CHANGE_ICON);
	_weights_next_change->tooltip("Next Weight Change");
	_weights_next_change->callback((Fl_Callback *)weights_next_change_cb, this);
	_weights_prev_selected->image(PREVIOUS_SELECTED_ICON);
	_weights_prev_selected->deimage(PREVIOUS_SELECTED_DISABLED_ICON);
	_weights_prev_selected->tooltip("Previous Selected Change");
	_weights_prev_selected->callback((Fl_Callback *)weights_prev_selected_cb, this);
	_weights_next_selected->image(NEXT_SELECTED_ICON);
	_weights_next_selected->deimage(NEXT_SELECTED_DISABLED_ICON);
	_weights_next_selected->tooltip("Next Selected Change");
	_weights_next_selected->callback((Fl_Callback *)weights_next_selected_cb, this);
	_weights_color_after->off_text("Pre-change");
	_weights_color_after->on_text("Post-change");
	_weights_color_after->value(opts.weights_color_after() ? 1 : 0);
	_weights_color_after->callback((Fl_Callback *)weights_color_after_cb, this);
	_weights_scale_center_spinner->type(FL_INT_INPUT);
	_weights_scale_center_spinner->range(-164.0, 164.0);
	_weights_scale_center_spinner->step(1.0);
	_weights_scale_center_spinner->default_value(0.0);
	_weights_scale_center_spinner->callback((Fl_Callback *)weights_scale_center_cb, this);
	_weights_scale_center_slider->callback((Fl_Callback *)weights_scale_center_cb, this);
	_weights_scale_center_slider->bounds(_weights_scale_center_spinner->minimum(), _weights_scale_center_spinner->maximum());
	_weights_scale_center_slider->step(_weights_scale_center_spinner->step());
	_weights_scale_center_slider->default_value(_weights_scale_center_spinner->value());
	_weights_scale_spread_spinner->type(FL_FLOAT_INPUT);
	_weights_scale_spread_spinner->range(-3.0, 3.0);
	_weights_scale_spread_spinner->step(0.1);
	_weights_scale_spread_spinner->default_value(0.0);
	_weights_scale_spread_spinner->callback((Fl_Callback *)weights_scale_spread_cb, this);
	_weights_scale_spread_slider->callback((Fl_Callback *)weights_scale_spread_cb, this);
	_weights_scale_spread_slider->bounds(_weights_scale_spread_spinner->minimum(), _weights_scale_spread_spinner->maximum());
	_weights_scale_spread_slider->step(_weights_scale_spread_spinner->step());
	_weights_scale_spread_slider->default_value(_weights_scale_spread_spinner->value());
	// Initialize simulation bar's third row
	_firing_speed_spinner->type(FL_INT_INPUT);
	_firing_speed_spinner->range(1.0, 10.0);
	_firing_speed_spinner->step(1.0);
	_firing_speed_spinner->value(4.0);
	_play_pause_firing->tooltip("Play (.)");
	_play_pause_firing->callback((Fl_Callback *)play_pause_firing_cb, this);
	_play_pause_firing->image(PLAY_ICON);
	_step_firing->tooltip("Step (>)");
	_step_firing->callback((Fl_Callback *)step_firing_cb, this);
	_step_firing->image(STEP_ICON);
	_firing_time_spinner->type(FL_INT_INPUT);
	_firing_time_spinner->step(1.0);
	_firing_time_spinner->callback((Fl_Callback *)firing_start_time_cb, this);
	_firing_time_slider->labeltype(FL_NO_LABEL);
	_firing_time_slider->callback((Fl_Callback *)firing_start_time_cb, this);
	_firing_time_slider->step(_firing_time_spinner->step());
	// Initialize file choosers
	_model_chooser->title("Open Model");
	_model_chooser->filter("Model Files\t*.{vbm,vbm.gz,txt.gz}\nBinary Model Files\t*.vbm\nText Model Files\t*.txt\n"
		"Compressed Model Files\t*.{vbm.gz,txt.gz}\nAll Model Files\t*.{vbm,txt,vbm.gz,txt.gz}\n");
	_firing_chooser->title("Load Firing Spikes");
	_firing_chooser->filter("All Firing Spike Files\t*.{txt,txt.gz}\nText Firing Spike Files\t*.txt\n"
		"Compressed Firing Spike Files\t*.txt.gz\n");
	_voltages_chooser->title("Load Voltages");
	_voltages_chooser->filter("All Voltage Files\t*.{txt,txt.gz}\nText Voltage Files\t*.txt\n"
		"Compressed Voltage Files\t*.txt.gz\n");
	_weights_chooser->title("Load Weights");
	_weights_chooser->filter("All Weight Files\t*.{txt,txt.gz}\nText Weight Files\t*.txt\n"
		"Compressed Weight Files\t*.txt.gz\n");
	_prunings_chooser->title("Load Prunings");
	_prunings_chooser->filter("All Pruning Files\t*.{txt,txt.gz}\nText Pruning Files\t*.txt\n"
		"Compressed Pruning Files\t*.txt.gz\n");
	_image_chooser->title("Export Image");
	_image_chooser->filter(Image::FILE_CHOOSER_FILTER);
	_image_chooser->preset_file("viz_screenshot.png");
	_state_load_chooser->title("Load State");
	_state_load_chooser->filter("State File\t*.dat\n");
	_state_save_chooser->title("Save State");
	_state_save_chooser->filter("State File\t*.dat\n");
	_state_save_chooser->preset_file("viz_state.dat");
	_config_load_chooser->title("Load Configuration");
	_config_load_chooser->filter("Configuration File\t*.cfg\n");
	_config_save_chooser->title("Save Configuration");
	_config_save_chooser->filter("Configuration File\t*.cfg\n");
	_config_save_chooser->preset_file("viz_config.cfg");
	_text_report_chooser->title("Save Report");
	_text_report_chooser->filter("Text File\t*.txt\n");
	_text_report_chooser->preset_file("viz_report.txt");
	// Initialize help window
	_help_window->file("help.html");
	// Initialize dialogs
	_about_dialog->subject(PROGRAM_NAME " " VIZ_VERSION_STRING
#ifdef __64BIT__
		" (64-bit)"
#endif
		);
	_about_dialog->message("Built at " __TIME__ ", " __DATE__ ".\n\n"
#if defined(SHORT_COORDS)
		"Coordinates are 16-bit short integers.\n\n"
#elif defined(INT_COORDS)
		"Coordinates are 32-bit integers.\n\n"
#else
		"Coordinates are floating-point values.\n\n"
#endif
		"Developed by Remy Oukaour.\nAdvisor: Prof. Larry Wittie.\n\n"
		"Previous versions by Pablo Montes Arango,\nJordan Ponzo, Vikas Ashok, and Jack Zito.");
#ifdef LARGE_INTERFACE
	_about_dialog->width_range(340, 850);
	_error_dialog->width_range(340, 850);
	_success_dialog->width_range(340, 850);
	_summary_dialog->width_range(440, 880);
#else
	_about_dialog->width_range(280, 700);
	_error_dialog->width_range(280, 700);
	_success_dialog->width_range(280, 700);
	_summary_dialog->width_range(360, 720);
#endif
	_summary_dialog->model(&_model_area->const_model());
#ifdef LARGE_INTERFACE
	_large_icon_mi->set();
	large_icon_cb(NULL, this);
#endif
	refresh_model_file();
#undef VW_MENU_STYLE
#undef _P
}

void Viz_Window::show() {
	Fl_Double_Window::show();
#if defined(_WIN32)
	// Fix for 16x16 icon from <http://www.fltk.org/str.php?L925>
	HWND hwnd = fl_xid(this);
	HANDLE big_icon = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CXICON), 0);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(big_icon));
	HANDLE small_icon = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CXSMICON), 0);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small_icon));
#elif defined(__LINUX__)
	// Fix for X11 icon alpha mask <https://www.mail-archive.com/fltk@easysw.com/msg02863.html>
	XWMHints *hints = XGetWMHints(fl_display, fl_xid(this));
	hints->flags |= IconMaskHint;
	hints->icon_mask = _icon_mask;
	XSetWMHints(fl_display, fl_xid(this), hints);
	XFree(hints);
#endif
	_model_area->show(); // fix for invalid context
}

void Viz_Window::refresh_model_file(void) {
	const Brain_Model &bm = _model_area->const_model();
	if (!_model_area->opened()) {
		// Refresh title bar
		label(PROGRAM_NAME);
		// Refresh toolbar
		_load_firing_spikes_tb->deactivate();
		_load_voltages_tb->deactivate();
		_load_weights_tb->deactivate();
		_load_prunings_tb->deactivate();
		_export_image_tb->deactivate();
		_undo_tb->deactivate();
		_redo_tb->deactivate();
		_copy_tb->deactivate();
		_paste_tb->deactivate();
		_reset_tb->deactivate();
		_repeat_tb->deactivate();
		_snap_tb->deactivate();
		// Refresh sidebar
		refresh_config();
		// Refresh status bar
		_soma_count->reset_label();
		_synapse_count->reset_label();
		refresh_selected();
		// Refresh model area
		_model_area->clear();
	}
	else {
		// Refresh title bar
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		bm.print(ss);
		ss << " - " << PROGRAM_NAME;
		copy_label(ss.str().c_str());
		// Refresh toolbar
		_load_firing_spikes_tb->activate();
		_export_image_tb->activate();
		_undo_tb->activate();
		_redo_tb->activate();
		_copy_tb->activate();
		_paste_tb->activate();
		_reset_tb->activate();
		_repeat_tb->activate();
		_snap_tb->activate();
		// Refresh sidebar
		refresh_config();
		// Refresh model area
		_model_area->prepare();
		// Refresh status bar
		ss.precision(0);
		size32_t ns = _model_area->model().num_somas();
		ss.str("");
		ss << ns << " soma" << (ns == 1 ? "" : "s");
		_soma_count->copy_label(ss.str().c_str());
		size32_t ny = _model_area->model().num_synapses();
		ss.str("");
		ss << ny << " synapse" << (ny == 1 ? "" : "s");
		_synapse_count->copy_label(ss.str().c_str());
		// Refresh soma ID selector
		refresh_selected();
		redraw();
	}
}

void Viz_Window::refresh_static_model() {
	const Brain_Model &bm = _model_area->const_model();
	// Refresh display choice heading
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss << "File: ";
	bm.print(ss);
	_display_choice_heading->copy_label(ss.str().c_str());
	// Refresh display choice wizard
	_display_wizard->value(_display_static_group);
	// Refresh model size
	ss.str("");
	ss.imbue(std::locale("C"));
	ss.precision(0);
	coord_t size[3];
	const coord_t *bmin = bm.bounds().min(), *bmax = bm.bounds().max();
	// "BOSS.exe -x X -y Y" generates a model with somas from (1, 1) to (X-1, Y-1), so add 2 to compensate
	size[0] = bmax[0] - bmin[0] + 2; size[1] = bmax[1] - bmin[1] + 2; size[2] = bmax[2] - bmin[2] + 2;
	const char *TIMES = "\xC3\x97"; // UTF-8 encoding of U+00D7 "MULTIPLICATION SIGN"
	ss << size[0] << "X" << TIMES << size[1] << "Y" << TIMES << size[2] << "Z";
	_static_size->copy_label(ss.str().c_str());
	// Refresh window
	redraw();
}

void Viz_Window::refresh_firing_spikes() {
	const Brain_Model &bm = _model_area->const_model();
	Firing_Spikes *fd = _model_area->model().firing_spikes();
	if (!fd) {
		// Refresh toolbar
		_load_voltages_tb->deactivate();
		_load_weights_tb->deactivate();
		_load_prunings_tb->deactivate();
		// Refresh display choices
		_display_voltages->deactivate();
		_display_weights->deactivate();
		// Refresh display choice heading
		_display_choice_heading->label("File: None");
		// Refresh window
		if (contains(_simulation_bar)) {
			remove(_simulation_bar);
			_model_area->size(_model_area->w(), _model_area->h() + _simulation_bar->h());
		}
		// Refreshes model area
		set_simulation_display_tb_cb(_display_static_model, this);
	}
	else {
		// Refresh toolbar
		_load_voltages_tb->activate();
		_load_weights_tb->activate();
		// Refresh display choices
		if (!bm.has_voltages()) { _display_voltages->deactivate(); }
		if (!bm.has_weights()) { _display_weights->deactivate(); }
		// Refresh display choice heading
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "File: ";
		fd->print(ss);
		_display_choice_heading->copy_label(ss.str().c_str());
		// Refresh display choice wizard
		_display_wizard->value(_display_firing_group);
		// Refresh firing spikes rate
		ss.precision(1);
		ss.str("");
		size32_t nc = fd->num_cycles();
		ss << nc << " cycle" << (nc != 1 ? "s" : "") << "; ";
		ss << (1.0f / fd->timescale()) << " ms/cycle";
		_firing_cycle_length->copy_label(ss.str().c_str());
		if (!contains(_simulation_bar)) {
			// Refresh firing spikes
			double t = (double)fd->start_time(0);
			// Refresh firing time spinner and slider
			_firing_time_spinner->range(0.0, (double)fd->max_time());
			_firing_time_spinner->value(t);
			_firing_time_slider->bounds(_firing_time_spinner->minimum(), _firing_time_spinner->maximum());
			_firing_time_slider->value(_firing_time_spinner->value());
			// Refresh window
			add(_simulation_bar);
			_model_area->size(_model_area->w(), _model_area->h() - _simulation_bar->h());
			_simulation_bar->resize(_model_area->x(), _model_area->y() + _model_area->h(),
				_model_area->w(), _simulation_bar->h());
		}
	}
}

void Viz_Window::refresh_voltages(void) {
	Voltages *v = _model_area->model().voltages();
	if (!v) {
		// Refresh voltages choice
		_display_voltages->deactivate();
		// Refresh display choice heading
		_display_choice_heading->label("File: None");
		// Refresh recorded soma count
		_voltages_count->label("No recorded somas");
		// Refreshes model area
		set_simulation_display_tb_cb(_display_firing_spikes, this);
	}
	else {
		// Refresh voltages choice
		_display_voltages->activate();
		// Refresh display choice heading
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "File: ";
		v->print(ss);
		_display_choice_heading->copy_label(ss.str().c_str());
		// Refresh display choice wizard
		_display_wizard->value(_display_voltages_group);
		// Refresh recorded soma count
		ss.str("");
		size32_t nr = v->num_active_somas();
		ss << nr << " recorded soma" << (nr != 1 ? "s" : "");
		_voltages_count->copy_label(ss.str().c_str());
	}
}

void Viz_Window::refresh_weights(void) {
	Weights *w = _model_area->model().weights();
	if (!w) {
		// Refresh toolbar
		_load_prunings_tb->deactivate();
		// Refresh weights choice
		_display_weights->deactivate();
		// Refresh display choice heading
		_display_choice_heading->label("File: None");
		// Refreshes model area
		set_simulation_display_tb_cb(_display_firing_spikes, this);
		// Refresh scale center
		_weights_scale_center_spinner->default_value(0.0);
		_weights_scale_center_slider->default_value(0.0);
		// Refresh scale spread
		_weights_scale_spread_spinner->default_value(0.0);
		_weights_scale_spread_slider->default_value(0.0);
	}
	else {
		// Refresh toolbar
		_load_prunings_tb->activate();
		// Refresh weights choice
		_display_weights->activate();
		// Refresh display choice heading
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "File: ";
		w->print(ss);
		_display_choice_heading->copy_label(ss.str().c_str());
		// Refresh display choice wizard
		_display_wizard->value(_display_weights_group);
		// Refresh scale center
		double c = (double)w->center();
		_weights_scale_center_spinner->default_value(c);
		_weights_scale_center_slider->default_value(c);
		// Refresh scale spread
		double s = (double)w->spread();
		_weights_scale_spread_spinner->default_value(s);
		_weights_scale_spread_slider->default_value(s);
	}
}

void Viz_Window::refresh_selected(bool show_last) {
	const Model_State &ms = _model_area->const_state();
	size_t ns = ms.num_selected();
	if (show_last) {
		_shown_selected = ns - 1;
	}
	else if (_shown_selected >= ns) {
		_shown_selected = 0;
	}
	// Refresh sidebar and status bar
	if (ns > 1) {
		_prev_selected->activate();
		_next_selected->activate();
	}
	else {
		_prev_selected->deactivate();
		_next_selected->deactivate();
	}
	if (!ns) {
		_fetch_select_id->deactivate();
		_fetch_axon_id->deactivate();
		_fetch_den_id->deactivate();
		_report_synapses->deactivate();
		_report_selected->deactivate();
		_deselect_shown->deactivate();
		_weights_prev_selected->deactivate();
		_weights_next_selected->deactivate();
		_selected_count->reset_label();
		_selected_type->reset_label();
		_selected_coords->reset_label();
		_selected_hertz->reset_label();
		_selected_voltage->reset_label();
	}
	else {
		_fetch_select_id->activate();
		_fetch_axon_id->activate();
		_fetch_den_id->activate();
		_report_synapses->activate();
		_report_selected->activate();
		_deselect_shown->activate();
		_weights_prev_selected->activate();
		_weights_next_selected->activate();
		const Brain_Model &bm = _model_area->const_model();
		const Soma *s = ms.selected(_shown_selected);
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(0);
		// Refresh selected count
		ss << ns << " selected";
		_selected_count->copy_label(ss.str().c_str());
		// Refresh selected type
		ss.str("");
		const Soma_Type *t = bm.type(s->type_index());
		ss << t->name() << " #" << s->id();
		_selected_type->copy_label(ss.str().c_str());
		// Refresh selected coords
		ss.str("");
		const coord_t *c = s->coords();
		ss.imbue(std::locale("C"));
		ss << "(" << c[0] << ", " << c[1] << ", " << c[2] << ")";
		ss.imbue(std::locale(""));
		_selected_coords->copy_label(ss.str().c_str());
		// Refresh selected Hertz and voltage
		refresh_selected_sim_data();
	}
	// Refresh marked count
	size_t nm = ms.num_marked();
	if (!nm) {
		_report_marked->deactivate();
		_marked_count->reset_label();
	}
	else {
		_report_marked->activate();
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(0);
		ss << nm << " marked";
		_marked_count->copy_label(ss.str().c_str());
	}
	// Refresh model area
	_model_area->refresh();
}

void Viz_Window::refresh_selected_sim_data() {
	const Model_State &ms = _model_area->const_state();
	if (!ms.num_selected()) { return; }
	const Brain_Model &bm = _model_area->const_model();
	const Firing_Spikes *fd = bm.const_firing_spikes();
	const Voltages *vt = bm.const_voltages();
	size32_t sel_index = ms.selected_index(_shown_selected);
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(0);
	// Refresh selected Hertz
	ss.str("");
	if (bm.has_firing_spikes() && fd->active(sel_index)) {
		ss << fd->hertz(sel_index) << " Hz";
	}
	_selected_hertz->copy_label(ss.str().c_str());
	// Refresh selected voltage
	ss.str("");
	if (bm.has_voltages() && vt->active(sel_index)) {
		ss.unsetf(std::ios::floatfield);
		ss.precision(5);
		ss << vt->voltage(sel_index) << " mV";
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(0);
	}
	_selected_voltage->copy_label(ss.str().c_str());
}

void Viz_Window::refresh_config() {
	// Refresh sidebar
	_type_choice->clear();
	_type_is_letter->clear();
	_type_is_dot->clear();
	_type_is_hidden->clear();
	_type_is_disabled->clear();
	_type_color_choice->value(-1);
	_type_color_swatch->color(FL_BACKGROUND_COLOR);
	_select_type_choice->clear();
	_select_type_choice->value(-1);
	size8_t num_types = _model_area->model().num_types();
	if (!num_types) {
		Fl_Color tab_bgc = OS::current_theme() == OS::METAL ? FL_BACKGROUND_COLOR : _sidebar_tabs->selection_color();
		_config_tab->color(tab_bgc);
		_selection_tab->color(tab_bgc);
		_marking_tab->color(tab_bgc);
		_soma_letter_size_slider->color(tab_bgc);
		_include_disabled->color(tab_bgc);
		Fl_Color group_bgc = OS::current_theme() == OS::AQUA ? fl_rgb_color(0xE3, 0xE3, 0xE3) : tab_bgc;
		_type_group->color(group_bgc);
		_type_is_letter->color(group_bgc);
		_all_letter->color(group_bgc);
		_type_is_dot->color(group_bgc);
		_all_dot->color(group_bgc);
		_type_is_hidden->color(group_bgc);
		_all_hidden->color(group_bgc);
		_type_is_disabled->color(group_bgc);
		_all_disabled->color(group_bgc);
		_overview_heading->deactivate();
		_overview_area->deactivate();
		_overview_zoom->value(0.0);
		_overview_zoom->deactivate();
		_sidebar_spacer->deactivate();
		_sidebar_tabs->deactivate();
	}
	else {
		_overview_heading->activate();
		_overview_heading->do_callback();
		_overview_zoom->activate();
		_sidebar_spacer->activate();
		_sidebar_tabs->activate();
		Fl_Color tab_bgc = OS::current_theme() == OS::METAL ? FL_BACKGROUND_COLOR : _sidebar_tabs->selection_color();
		_config_tab->color(tab_bgc);
		_selection_tab->color(tab_bgc);
		_marking_tab->color(tab_bgc);
		_soma_letter_size_slider->color(tab_bgc);
		_include_disabled->color(tab_bgc);
		Fl_Color group_bgc = OS::current_theme() == OS::AQUA ? fl_rgb_color(0xE3, 0xE3, 0xE3) : tab_bgc;
		_type_group->color(group_bgc);
		_type_is_letter->color(group_bgc);
		_all_letter->color(group_bgc);
		_type_is_dot->color(group_bgc);
		_all_dot->color(group_bgc);
		_type_is_hidden->color(group_bgc);
		_all_hidden->color(group_bgc);
		_type_is_disabled->color(group_bgc);
		_all_disabled->color(group_bgc);
		for (size8_t i = 0; i < num_types; i++) {
			Soma_Type *t = _model_area->model().type(i);
			_type_choice->add(t->name().c_str(), 0, (Fl_Callback *)NULL, NULL, 0);
			_select_type_choice->add(t->name().c_str(), 0, (Fl_Callback *)NULL, NULL, 0);
		}
		_type_choice->value(0);
		_type_choice->do_callback();
		_select_type_choice->value(0);
	}
	// Refresh status bar
	if (!num_types) {
		_pivot_shown->deactivate();
		_summary->deactivate();
	}
	else {
		_pivot_shown->activate();
		_summary->activate();
	}
}

void Viz_Window::summary_dialog() {
	bool clipped = _model_area->const_state().clipped() && _model_area->draw_options().only_show_clipped();
	const Clip_Volume *v = clipped ? &_model_area->const_state().const_clip_volume() : NULL;
	_summary_dialog->show(this, v);
}

void Viz_Window::summary_dialog(const Soma *s, size32_t index) {
	bool clipped = _model_area->const_state().clipped() && _model_area->draw_options().only_show_clipped();
	const Clip_Volume *v = clipped ? &_model_area->const_state().const_clip_volume() : NULL;
	_summary_dialog->show(this, s, index, v);
}

void Viz_Window::summary_dialog(const Synapse *y, size32_t index) {
	_summary_dialog->show(this, y, index);
}

static bool ends_with(std::string const &s, std::string const &end) {
	std::string si = s;
	std::transform(si.begin(), si.end(), si.begin(), ::tolower);
	if (si.length() < end.length()) { return false; }
	return si.compare(si.length() - end.length(), end.length(), end) == 0;
}

bool Viz_Window::open_model(const char *filename) {
	const char *basename = fl_filename_name(filename);
	// Unload related files, if any
	unload_weights_cb(NULL, this);
	unload_voltages_cb(NULL, this);
	unload_firing_spikes_cb(NULL, this);
	// Open and parse model file
	Read_Status status;
	if (ends_with(basename, ".vbm.gz")) {
		// Open chosen file as compressed binary
		Gzip_Binary_Parser gbp(filename);
		if (!gbp.good()) {
			std::string msg = "Could not open ";
			msg = msg + basename + "!";
			_error_dialog->message(msg);
			_error_dialog->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Opening...");
		_progress_dialog->show(this);
		// Parse model file
		status = _model_area->read_model_from(gbp, _progress_dialog);
		_progress_dialog->hide();
	}
	else if (ends_with(basename, ".txt.gz")) {
		// Open chosen file as compressed text
		Gzip_Input_Parser gip(filename);
		if (!gip.good()) {
			std::string msg = "Could not open ";
			msg = msg + basename + "!";
			_error_dialog->message(msg);
			_error_dialog->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Opening...");
		_progress_dialog->show(this);
		// Parse model file
		status = _model_area->read_model_from(gip, _progress_dialog);
		_progress_dialog->hide();
	}
	else if (ends_with(basename, ".vbm")) {
		// Open chosen file as binary
		Binary_Parser bp(filename);
		if (!bp.good()) {
			std::string msg = "Could not open ";
			msg = msg + basename + "!";
			_error_dialog->message(msg);
			_error_dialog->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Opening...");
		_progress_dialog->show(this);
		// Parse model file
		status = _model_area->read_model_from(bp, _progress_dialog);
		_progress_dialog->hide();
	}
	else {
		// Open chosen file as text
		Input_Parser ip(filename);
		if (!ip.good()) {
			std::string msg = "Could not open ";
			msg = msg + basename + "!";
			_error_dialog->message(msg);
			_error_dialog->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Opening...");
		_progress_dialog->show(this);
		// Parse model file
		status = _model_area->read_model_from(ip, _progress_dialog);
		_progress_dialog->hide();
	}
	if (status != SUCCESS) {
		std::string msg = read_status_message(status, basename);
		_error_dialog->message(msg);
		_error_dialog->show(this);
		close_model_cb(NULL, this);
		return false;
	}
	// Open default configuration file
	std::ifstream ifs(DEFAULT_CONFIG_FILE);
	bool cfg_opened = ifs.good();
	// Parse default configuration file
	Read_Status cfg_status = _model_area->model().read_config_from(ifs);
	if (!cfg_opened) {
		// Create missing default config file
		std::ofstream ofs(DEFAULT_CONFIG_FILE);
		_model_area->model().write_config_to(ofs);
	}
	else if (cfg_status != SUCCESS) {
		std::string msg = read_status_message(status, fl_filename_name(DEFAULT_CONFIG_FILE));
		_error_dialog->message(msg);
		_error_dialog->show(this);
	}
	refresh_model_file();
	return true;
}

static std::string replace_last(std::string s, std::string a, std::string b) {
	size_t i = s.rfind(a);
	if (i == std::string::npos) { return s; }
	std::string t = s;
	t.replace(i, a.length(), b);
	return t;
}

static std::string convert_model_filename(const char *filename, const char *r) {
	std::string s = replace_last(filename, "_model_", std::string("_") + r + "_");
	s = replace_last(s, ".vbm", ".txt");
	s = replace_last(s, ".txt.gz", ".txt");
	return s;
}

bool Viz_Window::open_and_load_all(const char *filename) {
	const char *basename = fl_filename_name(filename);
	// Open model
	if (!open_model(filename)) { return false; }
	// Get directory path of all associated files
	std::string model_input_filename = filename;
	size_t d = model_input_filename.find_last_of("/\\");
	if (d == std::string::npos) {
		std::string msg = "Could not get directory path!";
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	std::string directory = model_input_filename.substr(0, d + 1);
	// Construct filenames for simulation data files
	std::string firing_spikes_path = directory + convert_model_filename(basename, "firings");
	std::string voltages_path = directory + convert_model_filename(basename, "voltages");
	std::string weights_path = directory + convert_model_filename(basename, "weights");
	std::string prunings_path = directory + convert_model_filename(basename, "prunings");
	// Load simulation data files
	if (!load_firing_spikes(firing_spikes_path.c_str(), true)) { return false; }
	load_voltages(voltages_path.c_str(), true);
	load_weights(weights_path.c_str(), true);
	load_prunings(prunings_path.c_str(), true);
	set_simulation_display_tb_cb(_display_static_model, this);
	return true;
}

bool Viz_Window::load_firing_spikes(const char *filename, bool warn) {
	const char *basename = fl_filename_name(filename);
	Firing_Spikes *fd = new(std::nothrow) Firing_Spikes(&_model_area->const_model());
	if (fd == NULL) {
		std::string msg = "Could not load ";
		msg = msg + basename + "!\nNot enough memory was available.";
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	Read_Status status;
	if (ends_with(basename, ".gz")) {
		// Open chosen file as compressed text
		Gzip_Input_Parser gip(filename);
		if (!gip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse firing spikes file
		status = fd->read_from(gip, _progress_dialog);
		_progress_dialog->hide();
	}
	else {
		// Open chosen file as text
		Input_Parser ip(filename);
		if (!ip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse firing spikes file
		status = fd->read_from(ip, _progress_dialog);
		_progress_dialog->hide();
	}
	if (status != SUCCESS) {
		delete fd;
		std::string msg = read_status_message(status, basename);
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	// Use firing spikes
	_model_area->model().firing_spikes(fd);
	set_simulation_display_tb_cb(_display_firing_spikes, this);
	return true;
}

bool Viz_Window::load_voltages(const char *filename, bool warn) {
	const char *basename = fl_filename_name(filename);
	const Brain_Model &bm = _model_area->const_model();
	Voltages *v = new(std::nothrow) Voltages(&bm, bm.const_firing_spikes()->num_cycles());
	if (v == NULL) {
		std::string msg = "Could not load ";
		msg = msg + basename + "!\nNot enough memory was available.";
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	Read_Status status;
	if (ends_with(basename, ".gz")) {
		// Open chosen file as compressed text
		Gzip_Input_Parser gip(filename);
		if (!gip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse voltages file
		status = v->read_from(gip, _progress_dialog);
		_progress_dialog->hide();
	}
	else {
		// Open chosen file as text
		Input_Parser ip(filename);
		if (!ip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse voltages file
		status = v->read_from(ip, _progress_dialog);
		_progress_dialog->hide();
	}
	if (status != SUCCESS) {
		delete v;
		std::string msg = read_status_message(status, basename);
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	// Use voltages
	_model_area->model().voltages(v);
	set_simulation_display_tb_cb(_display_voltages, this);
	return true;
}

bool Viz_Window::load_weights(const char *filename, bool warn) {
	const char *basename = fl_filename_name(filename);
	const Brain_Model &bm = _model_area->const_model();
	Weights *w = new(std::nothrow) Weights(&bm, bm.const_firing_spikes()->num_cycles());
	if (w == NULL) {
		std::string msg = "Could not load ";
		msg = msg + basename + "!\nNot enough memory was available.";
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	Read_Status status;
	if (ends_with(basename, ".gz")) {
		// Open chosen file as compressed text
		Gzip_Input_Parser gip(filename);
		if (!gip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse weights file
		status = w->read_from(gip, _progress_dialog);
		_progress_dialog->hide();
	}
	else {
		// Open chosen file as text
		Input_Parser ip(filename);
		if (!ip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse weights file
		status = w->read_from(ip, _progress_dialog);
		_progress_dialog->hide();
	}
	if (status != SUCCESS) {
		delete w;
		std::string msg = read_status_message(status, basename);
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	// Use weights
	_model_area->model().weights(w);
	set_simulation_display_tb_cb(_display_weights, this);
	return true;
}

bool Viz_Window::load_prunings(const char *filename, bool warn) {
	const char *basename = fl_filename_name(filename);
	Brain_Model &bm = _model_area->model();
	Weights *w = bm.weights();
	if (w == NULL) {
		std::string msg = "Could not load ";
		msg = msg + basename + "!\nWeights must already be loaded.";
		Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
		md->message(msg);
		md->show(this);
		return false;
	}
	Read_Status status;
	if (ends_with(basename, ".gz")) {
		// Open chosen file as compressed text
		Gzip_Input_Parser gip(filename);
		if (!gip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse weights file
		status = w->read_prunings_from(gip, _progress_dialog);
		_progress_dialog->hide();
	}
	else {
		// Open chosen file as text
		Input_Parser ip(filename);
		if (!ip.good()) {
			std::string msg = "Could not load ";
			msg = msg + basename + "!";
			Modal_Dialog *md = warn ? _warning_dialog : _error_dialog;
			md->message(msg);
			md->show(this);
			return false;
		}
		// Show progress
		_progress_dialog->title("Loading...");
		_progress_dialog->show(this);
		// Parse weights file
		status = w->read_prunings_from(ip, _progress_dialog);
		_progress_dialog->hide();
	}
	if (status != SUCCESS) {
		delete w;
		std::string msg = read_status_message(status, basename);
		_error_dialog->message(msg);
		_error_dialog->show(this);
		return false;
	}
	// Use weights
	set_simulation_display_tb_cb(_display_weights, this);
	return true;
}


void Viz_Window::overview_area(bool show) {
	if (show) {
		if (_model_area->opened()) {
			_overview_area->activate();
			_overview_zoom->activate();
		}
		if (!contains(_sidebar)) { return; }
		if (!_overview_area->visible() && !_overview_zoom->visible()) {
			_overview_area->show();
			_overview_zoom->show();
			int sh = _sidebar->h();
			_sidebar->size(_sidebar->w(), 1200); // allow space for components to shift
			int dy = _overview_area->h() + _overview_zoom->h() + 15;
			int n = _sidebar->children();
			int f = _sidebar->find(_overview_zoom);
			for (int i = f + 1; i < n; i++) {
				Fl_Widget *c = _sidebar->child(i);
				c->position(c->x(), c->y() + dy);
			}
			_sidebar->init_sizes();
			init_sizes();
			_sidebar->size(_sidebar->w(), sh);
			redraw();
		}
	}
	else {
		_overview_area->deactivate();
		_overview_zoom->deactivate();
		if (_overview_area->visible() && _overview_zoom->visible()) {
			_overview_area->hide();
			_overview_zoom->hide();
			int sh = _sidebar->h();
			_sidebar->size(_sidebar->w(), 1200); // allow space for components to shift
			int dy = _overview_area->h() + _overview_zoom->h() + 15;
			int n = _sidebar->children();
			int f = _sidebar->find(_overview_zoom);
			for (int i = f + 1; i < n; i++) {
				Fl_Widget *c = _sidebar->child(i);
				c->position(c->x(), c->y() - dy);
			}
			_sidebar->init_sizes();
			init_sizes();
			_sidebar->size(_sidebar->w(), sh);
			redraw();
		}
	}
}

void Viz_Window::drag_and_drop_cb(DnD_Receiver *dndr, Viz_Window *vw) {
	std::string filename = dndr->text().substr(0, dndr->text().find('\n'));
	vw->open_and_load_all(filename.c_str());
}

void Viz_Window::open_model_cb(Fl_Widget *, Viz_Window *vw) {
	int status = vw->_model_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_model_chooser->filename();
	if (status == -1) {
		const char *basename = fl_filename_name(filename);
		std::string msg = "Could not open ";
		msg = msg + basename + "!\n" + vw->_model_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	vw->open_model(filename);
}

void Viz_Window::close_model_cb(Fl_Widget *w, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	unload_weights_cb(w, vw);
	unload_voltages_cb(w, vw);
	unload_firing_spikes_cb(w, vw);
	vw->_model_area->clear();
	vw->refresh_model_file();
}

void Viz_Window::open_and_load_all_cb(Fl_Widget *, Viz_Window *vw) {
	int status = vw->_model_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_model_chooser->filename();
	const char *basename = fl_filename_name(filename);
	if (status == -1) {
		std::string msg = "Could not open ";
		msg = msg + basename + "!\n" + vw->_model_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	vw->open_and_load_all(filename);
}

void Viz_Window::load_firing_spikes_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	const char *model_filename = vw->_model_chooser->filename();
	const char *model_basename = fl_filename_name(model_filename);
	vw->_firing_chooser->preset_file(convert_model_filename(model_basename, "firings").c_str());
	int status = vw->_firing_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_firing_chooser->filename();
	if (status == -1) {
		const char *basename = fl_filename_name(filename);
		std::string msg = "Could not load ";
		msg = msg + basename + "!\n\n" + vw->_firing_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	vw->load_firing_spikes(filename);
}

void Viz_Window::unload_firing_spikes_cb(Fl_Widget *w, Viz_Window *vw) {
	Brain_Model &bm = vw->_model_area->model();
	if (!bm.has_firing_spikes()) { return; }
	unload_weights_cb(w, vw);
	unload_voltages_cb(w, vw);
	if (vw->_playing) {
		play_pause_firing_cb(w, vw);
	}
	bm.firing_spikes(NULL);
	vw->refresh_firing_spikes();
}

void Viz_Window::load_voltages_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened() || !vw->_model_area->const_model().has_firing_spikes()) { return; }
	const char *model_filename = vw->_model_chooser->filename();
	const char *model_basename = fl_filename_name(model_filename);
	vw->_voltages_chooser->preset_file(convert_model_filename(model_basename, "voltages").c_str());
	int status = vw->_voltages_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_voltages_chooser->filename();
	if (status == -1) {
		const char *basename = fl_filename_name(filename);
		std::string msg = "Could not load ";
		msg = msg + basename + "!\n\n" + vw->_voltages_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	vw->load_voltages(filename);
}

void Viz_Window::unload_voltages_cb(Fl_Widget *, Viz_Window *vw) {
	Brain_Model &bm = vw->_model_area->model();
	if (!bm.has_voltages()) { return; }
	bm.voltages(NULL);
	vw->refresh_voltages();
}

void Viz_Window::load_weights_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened() || !vw->_model_area->const_model().has_firing_spikes()) { return; }
	const char *model_filename = vw->_model_chooser->filename();
	const char *model_basename = fl_filename_name(model_filename);
	vw->_weights_chooser->preset_file(convert_model_filename(model_basename, "weights").c_str());
	int status = vw->_weights_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_weights_chooser->filename();
	if (status == -1) {
		const char *basename = fl_filename_name(filename);
		std::string msg = "Could not load ";
		msg = msg + basename + "!\n\n" + vw->_weights_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	vw->load_weights(filename);
}

void Viz_Window::load_prunings_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened() || !vw->_model_area->const_model().has_weights()) { return; }
	const char *model_filename = vw->_model_chooser->filename();
	const char *model_basename = fl_filename_name(model_filename);
	vw->_prunings_chooser->preset_file(convert_model_filename(model_basename, "prunings").c_str());
	int status = vw->_prunings_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_prunings_chooser->filename();
	if (status == -1) {
		const char *basename = fl_filename_name(filename);
		std::string msg = "Could not load ";
		msg = msg + basename + "!\n\n" + vw->_prunings_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	vw->load_prunings(filename);
}

void Viz_Window::unload_weights_cb(Fl_Widget *, Viz_Window *vw) {
	Brain_Model &bm = vw->_model_area->model();
	if (!bm.has_weights()) { return; }
	bm.weights(NULL);
	vw->refresh_weights();
}

void Viz_Window::export_image_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	int status = vw->_image_chooser->show();
	if (status == 1) { return; }
	// Get name of chosen file
	const char *filename = vw->_image_chooser->filename();
	const char *basename = fl_filename_name(filename);
	// Operation failed
	if (status == -1) {
		std::string msg = "Could not export ";
		msg = msg + basename + "!\n\n" + vw->_image_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	// Write image
	Image::Format fmt = (Image::Format)vw->_image_chooser->filter_value();
	int err = vw->_model_area->write_image(filename, fmt);
	if (err) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		std::string msg = "Exported ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::export_animation_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened() || !vw->_model_area->const_model().has_firing_spikes()) { return; }
	int status = vw->_animation_chooser->show();
	if (status == 1) { return; }
	// Get name of chosen directory
	const char *dirname = vw->_animation_chooser->filename();
	const char *basename = fl_filename_name(dirname);
	// Operation failed
	if (status == -1) {
		std::string msg = "Could not export to ";
		msg = msg + basename + "!\n\n" + vw->_animation_chooser->errmsg();
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	// Show progress
	vw->_progress_dialog->title("Exporting...");
	vw->_progress_dialog->show(vw);
	// Write animation frames
	if (vw->_model_area->draw_options().display() == Draw_Options::STATIC_MODEL) {
		set_simulation_display_tb_cb(vw->_display_firing_spikes, vw);
	}
	vw->_firing_time_spinner->value(0.0);
	firing_start_time_cb(vw->_firing_time_spinner, vw);
	// Get the number of cycles
	size32_t n = vw->_model_area->const_model().const_firing_spikes()->num_cycles();
	// Prepare to show frame-writing progress
	bool canceled = false;
	size_t denom = n / Progress_Dialog::PROGRESS_STEPS;
	if (!denom) { denom = 1; }
	vw->_progress_dialog->message("Exporting animation...");
	vw->_progress_dialog->progress(0.0f);
	Fl::check();
	canceled = vw->_progress_dialog->canceled();
	// Write animation frames
	std::ostringstream ss;
	int z = (int)ceil(log10((float)n));
	int err = 0;
	for (size32_t i = 0; !canceled && i < n; i++) {
		ss.str("");
		ss << dirname << "/viz_anim_" << std::setw(z) << std::setfill('0') << i << ".png";
		err = vw->_model_area->write_image(ss.str().c_str(), Image::PNG);
		if (err) { break; }
		step_firing_cb(vw->_step_firing, vw);
		if (!((i + 1) % denom)) {
			vw->_progress_dialog->progress((float)(i + 1) / n);
		}
		Fl::check();
		canceled = vw->_progress_dialog->canceled();
	}
	vw->_progress_dialog->hide();
	if (canceled) {
		std::string msg = "Canceled exporting to ";
		msg = msg + basename + "!";
		vw->_warning_dialog->message(msg);
		vw->_warning_dialog->show(vw);
	}
	else if (err) {
		std::string msg = "Could not export to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		std::string msg = "Exported to ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::exit_cb(Fl_Widget *, void *) {
	// Override default behavior of Esc to close main window
	if (Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape) { return; }
	exit(EXIT_SUCCESS);
}

void Viz_Window::undo_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->undo();
}

void Viz_Window::redo_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->redo();
}

void Viz_Window::repeat_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->repeat_action();
}

void Viz_Window::copy_state_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->copy();
}

void Viz_Window::paste_state_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->paste();
}

void Viz_Window::reset_state_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->reset();
}

static bool file_exists(const char *f) {
	std::ifstream ifs(f);
	return ifs.good();
}

void Viz_Window::load_state_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	int status = vw->_state_load_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_state_load_chooser->filename();
	const char *basename = fl_filename_name(filename);
	// Open chosen file
	Input_Parser ip(filename);
	if (!ip.good()) {
		std::string msg = "Could not load ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	// Parse state file
	Read_Status state_status = vw->_model_area->read_state_from(ip);
	if (state_status != SUCCESS) {
		std::string msg = read_status_message(state_status, basename);
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::save_state_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	int status = vw->_state_save_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_state_save_chooser->filename();
	const char *basename = fl_filename_name(filename);
	if (file_exists(filename)) {
		std::string msg = "You cannot save to ";
		msg = msg + basename + "!\nThe file already exists!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		vw->_model_area->const_state().write_to(ofs);
		std::string msg = "Saved state to ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::load_config_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	int status = vw->_config_load_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_config_load_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ifstream ifs(filename);
	bool opened = ifs.good();
	if (!opened) {
		std::string msg = "Could not open ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	Read_Status state_status = vw->_model_area->model().read_config_from(ifs);
	if (opened && state_status != SUCCESS) {
		std::string msg = read_status_message(state_status, basename);
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	vw->refresh_config();
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::save_config_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	int status = vw->_config_save_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_config_save_chooser->filename();
	const char *basename = fl_filename_name(filename);
	if (file_exists(filename)) {
		std::string msg = "You cannot save to ";
		msg = msg + basename + "!\nThe file already exists!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		vw->_model_area->model().write_config_to(ofs);
		std::string msg = "Saved configuration to ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::reset_config_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	size8_t num_types = vw->_model_area->model().num_types();
	if (!num_types) { return; }
	for (size8_t i = 0; i < num_types; i++) {
		Soma_Type *t = vw->_model_area->model().type(i);
		t->reset();
	}
	vw->_type_choice->do_callback();
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::toolbar_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int dy = vw->_toolbar->h();
	if (m->mvalue()->value()) {
		vw->add(vw->_toolbar);
		vw->_toolbar->size(vw->w(), dy);
		vw->_sidebar->resize(vw->_sidebar->x(), vw->_sidebar->y() + dy,
			vw->_sidebar->w(), vw->_sidebar->h() - dy);
		vw->_model_area->resize(vw->_model_area->x(), vw->_model_area->y() + dy,
			vw->_model_area->w(), vw->_model_area->h() - dy);
	}
	else {
		vw->remove(vw->_toolbar);
		vw->_sidebar->resize(vw->_sidebar->x(), vw->_sidebar->y() - dy,
			vw->_sidebar->w(), vw->_sidebar->h() + dy);
		vw->_model_area->resize(vw->_model_area->x(), vw->_model_area->y() - dy,
			vw->_model_area->w(), vw->_model_area->h() + dy);
	}
	vw->redraw();
}

void Viz_Window::sidebar_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int dx = vw->_sidebar->w();
	if (m->mvalue()->value()) {
		vw->add(vw->_sidebar);
		if (vw->_overview_heading->value()) {
			vw->overview_area(true);
		}
		vw->_model_area->resize(vw->_model_area->x() + dx, vw->_model_area->y(),
			vw->_model_area->w() - dx, vw->_model_area->h());
		vw->_sidebar->size(dx, vw->_model_area->h() + (vw->contains(vw->_simulation_bar) ? vw->_simulation_bar->h() : 0));
		vw->_simulation_bar->resize(vw->_simulation_bar->x() + dx, vw->_simulation_bar->y(),
			vw->_simulation_bar->w() - dx, vw->_simulation_bar->h());
	}
	else {
		vw->overview_area(false);
		vw->remove(vw->_sidebar);
		vw->_model_area->resize(vw->_model_area->x() - dx, vw->_model_area->y(),
			vw->_model_area->w() + dx, vw->_model_area->h());
		vw->_simulation_bar->resize(vw->_simulation_bar->x() - dx, vw->_simulation_bar->y(),
			vw->_simulation_bar->w() + dx, vw->_simulation_bar->h());
	}
	vw->redraw();
}

void Viz_Window::status_bar_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int dy = vw->_status_bar->h();
	if (m->mvalue()->value()) {
		vw->add(vw->_status_bar);
		vw->_status_bar->resize(vw->_status_bar->x(), vw->h() - dy, vw->w(), dy);
		vw->_sidebar->size(vw->_sidebar->w(), vw->_sidebar->h() - dy);
		vw->_model_area->size(vw->_model_area->w(), vw->_model_area->h() - dy);
		vw->_simulation_bar->position(vw->_simulation_bar->x(), vw->_simulation_bar->y() - dy);
	}
	else {
		vw->remove(vw->_status_bar);
		vw->_sidebar->size(vw->_sidebar->w(), vw->_sidebar->h() + dy);
		vw->_model_area->size(vw->_model_area->w(), vw->_model_area->h() + dy);
		vw->_simulation_bar->position(vw->_simulation_bar->x(), vw->_simulation_bar->y() + dy);
	}
	vw->redraw();
}

void Viz_Window::large_icon_cb(Fl_Menu_ *, Viz_Window *vw) {
	vw->_toolbar->alternate();
	int n = vw->_toolbar->children();
	for (int i = 0; i < n; i++) {
		Fl_Widget *w = vw->_toolbar->child(i);
		Alt_Widget *aw = dynamic_cast<Alt_Widget *>(w);
		if (aw) { aw->alternate(); }
	}
	if (vw->contains(vw->_toolbar)) {
		int dh = vw->_toolbar->alt_h() - vw->_toolbar->h();
		vw->_sidebar->resize(vw->_sidebar->x(), vw->_sidebar->y() - dh,
			vw->_sidebar->w(), vw->_sidebar->h() + dh);
		vw->_model_area->resize(vw->_model_area->x(), vw->_model_area->y() - dh,
			vw->_model_area->w(), vw->_model_area->h() + dh);
	}
	vw->_toolbar->init_sizes();
	vw->init_sizes();
	vw->redraw();
}

void Viz_Window::show_axes_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().show_axes(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::show_axis_labels_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().show_axis_labels(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::show_bulletin_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().show_bulletin(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::show_fps_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().show_fps(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::invert_background_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().invert_background(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::transparent_cb(Fl_Menu_ *m, Viz_Window *vw) {
	double alpha = !!m->mvalue()->value() ? 0.75 : 1.0;
#if defined(_WIN32)
	HWND hwnd = fl_xid(vw);
	LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if (!(exstyle & WS_EX_LAYERED)) {
		SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle | WS_EX_LAYERED);
	}
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)(alpha * 0xFF), LWA_ALPHA);
#elif defined(__APPLE__)
	setWindowTransparency(vw, alpha);
#else
	Atom atom = XInternAtom(fl_display, "_NET_WM_WINDOW_OPACITY", False);
	uint32_t opacity = (uint32_t)(UINT32_MAX * alpha);
	XChangeProperty(fl_display, fl_xid(vw), atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1);
#endif
}

void Viz_Window::full_screen_cb(Fl_Menu_ *m, Viz_Window *vw) {
	if (m->mvalue()->value()) {
		vw->_wx = vw->x(); vw->_wy = vw->y();
		vw->_ww = vw->w(); vw->_wh = vw->h();
		vw->fullscreen();
	}
	else {
		vw->fullscreen_off(vw->_wx, vw->_wy, vw->_ww, vw->_wh);
	}
}

void Viz_Window::axon_conns_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().axon_conns(!!v);
	vw->_axon_conns_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::axon_conns_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_axon_conns_tb->value();
	vw->_model_area->draw_options().axon_conns(!!v);
	Fl_Menu_Item *mi = vw->_axon_conns_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::den_conns_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().den_conns(!!v);
	vw->_den_conns_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::den_conns_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_den_conns_tb->value();
	vw->_model_area->draw_options().den_conns(!!v);
	Fl_Menu_Item *mi = vw->_den_conns_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::gap_junctions_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().gap_junctions(!!v);
	vw->_gap_junctions_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::gap_junctions_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_gap_junctions_tb->value();
	vw->_model_area->draw_options().gap_junctions(!!v);
	Fl_Menu_Item *mi = vw->_gap_junctions_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::neur_fields_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().neur_fields(!!v);
	vw->_neur_fields_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::neur_fields_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_neur_fields_tb->value();
	vw->_model_area->draw_options().neur_fields(!!v);
	Fl_Menu_Item *mi = vw->_neur_fields_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::conn_fields_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().conn_fields(!!v);
	vw->_conn_fields_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::conn_fields_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_conn_fields_tb->value();
	vw->_model_area->draw_options().conn_fields(!!v);
	Fl_Menu_Item *mi = vw->_conn_fields_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::syn_dots_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().syn_dots(!!v);
	vw->_syn_dots_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::syn_dots_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_syn_dots_tb->value();
	vw->_model_area->draw_options().syn_dots(!!v);
	Fl_Menu_Item *mi = vw->_syn_dots_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_axon_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().to_axon(!!v);
	vw->_to_axon_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_axon_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_to_axon_tb->value();
	vw->_model_area->draw_options().to_axon(!!v);
	Fl_Menu_Item *mi = vw->_to_axon_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_via_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().to_via(!!v);
	vw->_to_via_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_via_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_to_via_tb->value();
	vw->_model_area->draw_options().to_via(!!v);
	Fl_Menu_Item *mi = vw->_to_via_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_synapse_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().to_synapse(!!v);
	vw->_to_synapse_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_synapse_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_to_synapse_tb->value();
	vw->_model_area->draw_options().to_synapse(!!v);
	Fl_Menu_Item *mi = vw->_to_synapse_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_den_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().to_den(!!v);
	vw->_to_den_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::to_den_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_to_den_tb->value();
	vw->_model_area->draw_options().to_den(!!v);
	Fl_Menu_Item *mi = vw->_to_den_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::allow_letters_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().allow_letters(!!v);
	vw->_allow_letters_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::allow_letters_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_allow_letters_tb->value();
	vw->_model_area->draw_options().allow_letters(!!v);
	Fl_Menu_Item *mi = vw->_allow_letters_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_show_selected_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().only_show_selected(!!v);
	vw->_only_show_selected_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_show_selected_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_only_show_selected_tb->value();
	vw->_model_area->draw_options().only_show_selected(!!v);
	Fl_Menu_Item *mi = vw->_only_show_selected_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_conn_selected_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().only_conn_selected(!!v);
	vw->_only_conn_selected_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_conn_selected_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_only_conn_selected_tb->value();
	vw->_model_area->draw_options().only_conn_selected(!!v);
	Fl_Menu_Item *mi = vw->_only_conn_selected_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_show_marked_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().only_show_marked(!!v);
	vw->_only_show_marked_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_show_marked_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_only_show_marked_tb->value();
	vw->_model_area->draw_options().only_show_marked(!!v);
	Fl_Menu_Item *mi = vw->_only_show_marked_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_show_clipped_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().only_show_clipped(!!v);
	vw->_only_show_clipped_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_show_clipped_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_only_show_clipped_tb->value();
	vw->_model_area->draw_options().only_show_clipped(!!v);
	Fl_Menu_Item *mi = vw->_only_show_clipped_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::only_enable_clipped_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().only_enable_clipped(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::orthographic_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().orthographic(!!v);
	vw->_orthographic_tb->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::orthographic_tb_cb(Toolbar_Toggle_Button *, Viz_Window *vw) {
	int v = vw->_orthographic_tb->value();
	vw->_model_area->draw_options().orthographic(!!v);
	Fl_Menu_Item *mi = vw->_orthographic_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::left_handed_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().left_handed(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::select_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::SELECT);
	vw->_select_tb->setonly();
	vw->redraw();
}

void Viz_Window::select_tb_cb(Toolbar_Radio_Button *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::SELECT);
	vw->_select_mi->setonly();
	vw->redraw();
}

void Viz_Window::clip_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::CLIP);
	vw->_clip_tb->setonly();
	vw->redraw();
}

void Viz_Window::clip_tb_cb(Toolbar_Radio_Button *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::CLIP);
	vw->_clip_mi->setonly();
	vw->redraw();
}

void Viz_Window::rotate_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::ROTATE);
	vw->_rotate_tb->setonly();
	vw->redraw();
}

void Viz_Window::rotate_tb_cb(Toolbar_Radio_Button *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::ROTATE);
	vw->_rotate_mi->setonly();
	vw->redraw();
}

void Viz_Window::pan_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::PAN);
	vw->_pan_tb->setonly();
	vw->redraw();
}

void Viz_Window::pan_tb_cb(Toolbar_Radio_Button *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::PAN);
	vw->_pan_mi->setonly();
	vw->redraw();
}

void Viz_Window::zoom_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::ZOOM);
	vw->_zoom_tb->setonly();
	vw->redraw();
}

void Viz_Window::zoom_tb_cb(Toolbar_Radio_Button *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::ZOOM);
	vw->_zoom_mi->setonly();
	vw->redraw();
}

void Viz_Window::mark_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::MARK);
	vw->_mark_tb->setonly();
	vw->redraw();
}

void Viz_Window::mark_tb_cb(Toolbar_Radio_Button *, Viz_Window *vw) {
	vw->_model_area->action(Model_Area::MARK);
	vw->_mark_mi->setonly();
	vw->redraw();
}

void Viz_Window::snap_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->snap_to_axes();
	vw->redraw();
}

void Viz_Window::reset_settings_cb(Fl_Widget *, Viz_Window *vw) {
	Draw_Options &d = vw->_model_area->draw_options();
	d.axon_conns(true); vw->_axon_conns_tb->set(); vw->_axon_conns_mi->set();
	d.den_conns(false); vw->_den_conns_tb->clear(); vw->_den_conns_mi->clear();
	d.gap_junctions(true); vw->_gap_junctions_tb->set(); vw->_gap_junctions_mi->set();
	d.neur_fields(true); vw->_neur_fields_tb->set(); vw->_neur_fields_mi->set();
	d.conn_fields(false); vw->_conn_fields_tb->clear(); vw->_conn_fields_mi->clear();
	d.syn_dots(true); vw->_syn_dots_tb->set(); vw->_syn_dots_mi->set();
	d.to_axon(true); vw->_to_axon_tb->set(); vw->_to_axon_mi->set();
	d.to_via(true); vw->_to_via_tb->set(); vw->_to_via_mi->set();
	d.to_synapse(true); vw->_to_synapse_tb->set(); vw->_to_synapse_mi->set();
	d.to_den(true); vw->_to_den_tb->set(); vw->_to_den_mi->set();
	d.allow_letters(false); vw->_allow_letters_tb->clear(); vw->_allow_letters_mi->clear();
	d.only_show_selected(false); vw->_only_show_selected_tb->clear(); vw->_only_show_selected_mi->clear();
	d.only_conn_selected(false); vw->_only_conn_selected_tb->clear(); vw->_only_conn_selected_mi->clear();
	d.only_show_marked(false); vw->_only_show_marked_tb->clear(); vw->_only_show_marked_mi->clear();
	d.only_show_clipped(true); vw->_only_show_clipped_tb->set(); vw->_only_show_clipped_mi->set();
	d.orthographic(false); vw->_orthographic_tb->clear(); vw->_orthographic_mi->clear();
	vw->_model_area->action(Model_Area::SELECT); vw->_select_tb->setonly(); vw->_select_mi->setonly();
#define VW_WITH_MENU_ITEM_CB(C, M) do { \
	Fl_Menu_Item *mi = const_cast<Fl_Menu_Item *>(vw->_menu_bar->find_item((Fl_Callback *)(C))); \
	if (mi != NULL) { (mi->M)(); } \
	} while (false)
#pragma warning(push)
#pragma warning(disable: 4127) // Disable "conditional expression is constant" warning due to "while (false)" in _WITH_MENU_ITEM_CB
	d.only_enable_clipped(false); VW_WITH_MENU_ITEM_CB(only_enable_clipped_cb, clear);
	d.left_handed(false); VW_WITH_MENU_ITEM_CB(left_handed_cb, clear);
	d.show_rotation_guide(true); VW_WITH_MENU_ITEM_CB(show_rotation_guide_cb, set);
	d.show_axes(true); VW_WITH_MENU_ITEM_CB(show_axes_cb, set);
	d.show_axis_labels(true); VW_WITH_MENU_ITEM_CB(show_axis_labels_cb, set);
	d.show_bulletin(false); VW_WITH_MENU_ITEM_CB(show_bulletin_cb, clear);
	d.show_fps(false); VW_WITH_MENU_ITEM_CB(show_fps_cb, clear);
	d.invert_background(false); VW_WITH_MENU_ITEM_CB(invert_background_cb, clear);
	d.display_value_for_somas(false); VW_WITH_MENU_ITEM_CB(display_value_for_somas_cb, clear);
	vw->_display_value_for_somas->clear();
	d.show_inactive_somas(true); VW_WITH_MENU_ITEM_CB(show_inactive_somas_cb, set);
	vw->_show_inactive_somas->set();
	d.show_color_scale(true); VW_WITH_MENU_ITEM_CB(show_color_scale_cb, set);
	vw->_show_color_scale->set();
#pragma warning(pop)
#undef VW_WITH_MENU_ITEM_CB
	d.weights_color_after(true); vw->_weights_color_after->set();
	rotate_3d_arcball_cb(vw->_menu_bar, vw);
	const_cast<Fl_Menu_Item *>(vw->_menu_bar->find_item((Fl_Callback *)transparent_cb))->clear();
	transparent_cb(vw->_menu_bar, vw);
#if defined(_WIN32)
	if (OS::is_classic_windows()) {
		classic_theme_cb(vw->_menu_bar, vw);
	}
	else if (OS::is_modern_windows()) {
		metro_theme_cb(vw->_menu_bar, vw);
	}
	else {
		aero_theme_cb(vw->_menu_bar, vw);
	}
#elif defined(__APPLE__)
	aqua_theme_cb(vw->_menu_bar, vw);
#else
	greybird_theme_cb(vw->_menu_bar, vw);
#endif
	vw->_model_area->refresh();
}

void Viz_Window::rotate_2d_arcball_cb(Fl_Menu_ *, Viz_Window *vw) {
	vw->_model_area->rotation_mode(Model_Area::ARCBALL_2D);
	vw->_2d_arcball_mi->setonly();
	vw->_2d_arcball_dd_mi->setonly();
	if (vw->_rotate_tb->alternated()) {
		vw->_rotate_tb->overlay_image(ROTATE_2D_LARGE_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_2D_SMALL_ICON);
	}
	else {
		vw->_rotate_tb->overlay_image(ROTATE_2D_SMALL_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_2D_LARGE_ICON);
	}
	vw->redraw();
}

void Viz_Window::rotate_3d_arcball_cb(Fl_Menu_ *, Viz_Window *vw) {
	vw->_model_area->rotation_mode(Model_Area::ARCBALL_3D);
	vw->_3d_arcball_mi->setonly();
	vw->_3d_arcball_dd_mi->setonly();
	if (vw->_rotate_tb->alternated()) {
		vw->_rotate_tb->overlay_image(ROTATE_3D_LARGE_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_3D_SMALL_ICON);
	}
	else {
		vw->_rotate_tb->overlay_image(ROTATE_3D_SMALL_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_3D_LARGE_ICON);
	}
	vw->redraw();
}

void Viz_Window::rotate_x_axis_cb(Fl_Menu_ *, Viz_Window *vw) {
	vw->_model_area->rotation_mode(Model_Area::AXIS_X);
	vw->_x_axis_mi->setonly();
	vw->_x_axis_dd_mi->setonly();
	if (vw->_rotate_tb->alternated()) {
		vw->_rotate_tb->overlay_image(ROTATE_X_LARGE_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_X_SMALL_ICON);
	}
	else {
		vw->_rotate_tb->overlay_image(ROTATE_X_SMALL_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_X_LARGE_ICON);
	}
	vw->redraw();
}

void Viz_Window::rotate_y_axis_cb(Fl_Menu_ *, Viz_Window *vw) {
	vw->_model_area->rotation_mode(Model_Area::AXIS_Y);
	vw->_y_axis_mi->setonly();
	vw->_y_axis_dd_mi->setonly();
	if (vw->_rotate_tb->alternated()) {
		vw->_rotate_tb->overlay_image(ROTATE_Y_LARGE_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_Y_SMALL_ICON);
	}
	else {
		vw->_rotate_tb->overlay_image(ROTATE_Y_SMALL_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_Y_LARGE_ICON);
	}
	vw->redraw();
}

void Viz_Window::rotate_z_axis_cb(Fl_Menu_ *, Viz_Window *vw) {
	vw->_model_area->rotation_mode(Model_Area::AXIS_Z);
	vw->_z_axis_mi->setonly();
	vw->_z_axis_dd_mi->setonly();
	if (vw->_rotate_tb->alternated()) {
		vw->_rotate_tb->overlay_image(ROTATE_Z_LARGE_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_Z_SMALL_ICON);
	}
	else {
		vw->_rotate_tb->overlay_image(ROTATE_Z_SMALL_ICON);
		vw->_rotate_tb->alt_overlay_image(ROTATE_Z_LARGE_ICON);
	}
	vw->redraw();
}

void Viz_Window::show_rotation_guide_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int g = m->mvalue()->value();
	vw->_model_area->draw_options().show_rotation_guide(!!g);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::scale_rotation_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int s = m->mvalue()->value();
	vw->_model_area->scale_rotation(!!s);
	vw->redraw();
}

void Viz_Window::invert_zoom_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int z = m->mvalue()->value();
	vw->_model_area->invert_zoom(!!z);
	vw->redraw();
}

void Viz_Window::help_cb(Fl_Widget *, Viz_Window *vw) { vw->_help_window->show(vw); }

void Viz_Window::about_cb(Fl_Widget *, Viz_Window *vw) { vw->_about_dialog->show(vw); }

void Viz_Window::classic_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_classic_theme();
	vw->_classic_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::aero_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_aero_theme();
	vw->_aero_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::metro_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_metro_theme();
	vw->_metro_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::aqua_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_aqua_theme();
	vw->_aqua_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::greybird_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_greybird_theme();
	vw->_greybird_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::metal_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_metal_theme();
	vw->_metal_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::blue_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_blue_theme();
	vw->_blue_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::dark_theme_cb(Fl_Menu_ *, Viz_Window *vw) {
	OS::use_dark_theme();
	vw->_dark_theme_mi->setonly();
	vw->refresh_config();
	vw->_overview_area->refresh();
	vw->redraw();
}

void Viz_Window::overview_area_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->overview_area(!!v);
	vw->_overview_heading->value(v);
	vw->_overview_area->refresh();
}

void Viz_Window::overview_area_sb_cb(Expander_Collapser *w, Viz_Window *vw) {
	int v = w->value();
	vw->overview_area(!!v);
	Fl_Menu_Item *mi = vw->_overview_area_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_overview_area->refresh();
}

void Viz_Window::overview_zoom_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_overview_area->zoom(vw->_overview_zoom->value());
	vw->_overview_area->invalidate();
	vw->_overview_area->redraw();
}

void Viz_Window::soma_letter_size_cb(Default_Value_Slider *s, Viz_Window *vw) {
	double v = s->value();
	Soma::soma_letter_size((int)v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::type_get_cb(Fl_Widget *, Viz_Window *vw) {
	size8_t index = (size8_t)vw->_type_choice->value();
	Soma_Type *t = vw->_model_area->model().type(index);
	switch (t->display_state()) {
	case Soma_Type::LETTER:
		vw->_type_is_letter->setonly();
		break;
	case Soma_Type::DOT:
		vw->_type_is_dot->setonly();
		break;
	case Soma_Type::HIDDEN:
		vw->_type_is_hidden->setonly();
		break;
	case Soma_Type::DISABLED:
		vw->_type_is_disabled->setonly();
		break;
	}
	const Color *c = t->color();
	vw->_type_color_choice->value((int)c->index());
	vw->_type_color_swatch->color(c->as_fl_color());
	vw->redraw();
}

void Viz_Window::type_set_cb(Fl_Widget *, Viz_Window *vw) {
	size8_t index = (size8_t)vw->_type_choice->value();
	Soma_Type *t = vw->_model_area->model().type(index);
	if (vw->_type_is_letter->value()) {
		t->display_state(Soma_Type::LETTER);
	}
	else if (vw->_type_is_dot->value()) {
		t->display_state(Soma_Type::DOT);
	}
	else if (vw->_type_is_hidden->value()) {
		t->display_state(Soma_Type::HIDDEN);
	}
	else if (vw->_type_is_disabled->value()) {
		t->display_state(Soma_Type::DISABLED);
	}
	const Color *c = Color::color((size8_t)vw->_type_color_choice->value());
	t->color(c);
	vw->_type_color_swatch->color(c->as_fl_color());
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::all_types_use_letter(Fl_Widget *, Viz_Window *vw) {
	Brain_Model &bm = vw->_model_area->model();
	size8_t n = bm.num_types();
	for (size8_t i = 0; i < n; i++) {
		Soma_Type *t = bm.type(i);
		t->display_state(Soma_Type::LETTER);
	}
	vw->_type_is_letter->setonly();
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::all_types_use_dot(Fl_Widget *, Viz_Window *vw) {
	Brain_Model &bm = vw->_model_area->model();
	size8_t n = bm.num_types();
	for (size8_t i = 0; i < n; i++) {
		Soma_Type *t = bm.type(i);
		t->display_state(Soma_Type::DOT);
	}
	vw->_type_is_dot->setonly();
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::all_types_hidden(Fl_Widget *, Viz_Window *vw) {
	Brain_Model &bm = vw->_model_area->model();
	size8_t n = bm.num_types();
	for (size8_t i = 0; i < n; i++) {
		Soma_Type *t = bm.type(i);
		t->display_state(Soma_Type::HIDDEN);
	}
	vw->_type_is_hidden->setonly();
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::all_types_disabled(Fl_Widget *, Viz_Window *vw) {
	Brain_Model &bm = vw->_model_area->model();
	size8_t n = bm.num_types();
	for (size8_t i = 0; i < n; i++) {
		Soma_Type *t = bm.type(i);
		t->display_state(Soma_Type::DISABLED);
	}
	vw->_type_is_disabled->setonly();
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::select_id_cb(Fl_Widget *, Viz_Window *vw) {
	size32_t id = (size32_t)vw->_select_id_spinner->value();
	vw->_model_area->select_id(id);
	vw->_model_area->refresh();
	vw->refresh_selected();
	vw->redraw();
}

void Viz_Window::deselect_id_cb(Fl_Widget *, Viz_Window *vw) {
	size32_t id = (size32_t)vw->_select_id_spinner->value();
	vw->_model_area->deselect_id(id);
	vw->_model_area->refresh();
	vw->refresh_selected();
	vw->redraw();
}

void Viz_Window::select_syn_count_cb(Fl_Widget *, Viz_Window *vw) {
	size8_t t_index = (size8_t)vw->_select_type_choice->value();
	size_t y_count = (size_t)vw->_syn_count_spinner->value();
	bool count_den = !!vw->_syn_kind->value();
	vw->_progress_dialog->title(count_den ? "Counting somas' dendritic synapses..." : "Counting somas' axonal synapses...");
	vw->_progress_dialog->show(vw);
	size32_t n_found = vw->_model_area->select_syn_count(t_index, y_count, count_den, vw->_progress_dialog);
	vw->_progress_dialog->hide();
	if (vw->_progress_dialog->canceled()) {
		std::string msg = "Canceled selecting somas by synapse count!";
		vw->_warning_dialog->message(msg);
		vw->_warning_dialog->show(vw);
	}
	else if (n_found == 0) {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		const Soma_Type *t = vw->_model_area->const_model().type(t_index);
		ss << "No " << t->name() << " somas exist with " << y_count << " " << (count_den ? "dendritic" : "axonal") <<
			" synapse" << (y_count != 1 ? "s" : "") << "!";
		vw->_warning_dialog->message(ss.str());
		vw->_warning_dialog->show(vw);
	}
	else {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		const Soma_Type *t = vw->_model_area->const_model().type(t_index);
		ss << "Found " << n_found << " " << t->name() << " soma" << (n_found != 1 ? "s" : "") << " with " << y_count <<
			" " << (count_den ? "dendritic" : "axonal") << " synapse" << (y_count != 1 ? "s" : "") << "!";
		vw->_success_dialog->message(ss.str());
		vw->_success_dialog->show(vw);
	}
	vw->_model_area->refresh();
	vw->refresh_selected();
	vw->redraw();
}

void Viz_Window::report_syn_count_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_text_report_chooser->preset_file("viz_somas_by_syn_count.txt");
	int status = vw->_text_report_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_text_report_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	size8_t t_index = (size8_t)vw->_select_type_choice->value();
	size_t y_count = (size_t)vw->_syn_count_spinner->value();
	bool count_den = !!vw->_syn_kind->value();
	vw->_progress_dialog->title(count_den ? "Counting somas' dendritic synapses..." : "Counting somas' axonal synapses...");
	vw->_progress_dialog->show(vw);
	size32_t n_found = vw->_model_area->report_syn_count(ofs, t_index, y_count, count_den, vw->_progress_dialog);
	vw->_progress_dialog->hide();
	if (vw->_progress_dialog->canceled()) {
		std::string msg = "Canceled selecting somas by synapse count!";
		vw->_warning_dialog->message(msg);
		vw->_warning_dialog->show(vw);
	}
	else if (n_found == 0) {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		const Soma_Type *t = vw->_model_area->const_model().type(t_index);
		ss << "No " << t->name() << " somas exist with " << y_count << " " << (count_den ? "dendritic" : "axonal") <<
			" synapse" << (y_count != 1 ? "s" : "") << "!";
		vw->_warning_dialog->message(ss.str());
		vw->_warning_dialog->show(vw);
	}
	else {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		const Soma_Type *t = vw->_model_area->const_model().type(t_index);
		ss << "Found " << n_found << " " << t->name() << " soma" << (n_found != 1 ? "s" : "") << " with " << y_count <<
			" " << (count_den ? "dendritic" : "axonal") << " synapse" << (y_count != 1 ? "s" : "") << "!\n";
		ss << "Saved report to " << basename << "!";
		vw->_success_dialog->message(ss.str());
		vw->_success_dialog->show(vw);
	}
	vw->_model_area->refresh();
	vw->refresh_selected();
	vw->redraw();
}

void Viz_Window::mark_conn_paths_cb(Fl_Widget *, Viz_Window *vw) {
	size32_t a_id = (size32_t)vw->_axon_id_spinner->value();
	size32_t d_id = (size32_t)vw->_den_id_spinner->value();
	size_t limit = (size_t)vw->_limit_paths_spinner->value();
	bool include_disabled = !!vw->_include_disabled->value();
	vw->_waiting_dialog->title("Traversing dendritic trees...");
	vw->_waiting_dialog->show(vw);
	size_t n_paths = vw->_model_area->mark_conn_paths(a_id, d_id, limit, include_disabled, vw->_waiting_dialog);
	vw->_waiting_dialog->hide();
	if (vw->_waiting_dialog->canceled()) {
		std::string msg = "Canceled searching for synapse paths!";
		vw->_warning_dialog->message(msg);
		vw->_warning_dialog->show(vw);
	}
	else if (n_paths == 0) {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "No synapse paths exist from soma #" << a_id << " to #" << d_id << "!";
		vw->_warning_dialog->message(ss.str());
		vw->_warning_dialog->show(vw);
	}
	else {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "Found " << n_paths << " path" << (n_paths != 1 ? "s" : "") << " from soma #" << a_id << " to #" << d_id << "!";
		vw->_success_dialog->message(ss.str());
		vw->_success_dialog->show(vw);
	}
	vw->_model_area->refresh();
	vw->refresh_selected();
	vw->redraw();
}

void Viz_Window::select_conn_paths_cb(Fl_Widget *, Viz_Window *vw) {
	size32_t a_id = (size32_t)vw->_axon_id_spinner->value();
	size32_t d_id = (size32_t)vw->_den_id_spinner->value();
	size_t limit = (size_t)vw->_limit_paths_spinner->value();
	bool include_disabled = !!vw->_include_disabled->value();
	vw->_waiting_dialog->title("Traversing dendritic trees...");
	vw->_waiting_dialog->show(vw);
	size_t n_paths = vw->_model_area->select_conn_paths(a_id, d_id, limit, include_disabled, vw->_waiting_dialog);
	vw->_waiting_dialog->hide();
	if (vw->_waiting_dialog->canceled()) {
		std::string msg = "Canceled searching for synapse paths!";
		vw->_warning_dialog->message(msg);
		vw->_warning_dialog->show(vw);
	}
	else if (n_paths == 0) {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "No synapse paths exist from soma #" << a_id << " to #" << d_id << "!";
		vw->_warning_dialog->message(ss.str());
		vw->_warning_dialog->show(vw);
	}
	else {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "Found " << n_paths << " path" << (n_paths != 1 ? "s" : "") << " from soma #" << a_id << " to #" << d_id << "!";
		vw->_success_dialog->message(ss.str());
		vw->_success_dialog->show(vw);
	}
	vw->_model_area->refresh();
	vw->refresh_selected();
	vw->redraw();
}

void Viz_Window::report_conn_paths_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_text_report_chooser->preset_file("viz_connecting_paths.txt");
	int status = vw->_text_report_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_text_report_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
		return;
	}
	size32_t a_id = (size32_t)vw->_axon_id_spinner->value();
	size32_t d_id = (size32_t)vw->_den_id_spinner->value();
	size_t limit = (size_t)vw->_limit_paths_spinner->value();
	bool include_disabled = !!vw->_include_disabled->value();
	vw->_waiting_dialog->title("Traversing dendritic trees...");
	vw->_waiting_dialog->show(vw);
	size_t n_paths = vw->_model_area->report_conn_paths(ofs, a_id, d_id, limit, include_disabled,
		vw->_waiting_dialog);
	vw->_waiting_dialog->hide();
	if (vw->_waiting_dialog->canceled()) {
		std::string msg = "Canceled searching for synapse paths!";
		vw->_warning_dialog->message(msg);
		vw->_warning_dialog->show(vw);
	}
	else if (n_paths == 0) {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "No synapse paths exist from soma #" << a_id << " to #" << d_id << "!";
		vw->_warning_dialog->message(ss.str());
		vw->_warning_dialog->show(vw);
	}
	else {
		std::ostringstream ss;
		ss.imbue(std::locale(""));
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss << "Found " << n_paths << " path" << (n_paths != 1 ? "s" : "") << " from soma #" << a_id << " to #" << d_id << "!\n";
		ss << "Saved report to " << basename << "!";
		vw->_success_dialog->message(ss.str());
		vw->_success_dialog->show(vw);
	}
	vw->_model_area->refresh();
	vw->refresh_selected();
	vw->redraw();
}

void Viz_Window::fetch_select_id_cb(Fl_Widget *, Viz_Window *vw) {
	const Model_State &ms = vw->_model_area->const_state();
	const Soma *s = ms.selected(vw->_shown_selected);
	vw->_select_id_spinner->value((double)s->id());
	vw->redraw();
}

void Viz_Window::fetch_axon_id_cb(Fl_Widget *, Viz_Window *vw) {
	const Model_State &ms = vw->_model_area->const_state();
	const Soma *s = ms.selected(vw->_shown_selected);
	vw->_axon_id_spinner->value((double)s->id());
	vw->redraw();
}

void Viz_Window::fetch_den_id_cb(Fl_Widget *, Viz_Window *vw) {
	const Model_State &ms = vw->_model_area->const_state();
	const Soma *s = ms.selected(vw->_shown_selected);
	vw->_den_id_spinner->value((double)s->id());
	vw->redraw();
}

void Viz_Window::report_synapses_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_text_report_chooser->preset_file("viz_selected_synapses.txt");
	int status = vw->_text_report_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_text_report_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		vw->_model_area->write_selected_synapses_to(ofs);
		std::string msg = "Saved report to ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::report_marked_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_text_report_chooser->preset_file("viz_marked_synapses.txt");
	int status = vw->_text_report_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_text_report_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		vw->_model_area->write_marked_synapses_to(ofs);
		std::string msg = "Saved report to ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::report_selected_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_report_somas_dialog->show(vw);
	bool canceled = vw->_report_somas_dialog->canceled();
	if (canceled) { return; }
	bool bounding_box = vw->_report_somas_dialog->report_bounding_box(),
		relations = vw->_report_somas_dialog->report_relations();
	vw->_text_report_chooser->preset_file("viz_selected_cells.txt");
	int status = vw->_text_report_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_text_report_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		vw->_model_area->write_selected_somas_to(ofs, bounding_box, relations);
		std::string msg = "Saved report to ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::prev_selected_cb(Fl_Widget *, Viz_Window *vw) {
	size_t n = vw->_model_area->const_state().num_selected();
	if (n < 2) { return; }
	if (!vw->_shown_selected) { vw->_shown_selected = n - 1; }
	else { vw->_shown_selected--; }
	vw->refresh_selected(false);
}

void Viz_Window::next_selected_cb(Fl_Widget *, Viz_Window *vw) {
	size_t n = vw->_model_area->const_state().num_selected();
	if (n < 2) { return; }
	if (vw->_shown_selected == n - 1) { vw->_shown_selected = 0; }
	else { vw->_shown_selected++; }
	vw->refresh_selected(false);
}

void Viz_Window::deselect_shown_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_model_area->deselect_ith(vw->_shown_selected);
	size_t n = vw->_model_area->const_state().num_selected();
	if (vw->_shown_selected == n) { vw->_shown_selected = 0; }
	vw->refresh_selected(false);
}

void Viz_Window::pivot_shown_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	vw->_model_area->pivot_ith(vw->_shown_selected);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::summary_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->_model_area->opened()) { return; }
	const Model_State &ms = vw->_model_area->const_state();
	if (!ms.num_selected()) {
		vw->summary_dialog();
	}
	else {
		const Soma *sel = ms.selected(vw->_shown_selected);
		size32_t sel_index = ms.selected_index(vw->_shown_selected);
		vw->summary_dialog(sel, sel_index);
	}
}

void Viz_Window::set_simulation_display_cb(Fl_Menu_ *m, Viz_Window *vw) {
	OS_Radio_Button *b = NULL;
	const Fl_Menu_Item *mi = m ? m->mvalue() : NULL;
	if (mi == vw->_display_static_model_mi) {
		b = vw->_display_static_model;
	}
	else if (mi == vw->_display_firing_spikes_mi) {
		b = vw->_display_firing_spikes;
	}
	else if (mi == vw->_display_voltages_mi) {
		b = vw->_display_voltages;
	}
	else if (mi == vw->_display_weights_mi) {
		b = vw->_display_weights;
	}
	set_simulation_display_tb_cb(b, vw);
}

void Viz_Window::set_simulation_display_tb_cb(OS_Radio_Button *b, Viz_Window *vw) {
	if (b == vw->_display_static_model) {
		vw->_model_area->draw_options().display(Draw_Options::STATIC_MODEL);
		vw->_display_static_model->setonly();
		vw->_display_static_model_mi->setonly();
		vw->refresh_static_model();
	}
	else if (b == vw->_display_firing_spikes) {
		vw->_model_area->draw_options().display(Draw_Options::FIRING_SPIKES);
		vw->_display_firing_spikes->setonly();
		vw->_display_firing_spikes_mi->setonly();
		vw->refresh_firing_spikes();
	}
	else if (b == vw->_display_voltages) {
		vw->_model_area->draw_options().display(Draw_Options::VOLTAGES);
		vw->_display_voltages->setonly();
		vw->_display_voltages_mi->setonly();
		vw->refresh_voltages();
	}
	else if (b == vw->_display_weights) {
		vw->_model_area->draw_options().display(Draw_Options::WEIGHTS);
		vw->_display_weights->setonly();
		vw->_display_weights_mi->setonly();
		vw->refresh_weights();
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::display_value_for_somas_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().display_value_for_somas(!!v);
	vw->_display_value_for_somas->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::display_value_for_somas_tb_cb(OS_Check_Button *, Viz_Window *vw) {
	int v = vw->_display_value_for_somas->value();
	vw->_model_area->draw_options().display_value_for_somas(!!v);
	Fl_Menu_Item *mi = vw->_display_value_for_somas_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::show_inactive_somas_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().show_inactive_somas(!!v);
	vw->_show_inactive_somas->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::show_inactive_somas_tb_cb(OS_Check_Button *, Viz_Window *vw) {
	int v = vw->_show_inactive_somas->value();
	vw->_model_area->draw_options().show_inactive_somas(!!v);
	Fl_Menu_Item *mi = vw->_show_inactive_somas_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::show_color_scale_cb(Fl_Menu_ *m, Viz_Window *vw) {
	int v = m->mvalue()->value();
	vw->_model_area->draw_options().show_color_scale(!!v);
	vw->_show_color_scale->value(v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::show_color_scale_tb_cb(OS_Check_Button *, Viz_Window *vw) {
	int v = vw->_show_color_scale->value();
	vw->_model_area->draw_options().show_color_scale(!!v);
	Fl_Menu_Item *mi = vw->_show_color_scale_mi;
	if (mi) {
		if (v) { mi->set(); }
		else { mi->clear(); }
	}
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::play_pause_firing_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->contains(vw->_simulation_bar)) { return; }
	if (vw->_playing) {
		vw->_play_pause_firing->image(PLAY_ICON);
		vw->_play_pause_firing->tooltip("Play (.)");
		Fl::remove_timeout((Fl_Timeout_Handler)do_step_firing_time_cb, vw);
		vw->_playing = false;
	}
	else {
		// Don't play while showing static model
		if (vw->_model_area->draw_options().display() == Draw_Options::STATIC_MODEL) {
			set_simulation_display_tb_cb(vw->_display_firing_spikes, vw);
		}
		// Rewind playback to start if already at end
		const Firing_Spikes *fd = vw->_model_area->const_model().const_firing_spikes();
		if (fd->time() == fd->max_time()) {
				vw->_firing_time_spinner->value(0.0);
				firing_start_time_cb(vw->_firing_time_spinner, vw);
		}
		vw->_play_pause_firing->image(PAUSE_ICON);
		vw->_play_pause_firing->tooltip("Pause (.)");
		Fl::add_timeout(0.0, (Fl_Timeout_Handler)do_step_firing_time_cb, vw);
		vw->_playing = true;
	}
	vw->redraw();
}

void Viz_Window::step_firing_cb(Fl_Widget *, Viz_Window *vw) {
	if (!vw->contains(vw->_simulation_bar)) { return; }
	size32_t t = vw->_model_area->model().step_time();
	vw->_firing_time_spinner->value((double)t);
	vw->_firing_time_slider->value((double)t);
	vw->_model_area->refresh();
	vw->refresh_selected_sim_data();
	vw->redraw();
}

void Viz_Window::firing_start_time_cb(Fl_Widget *w, Viz_Window *vw) {
	double ut = w == vw->_firing_time_spinner ? vw->_firing_time_spinner->value() : vw->_firing_time_slider->value();
	size32_t tt = vw->_model_area->model().start_time((size32_t)ut);
	vw->_firing_time_spinner->value((double)tt);
	vw->_firing_time_slider->value((double)tt);
	vw->_model_area->refresh();
	vw->refresh_selected_sim_data();
	vw->redraw();
}

void Viz_Window::do_step_firing_time_cb(Viz_Window *vw) {
	const Firing_Spikes *fd = vw->_model_area->const_model().const_firing_spikes();
	size32_t t = fd->time();
	if (t == fd->max_time()) {
		play_pause_firing_cb(vw->_play_pause_firing, vw);
		return;
	}
	step_firing_cb(vw->_step_firing, vw);
	double r = FIRING_SPEED_TIMEOUTS[(int)(vw->_firing_speed_spinner->value() - 1.0)];
	Fl::repeat_timeout(r, (Fl_Timeout_Handler)do_step_firing_time_cb, vw);
}

void Viz_Window::firing_report_current_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_text_report_chooser->preset_file("viz_current_frequencies.txt");
	int status = vw->_text_report_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_text_report_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		vw->_model_area->write_current_frequencies_to(ofs);
		std::string msg = "Saved report to ";
		msg = msg + basename + "!";
		vw->_success_dialog->message(msg);
		vw->_success_dialog->show(vw);
	}
}

void Viz_Window::firing_report_average_cb(Fl_Widget *, Viz_Window *vw) {
	vw->_report_averages_dialog->limit_spinners(vw->_model_area->model().const_firing_spikes());
	vw->_report_averages_dialog->show(vw);
	bool canceled = vw->_report_averages_dialog->canceled();
	if (canceled) { return; }
	vw->_text_report_chooser->preset_file("viz_average_frequencies.txt");
	int status = vw->_text_report_chooser->show();
	if (status == 1) { return; }
	const char *filename = vw->_text_report_chooser->filename();
	const char *basename = fl_filename_name(filename);
	std::ofstream ofs(filename);
	if (!ofs.good()) {
		std::string msg = "Could not write to ";
		msg = msg + basename + "!";
		vw->_error_dialog->message(msg);
		vw->_error_dialog->show(vw);
	}
	else {
		size32_t start = vw->_report_averages_dialog->start_time();
		size32_t stop = vw->_report_averages_dialog->stop_time();
		size32_t step = vw->_report_averages_dialog->step_time();
		vw->_progress_dialog->title("Averaging...");
		vw->_progress_dialog->show(vw);
		vw->_model_area->write_average_frequencies_to(ofs, start, stop, step, vw->_progress_dialog);
		vw->_progress_dialog->hide();
		if (vw->_progress_dialog->canceled()) {
			std::string msg = "Canceled reporting average soma frequencies!";
			vw->_warning_dialog->message(msg);
			vw->_warning_dialog->show(vw);
		}
		else {
			std::string msg = "Saved report to ";
			msg = msg + basename + "!";
			vw->_success_dialog->message(msg);
			vw->_success_dialog->show(vw);
		}
	}
}

void Viz_Window::firing_select_top_cb(Fl_Widget *, Viz_Window *vw) {
	int n = (int)vw->_firing_select_top_spinner->value();
	vw->_model_area->select_top(n);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::voltages_graph_selected_cb(Fl_Widget *, Viz_Window *vw) {
	const Model_State &ms = vw->_model_area->const_state();
	if (vw->_shown_selected >= ms.num_selected()) { return; }
	const Brain_Model &bm = vw->_model_area->const_model();
	const Soma *sel = ms.selected(vw->_shown_selected);
	size32_t sel_index = ms.selected_index(vw->_shown_selected);
	const Voltages *v = bm.const_voltages();
	if (!v->active(sel_index)) { return; }
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss << "Soma #" << sel->id() << " Voltage over Time - ";
	bm.print(ss);
	ss << " - " << PROGRAM_NAME;
#ifdef LARGE_INTERFACE
	Voltage_Graph_Window vgw(32, 32, 960, 480, ss.str().c_str());
#else
	Voltage_Graph_Window vgw(32, 32, 720, 360, ss.str().c_str());
#endif
	vgw.soma(sel->id(), sel_index);
	vgw.voltages(v);
	vgw.show(vw);
}

void Viz_Window::weights_prev_change_cb(Fl_Widget *, Viz_Window *vw) {
	size32_t t = vw->_model_area->const_model().const_weights()->prev_change_time();
	vw->_firing_time_spinner->value(t);
	firing_start_time_cb(vw->_firing_time_spinner, vw);
}

void Viz_Window::weights_next_change_cb(Fl_Widget *, Viz_Window *vw) {
	size32_t t = vw->_model_area->const_model().const_weights()->next_change_time();
	vw->_firing_time_spinner->value(t);
	firing_start_time_cb(vw->_firing_time_spinner, vw);
}

void Viz_Window::weights_prev_selected_cb(Fl_Widget *, Viz_Window *vw) {
	const Model_State &ms = vw->_model_area->const_state();
	if (!ms.num_selected()) { return; }
	size32_t sel_index = ms.selected_index(vw->_shown_selected);
	size32_t t = vw->_model_area->const_model().const_weights()->prev_change_time(sel_index);
	vw->_firing_time_spinner->value(t);
	firing_start_time_cb(vw->_firing_time_spinner, vw);
}

void Viz_Window::weights_next_selected_cb(Fl_Widget *, Viz_Window *vw) {
	const Model_State &ms = vw->_model_area->const_state();
	if (!ms.num_selected()) { return; }
	size32_t sel_index = ms.selected_index(vw->_shown_selected);
	size32_t t = vw->_model_area->const_model().const_weights()->next_change_time(sel_index);
	vw->_firing_time_spinner->value(t);
	firing_start_time_cb(vw->_firing_time_spinner, vw);
}

void Viz_Window::weights_color_after_cb(Toggle_Switch *, Viz_Window *vw) {
	int v = vw->_weights_color_after->value();
	vw->_model_area->draw_options().weights_color_after(!!v);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::weights_scale_center_cb(Fl_Widget *w, Viz_Window *vw) {
	double c = w == vw->_weights_scale_center_spinner ? vw->_weights_scale_center_spinner->value() :
		vw->_weights_scale_center_slider->value();
	vw->_model_area->model().weights()->center((float)c);
	vw->_weights_scale_center_spinner->value(c);
	vw->_weights_scale_center_slider->value(c);
	vw->_model_area->refresh();
	vw->redraw();
}

void Viz_Window::weights_scale_spread_cb(Fl_Widget *w, Viz_Window *vw) {
	double s = w == vw->_weights_scale_spread_spinner ? vw->_weights_scale_spread_spinner->value() :
		vw->_weights_scale_spread_slider->value();
	vw->_model_area->model().weights()->spread((float)s);
	vw->_weights_scale_spread_spinner->value(s);
	vw->_weights_scale_spread_slider->value(s);
	vw->_model_area->refresh();
	vw->redraw();
}
