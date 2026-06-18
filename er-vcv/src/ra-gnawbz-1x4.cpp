#include "ra-widgets.hpp"

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
        }
    }

    void process(const ProcessArgs &args) override {
        float macroV = params[MACRO_PARAM].getValue();
        if (params[RANGE_PARAM].getValue() > 0.5f) {
            macroV *= 10.f;
        } else {
            macroV = macroV * 10.f - 5.f;
        }

        for (int i = 0; i < 4; i++) {
            float attn = params[ATTN1_PARAM + i].getValue();
            float offset = params[OFFSET1_PARAM + i].getValue();
            float v = macroV * attn + offset;
            outputs[OUTPUT1 + i].setVoltage(v);
        }
    }
};

struct RaGnawbz1x4Widget : ModuleWidget {
    RaGnawbz1x4Widget(RaGnawbz1x4Module *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-gnawbz-1x4.svg")));

        addChild(createWidget<RaScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnobLarge>(Vec(box.size.x / 2, 28), module, RaGnawbz1x4Module::MACRO_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(box.size.x / 2, 56), module, RaGnawbz1x4Module::RANGE_PARAM));

        float colX[] = {24, 66};
        float rowY[] = {92, 210};

        for (int r = 0; r < 2; r++) {
            for (int c = 0; c < 2; c++) {
                int i = r * 2 + c;
                float cx = colX[c];
                float cy = rowY[r];
                addParam(createParamCentered<RaKnob>(Vec(cx, cy), module, RaGnawbz1x4Module::ATTN1_PARAM + i));
                addParam(createParamCentered<RaKnobSmall>(Vec(cx, cy + 30), module, RaGnawbz1x4Module::OFFSET1_PARAM + i));
                addOutput(createOutputCentered<RaPort>(Vec(cx, cy + 56), module, RaGnawbz1x4Module::OUTPUT1 + i));
            }
        }
    }
};

Model *modelRaGnawbz1x4 = createModel<RaGnawbz1x4Module, RaGnawbz1x4Widget>("ra-gnawbz-1x4");
