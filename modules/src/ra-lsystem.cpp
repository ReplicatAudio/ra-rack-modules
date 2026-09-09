// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: STEP_PARAM "Step"
// fname: STEP_BACK_PARAM "Step ba"
// fname: CLEAR_PARAM "Clear"
// fname: OUT_PARAM "Out"
// fname: STEP_TRIG_INPUT "Step"
// fname: STEP_BACK_TRIG_INPUT "Step ba"
// fname: POSITION_INPUT "Position"
// fname: START_PARAM "Start"
// fname: END_PARAM "End"
// fname: START_CV_INPUT "Start"
// fname: END_CV_INPUT "End"
// fname: RESET_PARAM "Reset po"
// fname: RESET_TRIG_INPUT "Reset tr"
// fname: OUT1_OUTPUT "Red"
// fname: OUT2_OUTPUT "Green"
// fname: OUT3_OUTPUT "Blue"
// fname: OUT4_OUTPUT "Yellow"
// fname: OUT5_OUTPUT "Cyan"
// fname: OUT6_OUTPUT "Magenta"
// fname: OUT7_OUTPUT "White"
#include "ra-components.hpp"
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

// The 8 symbols represented by colors:
//   0 = off/black (empty — skipped in the string)
//   1 = red, 2 = green, 3 = blue, 4 = yellow, 5 = cyan, 6 = magenta, 7 = white
static NVGcolor lsysColor(int c) {
    switch (c) {
        case 1:  return componentlibrary::SCHEME_RED;
        case 2:  return componentlibrary::SCHEME_GREEN;
        case 3:  return componentlibrary::SCHEME_BLUE;
        case 4:  return componentlibrary::SCHEME_YELLOW;
        case 5:  return componentlibrary::SCHEME_CYAN;
        case 6:  return nvgRGB(0xd0, 0x2a, 0xe0); // magenta
        case 7:  return componentlibrary::SCHEME_WHITE;
        default: return nvgRGB(0x12, 0x12, 0x12); // off/black cell (visible on the panel)
    }
}

static const NVGcolor LSYSTEM_BG = nvgRGB(0x0a, 0x0a, 0x0a);
static const NVGcolor LSYSTEM_BORDER = nvgRGB(0x4a, 0x40, 0x66);

static const int LSYSTEM_MAX_COLS = 8;
static const int LSYSTEM_MAX_ROWS = 24;
static const int LSYSTEM_MAX_CELLS = LSYSTEM_MAX_COLS * LSYSTEM_MAX_ROWS; // 8x24 output matrix
static const int LSYSTEM_NUM_RULES = 8;   // number of rewrite rules
static const int LSYSTEM_RULE_BODY = 6;   // result cells per rule
static const int LSYSTEM_AXIOM = 8;       // axiom symbol cells

// Shared cell renderer. `current` draws a bright halo for the live sequencer
// position; `dim` darkens cells outside the knob-selected start/end window.
static void drawCell(NVGcontext* vg, Rect r, int color, bool current, bool dim) {
    nvgBeginPath(vg);
    nvgRoundedRect(vg, RECT_ARGS(r), 1.5f);
    nvgFillColor(vg, lsysColor(color));
    nvgFill(vg);

    if (dim) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, RECT_ARGS(r), 1.5f);
        nvgFillColor(vg, nvgRGBA(0x0a, 0x0a, 0x0a, 150));
        nvgFill(vg);
    }

    if (color == 0) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, RECT_ARGS(r), 1.5f);
        nvgStrokeWidth(vg, 1.f);
        nvgStrokeColor(vg, nvgRGB(0x2e, 0x2e, 0x2e));
        nvgStroke(vg);
    }

    if (current) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, RECT_ARGS(r.grow(Vec(1.f, 1.f))), 2.f);
        nvgStrokeWidth(vg, 2.f);
        nvgStrokeColor(vg, nvgRGB(0xff, 0xff, 0xff));
        nvgStroke(vg);
    }
}

struct RaLsysModule : Module {
    enum ParamIds {
        OUT_PARAM,      // Output: Gate / Trig
        STEP_PARAM,     // Step forward button
        STEP_BACK_PARAM,// Step back button
        CLEAR_PARAM,    // Clear button
        RESET_PARAM,    // Reset position button
        START_PARAM,    // Start position knob
        END_PARAM,      // End position knob
        NUM_PARAMS
    };
    enum InputIds {
        STEP_TRIG_INPUT,
        STEP_BACK_TRIG_INPUT,
        POSITION_INPUT,
        RESET_TRIG_INPUT,
        START_CV_INPUT,
        END_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT, OUT2_OUTPUT, OUT3_OUTPUT,
        OUT4_OUTPUT, OUT5_OUTPUT, OUT6_OUTPUT, OUT7_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Editable L-system definition (persisted)
    int axiom[LSYSTEM_AXIOM] = {1, 2, 3, 0, 0, 0, 0, 0};
    int ruleTarget[LSYSTEM_NUM_RULES] = {};
    int ruleResult[LSYSTEM_NUM_RULES][LSYSTEM_RULE_BODY] = {};

    // Generated output string (row-major fill of the 8x16 matrix)
    int output[LSYSTEM_MAX_CELLS] = {};
    int outputLen = 0;

    unsigned long stepIndex = 0;
    bool hit[7] = {};   // gate-mode outputs held for the current position
    float pulse[7] = {}; // trig-mode decay timers

    // Arm the current position's outputs. Only called when the sequencer
    // advances (or the position otherwise changes) so trig pulses fire once
    // per step rather than every sample.
    void fireStep() {
        memset(hit, 0, sizeof(hit));
        if (outputLen > 0 && curIdx() < outputLen) {
            int sym = output[curIdx()];
            if (sym >= 1 && sym <= 7) {
                int o = sym - 1;
                hit[o] = true;
                if (params[OUT_PARAM].getValue() > 0.5f)
                    pulse[o] = 0.01f;
            }
        }
    }

    dsp::SchmittTrigger stepButtonTrigger;
    dsp::SchmittTrigger stepTrigTrigger;
    dsp::SchmittTrigger stepBackButtonTrigger;
    dsp::SchmittTrigger stepBackTrigTrigger;
    dsp::SchmittTrigger clearButtonTrigger;
    dsp::SchmittTrigger resetButtonTrigger;
    dsp::SchmittTrigger resetTrigTrigger;

    RaLsysModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(OUT_PARAM, 0.f, 1.f, 0.f, "Output", {"Gate", "Trig"});
        configButton(STEP_PARAM, "Step forward");
        configButton(STEP_BACK_PARAM, "Step back");
        configButton(CLEAR_PARAM, "Clear");
        configButton(RESET_PARAM, "Reset position");
        configParam(START_PARAM, 0.f, 1.f, 0.f, "Start position");
        configParam(END_PARAM, 0.f, 1.f, 1.f, "End position");

        configInput(STEP_TRIG_INPUT, "Step forward");
        configInput(STEP_BACK_TRIG_INPUT, "Step back");
        configInput(POSITION_INPUT, "Position");
        configInput(RESET_TRIG_INPUT, "Reset trigger");
        configInput(START_CV_INPUT, "Start CV");
        configInput(END_CV_INPUT, "End CV");

        configOutput(OUT1_OUTPUT, "Red");
        configOutput(OUT2_OUTPUT, "Green");
        configOutput(OUT3_OUTPUT, "Blue");
        configOutput(OUT4_OUTPUT, "Yellow");
        configOutput(OUT5_OUTPUT, "Cyan");
        configOutput(OUT6_OUTPUT, "Magenta");
        configOutput(OUT7_OUTPUT, "White");

        // Build the initial output from the default axiom (rules are empty)
        regenerate();
        fireStep();
    }

    // Effective start/end fractions of the sequence length. A connected CV
    // makes the knob act as an attenuator (0-10 V maps over the knob's
    // value); otherwise the knob sets the position directly.
    float startFraction() {
            float v = clamp(params[START_PARAM].getValue(), 0.f, 1.f);
            if (inputs[START_CV_INPUT].isConnected())
                v *= clamp(inputs[START_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
            return v;
        }

    float endFraction() {
            float v = clamp(params[END_PARAM].getValue(), 0.f, 1.f);
            if (inputs[END_CV_INPUT].isConnected())
                v *= clamp(inputs[END_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
            return v;
        }

    // Enabled window into the output string, set by the start/end knobs.
    // Both knobs are normalized 0..1 fractions of the sequence length and
    // resolve to cell indices, clamping so the window is never empty.
    int seqStart() {
        if (outputLen <= 1) return 0;
        return (int)std::lround(clamp(startFraction(), 0.f, 1.f) * (float)(outputLen - 1));
    }

    int seqEnd() {
        if (outputLen <= 1) return 0;
        int e = (int)std::lround(clamp(endFraction(), 0.f, 1.f) * (float)(outputLen - 1));
        return std::max(e, seqStart());
    }

    // Is matrix cell `idx` inside the knob-selected window?
    bool positionEnabled(int idx) {
        return idx >= seqStart() && idx <= seqEnd();
    }

    int curIdx() {
        if (outputLen <= 0) return 0;
        int s = seqStart();
        int len = seqEnd() - s + 1;
        return s + (int)(stepIndex % (unsigned long)len);
    }

    // Expand the L-system one generation: every non-black symbol is replaced by
    // the body of the first matching rule (black cells in a body are skipped);
    // symbols with no matching rule persist unchanged. Output is hard-capped at
    // LSYSTEM_MAX_CELLS so the buffer can never blow up.
    void expandStep(const int* src, int slen, int* dst, int& dlen) {
        dlen = 0;
        for (int i = 0; i < slen && dlen < LSYSTEM_MAX_CELLS; i++) {
            int sym = src[i];
            if (sym == 0) continue; // empty
            int ri = -1;
            for (int r = 0; r < LSYSTEM_NUM_RULES; r++) {
                if (ruleTarget[r] == sym) { ri = r; break; }
            }
            if (ri < 0) {
                dst[dlen++] = sym;
                continue;
            }
            for (int j = 0; j < LSYSTEM_RULE_BODY && dlen < LSYSTEM_MAX_CELLS; j++) {
                int c = ruleResult[ri][j];
                if (c != 0) dst[dlen++] = c;
            }
        }
    }

    // Recompute the output string from the current axiom + rules. Iterates the
    // rewrite rules until the string fills the 8x24 matrix (or a generous cap to
    // guarantee termination for pathological/stagnant systems), then truncates.
    void regenerate() {
        int bufA[LSYSTEM_MAX_CELLS];
        int bufB[LSYSTEM_MAX_CELLS];
        int* src = bufA;
        int slen = 0;
        for (int i = 0; i < LSYSTEM_AXIOM; i++) {
            if (axiom[i] != 0 && slen < LSYSTEM_MAX_CELLS)
                src[slen++] = axiom[i];
        }
        bool srcIsA = true;
        for (int iter = 0; iter < 64 && slen < LSYSTEM_MAX_CELLS; iter++) {
            int* dst = srcIsA ? bufB : bufA;
            int dlen = 0;
            expandStep(src, slen, dst, dlen);
            src = dst;
            slen = dlen;
            srcIsA = !srcIsA;
        }
        outputLen = slen;
        for (int i = 0; i < outputLen; i++)
            output[i] = src[i];
    }

    void onReset() override {
        stepIndex = 0;
        memset(hit, 0, sizeof(hit));
        memset(pulse, 0, sizeof(pulse));
        fireStep();
    }

    void process(const ProcessArgs& args) override {
        bool trigMode = params[OUT_PARAM].getValue() > 0.5f;

        // Clear: reset the axiom and all rules to off/black, emptying the L-system
        if (clearButtonTrigger.process(params[CLEAR_PARAM].getValue())) {
            memset(axiom, 0, sizeof(axiom));
            memset(ruleTarget, 0, sizeof(ruleTarget));
            memset(ruleResult, 0, sizeof(ruleResult));
            memset(hit, 0, sizeof(hit));
            memset(pulse, 0, sizeof(pulse));
            regenerate();
        }

        // Reset: return the playhead to the start of the sequence
        if (resetButtonTrigger.process(params[RESET_PARAM].getValue())
            || resetTrigTrigger.process(inputs[RESET_TRIG_INPUT].getVoltage())) {
            stepIndex = 0;
            fireStep();
        }

        // Position CV drives the playhead directly (0-10 V maps to an absolute
        // position across the sequence). While active it takes over from the
        // step buttons / triggers, mirroring ra-reflectingpool.
        float posVoltage = inputs[POSITION_INPUT].getVoltage();
        bool posActive = inputs[POSITION_INPUT].isConnected() && fabsf(posVoltage) >= 0.001f;

        if (!posActive) {
            // Step forward
            if (stepButtonTrigger.process(params[STEP_PARAM].getValue())
                || stepTrigTrigger.process(inputs[STEP_TRIG_INPUT].getVoltage())) {
                stepIndex++;
                fireStep();
            }
            // Step back
            if (stepBackButtonTrigger.process(params[STEP_BACK_PARAM].getValue())
                || stepBackTrigTrigger.process(inputs[STEP_BACK_TRIG_INPUT].getVoltage())) {
                if (outputLen > 0) {
                    int s = seqStart();
                    int c = curIdx();
                    stepIndex = (c == s) ? (unsigned long)(seqEnd() - s) : (unsigned long)(c - 1 - s);
                    fireStep();
                }
            }
        }
        else {
            if (outputLen > 0) {
                int s = seqStart();
                int e = seqEnd();
                int len = e - s + 1;
                float norm = clamp(posVoltage / 10.f, 0.f, 1.f);
                int newPos = (len > 1) ? s + (int)roundf(norm * (float)(len - 1)) : s;
                if ((int)curIdx() != newPos) {
                    stepIndex = (unsigned long)(newPos - s);
                    fireStep();
                }
            }
        }

        for (int k = 0; k < 7; k++) {
            float v = 0.f;
            if (trigMode) {
                if (pulse[k] > 0.f) {
                    v = 10.f;
                    pulse[k] -= args.sampleTime;
                }
            }
            else {
                v = hit[k] ? 10.f : 0.f;
            }
            outputs[OUT1_OUTPUT + k].setVoltage(v);
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_t* axiJ = json_array();
        for (int i = 0; i < LSYSTEM_AXIOM; i++)
            json_array_append_new(axiJ, json_integer(axiom[i]));
        json_object_set_new(rootJ, "axiom", axiJ);

        json_t* tgtJ = json_array();
        for (int i = 0; i < LSYSTEM_NUM_RULES; i++)
            json_array_append_new(tgtJ, json_integer(ruleTarget[i]));
        json_object_set_new(rootJ, "targets", tgtJ);

        json_t* resJ = json_array();
        for (int r = 0; r < LSYSTEM_NUM_RULES; r++) {
            json_t* rowJ = json_array();
            for (int j = 0; j < LSYSTEM_RULE_BODY; j++)
                json_array_append_new(rowJ, json_integer(ruleResult[r][j]));
            json_array_append_new(resJ, rowJ);
        }
        json_object_set_new(rootJ, "results", resJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* axiJ = json_object_get(rootJ, "axiom");
        if (axiJ) {
            for (int i = 0; i < LSYSTEM_AXIOM; i++) {
                json_t* v = json_array_get(axiJ, i);
                if (v) axiom[i] = clamp(json_integer_value(v), 0, 7);
            }
        }
        json_t* tgtJ = json_object_get(rootJ, "targets");
        if (tgtJ) {
            for (int i = 0; i < LSYSTEM_NUM_RULES; i++) {
                json_t* v = json_array_get(tgtJ, i);
                if (v) ruleTarget[i] = clamp(json_integer_value(v), 0, 7);
            }
        }
        json_t* resJ = json_object_get(rootJ, "results");
        if (resJ) {
            for (int r = 0; r < LSYSTEM_NUM_RULES; r++) {
                json_t* rowJ = json_array_get(resJ, r);
                if (!rowJ) continue;
                for (int j = 0; j < LSYSTEM_RULE_BODY; j++) {
                    json_t* v = json_array_get(rowJ, j);
                    if (v) ruleResult[r][j] = clamp(json_integer_value(v), 0, 7);
                }
            }
        }

        // Rebuild the output string from the loaded axiom/rules and re-arm
        // the current position's outputs
        regenerate();
        fireStep();
    }
};

// Editable L-system cell: cycles through the 8 colors when clicked.
struct ColorCell : OpaqueWidget {
    RaLsysModule* module;
    int* value;

    void draw(const DrawArgs& args) override {
        drawCell(args.vg, box.zeroPos(), value ? *value : 0, false, false);
    }

    void onDragStart(const event::DragStart& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && value) {
            *value = (*value + 1) % 8;
            if (module) {
                module->regenerate();
                module->fireStep();
            }
        }
        OpaqueWidget::onDragStart(e);
    }
};

// Non-editable output matrix cell: shows a generated symbol and highlights the
// current sequencer position.
struct MatrixCell : OpaqueWidget {
    RaLsysModule* module;
    int index;

    void draw(const DrawArgs& args) override {
        int color = (module && index < module->outputLen) ? module->output[index] : 0;
        bool current = module && module->outputLen > 0 && module->curIdx() == index;
        bool dim = module && index < module->outputLen && !module->positionEnabled(index);
        drawCell(args.vg, box.zeroPos(), color, current, dim);
    }
};

static void paintBackdrop(NVGcontext* vg, math::Rect box) {
    nvgBeginPath(vg);
    nvgRoundedRect(vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
    nvgFillColor(vg, LSYSTEM_BG);
    nvgFill(vg);
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, LSYSTEM_BORDER);
    nvgStroke(vg);
}

// Top: the axiom row — 8 editable color cells.
struct AxiomDisplay : LedDisplay {
    RaLsysModule* module;

    void setModule(RaLsysModule* m) {
        module = m;
        const float cell = 12.f;
        const float pitch = 16.f;
        const float startX = 8.f;
        const float cy = box.size.y / 2.f;
        for (int i = 0; i < 8; i++) {
            ColorCell* c = new ColorCell;
            c->module = module;
            c->value = (module) ? &module->axiom[i] : NULL;
            c->box.pos = Vec(startX + i * pitch - cell / 2.f, cy - cell / 2.f);
            c->box.size = Vec(cell, cell);
            addChild(c);
        }
    }

    void draw(const DrawArgs& args) override {
        paintBackdrop(args.vg, box);
        Widget::draw(args);
    }
};

// Below the axiom: the rule rows. Each row has 1 target cell (left) and
// LSYSTEM_RULE_BODY result cells.
struct RulesDisplay : LedDisplay {
    RaLsysModule* module;

    void setModule(RaLsysModule* m) {
        module = m;
        const float cell = 12.f;
        // Match the axiom row layout: same pitch/startX so the rule buttons line
        // up with the axiom buttons. Target sits on axiom column 0; the 6 result
        // cells sit on axiom columns 2..7 (column 1 is left for the arrow).
        const float pitch = 16.f;   // horizontal column pitch, matches the axiom
        const float startX = 8.f;
        const float rowPitch = 30.f; // vertical spacing of rule rows
        const float half = cell / 2.f;
        for (int r = 0; r < LSYSTEM_NUM_RULES; r++) {
            float y = 10.f + r * rowPitch;
            ColorCell* t = new ColorCell;
            t->module = module;
            t->value = (module) ? &module->ruleTarget[r] : NULL;
            t->box.pos = Vec(startX - half, y - half);
            t->box.size = Vec(cell, cell);
            addChild(t);

            for (int j = 0; j < LSYSTEM_RULE_BODY; j++) {
                float x = startX + (j + 2) * pitch;
                ColorCell* c = new ColorCell;
                c->module = module;
                c->value = (module) ? &module->ruleResult[r][j] : NULL;
                c->box.pos = Vec(x - half, y - half);
                c->box.size = Vec(cell, cell);
                addChild(c);
            }
        }
    }

    void draw(const DrawArgs& args) override {
        paintBackdrop(args.vg, box);
        // Arrow from each target cell (left) toward its result cells (right),
        // spanning the width of axiom column 1 so it matches the button width.
        const float cell = 12.f;
        const float startX = 8.f;
        const float colPitch = 16.f;
        const float rowPitch = 30.f;
        const float x1 = startX + cell / 2.f;          // target cell right edge
        const float x2 = startX + 2.f * colPitch - cell / 2.f; // first result left edge
        // Draw the arrow narrower than its cell, padded inward from both edges
        const float pad = 4.f;
        const float ax1 = x1 + pad;   // arrow tail (left)
        const float ax2 = x2 - pad;   // arrow tip (right)
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 90));
        for (int r = 0; r < LSYSTEM_NUM_RULES; r++) {
            float y = 10.f + r * rowPitch;
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, ax1, y);
            nvgLineTo(args.vg, ax2, y);
            nvgStroke(args.vg);
            // arrowhead
            float hx = ax2;
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, hx, y);
            nvgLineTo(args.vg, hx - 2.5f, y - 2.f);
            nvgMoveTo(args.vg, hx, y);
            nvgLineTo(args.vg, hx - 2.5f, y + 2.f);
            nvgStroke(args.vg);
        }
        Widget::draw(args);
    }
};

// Right: the non-editable 8x24 output matrix.
struct MatrixDisplay : LedDisplay {
    RaLsysModule* module;

    void setModule(RaLsysModule* m) {
        module = m;
        // Fill the whole display box: pitch the cells to spread across the
        // full width/height, so both 8 columns and all rows are centered.
        const float margin = 3.f;
        const float gap = 2.f;
        const int cols = LSYSTEM_MAX_COLS;
        const int rows = LSYSTEM_MAX_ROWS;
        float pitchX = (box.size.x - 2.f * margin) / cols;
        float pitchY = (box.size.y - 2.f * margin) / rows;
        float cell = std::min(pitchX, pitchY) - gap;
        float startX = (box.size.x - (cols - 1) * pitchX - cell) / 2.f;
        float startY = (box.size.y - (rows - 1) * pitchY - cell) / 2.f;
        for (int i = 0; i < LSYSTEM_MAX_CELLS; i++) {
            MatrixCell* c = new MatrixCell;
            c->module = module;
            c->index = i;
            int row = i / cols;
            int col = i % cols;
            c->box.pos = Vec(startX + col * pitchX, startY + row * pitchY);
            c->box.size = Vec(cell, cell);
            addChild(c);
        }
    }

    void draw(const DrawArgs& args) override {
        paintBackdrop(args.vg, box);
        Widget::draw(args);
    }
};

struct RaLsysWidget : ModuleWidget {
    RaLsysWidget(RaLsysModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-lsystem.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Left: axiom row on top, 6 rule rows below
        AxiomDisplay* axiom = new AxiomDisplay;
        axiom->box.pos = Vec(4, 20);
        axiom->box.size = Vec(160, 22);
        axiom->setModule(module);
        addChild(axiom);

        RulesDisplay* rules = new RulesDisplay;
        rules->box.pos = Vec(4, 57);
        rules->box.size = Vec(160, 250);
        rules->setModule(module);
        addChild(rules);

        // Above the matrix: start/end knobs (with their CV inputs, in the
        // mothership style of knob-then-port pairs) select the enabled window.
        // The start knob's left edge is aligned with the matrix's left edge.
        addParam(createParamCentered<RaKnobSmall>(Vec(191.34, 31), module, RaLsysModule::START_PARAM));
        addInput(createInputCentered<RaPort>(Vec(226.34, 31), module, RaLsysModule::START_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(261.34, 31), module, RaLsysModule::END_PARAM));
        addInput(createInputCentered<RaPort>(Vec(296.34, 31), module, RaLsysModule::END_CV_INPUT));

        // Right: the non-editable 8x24 output matrix, same height as the rules screen
        MatrixDisplay* matrix = new MatrixDisplay;
        matrix->box.pos = Vec(180, 57);
        matrix->box.size = Vec(96, 250);
        matrix->setModule(module);
        addChild(matrix);

        // Right of the matrix: one output per active color (7 total). The top
        // (outer) edge of red is aligned with the top edge of the output screen;
        // the rest follow at a constant 39u pitch so the column fits within the
        // screen height and clears the bottom row of controls.
        addOutput(createOutputCentered<RaPort>(Vec(305, 68.85), module, RaLsysModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(305, 107.85), module, RaLsysModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(305, 146.85), module, RaLsysModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(305, 185.85), module, RaLsysModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(305, 224.85), module, RaLsysModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(305, 263.85), module, RaLsysModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(305, 302.85), module, RaLsysModule::OUT7_OUTPUT));

        // Bottom row: step back (cv+btn), step forward (cv+btn), position CV,
        // Bottom row: step back, step forward, position CV, clear, reset, and
        // the output mode switch, evenly spaced along the x axis. CV inputs
        // precede their buttons.
        addInput(createInputCentered<RaPort>(Vec(33, 346), module, RaLsysModule::STEP_BACK_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(66, 346), module, RaLsysModule::STEP_BACK_PARAM));
        addInput(createInputCentered<RaPort>(Vec(99, 346), module, RaLsysModule::STEP_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(132, 346), module, RaLsysModule::STEP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(165, 346), module, RaLsysModule::POSITION_INPUT));
        addParam(createParamCentered<RaButton>(Vec(198, 346), module, RaLsysModule::CLEAR_PARAM));
        addInput(createInputCentered<RaPort>(Vec(231, 346), module, RaLsysModule::RESET_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(264, 346), module, RaLsysModule::RESET_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(297, 346), module, RaLsysModule::OUT_PARAM));
    }
};

Model* modelRaLsys = createModel<RaLsysModule, RaLsysWidget>("ra-lsystem");