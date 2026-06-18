#pragma once
#include "rack.hpp"

namespace rack {
namespace componentlibrary {

// Knobs
using RaKnob = Davies1900hBlackKnob;
using RaKnobLarge = Davies1900hLargeBlackKnob;
using RaKnobSmall = RoundSmallBlackKnob;
using RaKnobTrim = Trimpot;

// Ports
using RaPort = ThemedPJ301MPort;

// Switches
using RaSwitch2 = CKSS;
using RaSwitch3 = NKK;

// Buttons
using RaButton = VCVButton;

// Lights
using RaRGBLight = MediumLight<RedGreenBlueLight>;

// Screws
using RaScrew = ThemedScrew;

} // namespace componentlibrary
} // namespace rack
