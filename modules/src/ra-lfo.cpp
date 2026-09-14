// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: FREQ_PARAM "Freq"
// fname: PW_PARAM "Pulse"
// fname: FM_PARAM "FM"
// fname: INVERT_PARAM "Invert"
// fname: OFFSET_PARAM "Offset"
// fname: PWM_PARAM "PWM"
// fname: PHASE_PARAM "Phase"
// fname: FM_INPUT "FM"
// fname: CLOCK_INPUT "Clock"
// fname: RESET_INPUT "Reset"
// fname: PW_INPUT "PWM"
// fname: PHASE_CV_INPUT "Phase"
// fname: INVERT_GATE_INPUT "Inv G"
// fname: OFFSET_GATE_INPUT "Off G"
// fname: SIN_OUTPUT "Sine"
// fname: TRI_OUTPUT "Tri"
// fname: SAW_OUTPUT "Saw"
// fname: SQR_OUTPUT "Sqr"
#include "ra-components.hpp"

using namespace rack;
using simd::float_4;

extern Plugin *pluginInstance;

struct RaLfoModule : Module {
	enum ParamIds {
		OFFSET_PARAM,
		INVERT_PARAM,
		FREQ_PARAM,
		FM_PARAM,
		PW_PARAM,
		PWM_PARAM,
		PHASE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		FM_INPUT,
		RESET_INPUT,
		PW_INPUT,
		CLOCK_INPUT,
		INVERT_GATE_INPUT,
		OFFSET_GATE_INPUT,
		PHASE_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIN_OUTPUT,
		TRI_OUTPUT,
		SAW_OUTPUT,
		SQR_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(PHASE_LIGHT, 3),
		INVERT_LIGHT,
		OFFSET_LIGHT,
		NUM_LIGHTS
	};

	// Output range (peak) in volts
	static constexpr float RANGE = 5.f;

	bool offset = false;
	bool invert = false;
	float_4 phases[4];
	dsp::TSchmittTrigger<float_4> resetTriggers[4];
	dsp::SchmittTrigger clockTrigger;
	float clockFreq = 1.f;
	dsp::Timer clockTimer;
	dsp::ClockDivider lightDivider;

	RaLfoModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(OFFSET_PARAM, 0.f, 1.f, 1.f, "Offset", {"Bipolar", "Unipolar"});
		configSwitch(INVERT_PARAM, 0.f, 1.f, 0.f, "Invert");
		configParam(FREQ_PARAM, -8.f, 10.f, 1.f, "Frequency", " Hz", 2, 1);
		configParam(FM_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
		getParamQuantity(FM_PARAM)->randomizeEnabled = false;
		configParam(PW_PARAM, 0.01f, 0.99f, 0.5f, "Pulse width", "%", 0.f, 100.f);
		configParam(PWM_PARAM, -1.f, 1.f, 0.f, "Pulse width modulation", "%", 0.f, 100.f);
		getParamQuantity(PWM_PARAM)->randomizeEnabled = false;
		configParam(PHASE_PARAM, 0.f, 1.f, 0.f, "Phase offset");

		configInput(FM_INPUT, "Frequency modulation");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(PW_INPUT, "Pulse width modulation");
		configInput(INVERT_GATE_INPUT, "Invert gate");
		configInput(OFFSET_GATE_INPUT, "Offset gate");
		configInput(PHASE_CV_INPUT, "Phase CV");

		configOutput(SIN_OUTPUT, "Sine");
		configOutput(TRI_OUTPUT, "Triangle");
		configOutput(SAW_OUTPUT, "Sawtooth");
		configOutput(SQR_OUTPUT, "Square");

		configLight(PHASE_LIGHT, "Phase");

		lightDivider.setDivision(16);
		onReset();
	}

	void onReset() override {
		for (int c = 0; c < 16; c += 4) {
			phases[c / 4] = 0.f;
		}
		clockFreq = 1.f;
		clockTimer.reset();
	}

	void process(const ProcessArgs& args) override {
		float freqParam = params[FREQ_PARAM].getValue();
		float fmParam = params[FM_PARAM].getValue();
		float pwParam = params[PW_PARAM].getValue();
		float pwmParam = params[PWM_PARAM].getValue();

		// Gate input inverts the button state while held high (hold toggle).
		// Effective = button XOR gate-high, so the LED and output follow the
		// inverted state whenever the gate is up.
		offset = (params[OFFSET_PARAM].getValue() > 0.f) != (inputs[OFFSET_GATE_INPUT].getVoltage() > 1.f);
		invert = (params[INVERT_PARAM].getValue() > 0.f) != (inputs[INVERT_GATE_INPUT].getVoltage() > 1.f);

		// Clock sync
		if (inputs[CLOCK_INPUT].isConnected()) {
			clockTimer.process(args.sampleTime);
			if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f)) {
				float freq = 1.f / clockTimer.getTime();
				clockTimer.reset();
				if (0.001f <= freq && freq <= 1000.f) {
					clockFreq = freq;
				}
			}
		}
		else {
			// Default frequency when clock is unpatched
			clockFreq = 2.f;
		}

		// Phase offset — when the phase CV input is connected the knob becomes
		// a 0–1 attenuator scaling the CV (0–10 V) into the 0–1 phase range.
		float phaseOffset = params[PHASE_PARAM].getValue();
		if (inputs[PHASE_CV_INPUT].isConnected()) {
			phaseOffset *= clamp(inputs[PHASE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		}

		int channels = std::max(1, inputs[FM_INPUT].getChannels());

		for (int c = 0; c < channels; c += 4) {
			// Pitch and frequency
			float_4 pitch = freqParam;
			pitch += inputs[FM_INPUT].getVoltageSimd<float_4>(c) * fmParam;
			float_4 freq = clockFreq / 2.f * dsp::exp2_taylor5(pitch);

			// Pulse width
			float_4 pw = pwParam;
			pw += inputs[PW_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f * pwmParam;
			pw = clamp(pw, 0.01f, 0.99f);

			// Advance phase
			float_4 deltaPhase = simd::fmin(freq * args.sampleTime, 0.5f);
			phases[c / 4] += deltaPhase;
			phases[c / 4] -= simd::trunc(phases[c / 4]);

			// Reset
			float_4 reset = inputs[RESET_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 resetTriggered = resetTriggers[c / 4].process(reset, 0.1f, 2.f);
			phases[c / 4] = simd::ifelse(resetTriggered, 0.f, phases[c / 4]);

			// Output phase with the phase offset applied (same for all channels)
			float_4 pShift = phases[c / 4] + phaseOffset;

			// Sine
			if (outputs[SIN_OUTPUT].isConnected()) {
				float_4 p = pShift;
				if (offset)
					p -= 0.25f;
				float_4 v = simd::sin(2 * M_PI * p);
				if (invert)
					v *= -1.f;
				if (offset)
					v += 1.f;
				outputs[SIN_OUTPUT].setVoltageSimd(RANGE * v, c);
			}

			// Triangle
			if (outputs[TRI_OUTPUT].isConnected()) {
				float_4 p = pShift;
				if (!offset)
					p += 0.25f;
				float_4 v = 4.f * simd::fabs(p - simd::round(p)) - 1.f;
				if (invert)
					v *= -1.f;
				if (offset)
					v += 1.f;
				outputs[TRI_OUTPUT].setVoltageSimd(RANGE * v, c);
			}

			// Sawtooth
			if (outputs[SAW_OUTPUT].isConnected()) {
				float_4 p = pShift;
				if (offset)
					p -= 0.5f;
				float_4 v = 2.f * (p - simd::round(p));
				if (invert)
					v *= -1.f;
				if (offset)
					v += 1.f;
				outputs[SAW_OUTPUT].setVoltageSimd(RANGE * v, c);
			}

			// Square
			if (outputs[SQR_OUTPUT].isConnected()) {
				float_4 v = simd::ifelse(pShift < pw, 1.f, -1.f);
				if (invert)
					v *= -1.f;
				if (offset)
					v += 1.f;
				outputs[SQR_OUTPUT].setVoltageSimd(RANGE * v, c);
			}
		}

		outputs[SIN_OUTPUT].setChannels(channels);
		outputs[TRI_OUTPUT].setChannels(channels);
		outputs[SAW_OUTPUT].setChannels(channels);
		outputs[SQR_OUTPUT].setChannels(channels);

		// Phase light
		if (lightDivider.process()) {
			if (channels == 1) {
				float p = phases[0][0] + phaseOffset;
				p -= std::floor(p);
				float b = 1.f - p;
				// Purple: red and blue channels driven together
				lights[PHASE_LIGHT + 0].setSmoothBrightness(b, args.sampleTime * lightDivider.getDivision());
				lights[PHASE_LIGHT + 1].setBrightness(0.f);
				lights[PHASE_LIGHT + 2].setSmoothBrightness(b, args.sampleTime * lightDivider.getDivision());
			}
			else {
				// Poly: purple (red+blue) at full brightness
				lights[PHASE_LIGHT + 0].setBrightness(1.f);
				lights[PHASE_LIGHT + 1].setBrightness(0.f);
				lights[PHASE_LIGHT + 2].setBrightness(1.f);
			}
			lights[OFFSET_LIGHT].setBrightness(offset);
			lights[INVERT_LIGHT].setBrightness(invert);
		}
	}
};


struct RaLfoWidget : ModuleWidget {
	RaLfoWidget(RaLfoModule* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-lfo.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RaKnob>(mm2px(Vec(22.902, 29.803)), module, RaLfoModule::FREQ_PARAM));
		addParam(createParamCentered<RaKnob>(mm2px(Vec(14, 56.388)), module, RaLfoModule::PW_PARAM));
		addParam(createParamCentered<RaKnob>(mm2px(Vec(32, 56.388)), module, RaLfoModule::PHASE_PARAM));
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(6.604, 80.603)), module, RaLfoModule::FM_PARAM));
		addParam(createLightParamCentered<VCVLightBezelLatch<WhiteLight>>(mm2px(Vec(17.441, 80.603)), module, RaLfoModule::INVERT_PARAM, RaLfoModule::INVERT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezelLatch<WhiteLight>>(mm2px(Vec(28.279, 80.603)), module, RaLfoModule::OFFSET_PARAM, RaLfoModule::OFFSET_LIGHT));
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(39.116, 80.603)), module, RaLfoModule::PWM_PARAM));

		addInput(createInputCentered<RaPort>(mm2px(Vec(6.604, 94.5)), module, RaLfoModule::FM_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(17.441, 94.5)), module, RaLfoModule::INVERT_GATE_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(28.279, 94.5)), module, RaLfoModule::OFFSET_GATE_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(39.116, 94.5)), module, RaLfoModule::PW_INPUT));

		addInput(createInputCentered<RaPort>(mm2px(Vec(17.441, 107)), module, RaLfoModule::CLOCK_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(28.279, 107)), module, RaLfoModule::RESET_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(39.116, 107)), module, RaLfoModule::PHASE_CV_INPUT));

		addOutput(createOutputCentered<RaPort>(mm2px(Vec(6.604, 119.5)), module, RaLfoModule::SIN_OUTPUT));
		addOutput(createOutputCentered<RaPort>(mm2px(Vec(17.441, 119.5)), module, RaLfoModule::TRI_OUTPUT));
		addOutput(createOutputCentered<RaPort>(mm2px(Vec(28.279, 119.5)), module, RaLfoModule::SAW_OUTPUT));
		addOutput(createOutputCentered<RaPort>(mm2px(Vec(39.116, 119.5)), module, RaLfoModule::SQR_OUTPUT));

		addChild(createLightCentered<RaRGBLight>(mm2px(Vec(31.085, 16.428)), module, RaLfoModule::PHASE_LIGHT));
	}
};


Model* modelRaLfo = createModel<RaLfoModule, RaLfoWidget>("ra-lfo");