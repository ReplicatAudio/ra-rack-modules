// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: RATE_PARAM "Rate"
// fname: RATE_CV_INPUT "Rate CV"
// fname: SOURCE_PARAM "Source"
// fname: RANGE_PARAM "Range"
// fname: SCALE_PARAM "Scale"
// fname: SCALE_CV_INPUT "Scale CV"
// fname: OUTPUT "Out"
#include "ra-components.hpp"

#include <cmath>
#include <algorithm>

using namespace rack;

extern Plugin *pluginInstance;

struct RaSlowRandomModule : Module {
    enum ParamIds {
        RATE_PARAM,
        SOURCE_PARAM,
        RANGE_PARAM,
        SCALE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        RATE_CV_INPUT,
        SCALE_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float noisePos = 0.f;
    float smoothCurrent = 0.5f;
    float smoothTarget = 0.5f;
    float brownCurrent = 0.5f;
    float lastSmoothTime = 0.f;

    RaSlowRandomModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(RATE_PARAM, 0.01f, 10.f, 1.f, "Rate", " Hz");
        configSwitch(SOURCE_PARAM, 0.f, 2.f, 0.f, "Source", {"Perlin", "Smooth", "Brown"});
        configSwitch(RANGE_PARAM, 0.f, 2.f, 1.f, "Range", {"0-1V", "0-10V", "±5V"});
        configParam(SCALE_PARAM, 0.f, 1.f, 1.f, "Scale", "%", 0.f, 100.f);
        configInput(RATE_CV_INPUT, "Rate CV");
        configInput(SCALE_CV_INPUT, "Scale CV");
        configOutput(OUTPUT, "Out");
    }

    float hash(int x) const {
        x = (x << 13) ^ x;
        return 1.f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.f;
    }

    float perlin(float x) const {
        int xi = (int)x;
        float xf = x - (float)xi;
        float u = xf * xf * (3.f - 2.f * xf);
        float a = hash(xi);
        float b = hash(xi + 1);
        return a + u * (b - a);
    }

    void process(const ProcessArgs &args) override {
        float rate = clamp(params[RATE_PARAM].getValue() + inputs[RATE_CV_INPUT].getVoltage(), 0.01f, 10.f);
        float scale = clamp(params[SCALE_PARAM].getValue() + inputs[SCALE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        int range = (int)std::round(params[RANGE_PARAM].getValue());
        int source = (int)std::round(params[SOURCE_PARAM].getValue());

        noisePos += rate * args.sampleTime;

        float noise;
        switch (source) {
            case 0:
                noise = perlin(noisePos) * 0.5f + 0.5f;
                break;
            case 1:
                if (noisePos - lastSmoothTime > 1.f / rate) {
                    lastSmoothTime = noisePos;
                    smoothTarget = random::uniform();
                }
                smoothCurrent += (smoothTarget - smoothCurrent) * 0.01f;
                noise = smoothCurrent;
                break;
            case 2:
                brownCurrent += (random::uniform() - 0.5f) * rate * 0.1f;
                brownCurrent = clamp(brownCurrent, 0.f, 1.f);
                noise = brownCurrent;
                break;
            default:
                noise = 0.5f;
                break;
        }

        float scaled = noise * scale;

        float out;
        switch (range) {
            case 0: out = scaled * 1.f; break;
            case 1: out = scaled * 10.f; break;
            case 2: out = scaled * 10.f - 5.f; break;
            default: out = scaled * 10.f; break;
        }

        outputs[OUTPUT].setVoltage(out);
    }
};

struct RaSlowRandomWidget : ModuleWidget {
    RaSlowRandomWidget(RaSlowRandomModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-slowrandom.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[4] = {20.f, 55.f, 90.f, 125.f};
        float rowY[3] = {40.f, 100.f, 180.f};

        addParam(createParamCentered<RaKnobTrim>(Vec(colX[0], rowY[0]), module, RaSlowRandomModule::RATE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[0], rowY[1]), module, RaSlowRandomModule::RATE_CV_INPUT));

        addParam(createParamCentered<RaSwitch3>(Vec(colX[1], rowY[0]), module, RaSlowRandomModule::SOURCE_PARAM));

        addParam(createParamCentered<RaSwitch3>(Vec(colX[2], rowY[0]), module, RaSlowRandomModule::RANGE_PARAM));

        addParam(createParamCentered<RaKnobTrim>(Vec(colX[3], rowY[0]), module, RaSlowRandomModule::SCALE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[3], rowY[1]), module, RaSlowRandomModule::SCALE_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(colX[1] + (colX[2] - colX[1]) / 2.f, rowY[2]), module, RaSlowRandomModule::OUTPUT));
    }
};

Model *modelRaSlowRandom = createModel<RaSlowRandomModule, RaSlowRandomWidget>("ra-slowrandom");
