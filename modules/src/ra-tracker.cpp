#include "ra-components.hpp"
using namespace rack;
extern Plugin *pluginInstance;

struct RaTrackerModule : Module {
    static constexpr int NUM_CHANNELS = 4;

    struct Step {
        float ch[NUM_CHANNELS] = {};
    };
    std::vector<std::vector<Step>> sequences;
    int currentSeq = 0;
    int currentStep = 0;

    // Pickup state for buttons (release-to-fire)
    bool stepUpHeld = false;
    bool stepDownHeld = false;
    bool seqNextHeld = false;
    bool seqPrevHeld = false;
    bool writeHeld = false;
    bool clearHeld = false;

    dsp::SchmittTrigger stepUpExtTrigger;
    dsp::SchmittTrigger stepDownExtTrigger;
    dsp::SchmittTrigger seqNextExtTrigger;
    dsp::SchmittTrigger seqPrevExtTrigger;
    dsp::SchmittTrigger writeExtTrigger;
    dsp::SchmittTrigger clearExtTrigger;

    enum ParamIds {
        CH1_PARAM, CH2_PARAM, CH3_PARAM, CH4_PARAM,
        WRITE_PARAM, CLEAR_PARAM,
        STEP_UP_PARAM, STEP_DOWN_PARAM,
        SEQ_NEXT_PARAM, SEQ_PREV_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        STEP_UP_TRIG, STEP_DOWN_TRIG,
        SEQ_NEXT_TRIG, SEQ_PREV_TRIG,
        WRITE_TRIG, CLEAR_TRIG,
        NUM_INPUTS
    };
    enum OutputIds {
        CV1_OUT, CV2_OUT, CV3_OUT, CV4_OUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        WRITE_LIGHT, CLEAR_LIGHT,
        STEP_UP_LIGHT, STEP_DOWN_LIGHT,
        SEQ_NEXT_LIGHT, SEQ_PREV_LIGHT,
        NUM_LIGHTS
    };

    RaTrackerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_CHANNELS; i++)
            configParam(CH1_PARAM + i, 0.f, 10.f, 0.f, string::f("Channel %d", i + 1), " V", 0.f, 1.f, 0.f);
        configParam(WRITE_PARAM, 0.f, 1.f, 0.f, "Write");
        configParam(CLEAR_PARAM, 0.f, 1.f, 0.f, "Clear");
        configParam(STEP_UP_PARAM, 0.f, 1.f, 0.f, "Step up");
        configParam(STEP_DOWN_PARAM, 0.f, 1.f, 0.f, "Step down");
        configParam(SEQ_NEXT_PARAM, 0.f, 1.f, 0.f, "Sequence next");
        configParam(SEQ_PREV_PARAM, 0.f, 1.f, 0.f, "Sequence prev");
        for (int i = 0; i < NUM_INPUTS; i++)
            configInput(STEP_UP_TRIG + i, "");
        for (int i = 0; i < NUM_OUTPUTS; i++)
            configOutput(CV1_OUT + i, string::f("CV %d", i + 1));
        configLight(WRITE_LIGHT, "Write");
        configLight(CLEAR_LIGHT, "Clear");
        configLight(STEP_UP_LIGHT, "Step up");
        configLight(STEP_DOWN_LIGHT, "Step down");
        configLight(SEQ_NEXT_LIGHT, "Sequence next");
        configLight(SEQ_PREV_LIGHT, "Sequence prev");

        sequences.push_back({});
    }

    void writeStep() {
        auto& seq = sequences[currentSeq];
        Step s;
        for (int i = 0; i < NUM_CHANNELS; i++)
            s.ch[i] = params[CH1_PARAM + i].getValue();
        if (currentStep >= (int)seq.size())
            seq.push_back(s);
        else
            seq[currentStep] = s;
        currentStep++;
    }

    void clearAll() {
        for (auto& seq : sequences)
            seq.clear();
        currentStep = 0;
    }

    void stepUp() {
        auto& seq = sequences[currentSeq];
        if (seq.empty()) return;
        currentStep++;
        if (currentStep >= (int)seq.size())
            currentStep = 0;
    }

    void stepDown() {
        auto& seq = sequences[currentSeq];
        if (seq.empty()) return;
        currentStep--;
        if (currentStep < 0)
            currentStep = (int)seq.size() - 1;
    }

    void seqNext() {
        currentSeq++;
        if (currentSeq >= (int)sequences.size())
            currentSeq = 0;
        currentStep = 0;
    }

    void seqPrev() {
        currentSeq--;
        if (currentSeq < 0)
            currentSeq = (int)sequences.size() - 1;
        currentStep = 0;
    }

    void process(const ProcessArgs &args) override {
        // Step navigation (pickup: release-to-fire)
        bool su = params[STEP_UP_PARAM].getValue() > 0.5f;
        if (stepUpHeld && !su) stepUp();
        stepUpHeld = su;

        bool sd = params[STEP_DOWN_PARAM].getValue() > 0.5f;
        if (stepDownHeld && !sd) stepDown();
        stepDownHeld = sd;

        // External step triggers (normal rising edge)
        if (stepUpExtTrigger.process(inputs[STEP_UP_TRIG].getVoltage()))
            stepUp();
        if (stepDownExtTrigger.process(inputs[STEP_DOWN_TRIG].getVoltage()))
            stepDown();

        // Sequence navigation (pickup: release-to-fire)
        bool sn = params[SEQ_NEXT_PARAM].getValue() > 0.5f;
        if (seqNextHeld && !sn) seqNext();
        seqNextHeld = sn;

        bool sp = params[SEQ_PREV_PARAM].getValue() > 0.5f;
        if (seqPrevHeld && !sp) seqPrev();
        seqPrevHeld = sp;

        if (seqNextExtTrigger.process(inputs[SEQ_NEXT_TRIG].getVoltage()))
            seqNext();
        if (seqPrevExtTrigger.process(inputs[SEQ_PREV_TRIG].getVoltage()))
            seqPrev();

        // Write (pickup: release-to-fire)
        bool w = params[WRITE_PARAM].getValue() > 0.5f;
        if (writeHeld && !w) writeStep();
        writeHeld = w;

        if (writeExtTrigger.process(inputs[WRITE_TRIG].getVoltage()))
            writeStep();

        // Clear (pickup: release-to-fire)
        bool c = params[CLEAR_PARAM].getValue() > 0.5f;
        if (clearHeld && !c) clearAll();
        clearHeld = c;

        if (clearExtTrigger.process(inputs[CLEAR_TRIG].getVoltage()))
            clearAll();

        // Set outputs from current step
        auto& seq = sequences[currentSeq];
        for (int i = 0; i < NUM_CHANNELS; i++) {
            float v = 0.f;
            if (!seq.empty() && currentStep < (int)seq.size())
                v = seq[currentStep].ch[i];
            outputs[CV1_OUT + i].setVoltage(v);
        }

        // Lights
        lights[WRITE_LIGHT].setBrightness(w ? 0.5f : 0.f);
        lights[CLEAR_LIGHT].setBrightness(c ? 0.5f : 0.f);
        lights[STEP_UP_LIGHT].setBrightness(su ? 0.5f : 0.f);
        lights[STEP_DOWN_LIGHT].setBrightness(sd ? 0.5f : 0.f);
        lights[SEQ_NEXT_LIGHT].setBrightness(sn ? 0.5f : 0.f);
        lights[SEQ_PREV_LIGHT].setBrightness(sp ? 0.5f : 0.f);
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "currentSeq", json_integer(currentSeq));
        json_object_set_new(rootJ, "currentStep", json_integer(currentStep));
        json_t *seqsJ = json_array();
        for (auto& seq : sequences) {
            json_t *stepsJ = json_array();
            for (auto& step : seq) {
                json_t *valsJ = json_array();
                for (int i = 0; i < NUM_CHANNELS; i++)
                    json_array_append_new(valsJ, json_real(step.ch[i]));
                json_array_append_new(stepsJ, valsJ);
            }
            json_array_append_new(seqsJ, stepsJ);
        }
        json_object_set_new(rootJ, "sequences", seqsJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *seqJ = json_object_get(rootJ, "currentSeq");
        if (seqJ) currentSeq = json_integer_value(seqJ);
        json_t *stepJ = json_object_get(rootJ, "currentStep");
        if (stepJ) currentStep = json_integer_value(stepJ);

        sequences.clear();
        json_t *seqsJ = json_object_get(rootJ, "sequences");
        if (seqsJ) {
            size_t si;
            json_t *stepsJ;
            json_array_foreach(seqsJ, si, stepsJ) {
                std::vector<Step> seq;
                size_t ti;
                json_t *valsJ;
                json_array_foreach(stepsJ, ti, valsJ) {
                    Step s;
                    size_t ci;
                    json_t *vJ;
                    json_array_foreach(valsJ, ci, vJ) {
                        if (ci < NUM_CHANNELS)
                            s.ch[ci] = (float)json_real_value(vJ);
                    }
                    seq.push_back(s);
                }
                sequences.push_back(seq);
            }
        }
        if (sequences.empty())
            sequences.push_back({});
    }
};

static constexpr int DISPLAY_ROWS = 6;

struct TrackerDisplay : LedDisplay {
    RaTrackerModule *module;
    std::shared_ptr<Font> font;

    TrackerDisplay() {
        font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x08, 0x0c, 0x08));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1);
        nvgStrokeColor(args.vg, nvgRGB(0x44, 0x44, 0x44));
        nvgStroke(args.vg);

        if (!module) return;

        auto& seq = module->sequences[module->currentSeq];
        int len = (int)seq.size();
        int pos = module->currentStep;
        int numSeqs = (int)module->sequences.size();
        int seqNum = module->currentSeq + 1;

        if (!font) return;
        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, 16);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

        // Header
        nvgFillColor(args.vg, nvgRGB(0x77, 0x88, 0x77));
        std::string header = string::f("SEQ:%02d/%02d  STP:%02d  LEN:%02d", seqNum, numSeqs, pos, len);
        nvgText(args.vg, 6, 4, header.c_str(), nullptr);

        // Data rows
        int firstRow = 0;
        if (len > 0) {
            firstRow = pos - 2;
            firstRow = clamp(firstRow, 0, std::max(0, len - DISPLAY_ROWS));
        }

        float rowH = (box.size.y - 20) / (float)DISPLAY_ROWS;
        for (int r = 0; r < DISPLAY_ROWS; r++) {
            int idx = firstRow + r;
            float y = 20 + r * rowH;

            if (idx >= len) break;

            bool isCurrent = (idx == pos);
            auto& step = seq[idx];

            // Highlight current step
            if (isCurrent) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 2, y - 1, box.size.x - 4, rowH);
                nvgFillColor(args.vg, nvgRGBA(0x44, 0x66, 0x44, 30));
                nvgFill(args.vg);
            }

            nvgFillColor(args.vg, isCurrent ? nvgRGB(0xcc, 0xee, 0x88) : nvgRGB(0x88, 0xaa, 0x66));

            std::string line = string::f("%c%02d %+.2f %+.2f %+.2f %+.2f",
                isCurrent ? '>' : ' ', idx,
                step.ch[0], step.ch[1], step.ch[2], step.ch[3]);
            nvgText(args.vg, 6, y, line.c_str(), nullptr);
        }
    }
};

struct RaTrackerWidget : ModuleWidget {
    RaTrackerWidget(RaTrackerModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-tracker.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        auto *display = new TrackerDisplay();
        display->box.pos = Vec(8, 15);
        display->box.size = Vec(224, 105);
        display->module = module;
        addChild(display);

        float xWide[] = {36, 84, 132, 180};
        float xOut[] = {36, 84, 132, 180};

        // Value knobs
        addParam(createParamCentered<RaKnobSmall>(Vec(xWide[0], 145), module, RaTrackerModule::CH1_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(xWide[1], 145), module, RaTrackerModule::CH2_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(xWide[2], 145), module, RaTrackerModule::CH3_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(xWide[3], 145), module, RaTrackerModule::CH4_PARAM));

        // Navigation buttons
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(xWide[0], 188), module, RaTrackerModule::STEP_DOWN_PARAM, RaTrackerModule::STEP_DOWN_LIGHT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(xWide[1], 188), module, RaTrackerModule::STEP_UP_PARAM, RaTrackerModule::STEP_UP_LIGHT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(xWide[2], 188), module, RaTrackerModule::SEQ_PREV_PARAM, RaTrackerModule::SEQ_PREV_LIGHT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(xWide[3], 188), module, RaTrackerModule::SEQ_NEXT_PARAM, RaTrackerModule::SEQ_NEXT_LIGHT));

        // Trigger inputs
        addInput(createInputCentered<RaPort>(Vec(xWide[0], 224), module, RaTrackerModule::STEP_DOWN_TRIG));
        addInput(createInputCentered<RaPort>(Vec(xWide[1], 224), module, RaTrackerModule::STEP_UP_TRIG));
        addInput(createInputCentered<RaPort>(Vec(xWide[2], 224), module, RaTrackerModule::SEQ_PREV_TRIG));
        addInput(createInputCentered<RaPort>(Vec(xWide[3], 224), module, RaTrackerModule::SEQ_NEXT_TRIG));

        // Write and Clear buttons
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(84, 270), module, RaTrackerModule::WRITE_PARAM, RaTrackerModule::WRITE_LIGHT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(156, 270), module, RaTrackerModule::CLEAR_PARAM, RaTrackerModule::CLEAR_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(84, 306), module, RaTrackerModule::WRITE_TRIG));
        addInput(createInputCentered<RaPort>(Vec(156, 306), module, RaTrackerModule::CLEAR_TRIG));

        // CV outputs
        for (int i = 0; i < RaTrackerModule::NUM_CHANNELS; i++)
            addOutput(createOutputCentered<RaPort>(Vec(xOut[i], 355), module, RaTrackerModule::CV1_OUT + i));
    }
};

Model *modelRaTracker = createModel<RaTrackerModule, RaTrackerWidget>("ra-tracker");
