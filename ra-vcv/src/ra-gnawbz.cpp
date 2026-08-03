#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaGnawbz4xModule : Module {
    enum ParamIds {
        KNOB1_PARAM,
        KNOB2_PARAM,
        KNOB3_PARAM,
        RANGE1_PARAM,
        RANGE2_PARAM,
        RANGE3_PARAM,
        SCALE1_PARAM,
        SCALE2_PARAM,
        SCALE3_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT1,
        OUTPUT2,
        OUTPUT3,
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT1_R,
        LIGHT1_G,
        LIGHT1_B,
        LIGHT2_R,
        LIGHT2_G,
        LIGHT2_B,
        LIGHT3_R,
        LIGHT3_G,
        LIGHT3_B,
        NUM_LIGHTS
    };

    RaGnawbz4xModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 3; i++) {
            configParam(KNOB1_PARAM + i, 0.f, 1.f, 0.5f, string::f("Knob %d", i + 1));
            configSwitch(RANGE1_PARAM + i, 0.f, 1.f, 0.f, string::f("Range %d", i + 1), {"\u00B15V", "0\u201310V"});
            configParam(SCALE1_PARAM + i, 0.f, 1.f, 1.f, string::f("Scale %d", i + 1), "%", 0.f, 100.f);
            configOutput(OUTPUT1 + i, string::f("Output %d", i + 1));
            for (int c = 0; c < 3; c++)
                configLight(LIGHT1_R + i * 3 + c, string::f("Light %d", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < 3; i++) {
            float v = params[KNOB1_PARAM + i].getValue();
            if (params[RANGE1_PARAM + i].getValue() > 0.5f) {
                v *= 10.f;
            } else {
                v = v * 10.f - 5.f;
            }
            v *= params[SCALE1_PARAM + i].getValue();
            outputs[OUTPUT1 + i].setVoltage(v);

            float hue;
            if (params[RANGE1_PARAM + i].getValue() > 0.5f) {
                hue = clamp(v / 10.f, 0.f, 1.f);
            } else {
                hue = clamp((v + 5.f) / 10.f, 0.f, 1.f);
            }
            float s = 1.f, b = 1.f;
            int hi = (int)(hue * 6.f);
            float f = hue * 6.f - hi;
            float p = b * (1.f - s);
            float q = b * (1.f - f * s);
            float t = b * (1.f - (1.f - f) * s);
            float r, g;
            switch (hi % 6) {
                case 0: r = b; g = t; break;
                case 1: r = q; g = b; break;
                case 2: r = p; g = b; break;
                case 3: r = p; g = q; break;
                case 4: r = t; g = p; break;
                default: r = b; g = p; break;
            }
            float bl;
            switch (hi % 6) {
                case 0: bl = p; break;
                case 1: bl = p; break;
                case 2: bl = t; break;
                case 3: bl = b; break;
                case 4: bl = b; break;
                default: bl = q; break;
            }
            lights[LIGHT1_R + i * 3 + 0].setBrightness(r);
            lights[LIGHT1_R + i * 3 + 1].setBrightness(g);
            lights[LIGHT1_R + i * 3 + 2].setBrightness(bl);
        }
    }
};

struct RaGnawbz4xWidget : ModuleWidget {
    RaGnawbz4xWidget(RaGnawbz4xModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-gnawbz.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float y[] = {54, 159, 264};

        for (int i = 0; i < 3; i++) {
            addParam(createParamCentered<RaKnob>(Vec(18, y[i]), module, RaGnawbz4xModule::KNOB1_PARAM + i));
            addParam(createParamCentered<RaKnobSmall>(Vec(18, y[i] + 36), module, RaGnawbz4xModule::SCALE1_PARAM + i));
            addOutput(createOutputCentered<RaPort>(Vec(18, y[i] + 64), module, RaGnawbz4xModule::OUTPUT1 + i));
            addChild(createLightCentered<RaRGBLight>(Vec(48, y[i] + 2), module, RaGnawbz4xModule::LIGHT1_R + i * 3));
            addParam(createParamCentered<RaSwitch2>(Vec(48, y[i] + 26), module, RaGnawbz4xModule::RANGE1_PARAM + i));
        }
    }
};

Model *modelRaGnawbz = createModel<RaGnawbz4xModule, RaGnawbz4xWidget>("ra-gnawbz");
