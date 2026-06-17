#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaEndlessModule : Module {
    std::vector<float> steps;
    int currentPos = 0;

    dsp::SchmittTrigger writeExtTrigger;
    dsp::SchmittTrigger restExtTrigger;
    dsp::SchmittTrigger clearExtTrigger;
    dsp::SchmittTrigger resetExtTrigger;
    dsp::SchmittTrigger stepFwdExtTrigger;
    dsp::SchmittTrigger stepBackExtTrigger;
    dsp::SchmittTrigger writeBtnTrigger;
    dsp::SchmittTrigger restBtnTrigger;
    dsp::SchmittTrigger clearBtnTrigger;
    dsp::SchmittTrigger resetBtnTrigger;
    dsp::SchmittTrigger stepFwdBtnTrigger;
    dsp::SchmittTrigger stepBackBtnTrigger;
    dsp::PulseGenerator trigPulse;
    dsp::PulseGenerator endPulse;
    dsp::PulseGenerator startPulse;

    static constexpr float REST_VALUE = -20.f;

    enum ParamIds {
        WRITE_PARAM,
        REST_PARAM,
        CLEAR_PARAM,
        RESET_PARAM,
        STEP_FWD_PARAM,
        STEP_BACK_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CV_INPUT,
        WRITE_TRIG_INPUT,
        REST_TRIG_INPUT,
        CLEAR_TRIG_INPUT,
        RESET_TRIG_INPUT,
        STEP_FWD_TRIG_INPUT,
        STEP_BACK_TRIG_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CV_OUTPUT,
        TRIG_OUTPUT,
        END_TRIG_OUTPUT,
        START_TRIG_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    RaEndlessModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(WRITE_PARAM, 0.f, 1.f, 0.f, "Write");
        configParam(REST_PARAM, 0.f, 1.f, 0.f, "Rest");
        configParam(CLEAR_PARAM, 0.f, 1.f, 0.f, "Clear");
        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset position");
        configParam(STEP_FWD_PARAM, 0.f, 1.f, 0.f, "Step forward");
        configParam(STEP_BACK_PARAM, 0.f, 1.f, 0.f, "Step back");
        configInput(CV_INPUT, "Pitch CV (1V/oct)");
        configInput(WRITE_TRIG_INPUT, "Write trigger");
        configInput(REST_TRIG_INPUT, "Rest trigger");
        configInput(CLEAR_TRIG_INPUT, "Clear trigger");
        configInput(RESET_TRIG_INPUT, "Reset trigger");
        configInput(STEP_FWD_TRIG_INPUT, "Step forward trigger");
        configInput(STEP_BACK_TRIG_INPUT, "Step back trigger");
        configOutput(CV_OUTPUT, "Pitch CV");
        configOutput(TRIG_OUTPUT, "Step trigger");
        configOutput(END_TRIG_OUTPUT, "Sequence end trigger");
        configOutput(START_TRIG_OUTPUT, "Sequence start trigger");
    }

    void writeStep() {
        float v = inputs[CV_INPUT].getVoltage();
        if (currentPos >= (int)steps.size())
            steps.push_back(v);
        else
            steps[currentPos] = v;
        advance();
    }

    void insertRest() {
        if (currentPos >= (int)steps.size())
            steps.push_back(REST_VALUE);
        else
            steps[currentPos] = REST_VALUE;
        advance();
    }

    void advance() {
        currentPos++;
        trigPulse.trigger(1e-3f);
    }

    void stepFwd() {
        if (steps.empty()) return;
        currentPos++;
        trigPulse.trigger(1e-3f);
        if (currentPos >= (int)steps.size()) {
            currentPos = 0;
            endPulse.trigger(1e-3f);
        }
    }

    void stepBack() {
        if (steps.empty()) return;
        currentPos--;
        if (currentPos < 0)
            currentPos = (int)steps.size() - 1;
        trigPulse.trigger(1e-3f);
    }

    void process(const ProcessArgs &args) override {
        if (writeBtnTrigger.process(params[WRITE_PARAM].getValue() * 10.f) ||
            writeExtTrigger.process(inputs[WRITE_TRIG_INPUT].getVoltage()))
            writeStep();

        if (restBtnTrigger.process(params[REST_PARAM].getValue() * 10.f) ||
            restExtTrigger.process(inputs[REST_TRIG_INPUT].getVoltage()))
            insertRest();

        if (clearBtnTrigger.process(params[CLEAR_PARAM].getValue() * 10.f) ||
            clearExtTrigger.process(inputs[CLEAR_TRIG_INPUT].getVoltage())) {
            steps.clear();
            currentPos = 0;
        }

        if (resetBtnTrigger.process(params[RESET_PARAM].getValue() * 10.f) ||
            resetExtTrigger.process(inputs[RESET_TRIG_INPUT].getVoltage())) {
            currentPos = 0;
            startPulse.trigger(1e-3f);
        }

        if (stepFwdBtnTrigger.process(params[STEP_FWD_PARAM].getValue() * 10.f) ||
            stepFwdExtTrigger.process(inputs[STEP_FWD_TRIG_INPUT].getVoltage()))
            stepFwd();

        if (stepBackBtnTrigger.process(params[STEP_BACK_PARAM].getValue() * 10.f) ||
            stepBackExtTrigger.process(inputs[STEP_BACK_TRIG_INPUT].getVoltage()))
            stepBack();

        if (!steps.empty() && currentPos < (int)steps.size()) {
            float v = steps[currentPos];
            if (v == REST_VALUE) {
                outputs[CV_OUTPUT].setVoltage(0.f);
                trigPulse.reset();
            } else {
                outputs[CV_OUTPUT].setVoltage(v);
            }
        } else {
            outputs[CV_OUTPUT].setVoltage(0.f);
        }

        outputs[TRIG_OUTPUT].setVoltage(trigPulse.process(args.sampleTime) ? 10.f : 0.f);
        outputs[END_TRIG_OUTPUT].setVoltage(endPulse.process(args.sampleTime) ? 10.f : 0.f);
        outputs[START_TRIG_OUTPUT].setVoltage(startPulse.process(args.sampleTime) ? 10.f : 0.f);
    }
};

constexpr float RaEndlessModule::REST_VALUE;

struct EndlessDisplay : LedDisplay {
    RaEndlessModule *module;
    std::shared_ptr<Font> font;

    EndlessDisplay() {
        font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x11, 0x11, 0x11));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1);
        nvgStrokeColor(args.vg, nvgRGB(0x55, 0x55, 0x55));
        nvgStroke(args.vg);

        if (!module) return;

        float inV = module->inputs[RaEndlessModule::CV_INPUT].getVoltage();
        float outV = module->outputs[RaEndlessModule::CV_OUTPUT].getVoltage();
        int stepCount = (int)module->steps.size();
        int displayPos = (stepCount > 0) ? clamp(module->currentPos, 0, stepCount - 1) + 1 : 0;
        std::string stepStr = (stepCount == 0) ? "0" : rack::string::f("%d/%d", displayPos, stepCount);

        if (font) {
            nvgFontFaceId(args.vg, font->handle);
            nvgFontSize(args.vg, 18);
            nvgFillColor(args.vg, nvgRGB(0xaa, 0xcc, 0x88));
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            std::string line1 = rack::string::f("IN  %+.2fV", inV);
            std::string line2 = rack::string::f("OUT %+.2fV", outV);
            std::string line3 = rack::string::f("STEP %s", stepStr.c_str());

            nvgText(args.vg, box.size.x / 2, box.size.y * 0.25, line1.c_str(), nullptr);
            nvgText(args.vg, box.size.x / 2, box.size.y * 0.5, line2.c_str(), nullptr);
            nvgText(args.vg, box.size.x / 2, box.size.y * 0.75, line3.c_str(), nullptr);
        }
    }
};

struct RaEndlessWidget : ModuleWidget {
    RaEndlessWidget(RaEndlessModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-endless.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // 4x taller display spanning nearly full width
        auto *display = new EndlessDisplay();
        display->box.pos = Vec(12, 6);
        display->box.size = Vec(126, 96);
        display->module = module;
        addChild(display);

        // CV input at y=125
        addInput(createInputCentered<PJ301MPort>(Vec(50, 125), module, RaEndlessModule::CV_INPUT));

        // Function controls — 3 rows, each with Btn (x=23|91) + Trig (x=57|125)
        // Row 1: WRT / REST  at y=165
        addParam(createParamCentered<TL1105>(Vec(23, 165), module, RaEndlessModule::WRITE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(57, 165), module, RaEndlessModule::WRITE_TRIG_INPUT));
        addParam(createParamCentered<TL1105>(Vec(91, 165), module, RaEndlessModule::REST_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(125, 165), module, RaEndlessModule::REST_TRIG_INPUT));

        // Row 2: CLR / RST  at y=205
        addParam(createParamCentered<TL1105>(Vec(23, 205), module, RaEndlessModule::CLEAR_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(57, 205), module, RaEndlessModule::CLEAR_TRIG_INPUT));
        addParam(createParamCentered<TL1105>(Vec(91, 205), module, RaEndlessModule::RESET_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(125, 205), module, RaEndlessModule::RESET_TRIG_INPUT));

        // Row 3: FWD / BACK  at y=245
        addParam(createParamCentered<TL1105>(Vec(23, 245), module, RaEndlessModule::STEP_FWD_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(57, 245), module, RaEndlessModule::STEP_FWD_TRIG_INPUT));
        addParam(createParamCentered<TL1105>(Vec(91, 245), module, RaEndlessModule::STEP_BACK_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(125, 245), module, RaEndlessModule::STEP_BACK_TRIG_INPUT));

        // Outputs at y=290
        addOutput(createOutputCentered<PJ301MPort>(Vec(27, 290), module, RaEndlessModule::CV_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(59, 290), module, RaEndlessModule::TRIG_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(91, 290), module, RaEndlessModule::END_TRIG_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(123, 290), module, RaEndlessModule::START_TRIG_OUTPUT));
    }
};

Model *modelRaEndless = createModel<RaEndlessModule, RaEndlessWidget>("ra-endless");
