#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaZenoModule : Module {
    enum ParamIds {
        SLOW_PARAM,
        SNAP_PARAM,
        RESET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        SIGNAL_INPUT,
        SLOW_INPUT,
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SIGNAL_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RESET_LIGHT,
        NUM_LIGHTS
    };

    static constexpr float MAX_BUFFER_SECONDS = 2.f;

    // Circular buffer
    std::vector<float> buffer;
    int64_t writeIndex = 0;
    float readIndex = 0.f;

    // Dual-read-head crossfade state
    bool xfadeActive = false;
    float xfadePos = 0.f;
    float xfadeLen = 0.f;
    float xfadeTarget = 0.f;

    // Triggers
    dsp::SchmittTrigger resetExt;
    dsp::SchmittTrigger resetBtn;

    // Cached constants (updated on sample rate change)
    int bufferSize = 0;
    float targetDelay = 0.f;
    float triggerThreshold = 0.f;

    RaZenoModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SLOW_PARAM, 1.f, 16.f, 1.f, "Slow", "x");
        configSwitch(SNAP_PARAM, 0.f, 1.f, 0.f, "Snap", {"Off", "On"});
        configButton(RESET_PARAM, "Reset");
        configInput(SIGNAL_INPUT, "Signal");
        configInput(SLOW_INPUT, "Slow CV");
        configInput(RESET_INPUT, "Reset");
        configOutput(SIGNAL_OUTPUT, "Signal");
        configLight(RESET_LIGHT, "Reset");
        allocateBuffer(APP->engine->getSampleRate());
    }

    void allocateBuffer(float sr) {
        bufferSize = std::max((int)(sr * MAX_BUFFER_SECONDS), 4096);
        buffer.assign(bufferSize, 0.f);
        writeIndex = 0;
        readIndex = 0.f;

        targetDelay = clamp(sr * 0.05f, 256.f, (float)(bufferSize / 4));
        xfadeLen = clamp(sr * 0.025f, 64.f, (float)(bufferSize / 4));

        int margin = (int)(targetDelay + xfadeLen + 128);
        if (margin >= bufferSize / 2)
            margin = bufferSize / 2 - 1;
        triggerThreshold = (float)(bufferSize - margin);

        xfadeActive = false;
        xfadePos = 0.f;
    }

    void onSampleRateChange() override {
        allocateBuffer(APP->engine->getSampleRate());
    }

    float interp(float pos) const {
        int64_t i = (int64_t)pos;
        float f = pos - (float)i;
        int64_t i0 = i % bufferSize;
        int64_t i1 = (i0 + 1) % bufferSize;
        return buffer[i0] + f * (buffer[i1] - buffer[i0]);
    }

    int64_t findNearestZero(float center) {
        if (writeIndex <= 0)
            return (int64_t)center;
        int64_t ic = (int64_t)center;
        int64_t w = 256;
        int64_t lo = std::max((int64_t)0, ic - w);
        int64_t hi = std::min(writeIndex - 1, ic + w);
        if (lo > hi)
            return ic;

        int64_t best = ic;
        float minVal = std::numeric_limits<float>::max();
        for (int64_t i = lo; i <= hi; i++) {
            float v = std::fabs(buffer[i % bufferSize]);
            if (v < minVal) {
                minVal = v;
                best = i;
            }
        }
        return best;
    }

    void beginCrossfade() {
        xfadeActive = true;
        xfadePos = 0.f;
        float rawTarget = std::max(0.f, (float)(writeIndex - (int64_t)targetDelay));
        xfadeTarget = (float)findNearestZero(rawTarget);
    }

    void process(const ProcessArgs &args) override {
        // Speed
        float speed = params[SLOW_PARAM].getValue();
        if (inputs[SLOW_INPUT].isConnected())
            speed += inputs[SLOW_INPUT].getVoltage() / 10.f * 15.f;
        speed = clamp(speed, 1.f, 16.f);
        if (params[SNAP_PARAM].getValue() > 0.f) {
            speed = roundf(speed * 2.f) / 2.f;
            speed = clamp(speed, 1.f, 16.f);
        }

        // Write to buffer
        buffer[writeIndex % bufferSize] = inputs[SIGNAL_INPUT].getVoltage();

        // Read and output
        float out;
        if (xfadeActive) {
            float oldSamp = interp(readIndex);
            float newSamp = interp(xfadeTarget);
            float t = xfadePos / xfadeLen;
            // Equal-power crossfade (sin^2 / cos^2) for constant perceived loudness
            float a = cosf(t * M_PI / 2.f);
            float b = sinf(t * M_PI / 2.f);
            out = oldSamp * a + newSamp * b;
        } else {
            out = interp(readIndex);
        }
        outputs[SIGNAL_OUTPUT].setVoltage(out);

        // Auto-reset when delay approaches buffer boundary
        float delay = (float)writeIndex - readIndex;
        if (!xfadeActive && delay > triggerThreshold) {
            beginCrossfade();
        }

        // Manual reset
        bool reset = resetBtn.process(params[RESET_PARAM].getValue());
        reset |= resetExt.process(inputs[RESET_INPUT].getVoltage());
        if (reset) {
            beginCrossfade();
        }

        // Advance all positions
        writeIndex++;
        readIndex += 1.f / speed;
        if (xfadeActive) {
            xfadeTarget += 1.f / speed;
            xfadePos += 1.f;
            if (xfadePos >= xfadeLen) {
                xfadeActive = false;
                readIndex = xfadeTarget;
            }
        }

        // Light — glows during crossfade
        lights[RESET_LIGHT].setBrightnessSmooth(
            xfadeActive ? 1.f : 0.f, args.sampleTime);
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

        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(leftX, 248), module, RaZenoModule::RESET_PARAM, RaZenoModule::RESET_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(rightX, 248), module, RaZenoModule::RESET_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaZenoModule::SIGNAL_OUTPUT));
    }
};

Model *modelRaZeno = createModel<RaZenoModule, RaZenoWidget>("ra-zeno");
