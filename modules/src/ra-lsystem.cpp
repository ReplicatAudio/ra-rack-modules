// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: STEP_PARAM "Step"
// fname: OUT_PARAM "Out"
// fname: STEP_TRIG_INPUT "Step"
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

static const int LSYSTEM_MAX_CELLS = 64; // 8x8 output matrix

// Shared cell renderer. `current` draws a bright halo for the live sequencer position.
static void drawCell(NVGcontext* vg, Rect r, int color, bool current) {
    nvgBeginPath(vg);
    nvgRoundedRect(vg, RECT_ARGS(r), 1.5f);
    nvgFillColor(vg, lsysColor(color));
    nvgFill(vg);

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
        OUT_PARAM,   // Output: Gate / Trig
        STEP_PARAM,  // Step button
        NUM_PARAMS
    };
    enum InputIds {
        STEP_TRIG_INPUT,
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
    int axiom[8] = {1, 2, 3, 0, 0, 0, 0, 0};
    int ruleTarget[6] = {};
    int ruleResult[6][6] = {};

    // Generated output string (row-major fill of the 8x8 matrix)
    int output[LSYSTEM_MAX_CELLS] = {};
    int outputLen = 0;

    unsigned long stepIndex = 0;
    float pulse[7] = {};

    dsp::SchmittTrigger stepButtonTrigger;
    dsp::SchmittTrigger stepTrigTrigger;

    RaLsysModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(OUT_PARAM, 0.f, 1.f, 0.f, "Output", {"Gate", "Trig"});
        configButton(STEP_PARAM, "Step");

        configInput(STEP_TRIG_INPUT, "Step");

        configOutput(OUT1_OUTPUT, "Red");
        configOutput(OUT2_OUTPUT, "Green");
        configOutput(OUT3_OUTPUT, "Blue");
        configOutput(OUT4_OUTPUT, "Yellow");
        configOutput(OUT5_OUTPUT, "Cyan");
        configOutput(OUT6_OUTPUT, "Magenta");
        configOutput(OUT7_OUTPUT, "White");
    }

    int curIdx() {
        return outputLen > 0 ? (int)(stepIndex % (unsigned long)outputLen) : 0;
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
            for (int r = 0; r < 6; r++) {
                if (ruleTarget[r] == sym) { ri = r; break; }
            }
            if (ri < 0) {
                dst[dlen++] = sym;
                continue;
            }
            for (int j = 0; j < 6 && dlen < LSYSTEM_MAX_CELLS; j++) {
                int c = ruleResult[ri][j];
                if (c != 0) dst[dlen++] = c;
            }
        }
    }

    // Recompute the output string from the current axiom + rules. Iterates the
    // rewrite rules until the string fills the 8x8 matrix (or a generous cap to
    // guarantee termination for pathological/stagnant systems), then truncates.
    void regenerate() {
        int bufA[LSYSTEM_MAX_CELLS];
        int bufB[LSYSTEM_MAX_CELLS];
        int* src = bufA;
        int slen = 0;
        for (int i = 0; i < 8; i++) {
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
        memset(pulse, 0, sizeof(pulse));
    }

    void process(const ProcessArgs& args) override {
        regenerate();

        bool trigMode = params[OUT_PARAM].getValue() > 0.5f;

        // Step forward on the button or the CV trigger
        if (stepButtonTrigger.process(params[STEP_PARAM].getValue())
            || stepTrigTrigger.process(inputs[STEP_TRIG_INPUT].getVoltage()))
            stepIndex++;

        // Fire the current symbol's color output
        bool hit[7] = {};
        if (outputLen > 0) {
            int sym = output[curIdx()];
            if (sym >= 1 && sym <= 7) {
                int o = sym - 1;
                hit[o] = true;
                if (trigMode) pulse[o] = 0.01f;
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
        for (int i = 0; i < 8; i++)
            json_array_append_new(axiJ, json_integer(axiom[i]));
        json_object_set_new(rootJ, "axiom", axiJ);

        json_t* tgtJ = json_array();
        for (int i = 0; i < 6; i++)
            json_array_append_new(tgtJ, json_integer(ruleTarget[i]));
        json_object_set_new(rootJ, "targets", tgtJ);

        json_t* resJ = json_array();
        for (int r = 0; r < 6; r++) {
            json_t* rowJ = json_array();
            for (int j = 0; j < 6; j++)
                json_array_append_new(rowJ, json_integer(ruleResult[r][j]));
            json_array_append_new(resJ, rowJ);
        }
        json_object_set_new(rootJ, "results", resJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* axiJ = json_object_get(rootJ, "axiom");
        if (axiJ) {
            for (int i = 0; i < 8; i++) {
                json_t* v = json_array_get(axiJ, i);
                if (v) axiom[i] = clamp(json_integer_value(v), 0, 7);
            }
        }
        json_t* tgtJ = json_object_get(rootJ, "targets");
        if (tgtJ) {
            for (int i = 0; i < 6; i++) {
                json_t* v = json_array_get(tgtJ, i);
                if (v) ruleTarget[i] = clamp(json_integer_value(v), 0, 7);
            }
        }
        json_t* resJ = json_object_get(rootJ, "results");
        if (resJ && json_array_size(resJ) == 6) {
            for (int r = 0; r < 6; r++) {
                json_t* rowJ = json_array_get(resJ, r);
                if (!rowJ) continue;
                for (int j = 0; j < 6; j++) {
                    json_t* v = json_array_get(rowJ, j);
                    if (v) ruleResult[r][j] = clamp(json_integer_value(v), 0, 7);
                }
            }
        }
    }
};

// Editable L-system cell: cycles through the 8 colors when clicked.
struct ColorCell : OpaqueWidget {
    RaLsysModule* module;
    int* value;

    void draw(const DrawArgs& args) override {
        drawCell(args.vg, box.zeroPos(), value ? *value : 0, false);
    }

    void onDragStart(const event::DragStart& e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && value) {
            *value = (*value + 1) % 8;
            if (module) module->regenerate();
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
        drawCell(args.vg, box.zeroPos(), color, current);
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

// Below the axiom: 6 rule rows. Each row has 1 target cell (left) and
// 6 result cells.
struct RulesDisplay : LedDisplay {
    RaLsysModule* module;

    void setModule(RaLsysModule* m) {
        module = m;
        const float cell = 12.f;
        const float targetX = 8.f;
        const float resX[6] = {30.f, 45.f, 60.f, 75.f, 90.f, 105.f};
        const float rowY[6] = {10.f, 50.f, 90.f, 130.f, 170.f, 210.f};
        const float half = cell / 2.f;
        for (int r = 0; r < 6; r++) {
            ColorCell* t = new ColorCell;
            t->module = module;
            t->value = (module) ? &module->ruleTarget[r] : NULL;
            t->box.pos = Vec(targetX - half, rowY[r] - half);
            t->box.size = Vec(cell, cell);
            addChild(t);

            for (int j = 0; j < 6; j++) {
                ColorCell* c = new ColorCell;
                c->module = module;
                c->value = (module) ? &module->ruleResult[r][j] : NULL;
                c->box.pos = Vec(resX[j] - half, rowY[r] - half);
                c->box.size = Vec(cell, cell);
                addChild(c);
            }
        }
    }

    void draw(const DrawArgs& args) override {
        paintBackdrop(args.vg, box);
        Widget::draw(args);
    }
};

// Right: the non-editable 8x8 output matrix.
struct MatrixDisplay : LedDisplay {
    RaLsysModule* module;

    void setModule(RaLsysModule* m) {
        module = m;
        const float cell = 9.f;
        const float pitch = 11.f;
        const float startX = 4.f;
        const float startY = 4.f;
        for (int i = 0; i < 64; i++) {
            MatrixCell* c = new MatrixCell;
            c->module = module;
            c->index = i;
            int row = i / 8;
            int col = i % 8;
            c->box.pos = Vec(startX + col * pitch, startY + row * pitch);
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
        rules->box.pos = Vec(4, 54);
        rules->box.size = Vec(160, 250);
        rules->setModule(module);
        addChild(rules);

        // Right: the non-editable 8x8 output matrix
        MatrixDisplay* matrix = new MatrixDisplay;
        matrix->box.pos = Vec(180, 50);
        matrix->box.size = Vec(96, 96);
        matrix->setModule(module);
        addChild(matrix);

        // Right of the matrix: one output per active color (7 total)
        addOutput(createOutputCentered<RaPort>(Vec(292, 50), module, RaLsysModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(292, 92), module, RaLsysModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(292, 134), module, RaLsysModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(292, 176), module, RaLsysModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(292, 218), module, RaLsysModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(292, 260), module, RaLsysModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(292, 302), module, RaLsysModule::OUT7_OUTPUT));

        // Bottom-left: step button, output mode switch, step CV input
        addParam(createParamCentered<RaButton>(Vec(20, 330), module, RaLsysModule::STEP_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(52, 330), module, RaLsysModule::OUT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(84, 330), module, RaLsysModule::STEP_TRIG_INPUT));
    }
};

Model* modelRaLsys = createModel<RaLsysModule, RaLsysWidget>("ra-lsystem");