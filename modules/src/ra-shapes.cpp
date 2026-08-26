#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

static constexpr float SHAPES_MIN_FREQ = 2.f;
static constexpr float SHAPES_MAX_FREQ = 8000.f;
static constexpr int SHAPES_SLOW_PARAM = 1;

struct RaShapesFreqQuantity : ParamQuantity {
    float getDisplayValue() override {
        float v = getValue();
        float freq = SHAPES_MIN_FREQ * powf(SHAPES_MAX_FREQ / SHAPES_MIN_FREQ, v);
        if (module && module->params[SHAPES_SLOW_PARAM].getValue() > 0.5f)
            freq /= 8.f;
        return freq;
    }
};

struct RaShapesModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        SLOW_PARAM,
        FM1_ATTN_PARAM,
        FM2_ATTN_PARAM,
        PHASE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        FM1_INPUT,
        FM2_INPUT,
        PHASE_CV_INPUT,
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

    float phase[16] = {};
    int lastChannelCount = 0;

    static constexpr float MIN_FREQ = SHAPES_MIN_FREQ;
    static constexpr float MAX_FREQ = SHAPES_MAX_FREQ;

    RaShapesModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam<RaShapesFreqQuantity>(FREQ_PARAM, 0.f, 1.f, 0.5876f, "Frequency", " Hz");
        configSwitch(SLOW_PARAM, 0.f, 1.f, 0.f, "Slow mode", {"Normal", "/8"});
        configParam(FM1_ATTN_PARAM, 0.f, 1.f, 0.f, "FM 1 attenuation", "%", 0.f, 100.f);
        configParam(FM2_ATTN_PARAM, 0.f, 1.f, 0.f, "FM 2 attenuation", "%", 0.f, 100.f);
        configParam(PHASE_PARAM, 0.f, 1.f, 0.f, "Phase offset", "%", 0.f, 100.f);
        configInput(PITCH_INPUT, "1V/Oct");
        configInput(FM1_INPUT, "FM 1");
        configInput(FM2_INPUT, "FM 2");
        configInput(PHASE_CV_INPUT, "Phase CV");
        configOutput(SINE_OUTPUT, "Sine");
        configOutput(TRI_OUTPUT, "Triangle");
        configOutput(SAW_UP_OUTPUT, "Saw up");
        configOutput(SAW_DOWN_OUTPUT, "Saw down");
        configOutput(SQUARE_OUTPUT, "Square");
    }

    void process(const ProcessArgs &args) override {
        int channels = std::max(1, inputs[PITCH_INPUT].getChannels());
        channels = std::max(channels, inputs[FM1_INPUT].getChannels());
        channels = std::max(channels, inputs[FM2_INPUT].getChannels());
        channels = std::max(channels, inputs[PHASE_CV_INPUT].getChannels());
        channels = std::min(channels, 16);

        // Reset phases of channels that just disappeared
        if (channels != lastChannelCount) {
            for (int c = channels; c < lastChannelCount; c++)
                phase[c] = 0.f;
        }
        lastChannelCount = channels;

        for (int c = 0; c < 5; c++)
            outputs[SINE_OUTPUT + c].setChannels(channels);

        for (int c = 0; c < channels; c++) {
            float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
            float pitch = inputs[PITCH_INPUT].getPolyVoltage(c)
                + inputs[FM1_INPUT].getPolyVoltage(c) * params[FM1_ATTN_PARAM].getValue()
                + inputs[FM2_INPUT].getPolyVoltage(c) * params[FM2_ATTN_PARAM].getValue();
            freq *= powf(2.f, pitch);
            freq = clamp(freq, 0.1f, 20000.f);

            if (params[SLOW_PARAM].getValue() > 0.5f)
                freq /= 8.f;

            phase[c] += freq * args.sampleTime;
            if (phase[c] >= 1.f)
                phase[c] -= 1.f;

            float phaseOffset = params[PHASE_PARAM].getValue();
            if (inputs[PHASE_CV_INPUT].isConnected())
                phaseOffset += inputs[PHASE_CV_INPUT].getPolyVoltage(c) / 10.f;
            phaseOffset = clamp(phaseOffset, 0.f, 1.f);

            float p = fmodf(phase[c] + phaseOffset, 1.f);

            float sine = sinf(2.f * M_PI * p);
            float tri = 1.f - 4.f * fabsf(p - 0.5f);
            float sawUp = 2.f * p - 1.f;
            float sawDown = 1.f - 2.f * p;
            float square = (p < 0.5f) ? 1.f : -1.f;

            float scale = 5.f;
            outputs[SINE_OUTPUT].setVoltage(sine * scale, c);
            outputs[TRI_OUTPUT].setVoltage(tri * scale, c);
            outputs[SAW_UP_OUTPUT].setVoltage(sawUp * scale, c);
            outputs[SAW_DOWN_OUTPUT].setVoltage(sawDown * scale, c);
            outputs[SQUARE_OUTPUT].setVoltage(square * scale, c);
        }
    }
};

struct RaShapesWidget : ModuleWidget {
    RaShapesWidget(RaShapesModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-shapes.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnobLarge>(Vec(30, 68), module, RaShapesModule::FREQ_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(46, 112), module, RaShapesModule::SLOW_PARAM));
        addInput(createInputCentered<RaPort>(Vec(14, 112), module, RaShapesModule::PITCH_INPUT));

        addParam(createParamCentered<RaKnobSmall>(Vec(14, 146), module, RaShapesModule::FM1_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 146), module, RaShapesModule::FM1_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 178), module, RaShapesModule::FM2_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 178), module, RaShapesModule::FM2_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 220), module, RaShapesModule::PHASE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 220), module, RaShapesModule::PHASE_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(14, 274), module, RaShapesModule::SINE_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(46, 274), module, RaShapesModule::TRI_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(14, 306), module, RaShapesModule::SAW_UP_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(46, 306), module, RaShapesModule::SAW_DOWN_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(14, 340), module, RaShapesModule::SQUARE_OUTPUT));
    }
};

Model *modelRaShapes = createModel<RaShapesModule, RaShapesWidget>("ra-shapes");
