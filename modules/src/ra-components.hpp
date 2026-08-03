#pragma once
#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

namespace rack {
namespace componentlibrary {

// Custom knob classes — SVGs loaded from plugin res/ for easy customization
struct RaCustomKnobStd : app::SvgKnob {
	widget::SvgWidget* bg;

	RaCustomKnobStd() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-std.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-std-bg.svg")));
	}
};

struct RaCustomKnobLarge : app::SvgKnob {
	widget::SvgWidget* bg;

	RaCustomKnobLarge() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-large.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-large-bg.svg")));
	}
};

struct RaCustomKnobSmall : app::SvgKnob {
	widget::SvgWidget* bg;

	RaCustomKnobSmall() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-small.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-small-bg.svg")));
	}
};

struct RaCustomKnobTrim : app::SvgKnob {
	widget::SvgWidget* bg;

	RaCustomKnobTrim() {
		minAngle = -0.75 * M_PI;
		maxAngle = 0.75 * M_PI;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-trim.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ra-knob-trim-bg.svg")));
	}
};

// Knobs
using RaKnob = RaCustomKnobStd;
using RaKnobLarge = RaCustomKnobLarge;
using RaKnobSmall = RaCustomKnobSmall;
using RaKnobTrim = RaCustomKnobTrim;

// Ports
using RaPort = ThemedPJ301MPort;

// Switches
using RaSwitch2 = CKSS;
using RaSwitch3 = CKSSThree;

// Buttons
using RaButton = VCVButton;
using RaLightButton = VCVLightButton<WhiteLight>;
using RaLightBezel = VCVLightBezel<WhiteLight>;

// Lights
using RaRGBLight = MediumLight<RedGreenBlueLight>;

// Screws
using RaScrew = ThemedScrew;

} // namespace componentlibrary
} // namespace rack
