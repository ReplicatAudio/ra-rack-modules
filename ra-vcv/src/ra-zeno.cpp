#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaZenoModule : Module {
    enum ParamIds {
        SLOW_PARAM,
        SNAP_PARAM,
        TIME_PARAM,
        OVERFLOW_PARAM,
        RESET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        SIGNAL_INPUT,
        SLOW_INPUT,
        TIME_INPUT,
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SIGNAL_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RESET_LIGHT,
        OVERFLOW_LIGHT_R,
        OVERFLOW_LIGHT_G,
        OVERFLOW_LIGHT_B,
        NUM_LIGHTS
    };

    enum OverflowMode {
        OVERFLOW_LOOP,
        OVERFLOW_SILENCE
    };

    static constexpr float MAX_BUFFER_SECONDS = 120.f;

    std::vector<float> buffer;
    int64_t writeIndex = 0;
    float readIndex = 0.f;
    int bufferSize = 0;

    int lastOverflowMode = -1;

    bool inTransition = false;
    float transitionBlend = 0.f;
    float transitionDur = 0.f;
    float transitionOldOut = 0.f;

    static constexpr float XFADE_MS = 10.f;

    dsp::SchmittTrigger resetExtTrigger;
    dsp::SchmittTrigger resetBtnTrigger;

    RaZenoModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SLOW_PARAM, 1.f, 16.f, 1.f, "Slow", "x");
        configSwitch(SNAP_PARAM, 0.f, 1.f, 0.f, "Snap", {"Off", "On"});
        configParam(TIME_PARAM, 1.f, 120.f, 10.f, "Buffer time", " s");
        configSwitch(OVERFLOW_PARAM, 0.f, 1.f, 0.f, "Overflow", {"Loop", "Silence"});
        configButton(RESET_PARAM, "Reset");
        configInput(SIGNAL_INPUT, "Signal");
        configInput(SLOW_INPUT, "Slow CV");
        configInput(TIME_INPUT, "Buffer time CV");
        configInput(RESET_INPUT, "Reset");
        configOutput(SIGNAL_OUTPUT, "Signal");
        configLight(RESET_LIGHT, "Reset");
        for (int c = 0; c < 3; c++)
            configLight(OVERFLOW_LIGHT_R + c, "Overflow");
        allocateBuffer(APP->engine->getSampleRate());
    }

    void allocateBuffer(float sampleRate) {
        bufferSize = std::max((int)(sampleRate * MAX_BUFFER_SECONDS), 64);
        buffer.assign(bufferSize, 0.f);
        writeIndex = 0;
        readIndex = 0.f;
    }

    void onSampleRateChange() override {
        allocateBuffer(APP->engine->getSampleRate());
    }

    void startTransition(float currentOut, float sampleRate) {
        inTransition = true;
        transitionBlend = 0.f;
        transitionDur = sampleRate * (XFADE_MS / 1000.f);
        transitionOldOut = currentOut;
    }

    void process(const ProcessArgs &args) override {
        float speed = params[SLOW_PARAM].getValue();
        if (inputs[SLOW_INPUT].isConnected())
            speed += inputs[SLOW_INPUT].getVoltage() / 10.f * 15.f;
        speed = clamp(speed, 1.f, 16.f);

        if (params[SNAP_PARAM].getValue() > 0.f) {
            speed = roundf(speed * 2.f) / 2.f;
            speed = clamp(speed, 1.f, 16.f);
        }

        int overflowMode = (int)params[OVERFLOW_PARAM].getValue();

        float prevOut = outputs[SIGNAL_OUTPUT].getVoltage();

        if (overflowMode != lastOverflowMode) {
            startTransition(prevOut, args.sampleRate);
            lastOverflowMode = overflowMode;
            readIndex = (float)writeIndex;
        }

        float input = inputs[SIGNAL_INPUT].getVoltage();
        buffer[writeIndex % bufferSize] = input;

        float time = params[TIME_PARAM].getValue();
        if (inputs[TIME_INPUT].isConnected())
            time += inputs[TIME_INPUT].getVoltage() / 10.f * 119.f;
        time = clamp(time, 1.f, 120.f);
        int64_t effectiveBufferSize = (int64_t)(args.sampleRate * time);

        int64_t distance = writeIndex - (int64_t)readIndex;
        bool doOverflow = (distance >= effectiveBufferSize);

        int r0 = ((int)readIndex) % bufferSize;
        int r1 = (r0 + 1) % bufferSize;
        float frac = readIndex - floorf(readIndex);
        float rawOut = buffer[r0] + frac * (buffer[r1] - buffer[r0]);

        float out;
        if (inTransition) {
            transitionBlend += 1.f / transitionDur;
            if (transitionBlend >= 1.f) {
                transitionBlend = 1.f;
                inTransition = false;
            }
            out = transitionOldOut * (1.f - transitionBlend) + rawOut * transitionBlend;
        } else {
            out = rawOut;
        }

        if (doOverflow && overflowMode == OVERFLOW_SILENCE) {
            out = 0.f;
        }

        outputs[SIGNAL_OUTPUT].setVoltage(out);

        bool reset = resetBtnTrigger.process(params[RESET_PARAM].getValue());
        reset |= resetExtTrigger.process(inputs[RESET_INPUT].getVoltage());
        if (reset) {
            startTransition(out, args.sampleRate);
            std::fill(buffer.begin(), buffer.end(), 0.f);
            writeIndex = 0;
            readIndex = 0.0;
        }

        writeIndex++;
        readIndex += 1.f / speed;

        lights[RESET_LIGHT].setBrightnessSmooth(reset ? 1.f : 0.f, args.sampleTime);

        bool ovLight = doOverflow && overflowMode != OVERFLOW_LOOP;
        lights[OVERFLOW_LIGHT_R].setBrightness(ovLight ? 1.f : 0.f);
        lights[OVERFLOW_LIGHT_G].setBrightness(0.f);
        lights[OVERFLOW_LIGHT_B].setBrightness(0.f);
    }

    json_t *dataToJson() override {
        return json_object();
    }

    void dataFromJson(json_t *rootJ) override {
    }
};

struct RaZenoWidget : ModuleWidget {
    RaZenoWidget(RaZenoModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-zeno.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;
        float leftX = 16;
        float rightX = 44;

        addInput(createInputCentered<RaPort>(Vec(cx, 30), module, RaZenoModule::SIGNAL_INPUT));

        addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 72), module, RaZenoModule::SLOW_PARAM));
        addInput(createInputCentered<RaPort>(Vec(rightX, 72), module, RaZenoModule::SLOW_INPUT));

        addParam(createParamCentered<RaSwitch2>(Vec(cx, 112), module, RaZenoModule::SNAP_PARAM));

        addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 152), module, RaZenoModule::TIME_PARAM));
        addInput(createInputCentered<RaPort>(Vec(rightX, 152), module, RaZenoModule::TIME_INPUT));

        addParam(createParamCentered<RaSwitch2>(Vec(cx, 194), module, RaZenoModule::OVERFLOW_PARAM));

        addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(Vec(cx, 220), module, RaZenoModule::OVERFLOW_LIGHT_R));

        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(leftX, 248), module, RaZenoModule::RESET_PARAM, RaZenoModule::RESET_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(rightX, 248), module, RaZenoModule::RESET_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaZenoModule::SIGNAL_OUTPUT));
    }
};

Model *modelRaZeno = createModel<RaZenoModule, RaZenoWidget>("ra-zeno");
