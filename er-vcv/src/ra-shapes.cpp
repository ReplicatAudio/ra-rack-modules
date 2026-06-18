#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaShapesModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        FM1_ATTN_PARAM,
        FM2_ATTN_PARAM,
        FM3_ATTN_PARAM,
        FM4_ATTN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        FM1_INPUT,
        FM2_INPUT,
        FM3_INPUT,
        FM4_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SINE_OUTPUT,
        TRI_OUTPUT,
        SAW_UP_OUTPUT,
        SAW_DOWN_OUTPUT,
        SQUARE_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float phase = 0.f;

    static constexpr float MIN_FREQ = 2.f;
    static constexpr float MAX_FREQ = 8000.f;

    RaShapesModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.2f, "Frequency", " Hz", MAX_FREQ / MIN_FREQ, MIN_FREQ, 0.f);
        configParam(FM1_ATTN_PARAM, 0.f, 1.f, 0.f, "FM 1 attenuation", "%", 0.f, 100.f);
        configParam(FM2_ATTN_PARAM, 0.f, 1.f, 0.f, "FM 2 attenuation", "%", 0.f, 100.f);
        configParam(FM3_ATTN_PARAM, 0.f, 1.f, 0.f, "FM 3 attenuation", "%", 0.f, 100.f);
        configParam(FM4_ATTN_PARAM, 0.f, 1.f, 0.f, "FM 4 attenuation", "%", 0.f, 100.f);
        configInput(PITCH_INPUT, "1V/Oct");
        configInput(FM1_INPUT, "FM 1");
        configInput(FM2_INPUT, "FM 2");
        configInput(FM3_INPUT, "FM 3");
        configInput(FM4_INPUT, "FM 4");
        configOutput(SINE_OUTPUT, "Sine");
        configOutput(TRI_OUTPUT, "Triangle");
        configOutput(SAW_UP_OUTPUT, "Saw up");
        configOutput(SAW_DOWN_OUTPUT, "Saw down");
        configOutput(SQUARE_OUTPUT, "Square");
    }

    void process(const ProcessArgs &args) override {
        float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
        float pitch = inputs[PITCH_INPUT].getVoltage()
            + inputs[FM1_INPUT].getVoltage() * params[FM1_ATTN_PARAM].getValue()
            + inputs[FM2_INPUT].getVoltage() * params[FM2_ATTN_PARAM].getValue()
            + inputs[FM3_INPUT].getVoltage() * params[FM3_ATTN_PARAM].getValue()
            + inputs[FM4_INPUT].getVoltage() * params[FM4_ATTN_PARAM].getValue();
        freq *= powf(2.f, pitch);
        freq = clamp(freq, 0.1f, 20000.f);

        phase += freq * args.sampleTime;
        if (phase >= 1.f)
            phase -= 1.f;

        float p = phase;

        float sine = sinf(2.f * M_PI * p);
        float tri = 1.f - 4.f * fabsf(p - 0.5f);
        float sawUp = 2.f * p - 1.f;
        float sawDown = 1.f - 2.f * p;
        float square = (p < 0.5f) ? 1.f : -1.f;

        float scale = 5.f;
        outputs[SINE_OUTPUT].setVoltage(sine * scale);
        outputs[TRI_OUTPUT].setVoltage(tri * scale);
        outputs[SAW_UP_OUTPUT].setVoltage(sawUp * scale);
        outputs[SAW_DOWN_OUTPUT].setVoltage(sawDown * scale);
        outputs[SQUARE_OUTPUT].setVoltage(square * scale);
    }
};

struct RaShapesWidget : ModuleWidget {
    RaShapesWidget(RaShapesModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-shapes.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(Vec(30, 24), module, RaShapesModule::FREQ_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(30, 54), module, RaShapesModule::PITCH_INPUT));

        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(14, 82), module, RaShapesModule::FM1_ATTN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(46, 82), module, RaShapesModule::FM1_INPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(14, 110), module, RaShapesModule::FM2_ATTN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(46, 110), module, RaShapesModule::FM2_INPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(14, 138), module, RaShapesModule::FM3_ATTN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(46, 138), module, RaShapesModule::FM3_INPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(14, 166), module, RaShapesModule::FM4_ATTN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(46, 166), module, RaShapesModule::FM4_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(16, 196), module, RaShapesModule::SINE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(44, 196), module, RaShapesModule::TRI_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(16, 222), module, RaShapesModule::SAW_UP_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(44, 222), module, RaShapesModule::SAW_DOWN_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(30, 248), module, RaShapesModule::SQUARE_OUTPUT));
    }
};

Model *modelRaShapes = createModel<RaShapesModule, RaShapesWidget>("ra-shapes");
