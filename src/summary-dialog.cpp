#include <string>
#include <sstream>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "algebra.h"
#include "os-themes.h"
#include "coords.h"
#include "soma.h"
#include "synapse.h"
#include "soma-type.h"
#include "brain-model.h"
#include "firing-spikes.h"
#include "voltages.h"
#include "clip-volume.h"
#include "widgets.h"
#include "summary-dialog.h"

Summary_Dialog::Summary_Dialog(std::string t) : _title(t), _model(NULL), _min_w(0), _max_w(1000), _dialog(NULL),
	_body_scroll(NULL), _body(NULL), _copy_button(NULL), _ok_button(NULL) {}

Summary_Dialog::~Summary_Dialog() {
	delete _dialog;
	delete _body_scroll;
	delete _copy_button;
	delete _ok_button;
}

void Summary_Dialog::initialize() {
	if (_dialog) { return; }
	Fl_Group *prev_current = Fl_Group::current();
	Fl_Group::current(NULL);
	// Populate dialog
	_dialog = new Fl_Double_Window(0, 0, 0, 0, _title.c_str());
	_body_scroll = new Fl_Scroll(0, 0, 0, 0);
	_body = new Label(0, 0, 0, 0);
	_body_scroll->end();
	_dialog->begin();
	_copy_button = new OS_Button(0, 0, 0, 0, "Copy");
	_ok_button = new Default_Button(0, 0, 0, 0, "OK");
	_dialog->end();
	// Initialize dialog
	_dialog->resizable(NULL);
	_dialog->callback((Fl_Callback *)close_cb, this);
	_dialog->set_modal();
	// Initialize dialog's children
	_body_scroll->type(Fl_Scroll::VERTICAL);
	_body->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
	_copy_button->shortcut(FL_COMMAND + 'c');
	_copy_button->tooltip("Copy (" COMMAND_KEY_PLUS "C)");
	_copy_button->callback((Fl_Callback *)copy_cb, this);
	_ok_button->tooltip("OK (" ENTER_KEY_NAME ")");
	_ok_button->callback((Fl_Callback *)close_cb, this);
	Fl_Group::current(prev_current);
}

static const char *BULLET = "\xE2\x80\xA2"; // UTF-8 encoding of U+2022 "BULLET"
static const char *SUB_BULLET = "\xE2\x97\xA6"; // UTF-8 encoding of U+25E6 "WHITE BULLET"
static const char *DELTA = "\xCE\x94"; // UTF-8 encoding of U+0394 "GREEK CAPITAL LETTER DELTA"

void Summary_Dialog::refresh_body(const Soma *s, size32_t index, const Clip_Volume *v) {
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(0);
	size8_t nt = _model->num_types();
	size32_t *type_counts = new(std::nothrow) size32_t[nt]();
	if (type_counts == NULL) {
		_body->copy_label("Could not summarize soma!\nNot enough memory was available.");
		return;
	}
	// Show soma's name and coordinates
	const Soma_Type *t = _model->type(s->type_index());
	ss << t->name() << " soma #" << s->id();
	const coord_t *c = s->coords();
	ss.imbue(std::locale("C"));
	ss << " at (" << c[0] << ", " << c[1] << ", " << c[2] << ")\n";
	ss.imbue(std::locale(""));
	// Show soma's neuritic field counts and gap junction counts
	ss << "Neuritic fields: " << +s->num_axon_fields() << " axonal / " << +s->num_den_fields() << " dendritic\n";
	ss << "Gap junctions: " << _model->num_gap_junctions(index) << "\n\n";
	if (_model->has_firing_spikes()) {
		const Firing_Spikes *fd = _model->const_firing_spikes();
		ss << "At cycle " << fd->time() << ":\n";
		float hz = fd->hertz(index);
		if (hz > 0.0f) {
			ss.precision(2);
			ss << " " << BULLET << " Firing at " << fd->hertz(index) << " Hz\n";
			ss.precision(0);
		}
		else {
			ss << " " << BULLET << " Not firing\n";
		}
		const Voltages *vt = _model->const_voltages();
		if (_model->has_voltages() && vt->active(index)) {
			ss.unsetf(std::ios::floatfield);
			ss.precision(5);
			ss << " " << BULLET << " Membrane potential is " << vt->voltage(index) << " mV\n";
			ss.setf(std::ios::fixed, std::ios::floatfield);
			ss.precision(0);
		}
		ss << "\n";
	}
	// Count soma's axonal connections to other somas in clip volume
	size_t count = 0;
	size32_t ny = _model->num_synapses();
	const Synapse *y = NULL;
	for (size32_t y_index = s->first_axon_syn_index(); y_index < ny; y_index = y->next_axon_syn_index()) {
		y = _model->synapse(y_index);
		Soma *o = _model->soma(y->den_soma_index());
		if (v && !v->contains(o->coords())) { continue; }
		type_counts[o->type_index()]++;
		count++;
	}
	// Show soma's axonal connections by type
	ss << "Axonal connections";
	if (v) { ss << " in clip volume"; }
	ss << ": " << count;
	if (v) { ss << " (" << s->num_axon_syns() << " total)"; }
	if (!count) { ss << "\nNone"; }
	for (size8_t i = 0; i < nt; i++) {
		// False positive "C6001: Using uninitialized memory" error with Visual Studio 2013 code analysis
		if (!type_counts[i]) { continue; }
		ss << "\n " << BULLET << " " << type_counts[i] << " to " << _model->type(i)->name();
	}
	// Count soma's dendritic connections to other somas in clip volume
	for (size8_t i = 0; i < nt; i++) { type_counts[i] = 0; }
	count = 0;
	for (size32_t y_index = s->first_den_syn_index(); y_index < ny; y_index = y->next_den_syn_index()) {
		y = _model->synapse(y_index);
		Soma *o = _model->soma(y->axon_soma_index());
		if (v && !v->contains(o->coords())) { continue; }
		type_counts[o->type_index()]++;
		count++;
	}
	// Show soma's dendritic connections by type
	ss << "\n\nDendritic connections";
	if (v) { ss << " in clip volume"; }
	ss << ": " << count;
	if (v) { ss << " (" << s->num_den_syns() << " total)"; }
	if (!count) { ss << "\nNone"; }
	for (size8_t i = 0; i < nt; i++) {
		if (!type_counts[i]) { continue; }
		ss << "\n " << BULLET << " " << type_counts[i] << " from " << _model->type(i)->name();
	}
	_body->copy_label(ss.str().c_str());
	delete [] type_counts;
}

void Summary_Dialog::refresh_body(const Synapse *y, size32_t index) {
	std::ostringstream ss;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(0);
	// Show synapse's coordinates and via coordinates
	const coord_t *c = y->coords();
	ss.imbue(std::locale("C"));
	ss << "Synapse at (" << c[0] << ", " << c[1] << ", " << c[2] << ")";
	if (y->has_via()) {
		const coord_t *v = y->via_coords();
		ss << " via (" << v[0] << ", " << v[1] << ", " << v[2] << ")";
	}
	ss << "\n";
	ss.imbue(std::locale(""));
	// Show synapse's axonal and dendritic somas
	size32_t a_index = y->axon_soma_index();
	const Soma *a = _model->soma(a_index);
	const Soma_Type *t = _model->type(a->type_index());
	size32_t d_index = y->den_soma_index();
	const Soma *d = _model->soma(d_index);
	const Soma_Type *u = _model->type(d->type_index());
	ss << "From " << t->name() << " #" << a->id() << " to " << u->name() << " #" << d->id();
	if (_model->has_weights()) {
		const Firing_Spikes *fd = _model->const_firing_spikes();
		ss << "\n\nAt cycle " << fd->time() << ":\n";
		const Weights *wt = _model->const_weights();
		const weights_instance_t &wcs = wt->weight_changes();
		if (wcs.find(index) == wcs.end()) {
			ss << " " << BULLET << " No weight change";
		}
		else {
			float wb = wt->weight_before(index);
			float wa = wt->weight_after(index);
			float dw = wa - wb;
			ss.precision(2);
			ss << " " << BULLET << " Weight changed from " << wb << " mV to " << wa << " mV ("
				<< (dw < 0.0f ? "" : "+") << dw << " " << DELTA << "mV)";
			ss.precision(0);
		}
	}
	_body->copy_label(ss.str().c_str());
}

void Summary_Dialog::refresh_body(const Clip_Volume *v) {
	std::ostringstream ss;
	size32_t ncs = 0, ncy = 0;
	ss.imbue(std::locale(""));
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(0);
	size8_t nt = _model->num_types();
	size_t *type_counts = new(std::nothrow) size_t[nt * nt + nt]();
	if (type_counts == NULL) {
		_body->copy_label("Could not summarize model!\nNot enough memory was available.");
		return;
	}
	// Show model file name and size
	_model->print(ss);
	ss << ":";
	// Count somas in clip volume
	size32_t ns = _model->num_somas();
	size32_t ny = _model->num_synapses();
	for (size32_t i = 0; i < ns; i++) {
		Soma *s = _model->soma(i);
		if (v && !v->contains(s->coords())) { continue; }
		size16_t index = (size16_t)s->type_index() * (nt + 1);
		type_counts[index]++;
		ncs++;
		// Count soma's axonal connections
		const Synapse *y = NULL;
		for (size32_t a_index = s->first_axon_syn_index(); a_index < ny; a_index = y->next_axon_syn_index()) {
			y = _model->synapse(a_index);
			Soma *o = _model->soma(y->den_soma_index());
			type_counts[index + o->type_index() + 1]++;
			ncy++;
		}
	}
	// Show total soma and synapse counts
	ss << "\n" << ncs << " somas and " << ncy << " synapses";
	if (v) { ss << " in clip volume"; }
	// Show total gap junction count
	ss << "\n" << _model->num_gap_junctions() << " gap junctions";
	// Show soma counts by type
	ss << "\n\nSomas";
	if (v) { ss << " in clip volume"; }
	ss << ":";
	if (!ncs) { ss << "\nNone"; }
	for (size8_t i = 0; i < nt && ncs > 0; i++) {
		size16_t r = (size16_t)i * (nt + 1);
		// False positive "C6001: Using uninitialized memory" error with Visual Studio 2013 code analysis
		size_t c = type_counts[r];
		if (!c) { continue; }
		ss << "\n " << BULLET << " " << c << " " << _model->type(i)->name();
		double pct = c * 100.0 / ncs;
		ss.unsetf(std::ios::floatfield);
		ss.precision(pct < 1.0 ? 2 : 3);
		ss << " (" << pct << "%)";
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.precision(0);
		// Show soma's axonal connections by type
		for (size8_t j = 0; j < nt; j++) {
			size_t cy = type_counts[r + j + 1];
			if (!cy) { continue; }
			ss << "\n    " << SUB_BULLET << " " << cy << " connections to " << _model->type(j)->name();
			pct = cy * 100.0 / ncy;
			ss.unsetf(std::ios::floatfield);
			ss.precision(pct < 1.0 ? 2 : 3);
			ss << " (" << pct << "%)";
			ss.setf(std::ios::fixed, std::ios::floatfield);
			ss.precision(0);
		}
	}
	_body->copy_label(ss.str().c_str());
	delete [] type_counts;
}

void Summary_Dialog::refresh() {
	// Refresh widget labels (except for _body)
	_dialog->copy_label(_title.c_str());
	// Refresh widget positions and sizes
	fl_font(OS_FONT, OS_FONT_SIZE);
	int bw = _max_w - 20, bh = 0;
	fl_measure(_body->label(), bw, bh);
	int w = MAX(bw + 20 + OS_FONT_SIZE, _min_w) + Fl::scrollbar_size(), h = 10;
	int ww = w - 20;
#ifdef LARGE_INTERFACE
	int btn_w = 100, btn_h = 28;
#else
	int btn_w = 80, btn_h = 22;
#endif
	int bsh = MIN(bh, Fl::h()-60-10-10-btn_h-10);
	_body_scroll->resize(10, h, ww, bsh);
	_body->resize(10, h, ww, bh);
	h += _body_scroll->h() + 10;
	_copy_button->resize(w-10-btn_w-14-btn_w, h, btn_w, btn_h);
	_ok_button->resize(w-10-btn_w, h, btn_w, btn_h);
	h += _ok_button->h() + 10;
	_dialog->size_range(w, h, w, h);
	_dialog->size(w, h);
	_dialog->redraw();
}

void Summary_Dialog::show(const Fl_Widget *p) {
	int x = p->x() + (p->w() - _dialog->w()) / 2;
	int y = p->y() + (p->h() - _dialog->h()) / 2;
	_dialog->position(x, y);
	_ok_button->take_focus();
	_dialog->show();
	while (_dialog->shown()) { Fl::wait(); }
}

void Summary_Dialog::show(const Fl_Widget *p, const Soma *s, size32_t index, const Clip_Volume *v) {
	initialize();
	refresh_body(s, index, v);
	refresh();
	show(p);
}

void Summary_Dialog::show(const Fl_Widget *p, const Synapse *y, size32_t index) {
	initialize();
	refresh_body(y, index);
	refresh();
	show(p);
}

void Summary_Dialog::show(const Fl_Widget *p, const Clip_Volume *v) {
	initialize();
	refresh_body(v);
	refresh();
	show(p);
}

void Summary_Dialog::copy_cb(Fl_Widget *, Summary_Dialog *sd) {
	const char *s = sd->_body->label();
	Fl::copy(s, (int)strlen(s), 1);
}

void Summary_Dialog::close_cb(Fl_Widget *, Summary_Dialog *sd) { sd->_dialog->hide(); }
