#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

// 10 s of delay at any sample rate (same capacity as VCV Delay)
static const size_t HISTORY_SIZE = 1 << 21;

// Time knob mapping shared with VCV Delay: time = 0.001 * 10000^timeParam
static constexpr float TIME_DEFAULT = std::log10(500.f) / 4.f;

struct RaDelayCvModule : Module {
	enum ParamIds {
		TIME_PARAM,
		FEEDBACK_PARAM,
		TIME_CV_PARAM,
		FEEDBACK_CV_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TIME_INPUT,
		FEEDBACK_INPUT,
		IN_INPUT,
		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		WET_OUTPUT,
		ECHO_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		NUM_LIGHTS
	};

	float history[HISTORY_SIZE];
	int writePos = 0;
	float lastWet = 0.f;
	float clockFreq = 1.f;
	dsp::Timer clockTimer;
	dsp::SchmittTrigger clockTrigger;
	float clockPhase = 0.f;

	RaDelayCvModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(TIME_PARAM, 0.f, 1.f, TIME_DEFAULT, "Time", " s", 10.f / 1e-3, 1e-3);
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.5f, "Feedback", "%", 0, 100);
		configParam(TIME_CV_PARAM, -1.f, 1.f, 0.f, "Time CV", "%", 0, 100);
		getParamQuantity(TIME_CV_PARAM)->randomizeEnabled = false;
		configParam(FEEDBACK_CV_PARAM, -1.f, 1.f, 0.f, "Feedback CV", "%", 0, 100);
		getParamQuantity(FEEDBACK_CV_PARAM)->randomizeEnabled = false;

		configInput(TIME_INPUT, "Time");
		getInputInfo(TIME_INPUT)->description = "1V/octave when Time CV is 100%";
		configInput(FEEDBACK_INPUT, "Feedback");
		configInput(IN_INPUT, "CV / Trigger");
		getInputInfo(IN_INPUT)->description = "Signal to delay. DC-coupled: gates, triggers, and CV pass through unchanged.";
		configInput(CLOCK_INPUT, "Clock");
		configOutput(WET_OUTPUT, "Wet");
		configOutput(ECHO_OUTPUT, "Echo");
		getOutputInfo(ECHO_OUTPUT)->description = "Input plus delayed signal, full amplitude.";

		configBypass(IN_INPUT, WET_OUTPUT);
		configBypass(IN_INPUT, ECHO_OUTPUT);

		std::memset(history, 0, sizeof(history));
	}

	void onReset() override {
		std::memset(history, 0, sizeof(history));
		writePos = 0;
		lastWet = 0.f;
		clockFreq = 1.f;
		clockPhase = 0.f;
	}

	void process(const ProcessArgs& args) override {
		// Clock (same behavior as VCV Delay: Time becomes a ratio of the half clock period)
		if (inputs[CLOCK_INPUT].isConnected()) {
			clockTimer.process(args.sampleTime);
			if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f)) {
				float f = 1.f / clockTimer.getTime();
				clockTimer.reset();
				if (0.001f <= f && f <= 1000.f)
					clockFreq = f;
			}
		}
		else {
			clockFreq = 2.f;
		}

		float in = inputs[IN_INPUT].getVoltageSum();

		float feedback = params[FEEDBACK_PARAM].getValue() + inputs[FEEDBACK_INPUT].getVoltage() / 10.f * params[FEEDBACK_CV_PARAM].getValue();
		feedback = clamp(feedback, 0.f, 1.f);

		float dry = in + lastWet * feedback;

		// Compute delay time
		float pitch = std::log2(1000.f) - std::log2(10000.f) * params[TIME_PARAM].getValue();
		pitch += inputs[TIME_INPUT].getVoltage() * params[TIME_CV_PARAM].getValue();
		float freq = clockFreq / 2.f * dsp::exp2_taylor5(pitch);

		// Desired delay in samples
		float index = args.sampleRate / freq;
		index = clamp(index, 2.f, float(HISTORY_SIZE - 2));

		// Push input into history buffer
		history[writePos] = dry;

		// Read tap with linear interpolation. No resampling stage, no filters on
		// the read path: edges stay sharp and DC is preserved, so gates, triggers,
		// and steady CV come out at their original level and width.
		float posF = writePos - index;
		if (posF < 0.f)
			posF += HISTORY_SIZE;
		int i0 = (int) posF;
		int i1 = i0 + 1 == (int) HISTORY_SIZE ? 0 : i0 + 1;
		float wet = crossfade(history[i0], history[i1], posF - i0);

		wet = clamp(wet, -100.f, 100.f);

		// Wet output
		outputs[WET_OUTPUT].setVoltage(wet);
		lastWet = wet;

		// Echo output: input plus delayed signal, both full amplitude
		outputs[ECHO_OUTPUT].setVoltage(in + wet);

		// Clock light
		clockPhase += freq * args.sampleTime;
		if (clockPhase >= 1.f) {
			clockPhase -= 1.f;
			lights[CLOCK_LIGHT].setBrightness(1.f);
		}
		else {
			lights[CLOCK_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
		}

		writePos = (writePos + 1) % (int) HISTORY_SIZE;
	}
};

struct PurpleLight : GrayModuleLightWidget {
	PurpleLight() {
		addBaseColor(nvgRGB(0x99, 0x6d, 0xd2));
	}
};

struct RaDelayCvWidget : ModuleWidget {
	RaDelayCvWidget(RaDelayCvModule* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-delay-cv.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		const float cx = 10.16f;

		addChild(createLightCentered<LargeLight<PurpleLight>>(mm2px(Vec(cx, 13.f)), module, RaDelayCvModule::CLOCK_LIGHT));

		// Knobs
		addParam(createParamCentered<RaKnob>(mm2px(Vec(cx, 24.f)), module, RaDelayCvModule::TIME_PARAM));
		addParam(createParamCentered<RaKnob>(mm2px(Vec(cx, 38.f)), module, RaDelayCvModule::FEEDBACK_PARAM));

		// CV attenuverters
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(cx, 52.f)), module, RaDelayCvModule::TIME_CV_PARAM));
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(cx, 61.5f)), module, RaDelayCvModule::FEEDBACK_CV_PARAM));

		// CV inputs
		addInput(createInputCentered<RaPort>(mm2px(Vec(cx, 73.f)), module, RaDelayCvModule::TIME_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(cx, 83.f)), module, RaDelayCvModule::FEEDBACK_INPUT));

		// Signal I/O — 2x2 grid
		addInput(createInputCentered<RaPort>(mm2px(Vec(5.08f, 96.f)), module, RaDelayCvModule::IN_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(15.24f, 96.f)), module, RaDelayCvModule::CLOCK_INPUT));
		addOutput(createOutputCentered<RaPort>(mm2px(Vec(5.08f, 110.f)), module, RaDelayCvModule::WET_OUTPUT));
		addOutput(createOutputCentered<RaPort>(mm2px(Vec(15.24f, 110.f)), module, RaDelayCvModule::ECHO_OUTPUT));
	}
};

Model* modelRaDelayCv = createModel<RaDelayCvModule, RaDelayCvWidget>("ra-delay-cv");
