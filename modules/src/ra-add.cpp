#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaAddModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        FM_ATTN_PARAM,
        HARM1_PARAM,
        HARM2_PARAM,
        HARM3_PARAM,
        HARM4_PARAM,
        HARM5_PARAM,
        HARM6_PARAM,
        HARM7_PARAM,
        HARM8_PARAM,
        HARM9_PARAM,
        HARM10_PARAM,
        HARM11_PARAM,
        HARM12_PARAM,
        HARM13_PARAM,
        HARM14_PARAM,
        HARM15_PARAM,
        HARM16_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        FM_INPUT,
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

    static constexpr float MIN_FREQ = 2.f;
    static constexpr float MAX_FREQ = 8000.f;

    RaAddModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.588f, "Frequency", " Hz");
        configParam(FM_ATTN_PARAM, 0.f, 1.f, 0.f, "FM attenuation", "%", 0.f, 100.f);
        configParam(HARM1_PARAM, 0.f, 1.f, 1.f, "Harmonic 1", "%", 0.f, 100.f);
        configParam(HARM2_PARAM, 0.f, 1.f, 0.f, "Harmonic 2", "%", 0.f, 100.f);
        configParam(HARM3_PARAM, 0.f, 1.f, 0.f, "Harmonic 3", "%", 0.f, 100.f);
        configParam(HARM4_PARAM, 0.f, 1.f, 0.f, "Harmonic 4", "%", 0.f, 100.f);
        configParam(HARM5_PARAM, 0.f, 1.f, 0.f, "Harmonic 5", "%", 0.f, 100.f);
        configParam(HARM6_PARAM, 0.f, 1.f, 0.f, "Harmonic 6", "%", 0.f, 100.f);
        configParam(HARM7_PARAM, 0.f, 1.f, 0.f, "Harmonic 7", "%", 0.f, 100.f);
        configParam(HARM8_PARAM, 0.f, 1.f, 0.f, "Harmonic 8", "%", 0.f, 100.f);
        configParam(HARM9_PARAM, 0.f, 1.f, 0.f, "Harmonic 9", "%", 0.f, 100.f);
        configParam(HARM10_PARAM, 0.f, 1.f, 0.f, "Harmonic 10", "%", 0.f, 100.f);
        configParam(HARM11_PARAM, 0.f, 1.f, 0.f, "Harmonic 11", "%", 0.f, 100.f);
        configParam(HARM12_PARAM, 0.f, 1.f, 0.f, "Harmonic 12", "%", 0.f, 100.f);
        configParam(HARM13_PARAM, 0.f, 1.f, 0.f, "Harmonic 13", "%", 0.f, 100.f);
        configParam(HARM14_PARAM, 0.f, 1.f, 0.f, "Harmonic 14", "%", 0.f, 100.f);
        configParam(HARM15_PARAM, 0.f, 1.f, 0.f, "Harmonic 15", "%", 0.f, 100.f);
        configParam(HARM16_PARAM, 0.f, 1.f, 0.f, "Harmonic 16", "%", 0.f, 100.f);
        configInput(PITCH_INPUT, "1V/Oct");
        configInput(FM_INPUT, "FM");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs &args) override {
        float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
        float pitch = inputs[PITCH_INPUT].getVoltage()
            + inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue();
        freq *= powf(2.f, pitch);
        freq = clamp(freq, 0.1f, 20000.f);

        phase += freq * args.sampleTime;
        if (phase >= 1.f)
            phase -= 1.f;

        float output = 0.f;
        float totalAmp = 0.f;

        float theta = 2.f * M_PI * phase;
        float cosTheta = cosf(theta);
        float s0 = 0.f;
        float s1 = sinf(theta);

        for (int n = 1; n <= 16; n++) {
            float sn;
            if (n == 1) {
                sn = s1;
            } else {
                sn = 2.f * cosTheta * s1 - s0;
                s0 = s1;
                s1 = sn;
            }

            float amp = params[HARM1_PARAM + n - 1].getValue();
            totalAmp += amp;
            output += amp * sn;
        }

        if (totalAmp > 0.f)
            output /= totalAmp;

        outputs[AUDIO_OUTPUT].setVoltage(output * 5.f);
    }
};

struct RaAddWidget : ModuleWidget {
    RaAddWidget(RaAddModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-add.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnobLarge>(Vec(90, 50), module, RaAddModule::FREQ_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(90, 95), module, RaAddModule::FM_ATTN_PARAM));

        addInput(createInputCentered<RaPort>(Vec(50, 95), module, RaAddModule::PITCH_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 95), module, RaAddModule::FM_INPUT));

        static const float harmX[4] = {30.f, 70.f, 110.f, 150.f};
        static const float harmY[4] = {145.f, 200.f, 255.f, 310.f};

        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                int idx = row * 4 + col;
                addParam(createParamCentered<RaKnobSmall>(
                    Vec(harmX[col], harmY[row]),
                    module,
                    (int)RaAddModule::HARM1_PARAM + idx
                ));
            }
        }

        addOutput(createOutputCentered<RaPort>(Vec(90, 360), module, RaAddModule::AUDIO_OUTPUT));
    }
};

Model *modelRaAdd = createModel<RaAddModule, RaAddWidget>("ra-add");
