#include "ra-components.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RaAccumulatorModule : Module {
	enum ParamIds {
		ORIGIN_PARAM,
		DELTA_PARAM,
		SLEW_PARAM,
		WRITE_PARAM,
		RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ORIGIN_INPUT,
		DELTA_INPUT,
		SLEW_INPUT,
		WRITE_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		WRITE_LIGHT,
		RESET_LIGHT,
		NUM_LIGHTS
	};

	float deltaFromOrigin = 0.f;
	float slewedValue = 0.f;
	std::atomic<float> displayValue{0.f};
	dsp::SchmittTrigger writeExtTrigger;
	dsp::SchmittTrigger resetExtTrigger;
	dsp::SchmittTrigger writeBtnTrigger;
	dsp::SchmittTrigger resetBtnTrigger;

	RaAccumulatorModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ORIGIN_PARAM, -10.f, 10.f, 0.f, "Origin", " V");
		configParam(DELTA_PARAM, -10.f, 10.f, 0.f, "Delta", " V");
		configParam(SLEW_PARAM, 0.f, 10.f, 0.f, "Slew", " s");

		configButton(WRITE_PARAM, "Write");
		configButton(RESET_PARAM, "Reset");

		configInput(ORIGIN_INPUT, "Origin CV");
		configInput(DELTA_INPUT, "Delta CV");
		configInput(SLEW_INPUT, "Slew CV");
		configInput(WRITE_INPUT, "Write trig");
		configInput(RESET_INPUT, "Reset trig");

		configOutput(CV_OUTPUT, "CV");
	}

	void process(const ProcessArgs &args) override {
		float origin = inputs[ORIGIN_INPUT].isConnected()
			? inputs[ORIGIN_INPUT].getVoltage()
			: params[ORIGIN_PARAM].getValue();

		float delta = inputs[DELTA_INPUT].isConnected()
			? inputs[DELTA_INPUT].getVoltage()
			: params[DELTA_PARAM].getValue();

		float slewTime = inputs[SLEW_INPUT].isConnected()
			? clamp(inputs[SLEW_INPUT].getVoltage(), 0.f, 10.f)
			: params[SLEW_PARAM].getValue();

		bool reset = resetExtTrigger.process(inputs[RESET_INPUT].getVoltage());
		reset |= resetBtnTrigger.process(params[RESET_PARAM].getValue());
		if (reset)
			deltaFromOrigin = 0.f;

		bool write = writeExtTrigger.process(inputs[WRITE_INPUT].getVoltage());
		write |= writeBtnTrigger.process(params[WRITE_PARAM].getValue());
		if (write)
			deltaFromOrigin += delta;

		float target = origin + deltaFromOrigin;
		if (slewTime > 0.f) {
			float lambda = 1.f / slewTime;
			float coeff = 1.f - std::exp(-lambda * args.sampleTime);
			slewedValue += (target - slewedValue) * coeff;
			outputs[CV_OUTPUT].setVoltage(slewedValue);
		} else {
			slewedValue = target;
			outputs[CV_OUTPUT].setVoltage(target);
		}

		displayValue.store(outputs[CV_OUTPUT].getVoltage(), std::memory_order_relaxed);

		lights[WRITE_LIGHT].setBrightnessSmooth(write ? 1.f : 0.f, args.sampleTime);
		lights[RESET_LIGHT].setBrightnessSmooth(reset ? 1.f : 0.f, args.sampleTime);
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "deltaFromOrigin", json_real(deltaFromOrigin));
		json_object_set_new(rootJ, "slewedValue", json_real(slewedValue));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *d = json_object_get(rootJ, "deltaFromOrigin");
		if (d) deltaFromOrigin = json_real_value(d);
		json_t *sv = json_object_get(rootJ, "slewedValue");
		if (sv) slewedValue = json_real_value(sv);
	}
};

struct AccDisplay : Widget {
	RaAccumulatorModule *module;
	std::shared_ptr<Font> font;

	AccDisplay() {
		font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
	}

	void draw(const DrawArgs &args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
		nvgFill(args.vg);

		if (!module || !font) return;

		float v = module->displayValue.load(std::memory_order_relaxed);

		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 14);
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		char buf[32];
		snprintf(buf, sizeof(buf), "%+.2fV", v);
		nvgText(args.vg, box.size.x / 2, box.size.y / 2, buf, NULL);
	}
};

struct RaAccumulatorWidget : ModuleWidget {
	RaAccumulatorWidget(RaAccumulatorModule *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-accumulator.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		float leftX = 30;
		float rightX = 90;
		float centerX = box.size.x / 2;

		float ys[] = {30, 74, 118, 162, 206};
		float displayY = 246;
		float displayH = 46;
		float outY = 330;

		addParam(createParamCentered<RaKnob>(Vec(leftX, ys[0]), module, RaAccumulatorModule::ORIGIN_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[0]), module, RaAccumulatorModule::ORIGIN_INPUT));

		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(leftX, ys[1]), module, RaAccumulatorModule::WRITE_PARAM, RaAccumulatorModule::WRITE_LIGHT));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[1]), module, RaAccumulatorModule::WRITE_INPUT));

		addParam(createParamCentered<RaKnob>(Vec(leftX, ys[2]), module, RaAccumulatorModule::DELTA_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[2]), module, RaAccumulatorModule::DELTA_INPUT));

		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(leftX, ys[3]), module, RaAccumulatorModule::RESET_PARAM, RaAccumulatorModule::RESET_LIGHT));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[3]), module, RaAccumulatorModule::RESET_INPUT));

		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, ys[4]), module, RaAccumulatorModule::SLEW_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[4]), module, RaAccumulatorModule::SLEW_INPUT));

		auto *display = new AccDisplay();
		display->box.pos = Vec(30, displayY);
		display->box.size = Vec(60, displayH);
		display->module = module;
		addChild(display);

		addOutput(createOutputCentered<RaPort>(Vec(centerX, outY), module, RaAccumulatorModule::CV_OUTPUT));
	}
};

Model *modelRaAccumulator = createModel<RaAccumulatorModule, RaAccumulatorWidget>("ra-accumulator");
