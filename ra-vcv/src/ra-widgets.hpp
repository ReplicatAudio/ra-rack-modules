#pragma once
#include "rack.hpp"

using namespace rack;

namespace rack {
namespace componentlibrary {

// Custom knobs — white Rogan without foreground overlay (fg hides the indicator pointer)
struct RaRoganStd : Rogan {
	RaRoganStd() {
		setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan2PWhite.svg")));
		bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan2P_bg.svg")));
	}
};

struct RaRoganLarge : Rogan {
	RaRoganLarge() {
		setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan3PSWhite.svg")));
		bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan3PS_bg.svg")));
	}
};

struct RaRoganSmall : Rogan {
	RaRoganSmall() {
		setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan1PWhite.svg")));
		bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan1P_bg.svg")));
	}
};

// Knobs
using RaKnob = RaRoganStd;
using RaKnobLarge = RaRoganLarge;
using RaKnobSmall = RaRoganSmall;
using RaKnobTrim = Trimpot;

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
