#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaGnawbz1x4Module : Module {
    enum ParamIds {
        MACRO_PARAM,
        RANGE_PARAM,
        ATTN1_PARAM,
        ATTN2_PARAM,
        ATTN3_PARAM,
        ATTN4_PARAM,
        OFFSET1_PARAM,
        OFFSET2_PARAM,
        OFFSET3_PARAM,
        OFFSET4_PARAM,
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
        OUTPUT1_LED_R,
        OUTPUT1_LED_G,
        OUTPUT1_LED_B,
        OUTPUT2_LED_R,
        OUTPUT2_LED_G,
        OUTPUT2_LED_B,
        OUTPUT3_LED_R,
        OUTPUT3_LED_G,
        OUTPUT3_LED_B,
        OUTPUT4_LED_R,
        OUTPUT4_LED_G,
        OUTPUT4_LED_B,
        MACRO_LED_R,
        MACRO_LED_G,
        MACRO_LED_B,
        NUM_LIGHTS
    };

    RaGnawbz1x4Module() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(MACRO_PARAM, 0.f, 1.f, 0.5f, "Macro", "V", 0.f, 10.f);
        configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"\u00B15V", "0\u201310V"});
        for (int i = 0; i < 4; i++) {
            configParam(ATTN1_PARAM + i, -1.f, 1.f, 1.f, string::f("Atten %d", i + 1));
            configParam(OFFSET1_PARAM + i, -5.f, 5.f, 0.f, string::f("Offset %d", i + 1), "V");
            configOutput(OUTPUT1 + i, string::f("Output %d", i + 1));
            for (int c = 0; c < 3; c++)
                configLight(OUTPUT1_LED_R + i * 3 + c, string::f("Output %d LED", i + 1));
        }
        for (int c = 0; c < 3; c++)
            configLight(MACRO_LED_R + c, "Macro LED");
    }

    void process(const ProcessArgs &args) override {
        float macroV = params[MACRO_PARAM].getValue();
        float range = params[RANGE_PARAM].getValue();
        if (range > 0.5f) {
            macroV *= 10.f;
        } else {
            macroV = macroV * 10.f - 5.f;
        }

        for (int i = 0; i < 4; i++) {
            float attn = params[ATTN1_PARAM + i].getValue();
            float offset = params[OFFSET1_PARAM + i].getValue();
            float v = macroV * attn + offset;
            outputs[OUTPUT1 + i].setVoltage(v);

            float hue;
            if (range > 0.5f) {
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
            lights[OUTPUT1_LED_R + i * 3 + 0].setBrightness(r);
            lights[OUTPUT1_LED_R + i * 3 + 1].setBrightness(g);
            lights[OUTPUT1_LED_R + i * 3 + 2].setBrightness(bl);
        }

        float mHue;
        if (range > 0.5f) {
            mHue = clamp(macroV / 10.f, 0.f, 1.f);
        } else {
            mHue = clamp((macroV + 5.f) / 10.f, 0.f, 1.f);
        }
        float mS = 1.f, mB = 1.f;
        int mHi = (int)(mHue * 6.f);
        float mF = mHue * 6.f - mHi;
        float mP = mB * (1.f - mS);
        float mQ = mB * (1.f - mF * mS);
        float mT = mB * (1.f - (1.f - mF) * mS);
        float mR, mG;
        switch (mHi % 6) {
            case 0: mR = mB; mG = mT; break;
            case 1: mR = mQ; mG = mB; break;
            case 2: mR = mP; mG = mB; break;
            case 3: mR = mP; mG = mQ; break;
            case 4: mR = mT; mG = mP; break;
            default: mR = mB; mG = mP; break;
        }
        float mBl;
        switch (mHi % 6) {
            case 0: mBl = mP; break;
            case 1: mBl = mP; break;
            case 2: mBl = mT; break;
            case 3: mBl = mB; break;
            case 4: mBl = mB; break;
            default: mBl = mQ; break;
        }
        lights[MACRO_LED_R + 0].setBrightness(mR);
        lights[MACRO_LED_R + 1].setBrightness(mG);
        lights[MACRO_LED_R + 2].setBrightness(mBl);
    }
};

struct RaGnawbz1x4Widget : ModuleWidget {
    RaGnawbz1x4Widget(RaGnawbz1x4Module *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-macrow.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnobLarge>(Vec(box.size.x / 2, 63), module, RaGnawbz1x4Module::MACRO_PARAM));
        addChild(createLightCentered<RaRGBLight>(Vec(72, 102), module, RaGnawbz1x4Module::MACRO_LED_R));
        addParam(createParamCentered<RaSwitch2>(Vec(box.size.x / 2, 102), module, RaGnawbz1x4Module::RANGE_PARAM));

        float colX[] = {24, 66};
        float rowY[] = {134, 254};

        for (int r = 0; r < 2; r++) {
            for (int c = 0; c < 2; c++) {
                int i = r * 2 + c;
                float cx = colX[c];
                float cy = rowY[r];
                addParam(createParamCentered<RaKnob>(Vec(cx, cy), module, RaGnawbz1x4Module::ATTN1_PARAM + i));
                addParam(createParamCentered<RaKnobSmall>(Vec(cx, cy + 34), module, RaGnawbz1x4Module::OFFSET1_PARAM + i));
                addOutput(createOutputCentered<RaPort>(Vec(cx, cy + 62), module, RaGnawbz1x4Module::OUTPUT1 + i));
                addChild(createLightCentered<RaRGBLight>(Vec(cx, cy + 88), module, RaGnawbz1x4Module::OUTPUT1_LED_R + i * 3));
            }
        }
    }
};

Model *modelRaMacro = createModel<RaGnawbz1x4Module, RaGnawbz1x4Widget>("ra-macrow");
