#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

// Param/input indices used by RaUlfoFreqQuantity (must match enums in RaUlfoModule)
static constexpr int RA_ULFO_FREQ_CV_INPUT = 3;
static constexpr int RA_ULFO_FM_INPUT = 5;
static constexpr int RA_ULFO_FM_ATTN_PARAM = 8;

struct RaUlfoFreqQuantity : ParamQuantity {
	float getDisplayValue() override {
		float freq = getValue();
		if (module) {
			if (module->inputs[RA_ULFO_FREQ_CV_INPUT].isConnected())
				freq += module->inputs[RA_ULFO_FREQ_CV_INPUT].getVoltage() / 10.f;
			if (module->inputs[RA_ULFO_FM_INPUT].isConnected())
				freq *= powf(2.f, module->inputs[RA_ULFO_FM_INPUT].getVoltage() * module->params[RA_ULFO_FM_ATTN_PARAM].getValue());
		}
		return std::max(freq, 0.f);
	}
};

struct RaUlfoModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        ATTN_PARAM,
        RANGE_PARAM,
        PHASE_PARAM,
        A_PARAM,
        A_ATTN_PARAM,
        B_PARAM,
        B_ATTN_PARAM,
        FM_ATTN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        A_CV_INPUT,
        B_CV_INPUT,
        RESET_INPUT,
        FREQ_CV_INPUT,
        PHASE_CV_INPUT,
        FM_INPUT,
        ATTN_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SINE_OUTPUT,
        COSINE_OUTPUT,
        INV_SINE_OUTPUT,
        INV_COSINE_OUTPUT,
        FORMULA_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float phase = 0.f;
    float phaseB = 0.f;
    dsp::SchmittTrigger resetTrigger;

    RaUlfoModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam<RaUlfoFreqQuantity>(FREQ_PARAM, 0.f, 1.f, 0.1f, "Frequency", "Hz", 1.f, 0.f);
        configParam(ATTN_PARAM, 0.f, 1.f, 1.f, "Attenuation", "%", 0.f, 100.f);
        configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"\u00B15V", "0\u201310V"});
        configParam(PHASE_PARAM, 0.f, 1.f, 0.f, "Phase shift");
        configParam(A_PARAM, 0.f, 1.f, 0.5f, "A", "%", 0.f, 100.f);
        configParam(A_ATTN_PARAM, -1.f, 1.f, 0.f, "A CV attenuverter");
        configParam(B_PARAM, 0.f, 10.f, 0.f, "B");
        configParam(B_ATTN_PARAM, -1.f, 1.f, 0.f, "B CV attenuverter");
        configParam(FM_ATTN_PARAM, 0.f, 1.f, 0.f, "FM attenuation", "%", 0.f, 100.f);
        configInput(A_CV_INPUT, "A input");
        configInput(B_CV_INPUT, "B input");
        configInput(RESET_INPUT, "Reset");
        configInput(FREQ_CV_INPUT, "Frequency CV");
        configInput(PHASE_CV_INPUT, "Phase CV");
        configInput(FM_INPUT, "FM");
        configInput(ATTN_CV_INPUT, "Attenuation CV");
        configOutput(SINE_OUTPUT, "Sine");
        configOutput(COSINE_OUTPUT, "Cosine");
        configOutput(INV_SINE_OUTPUT, "Inverted sine");
        configOutput(INV_COSINE_OUTPUT, "Inverted cosine");
        configOutput(FORMULA_OUTPUT, "Formula");
    }

    void process(const ProcessArgs &args) override {
        float freq = params[FREQ_PARAM].getValue();
        if (inputs[FREQ_CV_INPUT].isConnected())
            freq += inputs[FREQ_CV_INPUT].getVoltage() / 10.f;
        if (inputs[FM_INPUT].isConnected())
            freq *= powf(2.f, inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue());
        freq = std::max(freq, 0.f);

        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
            phase = 0.f;
            phaseB = 0.f;
        }

        phase += freq * args.sampleTime;
        if (phase >= 1.f)
            phase -= 1.f;

        float phaseOffset = params[PHASE_PARAM].getValue();
        if (inputs[PHASE_CV_INPUT].isConnected())
            phaseOffset += inputs[PHASE_CV_INPUT].getVoltage() / 10.f;
        phaseOffset = clamp(phaseOffset, 0.f, 1.f);
        float x = 2.f * M_PI * (phase + phaseOffset);
        float s = sinf(x);
        float c = cosf(x);

        float attn;
        if (inputs[ATTN_CV_INPUT].isConnected())
            attn = clamp(inputs[ATTN_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f) * 5.f;
        else
            attn = params[ATTN_PARAM].getValue() * 5.f;
        float vSine, vCosine;
        if (params[RANGE_PARAM].getValue() > 0.5f) {
            vSine = (s * 0.5f + 0.5f) * attn * 2.f;
            vCosine = (c * 0.5f + 0.5f) * attn * 2.f;
        } else {
            vSine = s * attn;
            vCosine = c * attn;
        }

        outputs[SINE_OUTPUT].setVoltage(vSine);
        outputs[COSINE_OUTPUT].setVoltage(vCosine);
        outputs[INV_SINE_OUTPUT].setVoltage(-vSine);
        outputs[INV_COSINE_OUTPUT].setVoltage(-vCosine);

        float a = params[A_PARAM].getValue();
        if (inputs[A_CV_INPUT].isConnected()) {
            a += inputs[A_CV_INPUT].getVoltage() * params[A_ATTN_PARAM].getValue() / 10.f;
            a = clamp(a, 0.f, 1.f);
        }
        float b = params[B_PARAM].getValue();
        if (inputs[B_CV_INPUT].isConnected()) {
            b += inputs[B_CV_INPUT].getVoltage() * params[B_ATTN_PARAM].getValue();
            b = clamp(b, 0.f, 10.f);
        }

        phaseB += freq * b * args.sampleTime;
        if (phaseB >= 1.f)
            phaseB -= 1.f;

        float formula = ((2.f - a) * (s + cosf(2.f * M_PI * (phaseB + phaseOffset * b)) * a)) / 2.f;
        if (params[RANGE_PARAM].getValue() == 0.f)
            outputs[FORMULA_OUTPUT].setVoltage(formula * attn);
        else
            outputs[FORMULA_OUTPUT].setVoltage((formula * 0.5f + 0.5f) * attn * 2.f);
    }
};

struct RaUlfoWidget : ModuleWidget {
    RaUlfoWidget(RaUlfoModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ulfo.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // 8 HP panel (120 units wide) — three columns: 24 / 60 / 96
        addParam(createParamCentered<RaKnob>(Vec(60, 36), module, RaUlfoModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 70), module, RaUlfoModule::RESET_INPUT));
        addInput(createInputCentered<RaPort>(Vec(60, 70), module, RaUlfoModule::FREQ_CV_INPUT));
        addParam(createParamCentered<RaSwitch2>(Vec(96, 70), module, RaUlfoModule::RANGE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 104), module, RaUlfoModule::FM_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(60, 104), module, RaUlfoModule::ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(96, 104), module, RaUlfoModule::ATTN_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(24, 138), module, RaUlfoModule::FM_ATTN_PARAM));

        addOutput(createOutputCentered<RaPort>(Vec(24, 172), module, RaUlfoModule::SINE_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(96, 172), module, RaUlfoModule::COSINE_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(24, 204), module, RaUlfoModule::INV_SINE_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(96, 204), module, RaUlfoModule::INV_COSINE_OUTPUT));

        addParam(createParamCentered<RaKnobSmall>(Vec(60, 232), module, RaUlfoModule::PHASE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(60, 262), module, RaUlfoModule::PHASE_CV_INPUT));

        addParam(createParamCentered<RaKnobSmall>(Vec(24, 290), module, RaUlfoModule::A_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(96, 290), module, RaUlfoModule::B_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(24, 320), module, RaUlfoModule::A_ATTN_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(96, 320), module, RaUlfoModule::B_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 350), module, RaUlfoModule::A_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(96, 350), module, RaUlfoModule::B_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(60, 350), module, RaUlfoModule::FORMULA_OUTPUT));
    }
};

Model *modelRaUlfo = createModel<RaUlfoModule, RaUlfoWidget>("ra-ulfo");
