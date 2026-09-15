// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: CABLE_ENABLED_PARAM "Enable"
// fname: CABLE_LIGHT_PARAM "LED"
// fname: CABLE_BRIGHTNESS_PARAM "Bright"
// fname: CABLE_POLY_THICK_PARAM "PolyT"
// fname: CABLE_POLY_MODE_PARAM "Poly"
// fname: CABLE_FAST_PARAM "Fast"
// fname: CABLE_SLEW_PARAM "Slew"
// fname: CABLE_SCALE_PARAM "Scale"
// fname: SCOPE_ENABLED_PARAM "Enable"
// fname: SCOPE_MAJ_PARAM "Shift"
// fname: SCOPE_MODE_PARAM "Circle"
// fname: SCOPE_POSITION_PARAM "Pos"
// fname: SCOPE_SCALE_PARAM "Scale"
// fname: SCOPE_THICKNESS_PARAM "Width"
// fname: SCOPE_BACK_ALPHA_PARAM "Back"
// fname: SCOPE_VOLT_ALPHA_PARAM "Volt"
// fname: SCOPE_LABEL_ALPHA_PARAM "Label"
// fname: SCOPE_ALPHA_PARAM "Alpha"
// ============================================================
// ra-control — animated cable rendering + on-hover cable scope.
//
// Ported from gibbonjoyeux/VCV-Biset "Blank" module. Instead of a
// resizable blank panel with all options hidden in the context menu,
// ra-control has a fixed-width panel with every cable/scope option
// exposed as an actual on-panel control.
//
// The module injects two overlay widgets:
//   * A rack-sized widget that re-draws every cable in the patch with
//     the signal travelling along it (the animated cable effect).
//   * A scene-sized widget that draws an oscilloscope of the hovered
//     cable's waveform.
//
// Only a single instance drives the overlays (the global g_control),
// mirroring VCV-Biset's Blank behaviour.
// ============================================================
#include "ra-components.hpp"
#include <string.h>
#include <cmath>

using namespace rack;

extern Plugin *pluginInstance;

#define CONTROL_BUFFER				2048
#define CONTROL_DIST_MAX				300.0
#define CONTROL_PRECISION				128
#define CONTROL_PRECISION_SCOPE			256
#define CONTROL_CABLES					256
#define CONTROL_SCOPE_LABEL_BUFFER		128
#define CONTROL_SCOPE_LABEL				55

#define CONTROL_CABLE_POLY_FIRST			0
#define CONTROL_CABLE_POLY_SUM			1
#define CONTROL_CABLE_POLY_SUM_DIVIDED	2

#define CONTROL_SCOPE_TOP_LEFT			0
#define CONTROL_SCOPE_TOP_RIGHT			1
#define CONTROL_SCOPE_BOTTOM_LEFT		2
#define CONTROL_SCOPE_BOTTOM_RIGHT		3
#define CONTROL_SCOPE_CENTER			4
#define CONTROL_SCOPE_CIRCULAR			0
#define CONTROL_SCOPE_LINEAR			1
#define CONTROL_CABLE_INCOMPLETE_OFF	0
#define CONTROL_CABLE_INCOMPLETE_IN		1
#define CONTROL_CABLE_INCOMPLETE_OUT	2

////////////////////////////////////////////////////////////////////////////////
/// DATA STRUCTURE
////////////////////////////////////////////////////////////////////////////////

struct RaControlCables;
struct RaControlScope;

struct RaControlCable {
	int64_t		id;
	math::Vec	pos_in;
	math::Vec	pos_out;
	NVGcolor	color;
	bool		thick;
	float		buffer[CONTROL_BUFFER];
};

struct RaControlModule : Module {
	enum ParamIds {
		PARAM_CABLE_ENABLED,
		PARAM_CABLE_BRIGHTNESS,		// Cable impacted by brightness
		PARAM_CABLE_LIGHT,			// Cable plug light
		PARAM_CABLE_POLY_THICK,		// Polyphonic cables thicker
		PARAM_CABLE_POLY_MODE,		// Polyphonic cables behavior (1st or sum)
		PARAM_CABLE_FAST,			// Cable animation computation mode
		PARAM_CABLE_SLEW,			// Cable animation slew limiter
		PARAM_CABLE_SCALE,			// Cable animation scale

		PARAM_SCOPE_ENABLED,
		PARAM_SCOPE_MAJ,			// Scope appears only with MAJ pressed
		PARAM_SCOPE_MODE,			// Scope display mode (circular / redraw)
		PARAM_SCOPE_POSITION,		// Scope position mode
		PARAM_SCOPE_SCALE,			// Scope scale
		PARAM_SCOPE_THICKNESS,		// Scope line thickness
		PARAM_SCOPE_BACK_ALPHA,		// Scope background alpha
		PARAM_SCOPE_VOLT_ALPHA,		// Scope voltage indicator alpha
		PARAM_SCOPE_LABEL_ALPHA,	// Scope port name alpha
		PARAM_SCOPE_ALPHA,			// Scope alpha

		PARAM_COUNT
	};
	enum InputIds {
		INPUT_COUNT
	};
	enum OutputIds {
		OUTPUT_COUNT
	};
	enum LightIds {
		LIGHT_COUNT
	};

	int				cable_count;
	int				cable_incomplete;
	RaControlCable	cables[CONTROL_CABLES + 1];
	int				buffer_i;

	int				scope_index;
	RaControlScope	*scope;
	RaControlCables	*display;
	char			scope_label[CONTROL_SCOPE_LABEL_BUFFER];

	RaControlModule();
	~RaControlModule();
	void processBypass(const ProcessArgs& args) override;
	void process(const ProcessArgs& args) override;
};

struct RaControlWidget : ModuleWidget {
	RaControlModule	*module;

	RaControlWidget(RaControlModule *_module);
};

struct RaControlCables : Widget {
	RaControlModule	*module;

	RaControlCables();
	void draw(const DrawArgs &args) override;
	void drawLayer(const DrawArgs &args, int layer) override;
};

struct RaControlScope : Widget {
	RaControlModule	*module;
	std::string		font_path;

	RaControlScope();
	void draw(const DrawArgs &args) override;
};

extern RaControlModule	*g_control;

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL
////////////////////////////////////////////////////////////////////////////////

RaControlModule *g_control = NULL;

////////////////////////////////////////////////////////////////////////////////
/// CABLE POSITION HELPER
////////////////////////////////////////////////////////////////////////////////

static math::Vec get_pos_slump(math::Vec pos1, math::Vec pos2) {
	float		dist;
	math::Vec	avg;

	dist = pos1.minus(pos2).norm();
	avg = pos1.plus(pos2).div(2);
	// Lower average point as distance increases
	avg.y += (1.0 - settings::cableTension) * (150.0 + 1.0 * dist);
	return avg;
}

////////////////////////////////////////////////////////////////////////////////
/// MODULE
////////////////////////////////////////////////////////////////////////////////

RaControlModule::RaControlModule(void) {
	config(PARAM_COUNT, INPUT_COUNT, OUTPUT_COUNT, LIGHT_COUNT);

	configSwitch(PARAM_SCOPE_ENABLED, 0, 1, 1, "Scope enabled", {"Off", "On"});
	configSwitch(PARAM_SCOPE_MAJ, 0, 1, 0, "Scope on Shift", {"Off", "On"});
	configSwitch(PARAM_SCOPE_MODE, 0, 1, 0, "Scope display", {"Circular", "Linear"});
	configSwitch(PARAM_SCOPE_POSITION, 0, 4, 0, "Scope position",
		{"Top left", "Top right", "Bottom left", "Bottom right", "Center"});
	configParam(PARAM_SCOPE_SCALE, 0.02, 1, 0.2, "Scope scale", "%", 0, 100);
	configParam(PARAM_SCOPE_THICKNESS, 1, 10, 2, "Scope thickness", "");
	configParam(PARAM_SCOPE_BACK_ALPHA, 0, 1, 0.6, "Scope background alpha", "%", 0, 100);
	configParam(PARAM_SCOPE_VOLT_ALPHA, 0, 1, 0.3, "Scope voltage alpha", "%", 0, 100);
	configParam(PARAM_SCOPE_LABEL_ALPHA, 0, 1, 1, "Scope label alpha", "%", 0, 100);
	configParam(PARAM_SCOPE_ALPHA, 0, 1, 1, "Scope alpha", "%", 0, 100);

	configSwitch(PARAM_CABLE_ENABLED, 0, 1, 1, "Cable animation enabled", {"Off", "On"});
	configSwitch(PARAM_CABLE_BRIGHTNESS, 0, 1, 1, "Cable brightness", {"Off", "On"});
	configSwitch(PARAM_CABLE_LIGHT, 0, 1, 1, "Cable led", {"Off", "On"});
	configSwitch(PARAM_CABLE_POLY_THICK, 0, 1, 1, "Cable polyphonic thickness", {"Off", "On"});
	configSwitch(PARAM_CABLE_POLY_MODE, 0, 2, 0, "Cable polyphonic",
		{"1st channel", "Sum", "Sum / count"});
	configSwitch(PARAM_CABLE_FAST, 0, 1, 0, "Cable CPU fast", {"Off", "On"});
	configParam(PARAM_CABLE_SLEW, 0.0, 1.0, 0.0, "Cable slew", "%", 0, 100);
	configParam(PARAM_CABLE_SCALE, 0.0, 2.0, 1.0, "Cable scale", "%", 0, 100);

	this->buffer_i = 0;

	this->display = NULL;
	this->scope = NULL;
}

RaControlModule::~RaControlModule(void) {
	if (this->display) {
		if (APP->scene->rack->hasChild(this->display))
			APP->scene->rack->removeChild(this->display);
		delete this->display;
	}
	if (this->scope) {
		if (APP->scene->hasChild(this->scope))
			APP->scene->removeChild(this->scope);
		delete this->scope;
	}
	if (this == g_control) {
		g_control = NULL;
		APP->scene->rack->getCableContainer()->show();
	}
}

void RaControlModule::processBypass(const ProcessArgs& args) {
	if (this == g_control) {
		APP->scene->rack->getCableContainer()->show();
		if (this->display)
			this->display->hide();
	}
}

void RaControlModule::process(const ProcessArgs& args) {
	std::list<Widget*>::iterator	it, it_end;
	Widget							*container;
	CableWidget						*widget;
	engine::Cable					*cable;
	engine::PortInfo				*port_info;
	PortWidget						*port_widget;
	engine::Port					*port;
	engine::Output					*output;
	PortWidget						*hovered;
	bool							scope;
	bool							poly_thick;
	int								poly_mode;
	int								channels;
	int								i;

	if (args.frame % 32 != 0)
		return;
	if (g_control == NULL)
		g_control = this;
	if (g_control != this)
		return;

	/// [1] GET PARAMETERS
	if (this->params[PARAM_SCOPE_MAJ].getValue() && APP->window)
		scope = ((APP->window->getMods() & RACK_MOD_SHIFT) == RACK_MOD_SHIFT);
	else
		scope = true;
	poly_thick = this->params[PARAM_CABLE_POLY_THICK].getValue();
	poly_mode = this->params[PARAM_CABLE_POLY_MODE].getValue();
	if (this->params[PARAM_CABLE_ENABLED].getValue()) {
		APP->scene->rack->getCableContainer()->hide();
		if (this->display)
			this->display->show();
	} else {
		APP->scene->rack->getCableContainer()->show();
		if (this->display)
			this->display->hide();
	}
	hovered = dynamic_cast<PortWidget*>(APP->event->hoveredWidget);
	this->scope_label[0] = 0;

	/// [2] GET CABLE CONTAINER
	container = APP->scene->rack->getCableContainer();
	it = container->children.begin();
	it_end = container->children.end();

	/// [3] RECORD CABLES
	i = 0;
	this->scope_index = -1;
	while (it != it_end) {
		/// GET CABLE
		widget = dynamic_cast<CableWidget*>(*it);
		if (widget == NULL || widget->isComplete() == false) {
			++it;
			continue;
		}
		cable = widget->getCable();
		/// STORE CABLE POSITION
		this->cables[i].pos_out = widget->getInputPos();
		this->cables[i].pos_in = widget->getOutputPos();
		this->cables[i].color = widget->color;
		/// STORE CABLE BUFFER
		if (cable && cable->outputModule && cable->outputId >= 0) {
			output = &(cable->outputModule->outputs[cable->outputId]);
			channels = output->getChannels();
			if (channels == 0)
				channels = 1;
			this->cables[i].thick = (channels > 1 && poly_thick);
			if (poly_mode == CONTROL_CABLE_POLY_FIRST) {
				this->cables[i].buffer[this->buffer_i] =
				/**/ output->getVoltage();
			} else if (poly_mode == CONTROL_CABLE_POLY_SUM) {
				this->cables[i].buffer[this->buffer_i] =
				/**/ output->getVoltageSum();
			} else {
				this->cables[i].buffer[this->buffer_i] =
				/**/ output->getVoltageSum() / channels;
			}
		}
		/// CHECK HOVER
		if (scope &&
		(widget->outputPort == hovered || widget->inputPort == hovered)) {
			/// SET INDEX
			this->scope_index = i;
			/// SET LABEL
			port_info = widget->outputPort->getPortInfo();
			if (port_info) {
				strncpy(this->scope_label,
				/**/ port_info->name.c_str(),
				/**/ CONTROL_SCOPE_LABEL);
			}
			strcat(this->scope_label, " output to ");
			port_info = widget->inputPort->getPortInfo();
			if (port_info) {
				strncat(this->scope_label,
				/**/ port_info->name.c_str(),
				/**/ CONTROL_SCOPE_LABEL);
			}
			strcat(this->scope_label, " input");
		}
		/// LOOP
		++it;
		++i;
		if (i >= CONTROL_CABLES) {
			i = CONTROL_CABLES - 1;
			return;
		}
	}
	this->cable_count = i;

	/// [4] RECORD HOVERED UNCONNECTED PORT
	if (this->scope_index < 0 && hovered && scope) {
		port_widget = dynamic_cast<PortWidget*>(hovered);
		if (port_widget && port_widget->type == engine::Port::OUTPUT) {
			port = port_widget->getPort();
			if (port) {
				/// STORE PORT TO EXTRA CABLE
				this->scope_index = CONTROL_CABLES;
				this->cables[CONTROL_CABLES].color = {1, 1, 1, 1};
				this->cables[CONTROL_CABLES].buffer[this->buffer_i] =
				/**/ port->voltages[0];
				/// STORE PORT LABEL
				port_info = port_widget->getPortInfo();
				if (port_info) {
					strncpy(this->scope_label,
					/**/ port_info->name.c_str(),
					/**/ CONTROL_SCOPE_LABEL - 1);
				}
			}
		}
	}

	/// [5] RECORD INCOMPLETE CABLE
	{
		std::vector<CableWidget*> incomplete = APP->scene->rack->getIncompleteCables();
		this->cable_incomplete = CONTROL_CABLE_INCOMPLETE_OFF;
		if (!incomplete.empty()) {
			widget = incomplete.front();
			if (widget->inputPort)
				this->cable_incomplete = CONTROL_CABLE_INCOMPLETE_IN;
			else
				this->cable_incomplete = CONTROL_CABLE_INCOMPLETE_OUT;
			this->scope_index = -1;
			this->cables[CONTROL_CABLES].pos_out = widget->getInputPos();
			this->cables[CONTROL_CABLES].pos_in = widget->getOutputPos();
			this->cables[CONTROL_CABLES].color = widget->color;
		}
	}

	/// [6] STEP BUFFER
	this->buffer_i += 1;
	if (this->buffer_i >= CONTROL_BUFFER)
		this->buffer_i = 0;
}

////////////////////////////////////////////////////////////////////////////////
/// MODULE WIDGET
////////////////////////////////////////////////////////////////////////////////

RaControlWidget::RaControlWidget(RaControlModule *module) {
	RaControlScope	*scope;
	RaControlCables	*display;

	this->module = module;
	setModule(this->module);
	box.size.x = 22 * RACK_GRID_WIDTH;
	box.size.y = RACK_GRID_HEIGHT;
	setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-control.svg")));

	addChild(createWidget<RaScrew>(Vec(0, 0)));
	addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
	addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

	/// CABLE section — left column
	addParam(createParamCentered<RaSwitch2>(Vec(55, 50), module, RaControlModule::PARAM_CABLE_ENABLED));
	addParam(createParamCentered<RaSwitch2>(Vec(55, 92), module, RaControlModule::PARAM_CABLE_BRIGHTNESS));
	addParam(createParamCentered<RaSwitch2>(Vec(55, 134), module, RaControlModule::PARAM_CABLE_LIGHT));
	addParam(createParamCentered<RaSwitch2>(Vec(55, 176), module, RaControlModule::PARAM_CABLE_POLY_THICK));
	addParam(createParamCentered<RaSwitch3>(Vec(55, 218), module, RaControlModule::PARAM_CABLE_POLY_MODE));
	addParam(createParamCentered<RaSwitch2>(Vec(55, 260), module, RaControlModule::PARAM_CABLE_FAST));
	addParam(createParamCentered<RaKnobSmall>(Vec(55, 302), module, RaControlModule::PARAM_CABLE_SLEW));
	addParam(createParamCentered<RaKnobSmall>(Vec(55, 344), module, RaControlModule::PARAM_CABLE_SCALE));

	/// SCOPE section — middle column (toggles + position)
	addParam(createParamCentered<RaSwitch2>(Vec(165, 50), module, RaControlModule::PARAM_SCOPE_ENABLED));
	addParam(createParamCentered<RaSwitch2>(Vec(165, 92), module, RaControlModule::PARAM_SCOPE_MAJ));
	addParam(createParamCentered<RaSwitch2>(Vec(165, 134), module, RaControlModule::PARAM_SCOPE_MODE));
	addParam(createParamCentered<RaKnobSmall>(Vec(165, 176), module, RaControlModule::PARAM_SCOPE_POSITION));

	/// SCOPE section — right column (continuous)
	addParam(createParamCentered<RaKnobSmall>(Vec(275, 50), module, RaControlModule::PARAM_SCOPE_SCALE));
	addParam(createParamCentered<RaKnobSmall>(Vec(275, 92), module, RaControlModule::PARAM_SCOPE_THICKNESS));
	addParam(createParamCentered<RaKnobSmall>(Vec(275, 134), module, RaControlModule::PARAM_SCOPE_BACK_ALPHA));
	addParam(createParamCentered<RaKnobSmall>(Vec(275, 176), module, RaControlModule::PARAM_SCOPE_VOLT_ALPHA));
	addParam(createParamCentered<RaKnobSmall>(Vec(275, 218), module, RaControlModule::PARAM_SCOPE_LABEL_ALPHA));
	addParam(createParamCentered<RaKnobSmall>(Vec(275, 260), module, RaControlModule::PARAM_SCOPE_ALPHA));

	/// ADD CABLE DISPLAY (overlays the whole rack)
	if (this->module) {
		display = new RaControlCables;
		display->box.size = math::Vec(INFINITY, INFINITY);
		display->module = this->module;
		this->module->display = display;
		APP->scene->rack->addChild(display);
	}

	/// ADD SCOPE DISPLAY (overlays the whole scene)
	if (this->module) {
		scope = new RaControlScope;
		scope->box.size = math::Vec(INFINITY, INFINITY);
		scope->module = this->module;
		this->module->scope = scope;
		APP->scene->addChild(scope);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// CABLE DISPLAY
////////////////////////////////////////////////////////////////////////////////

RaControlCables::RaControlCables() {
}

void RaControlCables::draw(const DrawArgs &args) {
}

void RaControlCables::drawLayer(const DrawArgs &args, int layer) {
	RaControlCable		*cable;
	math::Vec		pos_in;
	math::Vec		pos_out;
	math::Vec		pos_slump;
	math::Vec		pos_point, pos_point_prev;
	math::Vec		pos_angle;
	NVGcolor		color, color_light;
	NVGcolor		col_in, col_out;
	Rect			rect;
	Rect			rect_module;
	bool			brightness;
	bool			fast;
	bool			light;
	float			t;
	float			angle;
	float			amp;
	float			length;
	float			scale;
	float			slew;
	float			radius, radius_out;
	float			voltage, voltage_prev;
	float			voltage_diff, voltage_diff_max, voltage_max;
	float			orientation;
	int				buffer_phase, buffer_phase_prev;
	int				i, j;

	if (layer != 1)
		return;

	if (this->module == NULL)
		return;
	if (g_control != this->module)
		return;
	brightness = this->module->params[RaControlModule::PARAM_CABLE_BRIGHTNESS].getValue();
	light = this->module->params[RaControlModule::PARAM_CABLE_LIGHT].getValue();
	scale = this->module->params[RaControlModule::PARAM_CABLE_SCALE].getValue();
	fast = this->module->params[RaControlModule::PARAM_CABLE_FAST].getValue();
	slew = this->module->params[RaControlModule::PARAM_CABLE_SLEW].getValue();
	slew = (slew * slew) * 0.8;

	rect = box.zeroPos();
	rect_module = this->parent->box;

	/// [1] DRAW CABLES
	nvgLineCap(args.vg, 1);
	nvgLineJoin(args.vg, 1);
	for (i = 0; i < this->module->cable_count; ++i) {

		cable = &(this->module->cables[i]);

		/// COMPUTE CABLE POSITION
		pos_in = cable->pos_in;
		pos_out = cable->pos_out;
		pos_slump = get_pos_slump(pos_out, pos_in);
		orientation = (pos_in.x > pos_out.x) ? 1.0 : -1.0;

		/// COMPUTE CABLE LENGTH
		length = sqrt(
		/**/ (pos_in.x - pos_out.x) * (pos_in.x - pos_out.x)
		/**/ + (pos_in.y - pos_out.y) * (pos_in.y - pos_out.y));
		if (length > CONTROL_DIST_MAX)
			length = CONTROL_DIST_MAX;
		if (length < 1.0)
			length = 1.0;
		length = length / CONTROL_DIST_MAX;

		/// SET CABLE COLOR
		if (brightness) {
			color_light = color::mult(cable->color, settings::rackBrightness);
			nvgStrokeColor(args.vg, color_light);
			nvgFillColor(args.vg, color_light);
		} else {
			nvgStrokeColor(args.vg, cable->color);
			nvgFillColor(args.vg, cable->color);
		}

		/// DRAW CABLE PLUGS
		nvgGlobalAlpha(args.vg, 1.0);
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cable->pos_in.x, cable->pos_in.y, 8.5);
		nvgStrokeWidth(args.vg, 4.0);
		nvgStroke(args.vg);
		if (light == false)
			nvgFill(args.vg);
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cable->pos_out.x, cable->pos_out.y, 8.5);
		nvgStrokeWidth(args.vg, 4.0);
		nvgStroke(args.vg);
		if (light == false)
			nvgFill(args.vg);

		/// DRAW CABLE
		nvgGlobalAlpha(args.vg, settings::cableOpacity);
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, VEC_ARGS(pos_in));
		buffer_phase_prev = this->module->buffer_i;
		voltage_prev = 0.0;
		for (j = 0; j < CONTROL_PRECISION; ++j) {
			t = (float)(j + 1) / (float)CONTROL_PRECISION;

			/// COMPUTE POINT TANGENT
			pos_angle.x = 2.0 * (1.0 - t) * (pos_slump.x - pos_in.x)
			/**/ + 2.0 * t * (pos_out.x - pos_slump.x);
			pos_angle.y = 2.0 * (1.0 - t) * (pos_slump.y - pos_in.y)
			/**/ + 2.0 * t * (pos_out.y - pos_slump.y);
			angle = atan2(pos_angle.y, pos_angle.x);

			/// COMPUTE POINT POSITION
			pos_point.x =
			/**/ (1.0 - t) * (1.0 - t) * pos_in.x
			/**/ + 2 * (1.0 - t) * t * pos_slump.x
			/**/ + t * t * pos_out.x;
			pos_point.y =
			/**/ (1.0 - t) * (1.0 - t) * pos_in.y
			/**/ + 2 * (1.0 - t) * t * pos_slump.y
			/**/ + t * t * pos_out.y;

			/// COMPUTE POINT PHASE
			buffer_phase = this->module->buffer_i
			/**/ - t * ((float)CONTROL_BUFFER * length);
			if (buffer_phase < 0)
				buffer_phase += CONTROL_BUFFER;

			/// COMPUTE POINT ANIMATED POSITION
			//// MODE FAST
			if (fast) {
				voltage = cable->buffer[buffer_phase];
			//// MODE PRECISION
			} else {
				voltage_diff_max = 0;
				voltage_max = voltage_prev;
				while (buffer_phase_prev != buffer_phase) {
					voltage = cable->buffer[buffer_phase_prev];
					voltage_diff = voltage_prev - voltage;
					if (voltage_diff < 0)
						voltage_diff = -voltage_diff;
					if (voltage_diff > voltage_diff_max) {
						voltage_diff_max = voltage_diff;
						voltage_max = voltage;
					}
					buffer_phase_prev -= 1;
					if (buffer_phase_prev < 0)
						buffer_phase_prev += CONTROL_BUFFER;
				}
				voltage = voltage_max;
			}
			voltage = voltage * (1.0 - slew) + voltage_prev * slew;
			voltage_prev = voltage;
			angle += M_PI * 0.5;
			if (t < 0.2)
				amp = t * 5.0;
			else if (t > 0.8)
				amp = (1.0 - t) * 5.0;
			else
				amp = 1.0;
			amp *= scale * orientation;
			pos_point.x += cos(angle) * voltage * amp;
			pos_point.y += sin(angle) * voltage * amp;

			nvgLineTo(args.vg, VEC_ARGS(pos_point));
		}
		nvgStrokeWidth(args.vg, (cable->thick) ? 9.0 : 6.0);
		nvgStroke(args.vg);

		if (light) {

			/// DRAW CABLE LIGHT
			buffer_phase = this->module->buffer_i - 1;
			if (buffer_phase < 0)
				buffer_phase += CONTROL_BUFFER;
			voltage = cable->buffer[buffer_phase];
			if (voltage > 10.0)
				voltage = 10.0;
			else if (voltage < -10.0)
				voltage = -10.0;
			if (voltage > 0)
				color = color::mult({0.6274, 0.8235, 0.2862}, voltage * 0.1);
			else
				color = color::mult({0.9450, 0.2078, 0.1725}, voltage * -0.1);
			color.a = 1.0;
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, cable->pos_in.x, cable->pos_in.y, 5.75);
			nvgCircle(args.vg, cable->pos_out.x, cable->pos_out.y, 5.75);
			nvgFillColor(args.vg, color);
			nvgFill(args.vg);
			if (brightness) {
				nvgStrokeColor(args.vg,
				/**/ color::mult({0.9019, 0.8823, 0.8823, 1.0},
				/**/ settings::rackBrightness));
			} else {
				nvgStrokeColor(args.vg, {0.9019, 0.8823, 0.8823, 1.0});
			}
			nvgStrokeWidth(args.vg, 1.5);
			nvgStroke(args.vg);

			/// DRAW CABLE LIGHT HALO
			if (settings::haloBrightness > 0) {

				nvgGlobalCompositeBlendFunc(args.vg,
				/**/ NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
				radius = 5.0;
				radius_out = radius + std::min(radius * 4.f, 15.f);
				col_in = color::mult(color, settings::haloBrightness);
				col_out = nvgRGBA(0, 0, 0, 0);

				/// INPUT PORT HALO
				pos_in = cable->pos_in;
				nvgBeginPath(args.vg);
				nvgRect(args.vg,
				/**/ pos_in.x - radius_out, pos_in.y - radius_out,
				/**/ 2 * radius_out, 2 * radius_out);
				NVGpaint paint = nvgRadialGradient(args.vg,
				/**/ pos_in.x, pos_in.y,
				/**/ radius, radius_out,
				/**/ col_in, col_out);
				nvgFillPaint(args.vg, paint);
				nvgFill(args.vg);

				/// OUTPUT PORT HALO
				pos_out = cable->pos_out;
				nvgBeginPath(args.vg);
				nvgRect(args.vg,
				/**/ pos_out.x - radius_out, pos_out.y - radius_out,
				/**/ 2 * radius_out, 2 * radius_out);
				paint = nvgRadialGradient(args.vg,
				/**/ pos_out.x, pos_out.y,
				/**/ radius, radius_out,
				/**/ col_in, col_out);
				nvgFillPaint(args.vg, paint);
				nvgFill(args.vg);

				nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
			}
		}
	}

	/// [2] DRAW INCOMPLETE CABLE
	if (this->module->cable_incomplete != CONTROL_CABLE_INCOMPLETE_OFF) {
		cable = &(this->module->cables[CONTROL_CABLES]);
		/// COMPUTE CABLE POSITION
		pos_in = cable->pos_in;
		pos_out = cable->pos_out;
		pos_slump = get_pos_slump(pos_out, pos_in);

		/// SET CABLE COLOR
		if (brightness) {
			color_light = color::mult(cable->color, settings::rackBrightness);
			nvgStrokeColor(args.vg, color_light);
			nvgFillColor(args.vg, color_light);
		} else {
			nvgStrokeColor(args.vg, cable->color);
			nvgFillColor(args.vg, cable->color);
		}
		nvgGlobalAlpha(args.vg, 1.0);

		/// DRAW CABLE PLUG
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cable->pos_in.x, cable->pos_in.y, 8.5);
		nvgStrokeWidth(args.vg, 4.0);
		nvgStroke(args.vg);
		nvgFill(args.vg);
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cable->pos_out.x, cable->pos_out.y, 8.5);
		nvgStrokeWidth(args.vg, 4.0);
		nvgStroke(args.vg);
		nvgFill(args.vg);

		/// DRAW CABLE
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, VEC_ARGS(pos_in));
		nvgQuadTo(args.vg, VEC_ARGS(pos_slump), VEC_ARGS(pos_out));
		nvgStrokeWidth(args.vg, 6.0);
		nvgStroke(args.vg);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// SCOPE DISPLAY
////////////////////////////////////////////////////////////////////////////////

RaControlScope::RaControlScope() {
	this->font_path = asset::system("res/fonts/ShareTechMono-Regular.ttf");
}

void RaControlScope::draw(const DrawArgs &args) {
	std::shared_ptr<Font>	font;
	math::Vec				pos_point;
	RaControlCable			*cable;
	Rect					box;
	bool					mode;
	float					label;
	float					details;
	float					background;
	float					scale;
	float					alpha;
	float					thickness;
	float					t;
	float					voltage, voltage_prev;
	float					voltage_diff, voltage_diff_max, voltage_max;
	int						buffer_phase, buffer_phase_prev;
	int						position;
	int						i;


	if (g_control != this->module)
		return;
	if (this->module->params[RaControlModule::PARAM_SCOPE_ENABLED].getValue() == 0.0)
		return;
	if (this->module->scope_index < 0)
		return;

	/// [1] GET PARAMETERS
	cable = &(this->module->cables[this->module->scope_index]);
	scale = this->module->params[RaControlModule::PARAM_SCOPE_SCALE].getValue();
	position = this->module->params[RaControlModule::PARAM_SCOPE_POSITION].getValue();
	mode = this->module->params[RaControlModule::PARAM_SCOPE_MODE].getValue();
	thickness = this->module->params[RaControlModule::PARAM_SCOPE_THICKNESS].getValue();
	details = this->module->params[RaControlModule::PARAM_SCOPE_VOLT_ALPHA].getValue();
	background = this->module->params[RaControlModule::PARAM_SCOPE_BACK_ALPHA].getValue();
	label = this->module->params[RaControlModule::PARAM_SCOPE_LABEL_ALPHA].getValue();
	alpha = this->module->params[RaControlModule::PARAM_SCOPE_ALPHA].getValue();
	box.size.x = scale * APP->scene->box.size.x;
	box.size.y = scale * APP->scene->box.size.x * 0.5;
	if (position == CONTROL_SCOPE_TOP_LEFT) {
		box.pos.x = 10.0;
		box.pos.y = 40.0;
	} else if (position == CONTROL_SCOPE_TOP_RIGHT) {
		box.pos.x = APP->scene->box.size.x - (box.size.x + 10.0);
		box.pos.y = 40.0;
	} else if (position == CONTROL_SCOPE_BOTTOM_LEFT) {
		box.pos.x = 10.0;
		box.pos.y = APP->scene->box.size.y - (box.size.y + 10.0);
	} else if (position == CONTROL_SCOPE_BOTTOM_RIGHT) {
		box.pos.x = APP->scene->box.size.x - (box.size.x + 10.0);
		box.pos.y = APP->scene->box.size.y - (box.size.y + 10.0);
	} else {
		box.pos.x = APP->scene->box.size.x * 0.5 - box.size.x * 0.5;
		box.pos.y = APP->scene->box.size.y * 0.5 - box.size.y * 0.5;
	}

	nvgAlpha(args.vg, alpha);

	/// [2] DRAW BACKGROUND
	if (background >= 0) {
		nvgBeginPath(args.vg);
		nvgFillColor(args.vg, (NVGcolor){0, 0, 0, background});
		nvgRect(args.vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
		nvgFill(args.vg);

	}

	/// [3] DRAW DETAILS (VOLTAGE)
	if (details >= 0) {
		nvgStrokeColor(args.vg, (NVGcolor){1, 1, 1, details});
		nvgStrokeWidth(args.vg, 1.0);
		/// 0V LINE
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg,
		/**/ box.pos.x, box.pos.y + box.size.y * 0.5);
		nvgLineTo(args.vg,
		/**/ box.pos.x + box.size.x, box.pos.y + box.size.y * 0.5);
		/// 5V LINES
		nvgMoveTo(args.vg,
		/**/ box.pos.x, box.pos.y + box.size.y * 0.3);
		nvgLineTo(args.vg,
		/**/ box.pos.x + box.size.x, box.pos.y + box.size.y * 0.3);
		nvgMoveTo(args.vg,
		/**/ box.pos.x, box.pos.y + box.size.y * 0.7);
		nvgLineTo(args.vg,
		/**/ box.pos.x + box.size.x, box.pos.y + box.size.y * 0.7);
		/// 10V LINES
		nvgMoveTo(args.vg,
		/**/ box.pos.x, box.pos.y + box.size.y * 0.1);
		nvgLineTo(args.vg,
		/**/ box.pos.x + box.size.x, box.pos.y + box.size.y * 0.1);
		nvgMoveTo(args.vg,
		/**/ box.pos.x, box.pos.y + box.size.y * 0.9);
		nvgLineTo(args.vg,
		/**/ box.pos.x + box.size.x, box.pos.y + box.size.y * 0.9);
		nvgStroke(args.vg);
	}

	/// [4] DRAW WAVE
	nvgScissor(args.vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
	nvgBeginPath(args.vg);
	buffer_phase_prev = this->module->buffer_i;
	voltage_prev = 0.0;
	for (i = 0; i < CONTROL_PRECISION_SCOPE; ++i) {
		t = (float)i / (float)CONTROL_PRECISION_SCOPE;

		/// COMPUTE VOLTAGE
		if (mode == CONTROL_SCOPE_CIRCULAR) {
			/// COMPUTE BUFFER PHASE
			buffer_phase = this->module->buffer_i - 1 - t * (float)CONTROL_BUFFER;
			if (buffer_phase < 0)
				buffer_phase += CONTROL_BUFFER;
			/// COMPUTE VOLTAGE
			voltage_diff_max = 0;
			voltage_max = voltage_prev;
			while (buffer_phase_prev != buffer_phase) {
				voltage = cable->buffer[buffer_phase_prev];
				voltage_diff = voltage_prev - voltage;
				if (voltage_diff < 0)
					voltage_diff = -voltage_diff;
				if (voltage_diff > voltage_diff_max) {
					voltage_diff_max = voltage_diff;
					voltage_max = voltage;
				}
				buffer_phase_prev -= 1;
				if (buffer_phase_prev < 0)
					buffer_phase_prev += CONTROL_BUFFER;
			}
			voltage_prev = voltage_max;
			voltage = voltage_max;

		} else {
			buffer_phase = t * (float)CONTROL_BUFFER;
			voltage = cable->buffer[buffer_phase];
		}

		/// DRAW POINT
		pos_point.x = box.pos.x + t * box.size.x;
		pos_point.y = box.pos.y + box.size.y * 0.5
		/**/ - voltage * 0.05 * box.size.y * 0.8;
		if (i == 0)
			nvgMoveTo(args.vg, VEC_ARGS(pos_point));
		else
			nvgLineTo(args.vg, VEC_ARGS(pos_point));
	}
	nvgStrokeColor(args.vg, cable->color);
	nvgStrokeWidth(args.vg, thickness);
	nvgStroke(args.vg);
	nvgResetScissor(args.vg);

	/// [5] DRAW PORT LABEL
	if (label > 0) {
		font = APP->window->loadFont(this->font_path);
		if (font == NULL)
			return;
		nvgFontSize(args.vg, 12.0 * scale * 5.0);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
		nvgFillColor(args.vg, (NVGcolor){1, 1, 1, label});
		nvgText(args.vg,
		/**/ box.pos.x + box.size.x * 0.5,
		/**/ box.pos.y + box.size.y * 0.905,
		/**/ this->module->scope_label, NULL);
	}
}

Model* modelRaControl = createModel<RaControlModule, RaControlWidget>("ra-control");