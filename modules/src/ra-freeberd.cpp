#include "ra-components.hpp"
#include <array>

using namespace rack;

extern Plugin *pluginInstance;

// Classic Freeverb (Jezar) — 8 combs + 4 allpasses per channel
struct RaFreeberdModule : Module {
    enum ParamIds {
        ROOM_PARAM,
        DAMP_PARAM,
        DRYWET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        L_INPUT,
        R_INPUT,
        ROOM_CV_INPUT,
        DAMP_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        L_OUTPUT,
        R_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_ALLPASSES = 4;

    static constexpr float STEREO_SPREAD = 23.f;

    static constexpr float COMB_TUNINGS[NUM_COMBS] = {1116.f, 1188.f, 1277.f, 1356.f, 1422.f, 1491.f, 1557.f, 1617.f};
    static constexpr float ALLPASS_TUNINGS[NUM_ALLPASSES] = {556.f, 441.f, 341.f, 225.f};

    struct Comb {
        std::vector<float> buffer;
        int idx = 0;
        float filterStore = 0.f;

        void init(int size) {
            buffer.assign(size, 0.f);
            idx = 0;
            filterStore = 0.f;
        }
        float process(float input, float feedback, float damp1) {
            float output = buffer[idx];
            filterStore = output * (1.f - damp1) + filterStore * damp1;
            buffer[idx] = input + filterStore * feedback;
            if (++idx >= (int)buffer.size()) idx = 0;
            return output;
        }
    };

    struct Allpass {
        std::vector<float> buffer;
        int idx = 0;

        void init(int size) {
            buffer.assign(size, 0.f);
            idx = 0;
        }
        float process(float input) {
            float bufout = buffer[idx];
            float output = -input + bufout;
            buffer[idx] = input + bufout * 0.5f;
            if (++idx >= (int)buffer.size()) idx = 0;
            return output;
        }
    };

    Comb combs[2][NUM_COMBS];
    Allpass allpasses[2][NUM_ALLPASSES];
    float scaleSampleRate = 1.f;
    bool buffersReady = false;

    // Smoothed parameter values (per-sample smoothing)
    float smoothRoom = 0.5f;
    float smoothDamp = 0.5f;
    float smoothDrywet = 0.5f;

    RaFreeberdModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(ROOM_PARAM, 0.f, 1.f, 0.5f, "Room size", "%", 0.f, 100.f);
        configParam(DAMP_PARAM, 0.f, 1.f, 0.5f, "Damping", "%", 0.f, 100.f);
        configParam(DRYWET_PARAM, 0.f, 1.f, 0.5f, "Dry/wet", "%", 0.f, 100.f);
        configInput(L_INPUT, "Left");
        configInput(R_INPUT, "Right");
        configInput(ROOM_CV_INPUT, "Room size CV");
        configInput(DAMP_CV_INPUT, "Damping CV");
        configOutput(L_OUTPUT, "Left");
        configOutput(R_OUTPUT, "Right");

        scaleSampleRate = APP->engine->getSampleRate() / 44100.f;
        allocBuffers();
    }

    void onReset() override {
        allocBuffers();
    }

    void onSampleRateChange() override {
        scaleSampleRate = APP->engine->getSampleRate() / 44100.f;
        allocBuffers();
    }

    void allocBuffers() {
        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < NUM_COMBS; i++)
                combs[ch][i].init((int)(COMB_TUNINGS[i] * scaleSampleRate));
            for (int i = 0; i < NUM_ALLPASSES; i++)
                allpasses[ch][i].init((int)(ALLPASS_TUNINGS[i] * scaleSampleRate));
        }
        buffersReady = true;
    }

    void process(const ProcessArgs &args) override {
        if (!buffersReady) return;

        float lIn = inputs[L_INPUT].getVoltage();
        float rIn = inputs[R_INPUT].isConnected() ? inputs[R_INPUT].getVoltage() : lIn;

        // Smooth parameters over ~10 ms to avoid zipper noise
        float lambda = args.sampleTime / 0.01f;
        float room = clamp(params[ROOM_PARAM].getValue() + inputs[ROOM_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float damp = clamp(params[DAMP_PARAM].getValue() + inputs[DAMP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float drywet = params[DRYWET_PARAM].getValue();
        smoothRoom += (room - smoothRoom) * lambda;
        smoothDamp += (damp - smoothDamp) * lambda;
        smoothDrywet += (drywet - smoothDrywet) * lambda;

        float feedback = 0.7f + 0.28f * smoothRoom;
        float damp1 = 0.2f + 0.4f * smoothDamp;

        float inL = lIn * 0.2f;
        float inR = rIn * 0.2f;

        float outL = 0.f;
        float outR = 0.f;
        for (int i = 0; i < NUM_COMBS; i++) {
            outL += combs[0][i].process(inL, feedback, damp1);
            outR += combs[1][i].process(inR, feedback, damp1);
        }
        for (int i = 0; i < NUM_ALLPASSES; i++) {
            outL = allpasses[0][i].process(outL);
            outR = allpasses[1][i].process(outR);
        }

        // Scale wet signal back to rack levels
        outL *= 0.9f;
        outR *= 0.9f;

        float dry = 1.f - smoothDrywet;
        float wet = smoothDrywet;
        outputs[L_OUTPUT].setVoltage(lIn * dry + outL * wet);
        outputs[R_OUTPUT].setVoltage(rIn * dry + outR * wet);
    }
};

// Out-of-class definitions (required for odr-used constexpr members in C++11)
constexpr float RaFreeberdModule::COMB_TUNINGS[];
constexpr float RaFreeberdModule::ALLPASS_TUNINGS[];

struct RaFreeberdWidget : ModuleWidget {
    RaFreeberdWidget(RaFreeberdModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-freeberd.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;
        float lx = 25;
        float rx = 65;

        addInput(createInputCentered<RaPort>(Vec(lx, 85), module, RaFreeberdModule::L_INPUT));
        addInput(createInputCentered<RaPort>(Vec(rx, 85), module, RaFreeberdModule::R_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(cx, 130), module, RaFreeberdModule::ROOM_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(cx, 175), module, RaFreeberdModule::DAMP_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(cx, 220), module, RaFreeberdModule::DRYWET_PARAM));

        addInput(createInputCentered<RaPort>(Vec(lx, 260), module, RaFreeberdModule::ROOM_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(rx, 260), module, RaFreeberdModule::DAMP_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(lx, 295), module, RaFreeberdModule::L_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(rx, 295), module, RaFreeberdModule::R_OUTPUT));
    }
};

Model *modelRaFreeberd = createModel<RaFreeberdModule, RaFreeberdWidget>("ra-freeberd");
