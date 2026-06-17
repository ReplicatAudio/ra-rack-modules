#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaUlfoModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        ATTN_PARAM,
        RANGE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        SINE_OUTPUT,
        COSINE_OUTPUT,
        INV_SINE_OUTPUT,
        INV_COSINE_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float phase = 0.f;

    RaUlfoModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.1f, "Frequency", "Hz", 0.f, 1.f);
        configParam(ATTN_PARAM, 0.f, 1.f, 1.f, "Attenuation", "%", 0.f, 100.f);
        configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"\u00B15V", "0\u201310V"});
        configOutput(SINE_OUTPUT, "Sine");
        configOutput(COSINE_OUTPUT, "Cosine");
        configOutput(INV_SINE_OUTPUT, "Inverted sine");
        configOutput(INV_COSINE_OUTPUT, "Inverted cosine");
    }

    void process(const ProcessArgs &args) override {
        float freq = params[FREQ_PARAM].getValue();

        phase += freq * args.sampleTime;
        if (phase >= 1.f)
            phase -= 1.f;

        float angle = 2.f * M_PI * phase;
        float s = sinf(angle);
        float c = cosf(angle);

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
    }
};

struct RaUlfoWidget : ModuleWidget {
    RaUlfoWidget(RaUlfoModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ulfo.svg")));

        addChild(createWidget<ScrewSilver>(Vec(0, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(Vec(box.size.x / 2, 24), module, RaUlfoModule::FREQ_PARAM));
        addParam(createParamCentered<CKSS>(Vec(box.size.x - 8, 46), module, RaUlfoModule::RANGE_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(box.size.x / 2, 72), module, RaUlfoModule::ATTN_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(Vec(16, 106), module, RaUlfoModule::SINE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(44, 106), module, RaUlfoModule::COSINE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(16, 128), module, RaUlfoModule::INV_SINE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(44, 128), module, RaUlfoModule::INV_COSINE_OUTPUT));
    }
};

Model *modelRaUlfo = createModel<RaUlfoModule, RaUlfoWidget>("ra-ulfo");
