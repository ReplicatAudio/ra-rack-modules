// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: CV_INPUT "Input"
// fname: IN_FLOOR_PARAM "In Fl"
// fname: IN_FLOOR_INPUT "In Fl"
// fname: OUT_FLOOR_PARAM "Out Fl"
// fname: OUT_FLOOR_INPUT "Out Fl"
// fname: IN_CEIL_PARAM "In Cei"
// fname: IN_CEIL_INPUT "In Cei"
// fname: OUT_CEIL_PARAM "Out Cei"
// fname: OUT_CEIL_INPUT "Out Cei"
// fname: CV_OUTPUT "Output"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaRanger2Module : Module {
    enum ParamIds {
        IN_FLOOR_PARAM,
        IN_CEIL_PARAM,
        OUT_FLOOR_PARAM,
        OUT_CEIL_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CV_INPUT,
        IN_FLOOR_INPUT,
        IN_CEIL_INPUT,
        OUT_FLOOR_INPUT,
        OUT_CEIL_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    RaRanger2Module() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(IN_FLOOR_PARAM, -5.f, 10.f, 0.f, "Input floor", " V");
        configParam(IN_CEIL_PARAM, -5.f, 10.f, 10.f, "Input ceil", " V");
        configParam(OUT_FLOOR_PARAM, -5.f, 10.f, 0.f, "Output floor", " V");
        configParam(OUT_CEIL_PARAM, -5.f, 10.f, 10.f, "Output ceil", " V");

        configInput(CV_INPUT, "Input");
        configInput(IN_FLOOR_INPUT, "Input floor CV");
        configInput(IN_CEIL_INPUT, "Input ceil CV");
        configInput(OUT_FLOOR_INPUT, "Output floor CV");
        configInput(OUT_CEIL_INPUT, "Output ceil CV");

        configOutput(CV_OUTPUT, "Output");
    }

    void process(const ProcessArgs &args) override {
        float inFloor = inputs[IN_FLOOR_INPUT].isConnected()
            ? inputs[IN_FLOOR_INPUT].getVoltage()
            : params[IN_FLOOR_PARAM].getValue();
        float inCeil = inputs[IN_CEIL_INPUT].isConnected()
            ? inputs[IN_CEIL_INPUT].getVoltage()
            : params[IN_CEIL_PARAM].getValue();
        float outFloor = inputs[OUT_FLOOR_INPUT].isConnected()
            ? inputs[OUT_FLOOR_INPUT].getVoltage()
            : params[OUT_FLOOR_PARAM].getValue();
        float outCeil = inputs[OUT_CEIL_INPUT].isConnected()
            ? inputs[OUT_CEIL_INPUT].getVoltage()
            : params[OUT_CEIL_PARAM].getValue();

        float in = inputs[CV_INPUT].getVoltage();

        // Avoid divide-by-zero when the input range collapses to a point.
        // In that case the input is effectively its own floor.
        float out;
        if (inCeil == inFloor)
            out = outFloor;
        else
            out = math::rescale(in, inFloor, inCeil, outFloor, outCeil);

        // Respect the requested output range regardless of the input range:
        // the rescaled result is clamped to live within floor/ceil.
        out = clamp(out, std::min(outFloor, outCeil), std::max(outFloor, outCeil));

        outputs[CV_OUTPUT].setVoltage(out);
    }
};

struct RaRanger2Widget : ModuleWidget {
    RaRanger2Widget(RaRanger2Module *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ranger2.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float knobX = 15;
        float portX = 45;
        float y[] = {40, 90, 140, 190};

        addParam(createParamCentered<RaKnobTrim>(Vec(knobX, y[0]), module, RaRanger2Module::IN_FLOOR_PARAM));
        addInput(createInputCentered<RaPort>(Vec(portX, y[0]), module, RaRanger2Module::IN_FLOOR_INPUT));

        addParam(createParamCentered<RaKnobTrim>(Vec(knobX, y[1]), module, RaRanger2Module::IN_CEIL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(portX, y[1]), module, RaRanger2Module::IN_CEIL_INPUT));

        addParam(createParamCentered<RaKnobTrim>(Vec(knobX, y[2]), module, RaRanger2Module::OUT_FLOOR_PARAM));
        addInput(createInputCentered<RaPort>(Vec(portX, y[2]), module, RaRanger2Module::OUT_FLOOR_INPUT));

        addParam(createParamCentered<RaKnobTrim>(Vec(knobX, y[3]), module, RaRanger2Module::OUT_CEIL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(portX, y[3]), module, RaRanger2Module::OUT_CEIL_INPUT));

        addInput(createInputCentered<RaPort>(Vec(box.size.x / 2, 240), module, RaRanger2Module::CV_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(box.size.x / 2, 330), module, RaRanger2Module::CV_OUTPUT));
    }
};

Model *modelRaRanger2 = createModel<RaRanger2Module, RaRanger2Widget>("ra-ranger2");