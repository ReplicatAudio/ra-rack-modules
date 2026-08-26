#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaThinkFreqQuantity : ParamQuantity {
    float getDisplayValue() override {
        float v = getValue();
        return 2.f * powf(4000.f, v);
    }
};

struct RaThinkModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        FINE_PARAM,
        SHAPE_PARAM,
        WIDTH_PARAM,
        CUTOFF_PARAM,
        RES_PARAM,
        DC_CORRECT_PARAM,
        GAIN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        SHAPE_CV_INPUT,
        WIDTH_CV_INPUT,
        CUTOFF_CV_INPUT,
        RES_CV_INPUT,
        GAIN_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float phase = 0.f;
    float low = 0.f;
    float band = 0.f;

    static constexpr float MIN_FREQ = 2.f;
    static constexpr float MAX_FREQ = 8000.f;

    RaThinkModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam<RaThinkFreqQuantity>(FREQ_PARAM, 0.f, 1.f, 0.5876f, "Frequency", " Hz");
        configParam(FINE_PARAM, 0.f, 1.f, 0.5f, "Fine tune", " st", 0.f, 2.f, -1.f);
        configParam(SHAPE_PARAM, 0.f, 1.f, 0.5f, "Shape", "%", 0.f, 100.f);
        configParam(WIDTH_PARAM, 0.f, 1.f, 0.5f, "Pulse width", "%", 0.f, 100.f);
        configParam(CUTOFF_PARAM, 0.f, 1.f, 0.8f, "Cutoff", "%", 0.f, 100.f);
        configParam(RES_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
        configSwitch(DC_CORRECT_PARAM, 0.f, 1.f, 1.f, "DC correction", {"Off", "On"});
        configParam(GAIN_PARAM, -5.f, 5.f, 1.f, "Gain");
        configInput(GAIN_CV_INPUT, "Gain CV");
        configInput(PITCH_INPUT, "1V/Oct");
        configInput(SHAPE_CV_INPUT, "Shape CV");
        configInput(WIDTH_CV_INPUT, "Pulse width CV");
        configInput(CUTOFF_CV_INPUT, "Cutoff CV");
        configInput(RES_CV_INPUT, "Resonance CV");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs &args) override {
        float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
        float fine = (params[FINE_PARAM].getValue() - 0.5f) * 2.f;
        freq *= powf(2.f, fine / 12.f);
        float pitch = inputs[PITCH_INPUT].getVoltage();
        freq *= powf(2.f, pitch);
        freq = clamp(freq, 0.1f, 20000.f);

        phase += freq * args.sampleTime;
        if (phase >= 1.f)
            phase -= 1.f;

        auto applyCV = [](float knob, float cv, bool connected) {
            if (connected)
                knob += cv * (knob - 0.5f) * 2.f * 0.1f;
            return clamp(knob, 0.f, 1.f);
        };

        float shape = applyCV(params[SHAPE_PARAM].getValue(), inputs[SHAPE_CV_INPUT].getVoltage(), inputs[SHAPE_CV_INPUT].isConnected());
        float width = applyCV(params[WIDTH_PARAM].getValue(), inputs[WIDTH_CV_INPUT].getVoltage(), inputs[WIDTH_CV_INPUT].isConnected());
        float cutoffNorm = applyCV(params[CUTOFF_PARAM].getValue(), inputs[CUTOFF_CV_INPUT].getVoltage(), inputs[CUTOFF_CV_INPUT].isConnected());
        float resNorm = applyCV(params[RES_PARAM].getValue(), inputs[RES_CV_INPUT].getVoltage(), inputs[RES_CV_INPUT].isConnected());

        float sawDown = 1.f - 2.f * phase;
        float sawUp = 2.f * phase - 1.f;
        float pw = 0.05f + 0.9f * width;
        float pulse = (phase < pw) ? 1.f : -1.f;
        float pulseDc = 2.f * pw - 1.f;

        float raw, dc;
        if (shape < 0.5f) {
            float t = shape * 2.f;
            raw = sawDown * (1.f - t) + pulse * t;
            dc = pulseDc * t;
        } else {
            float t = (shape - 0.5f) * 2.f;
            raw = pulse * (1.f - t) + sawUp * t;
            dc = pulseDc * (1.f - t);
        }
        if (params[DC_CORRECT_PARAM].getValue() > 0.5f)
            raw -= dc;

        float cutoffFreq = 20.f * powf(20000.f / 20.f, cutoffNorm);
        float g = tanf(M_PI * cutoffFreq * args.sampleTime);
        g = clamp(g, 0.f, 10.f);
        float R = 1.f - resNorm * 0.95f;
        float S = 1.f / (1.f + g * (g + R));

        float hp = (raw - low - R * band) * S;
        band += g * hp;
        low += g * band;

        float gain = params[GAIN_PARAM].getValue();
        if (inputs[GAIN_CV_INPUT].isConnected())
            gain += inputs[GAIN_CV_INPUT].getVoltage() * (gain / 5.f);
        gain = clamp(gain, -5.f, 5.f);
        float out = 5.f * tanh(low * gain);
        outputs[AUDIO_OUTPUT].setVoltage(out);
    }
};

struct RaThinkWidget : ModuleWidget {
    RaThinkWidget(RaThinkModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-think.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnobLarge>(Vec(30, 24), module, RaThinkModule::FREQ_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(30, 56), module, RaThinkModule::FINE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(30, 82), module, RaThinkModule::PITCH_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 124), module, RaThinkModule::SHAPE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 124), module, RaThinkModule::SHAPE_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 162), module, RaThinkModule::WIDTH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 162), module, RaThinkModule::WIDTH_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 200), module, RaThinkModule::CUTOFF_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 200), module, RaThinkModule::CUTOFF_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 238), module, RaThinkModule::RES_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 238), module, RaThinkModule::RES_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 276), module, RaThinkModule::GAIN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 276), module, RaThinkModule::GAIN_CV_INPUT));
        addParam(createParamCentered<RaSwitch2>(Vec(30, 306), module, RaThinkModule::DC_CORRECT_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(30, 336), module, RaThinkModule::AUDIO_OUTPUT));
    }
};

Model *modelRaThink = createModel<RaThinkModule, RaThinkWidget>("ra-think");
