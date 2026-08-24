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
        RANDOMIZE_PARAM,
        RAND_CHANCE_PARAM,
        RESET_PARAM,
        SEQ_RESET_PARAM,
        RUN_PARAM,
        GOL_STEP_PARAM,
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
        RANDOMIZE_TRIG_INPUT,
        RESET_TRIG_INPUT,
        SEQ_RESET_TRIG_INPUT,
        RUN_INPUT,
        GOL_TRIG_INPUT,
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
        RUN_LIGHT,
        NUM_LIGHTS
    };

    bool steps[8][64] = {};
    int seqLengths[8] = {16, 0, 0, 0, 0, 0, 0, 0}; // per-sequence loop length; 0 = sequence off
    int lengthKnobValue = -1;                      // last applied Length knob position (pickup; -1 = unset)
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
    dsp::SchmittTrigger randomizeTrigger;
    dsp::SchmittTrigger randomizeButtonTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger resetButtonTrigger;
    dsp::SchmittTrigger seqResetTrigger;
    dsp::SchmittTrigger seqResetButtonTrigger;
    dsp::SchmittTrigger runBtnTrigger;
    dsp::SchmittTrigger golTrigger;
    dsp::SchmittTrigger golButtonTrigger;
    bool running = false;
    bool hit[8] = {};

    RaDseqModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(LENGTH_PARAM, 0.f, 64.f, 16.f, "Length", " steps", 0.f, 1.f, 0.f);
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
        configButton(RANDOMIZE_PARAM, "Randomize");
        configButton(RESET_PARAM, "Reset");
        configButton(SEQ_RESET_PARAM, "Reset sequence");
        configButton(RUN_PARAM, "Run");
        configButton(GOL_STEP_PARAM, "GOL step");
        configParam(CHANCE_PARAM, 0.f, 1.f, 1.f, "Chance", "%", 0, 100);
        configParam(RAND_CHANCE_PARAM, 0.f, 1.f, 0.5f, "Randomize chance", "%", 0, 100);

        configInput(STEP_NEXT_TRIG_INPUT, "Step next");
        configInput(SEQ_PREV_TRIG_INPUT, "Seq prev");
        configInput(SEQ_NEXT_TRIG_INPUT, "Seq next");
        configInput(STEP_PREV_TRIG_INPUT, "Step prev");
        configInput(SHIFT_L_TRIG_INPUT, "Shift left");
        configInput(SHIFT_R_TRIG_INPUT, "Shift right");
        configInput(CHANCE_CV_INPUT, "Chance CV");
        configInput(RANDOMIZE_TRIG_INPUT, "Randomize");
        configInput(RESET_TRIG_INPUT, "Reset");
        configInput(SEQ_RESET_TRIG_INPUT, "Reset sequence");
        configInput(RUN_INPUT, "Run");
        configLight(RUN_LIGHT, "Run");
        configInput(GOL_TRIG_INPUT, "GOL step");

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
        // Loop length of the displayed sequence; 0 = sequence off
        return clamp(seqLengths[displayedSeq], 0, 64);
    }

    bool hasZeroLength(int seq) {
        return seqLengths[seq] <= 0;
    }

    void selectSeq(int delta, bool skipEmpty) {
        if (!skipEmpty) {
            displayedSeq = eucMod(displayedSeq + delta, 8);
            return;
        }
        // Trigger-driven navigation skips sequences with no set steps
        for (int i = 1; i <= 8; i++) {
            int d = (delta > 0) ? i : -i;
            int seq = eucMod(displayedSeq + d, 8);
            if (!hasZeroLength(seq)) {
                displayedSeq = seq;
                return;
            }
        }
        // Every sequence is empty — fall back to a plain step
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

    // Conway's Game of Life on the displayed sequence's 8x8 grid, with
    // toroidal wrap: live cells survive with 2-3 neighbours, dead cells
    // revive with exactly 3
    void runGolStep() {
        bool next[64] = {};
        for (int i = 0; i < 64; i++) {
            int row = i / 8;
            int col = i % 8;
            int alive = 0;
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0)
                        continue;
                    int ni = eucMod(row + dr, 8) * 8 + eucMod(col + dc, 8);
                    if (steps[displayedSeq][ni])
                        alive++;
                }
            }
            bool cur = steps[displayedSeq][i];
            next[i] = (cur && (alive == 2 || alive == 3)) || (!cur && alive == 3);
        }
        memcpy(steps[displayedSeq], next, sizeof(next));
    }

    void onReset() override {
        memset(steps, 0, sizeof(steps));
        memset(seqLengths, 0, sizeof(seqLengths));
        seqLengths[0] = 16;
        lengthKnobValue = -1;
        stepIndex = 0;
        memset(pulse, 0, sizeof(pulse));
        memset(hit, 0, sizeof(hit));
    }

    void process(const ProcessArgs& args) override {
        // Pickup: the Length knob only edits the currently displayed sequence
        // while it is being turned; switching sequences never rewrites values
        int lv = (int)std::lround(params[LENGTH_PARAM].getValue());
        if (lengthKnobValue < 0)
            lengthKnobValue = lv;
        if (lv != lengthKnobValue) {
            seqLengths[displayedSeq] = clamp(lv, 0, 64);
            lengthKnobValue = lv;
        }

        int length = getLength();
        bool songMode = params[MODE_PARAM].getValue() > 0.5f;
        bool trigMode = params[OUT_PARAM].getValue() > 0.5f;
        float chance = clamp(params[CHANCE_PARAM].getValue() + inputs[CHANCE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        // Run: the latching button toggles the transport; the Run input gates it.
        // While not running, external step triggers are ignored — the step buttons still work
        if (runBtnTrigger.process(params[RUN_PARAM].getValue()))
            running = !running;
        bool runActive = running || inputs[RUN_INPUT].getVoltage() > 1.f;
        lights[RUN_LIGHT].setBrightness(runActive ? 1.f : 0.f);

        // Sequence selection: trigger inputs skip empty sequences; the
        // buttons step through all eight regardless
        if (seqPrevTrigger.process(inputs[SEQ_PREV_TRIG_INPUT].getVoltage()))
            selectSeq(-1, true);
        if (seqNextTrigger.process(inputs[SEQ_NEXT_TRIG_INPUT].getVoltage()))
            selectSeq(1, true);
        if (seqPrevButtonTrigger.process(params[SEQ_PREV_PARAM].getValue()))
            selectSeq(-1, false);
        if (seqNextButtonTrigger.process(params[SEQ_NEXT_PARAM].getValue()))
            selectSeq(1, false);

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

        // Randomize the current sequence: each step independently has a
        // RAND_CHANCE probability of being flipped to a random on/off state
        if (randomizeTrigger.process(inputs[RANDOMIZE_TRIG_INPUT].getVoltage())
            || randomizeButtonTrigger.process(params[RANDOMIZE_PARAM].getValue())) {
            float rc = clamp(params[RAND_CHANCE_PARAM].getValue(), 0.f, 1.f);
            for (int i = 0; i < 64; i++) {
                if (random::uniform() < rc)
                    steps[displayedSeq][i] = random::uniform() < 0.5f;
            }
        }

        // Game of Life: button or trigger input advances the displayed
        // sequence's grid by one generation
        if (golButtonTrigger.process(params[GOL_STEP_PARAM].getValue())
            || golTrigger.process(inputs[GOL_TRIG_INPUT].getVoltage()))
            runGolStep();

        // Roll per-hit chance and fire the set steps at the current position
        auto fireStep = [&]() {
            for (int k = 0; k < 8; k++) {
                bool active = !songMode || k == 0;
                int seq = songMode ? displayedSeq : k;
                bool set = active && stepIndex < seqLengths[seq] && steps[seq][stepIndex];
                hit[k] = set && (random::uniform() < chance);
                if (hit[k] && trigMode)
                    pulse[k] = 0.01f;
            }
        };

        // Reset: jump back to step 1 of the current sequence and play it immediately
        if (resetButtonTrigger.process(params[RESET_PARAM].getValue())
            || resetTrigger.process(inputs[RESET_TRIG_INPUT].getVoltage())) {
            stepIndex = 0;
            memset(hit, 0, sizeof(hit));
            fireStep();
        }

        // Reset sequence: jump back to sequence 1 step 1 and play it immediately
        if (seqResetButtonTrigger.process(params[SEQ_RESET_PARAM].getValue())
            || seqResetTrigger.process(inputs[SEQ_RESET_TRIG_INPUT].getVoltage())) {
            displayedSeq = 0;
            stepIndex = 0;
            memset(hit, 0, sizeof(hit));
            fireStep();
        }

        if (stepNextButtonTrigger.process(params[STEP_NEXT_PARAM].getValue())
            || (runActive && stepNextTrigger.process(inputs[STEP_NEXT_TRIG_INPUT].getVoltage()))) {
            stepIndex++;
            if (stepIndex >= length)
                stepIndex = 0;
            fireStep();
        }

        if (stepPrevButtonTrigger.process(params[STEP_PREV_PARAM].getValue())
            || (runActive && stepPrevTrigger.process(inputs[STEP_PREV_TRIG_INPUT].getVoltage()))) {
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
        json_t* lensJ = json_array();
        for (int i = 0; i < 8; i++)
            json_array_append_new(lensJ, json_integer(seqLengths[i]));
        json_object_set_new(rootJ, "lengths", lensJ);
        json_object_set_new(rootJ, "seq", json_integer(displayedSeq));
        json_object_set_new(rootJ, "running", json_boolean(running));
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

        json_t* lensJ = json_object_get(rootJ, "lengths");
        if (lensJ && json_array_size(lensJ) == 8) {
            for (int i = 0; i < 8; i++)
                seqLengths[i] = clamp(json_integer_value(json_array_get(lensJ, i)), 0, 64);
        }
        else {
            // Legacy patches: the global length applied to every sequence
            int gl = clamp((int)std::lround(params[LENGTH_PARAM].getValue()), 1, 64);
            for (int i = 0; i < 8; i++)
                seqLengths[i] = gl;
        }
        json_t* runJ = json_object_get(rootJ, "running");
        if (runJ)
            running = json_boolean_value(runJ);

        lengthKnobValue = (int)std::lround(params[LENGTH_PARAM].getValue());
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
        const float seqColW = 14.f;  // sequence LED column, left of the grid
        float cell = (box.size.x - 2.f * margin - seqColW) / 8.f;
        for (int i = 0; i < 64; i++) {
            int row = i / 8;
            int col = i % 8;
            StepButton* button = new StepButton;
            button->module = module;
            button->index = i;
            button->box.pos = Vec(margin + seqColW + col * cell, margin + row * cell);
            button->box.size = Vec(cell - 2.f, cell - 2.f);
            addChild(button);
        }

        // Sequence selector LEDs to the left of the grid, top to bottom
        float ledW = 10.f;
        float ledH = 8.f;
        for (int i = 0; i < 8; i++) {
            SeqButton* seq = new SeqButton;
            seq->module = module;
            seq->index = i;
            seq->box.pos = Vec(margin + (seqColW - ledW) / 2.f, margin + i * cell + (cell - ledH) / 2.f);
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

struct PurpleLight : GrayModuleLightWidget {
    PurpleLight() {
        addBaseColor(nvgRGB(0x99, 0x6d, 0xd2));
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

        // 8x8 step grid + sequence LED column on the left, cleared of the
        // output stacks on the right (grid centre x=135; 21hp panel)
        StepGridDisplay* grid = createWidget<StepGridDisplay>(Vec(44, 40));
        grid->box.size = Vec(182, 168);
        grid->setModule(module);
        addChild(grid);

        // Left of the grid: Game of Life step button + trigger input
        addParam(createParamCentered<RaButton>(Vec(22, 100), module, RaDseqModule::GOL_STEP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(22, 144), module, RaDseqModule::GOL_TRIG_INPUT));

        // Controls below the grid on a strict 9-column grid: buttons over their
        // trig inputs, settings row underneath, centred on the module (x=158).
        // Columns: 26 59 92 125 158 191 224 257 290
        addParam(createParamCentered<RaButton>(Vec(26, 246), module, RaDseqModule::STEP_PREV_PARAM));
        addParam(createParamCentered<RaButton>(Vec(59, 246), module, RaDseqModule::STEP_NEXT_PARAM));
        addParam(createParamCentered<RaButton>(Vec(92, 246), module, RaDseqModule::SEQ_PREV_PARAM));
        addParam(createParamCentered<RaButton>(Vec(125, 246), module, RaDseqModule::SEQ_NEXT_PARAM));
        addParam(createParamCentered<RaButton>(Vec(158, 246), module, RaDseqModule::SHIFT_L_PARAM));
        addParam(createParamCentered<RaButton>(Vec(191, 246), module, RaDseqModule::SHIFT_R_PARAM));
        addParam(createParamCentered<RaButton>(Vec(224, 246), module, RaDseqModule::RANDOMIZE_PARAM));
        addParam(createParamCentered<RaButton>(Vec(257, 246), module, RaDseqModule::RESET_PARAM));
        addParam(createParamCentered<RaButton>(Vec(290, 246), module, RaDseqModule::SEQ_RESET_PARAM));

        addInput(createInputCentered<RaPort>(Vec(26, 288), module, RaDseqModule::STEP_PREV_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(59, 288), module, RaDseqModule::STEP_NEXT_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(92, 288), module, RaDseqModule::SEQ_PREV_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(125, 288), module, RaDseqModule::SEQ_NEXT_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(158, 288), module, RaDseqModule::SHIFT_L_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(191, 288), module, RaDseqModule::SHIFT_R_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(224, 288), module, RaDseqModule::RANDOMIZE_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(257, 288), module, RaDseqModule::RESET_TRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(290, 288), module, RaDseqModule::SEQ_RESET_TRIG_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(26, 330), module, RaDseqModule::LENGTH_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(59, 330), module, RaDseqModule::CHANCE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(92, 330), module, RaDseqModule::CHANCE_CV_INPUT));
        addParam(createParamCentered<RaButton>(Vec(125, 330), module, RaDseqModule::CLEAR_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(158, 330), module, RaDseqModule::MODE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(191, 330), module, RaDseqModule::OUT_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(224, 330), module, RaDseqModule::RAND_CHANCE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(257, 330), module, RaDseqModule::RUN_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(290, 330), module, RaDseqModule::RUN_PARAM, RaDseqModule::RUN_LIGHT));

        // Right, near the top: 8 trigger outputs in two stacks — 1–4 left, 5–8 right
        addOutput(createOutputCentered<RaPort>(Vec(260, 60), module, RaDseqModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(260, 100), module, RaDseqModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(260, 140), module, RaDseqModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(260, 180), module, RaDseqModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(294, 60), module, RaDseqModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(294, 100), module, RaDseqModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(294, 140), module, RaDseqModule::OUT7_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(294, 180), module, RaDseqModule::OUT8_OUTPUT));
    }
};

Model* modelRaDseq = createModel<RaDseqModule, RaDseqWidget>("ra-dseq");