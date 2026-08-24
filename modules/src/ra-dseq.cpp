#include "ra-components.hpp"
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

static const NVGcolor DSEQ_WHITE = nvgRGB(0xee, 0xee, 0xee);
static const NVGcolor DSEQ_WHITE_CURRENT = nvgRGB(0xff, 0xff, 0xff);
static const NVGcolor DSEQ_PURPLE = nvgRGB(0x99, 0x6d, 0xd2);
static const NVGcolor DSEQ_PURPLE_CURRENT = nvgRGB(0xc0, 0x9a, 0xe8);
static const NVGcolor DSEQ_DIM_WHITE = nvgRGB(0x40, 0x40, 0x40);
static const NVGcolor DSEQ_DIM_PURPLE = nvgRGB(0x3a, 0x2c, 0x4f);
static const NVGcolor DSEQ_BG = nvgRGB(0x0a, 0x0a, 0x0a);

struct RaDseqModule : Module {
    enum ParamIds {
        LENGTH_PARAM,
        SEQ_PARAM,
        MODE_PARAM,
        OUT_PARAM,
        STEP_NEXT_PARAM,
        CLEAR_PARAM,
        SEQ_PREV_PARAM,
        SEQ_NEXT_PARAM,
        STEP_PREV_PARAM,
        SHIFT_L_PARAM,
        SHIFT_R_PARAM,
        CHANCE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        STEP_NEXT_TRIG_INPUT,
        SEQ_PREV_TRIG_INPUT,
        SEQ_NEXT_TRIG_INPUT,
        STEP_PREV_TRIG_INPUT,
        SHIFT_L_TRIG_INPUT,
        SHIFT_R_TRIG_INPUT,
        CHANCE_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    bool steps[8][64] = {};
    int displayedSeq = 0;
    int stepIndex = 0;
    float pulse[8] = {};

    dsp::SchmittTrigger stepNextTrigger;
    dsp::SchmittTrigger stepNextButtonTrigger;
    dsp::SchmittTrigger clearTrigger;
    dsp::SchmittTrigger seqPrevTrigger;
    dsp::SchmittTrigger seqNextTrigger;
    dsp::SchmittTrigger seqPrevButtonTrigger;
    dsp::SchmittTrigger seqNextButtonTrigger;
    dsp::SchmittTrigger stepPrevTrigger;
    dsp::SchmittTrigger shiftLTrigger;
    dsp::SchmittTrigger shiftRTrigger;
    dsp::SchmittTrigger stepPrevButtonTrigger;
    dsp::SchmittTrigger shiftLButtonTrigger;
    dsp::SchmittTrigger shiftRButtonTrigger;
    bool hit[8] = {};

    RaDseqModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(LENGTH_PARAM, 1.f, 64.f, 16.f, "Length", " steps", 0.f, 1.f, 0.f);
        getParamQuantity(LENGTH_PARAM)->snapEnabled = true;
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"Multi", "Song"});
        configSwitch(OUT_PARAM, 0.f, 1.f, 0.f, "Output", {"Gate", "Trig"});
        configButton(SEQ_PREV_PARAM, "Seq prev");
        configButton(SEQ_NEXT_PARAM, "Seq next");
        configButton(STEP_PREV_PARAM, "Step prev");
        configButton(SHIFT_L_PARAM, "Shift left");
        configButton(SHIFT_R_PARAM, "Shift right");
        configButton(STEP_NEXT_PARAM, "Step next");
        configButton(CLEAR_PARAM, "Clear");
        configParam(CHANCE_PARAM, 0.f, 1.f, 1.f, "Chance", "%", 0, 100);

        configInput(STEP_NEXT_TRIG_INPUT, "Step next");
        configInput(SEQ_PREV_TRIG_INPUT, "Seq prev");
        configInput(SEQ_NEXT_TRIG_INPUT, "Seq next");
        configInput(STEP_PREV_TRIG_INPUT, "Step prev");
        configInput(SHIFT_L_TRIG_INPUT, "Shift left");
        configInput(SHIFT_R_TRIG_INPUT, "Shift right");
        configInput(CHANCE_CV_INPUT, "Chance CV");

        configOutput(OUT1_OUTPUT, "Out 1");
        configOutput(OUT2_OUTPUT, "Out 2");
        configOutput(OUT3_OUTPUT, "Out 3");
        configOutput(OUT4_OUTPUT, "Out 4");
        configOutput(OUT5_OUTPUT, "Out 5");
        configOutput(OUT6_OUTPUT, "Out 6");
        configOutput(OUT7_OUTPUT, "Out 7");
        configOutput(OUT8_OUTPUT, "Out 8");
    }

    int getLength() {
        return clamp((int)std::lround(params[LENGTH_PARAM].getValue()), 1, 64);
    }

    void selectSeq(int delta) {
        displayedSeq = eucMod(displayedSeq + delta, 8);
    }

    void shiftSteps(int dir) {
        // Rotate the current sequence's steps, wrapping around the full 64
        if (dir > 0) {
            bool last = steps[displayedSeq][63];
            for (int i = 63; i > 0; i--)
                steps[displayedSeq][i] = steps[displayedSeq][i - 1];
            steps[displayedSeq][0] = last;
        }
        else {
            bool first = steps[displayedSeq][0];
            for (int i = 0; i < 63; i++)
                steps[displayedSeq][i] = steps[displayedSeq][i + 1];
            steps[displayedSeq][63] = first;
        }
    }

    bool isStepSet(int step) {
        return steps[displayedSeq][step];
    }

    void setStep(int step, bool value) {
        steps[displayedSeq][step] = value;
    }

    void toggleStep(int step) {
        steps[displayedSeq][step] ^= true;
    }

    void onReset() override {
        memset(steps, 0, sizeof(steps));
        stepIndex = 0;
        memset(pulse, 0, sizeof(pulse));
        memset(hit, 0, sizeof(hit));
    }

    void process(const ProcessArgs& args) override {
        int length = getLength();
        bool songMode = params[MODE_PARAM].getValue() > 0.5f;
        bool trigMode = params[OUT_PARAM].getValue() > 0.5f;
        float chance = clamp(params[CHANCE_PARAM].getValue() + inputs[CHANCE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        // Sequence selection: prev/next trigger inputs and buttons
        if (seqPrevTrigger.process(inputs[SEQ_PREV_TRIG_INPUT].getVoltage()))
            selectSeq(-1);
        if (seqNextTrigger.process(inputs[SEQ_NEXT_TRIG_INPUT].getVoltage()))
            selectSeq(1);
        if (seqPrevButtonTrigger.process(params[SEQ_PREV_PARAM].getValue()))
            selectSeq(-1);
        if (seqNextButtonTrigger.process(params[SEQ_NEXT_PARAM].getValue()))
            selectSeq(1);

        // Shifts rotate the current sequence's steps, wrapping around
        if (shiftLTrigger.process(inputs[SHIFT_L_TRIG_INPUT].getVoltage()))
            shiftSteps(-1);
        if (shiftRTrigger.process(inputs[SHIFT_R_TRIG_INPUT].getVoltage()))
            shiftSteps(1);
        if (shiftLButtonTrigger.process(params[SHIFT_L_PARAM].getValue()))
            shiftSteps(-1);
        if (shiftRButtonTrigger.process(params[SHIFT_R_PARAM].getValue()))
            shiftSteps(1);

        // Keep the position in range if the length is shortened
        if (stepIndex >= length) {
            stepIndex = 0;
            memset(hit, 0, sizeof(hit));
        }

        if (clearTrigger.process(params[CLEAR_PARAM].getValue())) {
            for (int i = 0; i < 64; i++)
                steps[displayedSeq][i] = false;
        }

        // Roll per-hit chance and fire the set steps at the current position
        auto fireStep = [&]() {
            for (int k = 0; k < 8; k++) {
                bool active = !songMode || k == 0;
                int seq = songMode ? displayedSeq : k;
                bool set = active && steps[seq][stepIndex];
                hit[k] = set && (random::uniform() < chance);
                if (hit[k] && trigMode)
                    pulse[k] = 0.01f;
            }
        };

        if (stepNextTrigger.process(inputs[STEP_NEXT_TRIG_INPUT].getVoltage())
            || stepNextButtonTrigger.process(params[STEP_NEXT_PARAM].getValue())) {
            stepIndex++;
            if (stepIndex >= length)
                stepIndex = 0;
            fireStep();
        }

        if (stepPrevTrigger.process(inputs[STEP_PREV_TRIG_INPUT].getVoltage())
            || stepPrevButtonTrigger.process(params[STEP_PREV_PARAM].getValue())) {
            stepIndex--;
            if (stepIndex < 0)
                stepIndex = length - 1;
            fireStep();
        }

        // Output stage
        for (int k = 0; k < 8; k++) {
            float v = 0.f;
            if (trigMode) {
                if (pulse[k] > 0.f) {
                    v = 10.f;
                    pulse[k] -= args.sampleTime;
                }
            }
            else {
                if (hit[k])
                    v = 10.f;
            }
            outputs[OUT1_OUTPUT + k].setVoltage(v);
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_t* stepsJ = json_array();
        for (int s = 0; s < 8; s++) {
            json_t* seqJ = json_array();
            for (int i = 0; i < 64; i++)
                json_array_append_new(seqJ, json_boolean(steps[s][i]));
            json_array_append_new(stepsJ, seqJ);
        }
        json_object_set_new(rootJ, "steps", stepsJ);
        json_object_set_new(rootJ, "seq", json_integer(displayedSeq));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* seqJ = json_object_get(rootJ, "seq");
        if (seqJ)
            displayedSeq = clamp(json_integer_value(seqJ), 0, 7);

        json_t* stepsJ = json_object_get(rootJ, "steps");
        if (stepsJ) {
            for (int s = 0; s < 8; s++) {
                json_t* seqJ = json_array_get(stepsJ, s);
                if (!seqJ)
                    continue;
                for (int i = 0; i < 64; i++) {
                    json_t* stepJ = json_array_get(seqJ, i);
                    if (stepJ)
                        steps[s][i] = json_boolean_value(stepJ);
                }
            }
        }
    }
};

struct StepButton : OpaqueWidget {
    int index;
    RaDseqModule* module;

    void draw(const DrawArgs& args) override {
        bool set = module && module->isStepSet(index);
        bool current = module && (index == module->stepIndex);
        bool inRange = module && (index < module->getLength());
        Rect r = box.zeroPos();

        // Steps beyond the set length don't play — dim their LEDs
        if (!inRange) {
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, RECT_ARGS(r), 1.5f);
            nvgFillColor(args.vg, set ? DSEQ_DIM_WHITE : DSEQ_DIM_PURPLE);
            nvgFill(args.vg);
            return;
        }

        if (current) {
            // Glow halo behind the LED for the current position
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, RECT_ARGS(r.grow(Vec(2, 2))), 2.5f);
            nvgFillColor(args.vg, set ? nvgRGBA(0xff, 0xff, 0xff, 60) : nvgRGBA(0xc0, 0x9a, 0xe8, 60));
            nvgFill(args.vg);
        }

        if (set)
            nvgFillColor(args.vg, current ? DSEQ_WHITE_CURRENT : DSEQ_WHITE);
        else
            nvgFillColor(args.vg, current ? DSEQ_PURPLE_CURRENT : DSEQ_PURPLE);

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, RECT_ARGS(r), 1.5f);
        nvgFill(args.vg);
    }

    void onDragStart(const event::DragStart& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && module)
            module->toggleStep(index);
        OpaqueWidget::onDragStart(e);
    }

    void onDragEnter(const event::DragEnter& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && module) {
            // Paint: copy the drag-origin step state onto this button
            StepButton* origin = dynamic_cast<StepButton*>(e.origin);
            if (origin && origin->module)
                module->setStep(index, origin->module->isStepSet(origin->index));
        }
        OpaqueWidget::onDragEnter(e);
    }
};

struct SeqButton : OpaqueWidget {
    RaDseqModule* module;
    int index;

    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2.f);
        nvgFillColor(args.vg, (module && module->displayedSeq == index) ? DSEQ_WHITE_CURRENT : nvgRGB(0x2a, 0x2a, 0x2a));
        nvgFill(args.vg);
    }

    void onDragStart(const event::DragStart& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && module)
            module->displayedSeq = index;
        OpaqueWidget::onDragStart(e);
    }
};

struct StepGridDisplay : LedDisplay {
    RaDseqModule* module;

    void setModule(RaDseqModule* module) {
        this->module = module;

        const float margin = 4.f;
        float cell = (box.size.x - 2.f * margin) / 8.f;
        for (int i = 0; i < 64; i++) {
            int row = i / 8;
            int col = i % 8;
            StepButton* button = new StepButton;
            button->module = module;
            button->index = i;
            button->box.pos = Vec(margin + col * cell, margin + row * cell);
            button->box.size = Vec(cell - 2.f, cell - 2.f);
            addChild(button);
        }

        // Sequence selector row of LEDs below the grid (clickable)
        float ledY = margin + 8.f * cell + 6.f;
        float ledW = 10.f;
        float ledH = 8.f;
        for (int i = 0; i < 8; i++) {
            SeqButton* seq = new SeqButton;
            seq->module = module;
            seq->index = i;
            seq->box.pos = Vec(margin + i * cell + (cell - ledW) / 2.f, ledY);
            seq->box.size = Vec(ledW, ledH);
            addChild(seq);
        }
    }

    void draw(const DrawArgs& args) override {
        // Background first, then the step buttons (children)
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, DSEQ_BG);
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1);
        nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgStroke(args.vg);

        Widget::draw(args);
    }
};

struct RaDseqWidget : ModuleWidget {
    RaDseqWidget(RaDseqModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-dseq.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // 8x8 step grid (large, top) + sequence LED row
        StepGridDisplay* grid = createWidget<StepGridDisplay>(Vec(35, 40));
        grid->box.size = Vec(168, 184);
        grid->setModule(module);
        addChild(grid);

        // Controls below the grid — each trigger input sits directly beside its button
        addParam(createParamCentered<RaKnob>(Vec(26, 248), module, RaDseqModule::LENGTH_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(60, 248), module, RaDseqModule::CHANCE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(94, 248), module, RaDseqModule::CHANCE_CV_INPUT));
        addParam(createParamCentered<RaButton>(Vec(128, 248), module, RaDseqModule::CLEAR_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(162, 248), module, RaDseqModule::MODE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(196, 248), module, RaDseqModule::OUT_PARAM));

        addParam(createParamCentered<RaButton>(Vec(26, 290), module, RaDseqModule::SEQ_PREV_PARAM));
        addInput(createInputCentered<RaPort>(Vec(60, 290), module, RaDseqModule::SEQ_PREV_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(94, 290), module, RaDseqModule::SEQ_NEXT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(128, 290), module, RaDseqModule::SEQ_NEXT_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(162, 290), module, RaDseqModule::STEP_PREV_PARAM));
        addInput(createInputCentered<RaPort>(Vec(196, 290), module, RaDseqModule::STEP_PREV_TRIG_INPUT));

        addParam(createParamCentered<RaButton>(Vec(26, 332), module, RaDseqModule::STEP_NEXT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(60, 332), module, RaDseqModule::STEP_NEXT_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(94, 332), module, RaDseqModule::SHIFT_L_PARAM));
        addInput(createInputCentered<RaPort>(Vec(128, 332), module, RaDseqModule::SHIFT_L_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(162, 332), module, RaDseqModule::SHIFT_R_PARAM));
        addInput(createInputCentered<RaPort>(Vec(196, 332), module, RaDseqModule::SHIFT_R_TRIG_INPUT));

        // Right: 8 trigger outputs
        addOutput(createOutputCentered<RaPort>(Vec(224, 65), module, RaDseqModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(224, 105), module, RaDseqModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(224, 145), module, RaDseqModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(224, 185), module, RaDseqModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(224, 225), module, RaDseqModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(224, 265), module, RaDseqModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(224, 305), module, RaDseqModule::OUT7_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(224, 345), module, RaDseqModule::OUT8_OUTPUT));
    }
};

Model* modelRaDseq = createModel<RaDseqModule, RaDseqWidget>("ra-dseq");