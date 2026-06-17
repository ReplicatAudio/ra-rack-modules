#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaKnobModule : Module {
    enum ParamIds {
        MACRO1_PARAM,
        MACRO2_PARAM,
        MACRO3_PARAM,
        MACRO4_PARAM,
        RANGE1_PARAM,
        RANGE2_PARAM,
        RANGE3_PARAM,
        RANGE4_PARAM,
        SCALE1_PARAM,
        SCALE2_PARAM,
        SCALE3_PARAM,
        SCALE4_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT1,
        OUTPUT2,
        OUTPUT3,
        OUTPUT4,
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
        LIGHT4_R,
        LIGHT4_G,
        LIGHT4_B,
        NUM_LIGHTS
    };

    RaKnobModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 4; i++) {
            configParam(MACRO1_PARAM + i, 0.f, 1.f, 0.5f, string::f("Macro %d", i + 1));
            configSwitch(RANGE1_PARAM + i, 0.f, 1.f, 0.f, string::f("Range %d", i + 1), {"\u00B15V", "0\u201310V"});
            configParam(SCALE1_PARAM + i, 0.f, 1.f, 1.f, string::f("Scale %d", i + 1), "%", 0.f, 100.f);
            configOutput(OUTPUT1 + i, string::f("Output %d", i + 1));
            for (int c = 0; c < 3; c++)
                configLight(LIGHT1_R + i * 3 + c, string::f("Light %d", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < 4; i++) {
            float v = params[MACRO1_PARAM + i].getValue();
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

struct RaKnobWidget : ModuleWidget {
    RaKnobWidget(RaKnobModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-knob.svg")));

        addChild(createWidget<ScrewSilver>(Vec(0, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float y[] = {32, 120, 208, 296};

        for (int i = 0; i < 4; i++) {
            addParam(createParamCentered<RoundBlackKnob>(Vec(box.size.x / 2, y[i]), module, RaKnobModule::MACRO1_PARAM + i));
            addParam(createParamCentered<CKSS>(Vec(box.size.x - 8, y[i] + 18), module, RaKnobModule::RANGE1_PARAM + i));
            addParam(createParamCentered<RoundSmallBlackKnob>(Vec(box.size.x / 2, y[i] + 30), module, RaKnobModule::SCALE1_PARAM + i));
            addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x / 2, y[i] + 54), module, RaKnobModule::OUTPUT1 + i));
            addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(8, y[i] + 45), module, RaKnobModule::LIGHT1_R + i * 3));
        }
    }
};

Model *modelRaKnob = createModel<RaKnobModule, RaKnobWidget>("ra-knob");
