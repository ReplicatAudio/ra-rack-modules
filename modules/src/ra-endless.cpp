#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaEndlessModule : Module {
    static constexpr int NUM_TRACKS = 2;
    std::vector<std::vector<float>> sequences[NUM_TRACKS];
    int currentSeq[NUM_TRACKS] = {0, 0};
    int currentPos[NUM_TRACKS] = {0, 0};
    int selectedTrack = 0;
    bool screenMode = false; // false = Simple, true = Advanced

    dsp::SchmittTrigger trackSelTrigger;
    dsp::SchmittTrigger screenModeTrigger;
    dsp::SchmittTrigger writeExtTrigger;
    dsp::SchmittTrigger restExtTrigger;
    dsp::SchmittTrigger clearExtTrigger;
    dsp::SchmittTrigger resetExtTrigger;
    dsp::SchmittTrigger stepNextExtTrigger;
    dsp::SchmittTrigger stepPrevExtTrigger;
    dsp::SchmittTrigger seqNextExtTrigger;
    dsp::SchmittTrigger seqPrevExtTrigger;
    dsp::SchmittTrigger seqResetExtTrigger;
    dsp::SchmittTrigger writeBtnTrigger;
    dsp::SchmittTrigger restBtnTrigger;
    dsp::SchmittTrigger clearBtnTrigger;
    dsp::SchmittTrigger resetBtnTrigger;
    dsp::SchmittTrigger stepNextBtnTrigger;
    dsp::SchmittTrigger stepPrevBtnTrigger;
    dsp::SchmittTrigger seqNextBtnTrigger;
    dsp::SchmittTrigger seqPrevBtnTrigger;
    dsp::SchmittTrigger seqResetBtnTrigger;
    dsp::SchmittTrigger runBtnTrigger;
    bool running = false;
    float lastCvValue[NUM_TRACKS] = {0.f, 0.f};
    dsp::PulseGenerator trigPulse[NUM_TRACKS];
    dsp::PulseGenerator endPulse[NUM_TRACKS];
    float seqCountKnobValue = 0.f;
    float slewOutput[NUM_TRACKS] = {0.f, 0.f};
    bool endReached[NUM_TRACKS] = {false, false};
    int repeatCount[NUM_TRACKS] = {0, 0};

    struct SlewParamQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            float v = getValue();
            float slewMs = 10000.f * v * v * v * v;
            if (slewMs < 0.01f) return "0 ms";
            if (slewMs < 1.f) return string::f("%.2f ms", slewMs);
            if (slewMs < 100.f) return string::f("%.1f ms", slewMs);
            return string::f("%.0f ms", slewMs);
        }
    };

    static constexpr float REST_VALUE = -20.f;

    enum ParamIds {
        TRACK_SELECT_PARAM,
        WRITE_PARAM,
        REST_PARAM,
        CLEAR_PARAM,
        RESET_PARAM,
        STEP_NEXT_PARAM,
        STEP_PREV_PARAM,
        SEQ_NEXT_PARAM,
        SEQ_PREV_PARAM,
        SEQ_RESET_PARAM,
        SEQ_LENGTH_PARAM,
        REPEATS_PARAM,
        RUN_PARAM,
        PASSTHROUGH_PARAM,
        SONG_MODE_PARAM,
        SLEW_A_PARAM,
        SLEW_B_PARAM,
        SCREEN_MODE_PARAM,
        BMODE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CV_INPUT,
        POSITION_INPUT,
        WRITE_TRIG_INPUT,
        REST_TRIG_INPUT,
        CLEAR_TRIG_INPUT,
        RESET_TRIG_INPUT,
        STEP_NEXT_TRIG_INPUT,
        STEP_PREV_TRIG_INPUT,
        SEQ_NEXT_TRIG_INPUT,
        SEQ_PREV_TRIG_INPUT,
        SEQ_RESET_TRIG_INPUT,
        RUN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        TRACK_A_CV_OUTPUT,
        TRACK_A_TRIG_OUTPUT,
        TRACK_A_END_OUTPUT,
        TRACK_B_CV_OUTPUT,
        TRACK_B_TRIG_OUTPUT,
        TRACK_B_END_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RUN_LIGHT,
        STEP_NEXT_LIGHT_R,
        STEP_NEXT_LIGHT_G,
        STEP_NEXT_LIGHT_B,
        STEP_PREV_LIGHT_R,
        STEP_PREV_LIGHT_G,
        STEP_PREV_LIGHT_B,
        SEQ_NEXT_LIGHT_R,
        SEQ_NEXT_LIGHT_G,
        SEQ_NEXT_LIGHT_B,
        SEQ_PREV_LIGHT_R,
        SEQ_PREV_LIGHT_G,
        SEQ_PREV_LIGHT_B,
        SEQ_RESET_LIGHT_R,
        SEQ_RESET_LIGHT_G,
        SEQ_RESET_LIGHT_B,
        NUM_LIGHTS
    };

    std::vector<float>& getSeq(int t) {
        return sequences[t][currentSeq[t]];
    }

    void ensureSeqExists(int t) {
        if (sequences[t].empty())
            sequences[t].push_back({});
        while (currentSeq[t] >= (int)sequences[t].size())
            sequences[t].push_back({});
    }

    void writeStep(int t) {
        float v = inputs[CV_INPUT].getVoltage();
        lastCvValue[t] = v;
        auto& seq = getSeq(t);
        if (currentPos[t] >= (int)seq.size())
            seq.push_back(v);
        else
            seq[currentPos[t]] = v;
        currentPos[t]++;
        trigPulse[t].trigger(1e-3f);
    }

    void insertRest(int t) {
        auto& seq = getSeq(t);
        if (currentPos[t] >= (int)seq.size())
            seq.push_back(REST_VALUE);
        else
            seq[currentPos[t]] = REST_VALUE;
        currentPos[t]++;
    }

    void stepNext(int t) {
        auto& seq = getSeq(t);
        if (seq.empty()) return;
        currentPos[t]++;
        trigPulse[t].trigger(1e-3f);
        if (currentPos[t] >= (int)seq.size()) {
            currentPos[t] = 0;
            endPulse[t].trigger(1e-3f);
            endReached[t] = true;
        }
    }

    void stepPrev(int t) {
        auto& seq = getSeq(t);
        if (seq.empty()) return;
        currentPos[t]--;
        if (currentPos[t] < 0)
            currentPos[t] = (int)seq.size() - 1;
        trigPulse[t].trigger(1e-3f);
    }

    void seqNextTrack(int t) {
        currentSeq[t]++;
        if (currentSeq[t] >= (int)sequences[t].size())
            currentSeq[t] = 0;
        currentPos[t] = 0;
        ensureSeqExists(t);
        trigPulse[t].trigger(1e-3f);
        repeatCount[0] = 0;
        repeatCount[1] = 0;
    }

    void seqNext() {
        for (int t = 0; t < NUM_TRACKS; t++)
            seqNextTrack(t);
    }

    void seqPrevTrack(int t) {
        currentSeq[t]--;
        if (currentSeq[t] < 0)
            currentSeq[t] = (int)sequences[t].size() - 1;
        currentPos[t] = 0;
        trigPulse[t].trigger(1e-3f);
        repeatCount[0] = 0;
        repeatCount[1] = 0;
    }

    void seqPrev() {
        for (int t = 0; t < NUM_TRACKS; t++)
            seqPrevTrack(t);
    }

    void seqReset() {
        for (int t = 0; t < NUM_TRACKS; t++) {
            currentSeq[t] = 0;
            currentPos[t] = 0;
            trigPulse[t].trigger(1e-3f);
        }
        repeatCount[0] = 0;
        repeatCount[1] = 0;
    }

    void setNumSequences(int n) {
        n = clamp(n, 1, 16);
        for (int t = 0; t < NUM_TRACKS; t++) {
            while ((int)sequences[t].size() < n)
                sequences[t].push_back({});
            while ((int)sequences[t].size() > n)
                sequences[t].pop_back();
            if (currentSeq[t] >= (int)sequences[t].size())
                currentSeq[t] = (int)sequences[t].size() - 1;
        }
    }

    RaEndlessModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TRACK_SELECT_PARAM, 0.f, 1.f, 0.f, "Track select");
        configParam(WRITE_PARAM, 0.f, 1.f, 0.f, "Write");
        configParam(REST_PARAM, 0.f, 1.f, 0.f, "Rest");
        configParam(CLEAR_PARAM, 0.f, 1.f, 0.f, "Clear");
        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset position");
        configParam(STEP_NEXT_PARAM, 0.f, 1.f, 0.f, "Step next");
        configParam(STEP_PREV_PARAM, 0.f, 1.f, 0.f, "Step prev");
        configParam(SEQ_NEXT_PARAM, 0.f, 1.f, 0.f, "Sequence next");
        configParam(SEQ_PREV_PARAM, 0.f, 1.f, 0.f, "Sequence prev");
        configParam(SEQ_RESET_PARAM, 0.f, 1.f, 0.f, "Sequence reset");
        configParam(SEQ_LENGTH_PARAM, 0.f, 15.f, 0.f, "Sequences", "", 0.f, 1.f, 1.f);
        paramQuantities[SEQ_LENGTH_PARAM]->snapEnabled = true;
        configParam(REPEATS_PARAM, 0.f, 15.f, 0.f, "Repeats", "", 0.f, 1.f, 1.f);
        paramQuantities[REPEATS_PARAM]->snapEnabled = true;
        configInput(CV_INPUT, "Pitch CV (1V/oct)");
        configInput(POSITION_INPUT, "Position CV (0-10V)");
        configInput(WRITE_TRIG_INPUT, "Write trigger");
        configInput(REST_TRIG_INPUT, "Rest trigger");
        configInput(CLEAR_TRIG_INPUT, "Clear trigger");
        configInput(RESET_TRIG_INPUT, "Reset trigger");
        configInput(STEP_NEXT_TRIG_INPUT, "Step next trigger");
        configInput(STEP_PREV_TRIG_INPUT, "Step prev trigger");
        configInput(SEQ_NEXT_TRIG_INPUT, "Sequence next trigger");
        configInput(SEQ_PREV_TRIG_INPUT, "Sequence prev trigger");
        configInput(SEQ_RESET_TRIG_INPUT, "Sequence reset trigger");
        configParam(RUN_PARAM, 0.f, 1.f, 0.f, "Run");
        configSwitch(PASSTHROUGH_PARAM, 0.f, 1.f, 0.f, "Passthrough", {"Off", "On"});
        configSwitch(SONG_MODE_PARAM, 0.f, 2.f, 0.f, "Song mode", {"OFF", "A", "B"});
        configParam<SlewParamQuantity>(SLEW_A_PARAM, 0.f, 1.f, 0.f, "Track A slew");
        configParam<SlewParamQuantity>(SLEW_B_PARAM, 0.f, 1.f, 0.f, "Track B slew");
        configParam(SCREEN_MODE_PARAM, 0.f, 1.f, 0.f, "screen mode");
        configSwitch(BMODE_PARAM, 0.f, 1.f, 0.f, "bmode", {"Off", "On"});
        configInput(RUN_INPUT, "Run");
        configLight(RUN_LIGHT, "Run");
        for (int c = 0; c < 3; c++) {
            configLight(STEP_NEXT_LIGHT_R + c, "Step next light");
            configLight(STEP_PREV_LIGHT_R + c, "Step prev light");
            configLight(SEQ_NEXT_LIGHT_R + c, "Sequence next light");
            configLight(SEQ_PREV_LIGHT_R + c, "Sequence prev light");
            configLight(SEQ_RESET_LIGHT_R + c, "Sequence reset light");
        }
        configOutput(TRACK_A_CV_OUTPUT, "Track A pitch CV");
        configOutput(TRACK_A_TRIG_OUTPUT, "Track A step trigger");
        configOutput(TRACK_A_END_OUTPUT, "Track A sequence end");
        configOutput(TRACK_B_CV_OUTPUT, "Track B pitch CV");
        configOutput(TRACK_B_TRIG_OUTPUT, "Track B step trigger");
        configOutput(TRACK_B_END_OUTPUT, "Track B sequence end");

        setNumSequences(1);
        seqCountKnobValue = params[SEQ_LENGTH_PARAM].getValue();
    }

    void process(const ProcessArgs &args) override {
        if (trackSelTrigger.process(params[TRACK_SELECT_PARAM].getValue() * 10.f))
            selectedTrack = 1 - selectedTrack;
        if (screenModeTrigger.process(params[SCREEN_MODE_PARAM].getValue() * 10.f))
            screenMode = !screenMode;
        int t = selectedTrack;

        if (writeBtnTrigger.process(params[WRITE_PARAM].getValue() * 10.f) ||
            writeExtTrigger.process(inputs[WRITE_TRIG_INPUT].getVoltage()))
            writeStep(t);

        if (restBtnTrigger.process(params[REST_PARAM].getValue() * 10.f) ||
            restExtTrigger.process(inputs[REST_TRIG_INPUT].getVoltage()))
            insertRest(t);

        if (clearBtnTrigger.process(params[CLEAR_PARAM].getValue() * 10.f) ||
            clearExtTrigger.process(inputs[CLEAR_TRIG_INPUT].getVoltage())) {
            getSeq(t).clear();
            currentPos[t] = 0;
            running = false;
        }

        if (resetBtnTrigger.process(params[RESET_PARAM].getValue() * 10.f) ||
            resetExtTrigger.process(inputs[RESET_TRIG_INPUT].getVoltage())) {
            for (int i = 0; i < NUM_TRACKS; i++) {
                currentPos[i] = 0;
                endPulse[i].trigger(1e-3f);
            }
        }

        bool bmode = params[BMODE_PARAM].getValue() > 0.5f;

        if (seqNextBtnTrigger.process(params[SEQ_NEXT_PARAM].getValue() * 10.f))
            seqNext();
        if (seqPrevBtnTrigger.process(params[SEQ_PREV_PARAM].getValue() * 10.f))
            seqPrev();
        if (bmode) {
            if (seqNextExtTrigger.process(inputs[SEQ_NEXT_TRIG_INPUT].getVoltage()))
                seqNextTrack(0);
            if (seqPrevExtTrigger.process(inputs[SEQ_PREV_TRIG_INPUT].getVoltage()))
                seqNextTrack(1);
        } else {
            if (seqNextExtTrigger.process(inputs[SEQ_NEXT_TRIG_INPUT].getVoltage()))
                seqNext();
            if (seqPrevExtTrigger.process(inputs[SEQ_PREV_TRIG_INPUT].getVoltage()))
                seqPrev();
        }

        if (seqResetBtnTrigger.process(params[SEQ_RESET_PARAM].getValue() * 10.f) ||
            seqResetExtTrigger.process(inputs[SEQ_RESET_TRIG_INPUT].getVoltage()))
            seqReset();

        float sc = params[SEQ_LENGTH_PARAM].getValue();
        if (sc != seqCountKnobValue) {
            int target = (int)sc + 1;
            setNumSequences(target);
            seqCountKnobValue = sc;
        }

        if (runBtnTrigger.process(params[RUN_PARAM].getValue() * 10.f))
            running = !running;
        bool runActive = running || inputs[RUN_INPUT].getVoltage() > 1.f;
        lights[RUN_LIGHT].setBrightness(runActive ? 1.f : 0.f);

        float glow = !runActive ? 1.f : 0.f;

        lights[STEP_NEXT_LIGHT_R].setBrightness(glow);
        lights[STEP_PREV_LIGHT_R].setBrightness(glow);

        lights[SEQ_NEXT_LIGHT_R].setBrightness(glow);
        lights[SEQ_PREV_LIGHT_R].setBrightness(glow);
        lights[SEQ_RESET_LIGHT_R].setBrightness(glow);

        float posVoltage = inputs[POSITION_INPUT].getVoltage();
        bool posActive = inputs[POSITION_INPUT].isConnected() && fabsf(posVoltage) >= 0.001f && runActive;

        if (!posActive) {
            if (stepNextBtnTrigger.process(params[STEP_NEXT_PARAM].getValue() * 10.f)) {
                stepNext(0);
                stepNext(1);
            }
            if (stepPrevBtnTrigger.process(params[STEP_PREV_PARAM].getValue() * 10.f)) {
                stepPrev(0);
                stepPrev(1);
            }
            if (runActive) {
                if (bmode) {
                    if (stepNextExtTrigger.process(inputs[STEP_NEXT_TRIG_INPUT].getVoltage()))
                        stepNext(0);
                    if (stepPrevExtTrigger.process(inputs[STEP_PREV_TRIG_INPUT].getVoltage()))
                        stepNext(1);
                } else {
                    if (stepNextExtTrigger.process(inputs[STEP_NEXT_TRIG_INPUT].getVoltage())) {
                        stepNext(0);
                        stepNext(1);
                    }
                    if (stepPrevExtTrigger.process(inputs[STEP_PREV_TRIG_INPUT].getVoltage())) {
                        stepPrev(0);
                        stepPrev(1);
                    }
                }
            }
        }

        if (posActive) {
            float posNorm = clamp(posVoltage / 10.f, 0.f, 1.f);
            for (int i = 0; i < NUM_TRACKS; i++) {
                auto& seq = getSeq(i);
                if (!seq.empty()) {
                    int newPos = (int)roundf(posNorm * (seq.size() - 1));
                    if (newPos != currentPos[i]) {
                        currentPos[i] = newPos;
                        trigPulse[i].trigger(1e-3f);
                    }
                }
            }
        }

        // Song mode auto-advance
        int songMode = (int)roundf(params[SONG_MODE_PARAM].getValue());
        int repeats = (int)params[REPEATS_PARAM].getValue() + 1;
        bool didAdvance = false;
        for (int i = 0; i < NUM_TRACKS; i++) {
            if (!endReached[i]) continue;
            endReached[i] = false;
            if (didAdvance) continue;
            bool trigger = (songMode == 1 && i == 0) || (songMode == 2 && i == 1);
            if (!trigger) continue;
            repeatCount[i]++;
            if (repeatCount[i] >= repeats) {
                repeatCount[0] = 0;
                repeatCount[1] = 0;
                seqNext();
                didAdvance = true;
            }
        }

        bool passActive = params[PASSTHROUGH_PARAM].getValue() > 0.5f && !runActive;

        for (int i = 0; i < NUM_TRACKS; i++) {
            int cvOutId = (i == 0) ? TRACK_A_CV_OUTPUT : TRACK_B_CV_OUTPUT;
            int trigOutId = (i == 0) ? TRACK_A_TRIG_OUTPUT : TRACK_B_TRIG_OUTPUT;
            int endOutId = (i == 0) ? TRACK_A_END_OUTPUT : TRACK_B_END_OUTPUT;
            int slewParamId = (i == 0) ? SLEW_A_PARAM : SLEW_B_PARAM;

            float target;
            if (passActive && i == selectedTrack) {
                target = inputs[CV_INPUT].getVoltage();
                outputs[trigOutId].setVoltage(10.f);
                outputs[endOutId].setVoltage(0.f);
            } else {
                auto& seq = getSeq(i);
                int pos = currentPos[i];
                if (!seq.empty() && pos < (int)seq.size()) {
                    float v = seq[pos];
                    if (v == REST_VALUE) {
                        target = lastCvValue[i];
                        trigPulse[i].reset();
                    } else {
                        target = v;
                    }
                } else {
                    target = 0.f;
                }
                outputs[trigOutId].setVoltage(trigPulse[i].process(args.sampleTime) ? 10.f : 0.f);
                outputs[endOutId].setVoltage(endPulse[i].process(args.sampleTime) ? 10.f : 0.f);
            }

            float knobVal = params[slewParamId].getValue();
            float slewMs = 10000.f * knobVal * knobVal * knobVal * knobVal;
            float factor = (slewMs < 0.1f) ? 1.f : 1.f - expf(-args.sampleTime / (slewMs * 0.001f));
            slewOutput[i] += (target - slewOutput[i]) * factor;
            outputs[cvOutId].setVoltage(slewOutput[i]);
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "running", json_boolean(running));
        json_object_set_new(rootJ, "selectedTrack", json_integer(selectedTrack));

        json_t *tracksJ = json_array();
        for (int t = 0; t < NUM_TRACKS; t++) {
            json_t *trackJ = json_object();
            json_object_set_new(trackJ, "currentPos", json_integer(currentPos[t]));
            json_object_set_new(trackJ, "currentSeq", json_integer(currentSeq[t]));
            json_t *seqsJ = json_array();
            for (auto& seq : sequences[t]) {
                json_t *stepsJ = json_array();
                for (float v : seq)
                    json_array_append_new(stepsJ, json_real(v));
                json_array_append_new(seqsJ, stepsJ);
            }
            json_object_set_new(trackJ, "sequences", seqsJ);
            json_array_append_new(tracksJ, trackJ);
        }
        json_object_set_new(rootJ, "tracks", tracksJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *runJ = json_object_get(rootJ, "running");
        if (runJ) running = json_boolean_value(runJ);

        json_t *selJ = json_object_get(rootJ, "selectedTrack");
        if (selJ) selectedTrack = json_integer_value(selJ);

        json_t *tracksJ = json_object_get(rootJ, "tracks");
        if (tracksJ) {
            for (int t = 0; t < NUM_TRACKS; t++) {
                json_t *trackJ = json_array_get(tracksJ, t);
                if (!trackJ) continue;
                json_t *posJ = json_object_get(trackJ, "currentPos");
                if (posJ) currentPos[t] = json_integer_value(posJ);
                json_t *seqJ = json_object_get(trackJ, "currentSeq");
                if (seqJ) currentSeq[t] = json_integer_value(seqJ);

                sequences[t].clear();

                json_t *seqsJ = json_object_get(trackJ, "sequences");
                if (seqsJ) {
                    size_t si;
                    json_t *stepsJ;
                    json_array_foreach(seqsJ, si, stepsJ) {
                        std::vector<float> seq;
                        size_t i;
                        json_t *vJ;
                        json_array_foreach(stepsJ, i, vJ)
                            seq.push_back((float)json_real_value(vJ));
                        sequences[t].push_back(seq);
                    }
                } else {
                    json_t *stepsJ = json_object_get(trackJ, "steps");
                    if (stepsJ) {
                        std::vector<float> seq;
                        size_t i;
                        json_t *vJ;
                        json_array_foreach(stepsJ, i, vJ)
                            seq.push_back((float)json_real_value(vJ));
                        sequences[t].push_back(seq);
                    }
                }

                if (sequences[t].empty())
                    sequences[t].push_back({});
                if (currentSeq[t] < 0 || currentSeq[t] >= (int)sequences[t].size())
                    currentSeq[t] = 0;
                if (currentPos[t] < 0)
                    currentPos[t] = 0;
            }
        }
        seqCountKnobValue = params[SEQ_LENGTH_PARAM].getValue();
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

        if (!module->screenMode) {
            // Simple mode: A <step> | <sequence>  B <step> | <sequence>
            int t = module->selectedTrack;
            auto simpleInfo = [&](int i) -> std::string {
                auto& seq = module->sequences[i][module->currentSeq[i]];
                int sc = (int)seq.size();
                int pos = module->currentPos[i];
                int dp = (sc > 0) ? clamp(pos, 0, sc - 1) + 1 : 0;
                int sq = module->currentSeq[i] + 1;
                char label = (i == 0) ? 'A' : 'B';
                return rack::string::f("%c %d | %d", label, dp, sq);
            };
            if (font) {
                nvgFontFaceId(args.vg, font->handle);
                nvgFontSize(args.vg, 26);
                nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgSave(args.vg);
                nvgTranslate(args.vg, box.size.x * 0.25, box.size.y * 0.5);
                nvgScale(args.vg, 1.f, 1.5f);
                nvgFillColor(args.vg, t == 0 ? nvgRGB(0x99, 0x6d, 0xd2) : nvgRGB(0x55, 0x3d, 0x74));
                nvgText(args.vg, 0, 0, simpleInfo(0).c_str(), nullptr);
                nvgRestore(args.vg);
                nvgSave(args.vg);
                nvgTranslate(args.vg, box.size.x * 0.75, box.size.y * 0.5);
                nvgScale(args.vg, 1.f, 1.5f);
                nvgFillColor(args.vg, t == 1 ? nvgRGB(0x99, 0x6d, 0xd2) : nvgRGB(0x55, 0x3d, 0x74));
                nvgText(args.vg, 0, 0, simpleInfo(1).c_str(), nullptr);
                nvgRestore(args.vg);
            }
            return;
        }

        float inV = module->inputs[RaEndlessModule::CV_INPUT].getVoltage();
        int t = module->selectedTrack;

        auto trackInfo = [&](int i) -> std::string {
            auto& seq = module->sequences[i][module->currentSeq[i]];
            int sc = (int)seq.size();
            int pos = module->currentPos[i];
            int dp = (sc > 0) ? clamp(pos, 0, sc - 1) + 1 : 0;
            int sq = module->currentSeq[i] + 1;
            int totalSeqs = (int)module->sequences[i].size();
            char label = (i == 0) ? 'A' : 'B';
            int cvId = (i == 0) ? RaEndlessModule::TRACK_A_CV_OUTPUT : RaEndlessModule::TRACK_B_CV_OUTPUT;
            float outV = module->outputs[cvId].getVoltage();
            if (sc == 0) return rack::string::f("%c  0.00  0 [%d/%d]", label, sq, totalSeqs);
            return rack::string::f("%c %+.2f %d/%d [%d/%d]", label, outV, dp, sc, sq, totalSeqs);
        };

        if (font) {
            nvgFontFaceId(args.vg, font->handle);
            nvgFontSize(args.vg, 16);
            nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            std::string line1 = rack::string::f("IN  %+.2fV", inV);
            std::string line2 = trackInfo(0);
            std::string line3 = trackInfo(1);

            nvgText(args.vg, box.size.x / 2, box.size.y * 0.2, line1.c_str(), nullptr);
            nvgFillColor(args.vg, t == 0 ? nvgRGB(0x99, 0x6d, 0xd2) : nvgRGB(0x55, 0x3d, 0x74));
            nvgText(args.vg, box.size.x / 2, box.size.y * 0.5, line2.c_str(), nullptr);
            nvgFillColor(args.vg, t == 1 ? nvgRGB(0x99, 0x6d, 0xd2) : nvgRGB(0x55, 0x3d, 0x74));
            nvgText(args.vg, box.size.x / 2, box.size.y * 0.75, line3.c_str(), nullptr);
        }
    }
};

struct PurpleLight : GrayModuleLightWidget {
    PurpleLight() {
        addBaseColor(nvgRGB(0x99, 0x6d, 0xd2));
    }
};

struct RaEndlessWidget : ModuleWidget {
    RaEndlessWidget(RaEndlessModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-endless.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        auto *display = new EndlessDisplay();
        display->box.pos = Vec(12, 21);
        display->box.size = Vec(box.size.x - 24, 80);
        display->module = module;
        addChild(display);

        float xCol[] = {25, 63, 101, 139, 177, 215};
        float xOut[] = {24, 72, 120, 168};

        // Row 1 (y=128): CV In, Passthrough, Run Trig, Run Btn, Track Sel, Position CV
        addInput(createInputCentered<RaPort>(Vec(xCol[0], 128), module, RaEndlessModule::CV_INPUT));
        addParam(createParamCentered<RaSwitch2>(Vec(xCol[1], 128), module, RaEndlessModule::PASSTHROUGH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(xCol[2], 128), module, RaEndlessModule::RUN_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(xCol[3], 128), module, RaEndlessModule::RUN_PARAM, RaEndlessModule::RUN_LIGHT));
        addParam(createParamCentered<RaButton>(Vec(xCol[4], 128), module, RaEndlessModule::TRACK_SELECT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(xCol[5], 128), module, RaEndlessModule::POSITION_INPUT));

        // Row 2 (y=170): Write, Write Trig, Rest, Rest Trig, Reset, Reset Trig
        addParam(createParamCentered<RaButton>(Vec(xCol[0], 170), module, RaEndlessModule::WRITE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(xCol[1], 170), module, RaEndlessModule::WRITE_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(xCol[2], 170), module, RaEndlessModule::REST_PARAM));
        addInput(createInputCentered<RaPort>(Vec(xCol[3], 170), module, RaEndlessModule::REST_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(xCol[4], 170), module, RaEndlessModule::RESET_PARAM));
        addInput(createInputCentered<RaPort>(Vec(xCol[5], 170), module, RaEndlessModule::RESET_TRIG_INPUT));

        // Row 3 (y=212): Prev, Prev Trig, Next, Next Trig, Clear, Clear Trig
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(xCol[0], 212), module, RaEndlessModule::STEP_PREV_PARAM, RaEndlessModule::STEP_PREV_LIGHT_R));
        addInput(createInputCentered<RaPort>(Vec(xCol[1], 212), module, RaEndlessModule::STEP_PREV_TRIG_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(xCol[2], 212), module, RaEndlessModule::STEP_NEXT_PARAM, RaEndlessModule::STEP_NEXT_LIGHT_R));
        addInput(createInputCentered<RaPort>(Vec(xCol[3], 212), module, RaEndlessModule::STEP_NEXT_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(xCol[4], 212), module, RaEndlessModule::CLEAR_PARAM));
        addInput(createInputCentered<RaPort>(Vec(xCol[5], 212), module, RaEndlessModule::CLEAR_TRIG_INPUT));

        // Row 4 (y=254): Seq Prev, Seq Prev Trig, Seq Next, Seq Next Trig, Seq Reset, Seq Reset Trig
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(xCol[0], 254), module, RaEndlessModule::SEQ_PREV_PARAM, RaEndlessModule::SEQ_PREV_LIGHT_R));
        addInput(createInputCentered<RaPort>(Vec(xCol[1], 254), module, RaEndlessModule::SEQ_PREV_TRIG_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(xCol[2], 254), module, RaEndlessModule::SEQ_NEXT_PARAM, RaEndlessModule::SEQ_NEXT_LIGHT_R));
        addInput(createInputCentered<RaPort>(Vec(xCol[3], 254), module, RaEndlessModule::SEQ_NEXT_TRIG_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(xCol[4], 254), module, RaEndlessModule::SEQ_RESET_PARAM, RaEndlessModule::SEQ_RESET_LIGHT_R));
        addInput(createInputCentered<RaPort>(Vec(xCol[5], 254), module, RaEndlessModule::SEQ_RESET_TRIG_INPUT));

        // Screen Mode, Song Mode, Sequences, Repeats, Bmode
        addParam(createParamCentered<RaButton>(Vec(32, 295), module, RaEndlessModule::SCREEN_MODE_PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(72, 295), module, RaEndlessModule::SONG_MODE_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(120, 295), module, RaEndlessModule::SEQ_LENGTH_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(168, 295), module, RaEndlessModule::REPEATS_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(215, 295), module, RaEndlessModule::BMODE_PARAM));

        // Track A (y=338), Track B (y=365)
        addParam(createParamCentered<RaKnobTrim>(Vec(32, 338), module, RaEndlessModule::SLEW_A_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(xOut[1], 338), module, RaEndlessModule::TRACK_A_CV_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(xOut[2], 338), module, RaEndlessModule::TRACK_A_TRIG_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(xOut[3], 338), module, RaEndlessModule::TRACK_A_END_OUTPUT));
        addParam(createParamCentered<RaKnobTrim>(Vec(32, 365), module, RaEndlessModule::SLEW_B_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(xOut[1], 365), module, RaEndlessModule::TRACK_B_CV_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(xOut[2], 365), module, RaEndlessModule::TRACK_B_TRIG_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(xOut[3], 365), module, RaEndlessModule::TRACK_B_END_OUTPUT));
    }
};

Model *modelRaEndless = createModel<RaEndlessModule, RaEndlessWidget>("ra-endless");
