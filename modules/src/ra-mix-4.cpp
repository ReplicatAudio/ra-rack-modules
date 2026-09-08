#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaMix4Module : Module {
	enum ParamIds {
		GAIN1_PARAM,
		GAIN2_PARAM,
		GAIN3_PARAM,
		GAIN4_PARAM,
		PAN1_PARAM,
		PAN2_PARAM,
		PAN3_PARAM,
		PAN4_PARAM,
		MASTER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CH1_INPUT,
		CH2_INPUT,
		CH3_INPUT,
		CH4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_L,
		OUT_R,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	RaMix4Module() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(GAIN1_PARAM, 0.f, 1.5f, 1.f, "Ch 1");
		configParam(GAIN2_PARAM, 0.f, 1.5f, 1.f, "Ch 2");
		configParam(GAIN3_PARAM, 0.f, 1.5f, 1.f, "Ch 3");
		configParam(GAIN4_PARAM, 0.f, 1.5f, 1.f, "Ch 4");
		configParam(PAN1_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(PAN2_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(PAN3_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(PAN4_PARAM, -1.f, 1.f, 0.f, "Pan");
		configParam(MASTER_PARAM, 0.f, 1.f, 1.f, "Master");
		configInput(CH1_INPUT, "In 1");
		configInput(CH2_INPUT, "In 2");
		configInput(CH3_INPUT, "In 3");
		configInput(CH4_INPUT, "In 4");
		configOutput(OUT_L, "Left");
		configOutput(OUT_R, "Right");
	}

	void process(const ProcessArgs &args) override {
		float left = 0.f;
		float right = 0.f;

		for (int c = 0; c < 4; c++) {
			float in = inputs[CH1_INPUT + c].getVoltage();
			float gain = params[GAIN1_PARAM + c].getValue();
			float pan = params[PAN1_PARAM + c].getValue();
			// Equal-power pan: pan -1 = hard left, 0 = center, +1 = hard right
			float a = cosf((pan + 1.f) * M_PI / 4.f);
			float b = sinf((pan + 1.f) * M_PI / 4.f);
			left += in * gain * a;
			right += in * gain * b;
		}

		float master = params[MASTER_PARAM].getValue();
		outputs[OUT_L].setVoltage(clamp(left * master, -10.f, 10.f));
		outputs[OUT_R].setVoltage(clamp(right * master, -10.f, 10.f));
	}
};

struct RaMix4Widget : ModuleWidget {
	RaMix4Widget(RaMix4Module *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-mix-4.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		// Channel strips
		addParam(createParamCentered<RaKnobSmall>(Vec(22.5, 120), module, RaMix4Module::PAN1_PARAM));
		addInput(createInputCentered<RaPort>(Vec(22.5, 175), module, RaMix4Module::CH1_INPUT));
		addParam(createParamCentered<RaSlider>(Vec(22.5, 270), module, RaMix4Module::GAIN1_PARAM));

		addParam(createParamCentered<RaKnobSmall>(Vec(52.5, 120), module, RaMix4Module::PAN2_PARAM));
		addInput(createInputCentered<RaPort>(Vec(52.5, 175), module, RaMix4Module::CH2_INPUT));
		addParam(createParamCentered<RaSlider>(Vec(52.5, 270), module, RaMix4Module::GAIN2_PARAM));

		addParam(createParamCentered<RaKnobSmall>(Vec(82.5, 120), module, RaMix4Module::PAN3_PARAM));
		addInput(createInputCentered<RaPort>(Vec(82.5, 175), module, RaMix4Module::CH3_INPUT));
		addParam(createParamCentered<RaSlider>(Vec(82.5, 270), module, RaMix4Module::GAIN3_PARAM));

		addParam(createParamCentered<RaKnobSmall>(Vec(112.5, 120), module, RaMix4Module::PAN4_PARAM));
		addInput(createInputCentered<RaPort>(Vec(112.5, 175), module, RaMix4Module::CH4_INPUT));
		addParam(createParamCentered<RaSlider>(Vec(112.5, 270), module, RaMix4Module::GAIN4_PARAM));

		// Master section
		addParam(createParamCentered<RaSlider>(Vec(142.5, 270), module, RaMix4Module::MASTER_PARAM));
		addOutput(createOutputCentered<RaPort>(Vec(127.5, 345), module, RaMix4Module::OUT_L));
		addOutput(createOutputCentered<RaPort>(Vec(157.5, 345), module, RaMix4Module::OUT_R));
	}
};

Model *modelRaMix4 = createModel<RaMix4Module, RaMix4Widget>("ra-mix-4");