#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

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
        NUM_PARAMS
    };
    enum InputIds {
        A_CV_INPUT,
        B_CV_INPUT,
        RESET_INPUT,
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
        configParam(FREQ_PARAM, 0.f, 1.f, 0.1f, "Frequency", "Hz", 1.f, 0.f);
        configParam(ATTN_PARAM, 0.f, 1.f, 1.f, "Attenuation", "%", 0.f, 100.f);
        configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"\u00B15V", "0\u201310V"});
        configParam(PHASE_PARAM, 0.f, 1.f, 0.f, "Phase shift");
        configParam(A_PARAM, 0.f, 1.f, 0.5f, "A", "%", 0.f, 100.f);
        configParam(A_ATTN_PARAM, -1.f, 1.f, 0.f, "A CV attenuverter");
        configParam(B_PARAM, 0.f, 10.f, 0.f, "B");
        configParam(B_ATTN_PARAM, -1.f, 1.f, 0.f, "B CV attenuverter");
        configInput(A_CV_INPUT, "A input");
        configInput(B_CV_INPUT, "B input");
        configInput(RESET_INPUT, "Reset");
        configOutput(SINE_OUTPUT, "Sine");
        configOutput(COSINE_OUTPUT, "Cosine");
        configOutput(INV_SINE_OUTPUT, "Inverted sine");
        configOutput(INV_COSINE_OUTPUT, "Inverted cosine");
        configOutput(FORMULA_OUTPUT, "Formula");
    }

    void process(const ProcessArgs &args) override {
        float freq = params[FREQ_PARAM].getValue();

        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
            phase = 0.f;
            phaseB = 0.f;
        }

        phase += freq * args.sampleTime;
        if (phase >= 1.f)
            phase -= 1.f;

        float phaseOffset = params[PHASE_PARAM].getValue();
        float x = 2.f * M_PI * (phase + phaseOffset);
        float s = sinf(x);
        float c = cosf(x);

        float vSine, vCosine;
        if (params[RANGE_PARAM].getValue() > 0.5f) {
            float attn = params[ATTN_PARAM].getValue() * 5.f;
            vSine = (s * 0.5f + 0.5f) * attn * 2.f;
            vCosine = (c * 0.5f + 0.5f) * attn * 2.f;
        } else {
            float attn = params[ATTN_PARAM].getValue() * 5.f;
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
            outputs[FORMULA_OUTPUT].setVoltage((formula + 1.f) * 5.f);
        else
            outputs[FORMULA_OUTPUT].setVoltage(formula * 5.f);
    }
};

struct RaUlfoWidget : ModuleWidget {
    RaUlfoWidget(RaUlfoModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ulfo.svg")));

        addChild(createWidget<RaScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnob>(Vec(box.size.x / 2, 24), module, RaUlfoModule::FREQ_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(box.size.x - 8, 46), module, RaUlfoModule::RANGE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(14, 52), module, RaUlfoModule::RESET_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(box.size.x / 2, 72), module, RaUlfoModule::ATTN_PARAM));

        addOutput(createOutputCentered<RaPort>(Vec(16, 106), module, RaUlfoModule::SINE_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 106), module, RaUlfoModule::COSINE_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(16, 128), module, RaUlfoModule::INV_SINE_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 128), module, RaUlfoModule::INV_COSINE_OUTPUT));

        addParam(createParamCentered<RaKnobSmall>(Vec(box.size.x / 2, 158), module, RaUlfoModule::PHASE_PARAM));

        addParam(createParamCentered<RaKnobSmall>(Vec(16, 194), module, RaUlfoModule::A_PARAM));
        addInput(createInputCentered<RaPort>(Vec(44, 190), module, RaUlfoModule::A_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(44, 212), module, RaUlfoModule::A_ATTN_PARAM));

        addParam(createParamCentered<RaKnobSmall>(Vec(16, 242), module, RaUlfoModule::B_PARAM));
        addInput(createInputCentered<RaPort>(Vec(44, 238), module, RaUlfoModule::B_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(44, 260), module, RaUlfoModule::B_ATTN_PARAM));

        addOutput(createOutputCentered<RaPort>(Vec(box.size.x / 2, 290), module, RaUlfoModule::FORMULA_OUTPUT));
    }
};

Model *modelRaUlfo = createModel<RaUlfoModule, RaUlfoWidget>("ra-ulfo");
