#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaEndlessModule : Module {
    static constexpr int NUM_TRACKS = 2;
    std::vector<float> steps[NUM_TRACKS];
    int currentPos[NUM_TRACKS] = {0, 0};
    int selectedTrack = 0;

    dsp::SchmittTrigger trackSelTrigger;
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
    dsp::SchmittTrigger runBtnTrigger;
    bool running = false;
    float lastCvValue[NUM_TRACKS] = {0.f, 0.f};
    dsp::PulseGenerator trigPulse[NUM_TRACKS];
    dsp::PulseGenerator endPulse[NUM_TRACKS];

    static constexpr float REST_VALUE = -20.f;

    enum ParamIds {
        TRACK_SELECT_PARAM,
        WRITE_PARAM,
        REST_PARAM,
        CLEAR_PARAM,
        RESET_PARAM,
        STEP_FWD_PARAM,
        STEP_BACK_PARAM,
        RUN_PARAM,
        PASSTHROUGH_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CV_INPUT,
        POSITION_INPUT,
        WRITE_TRIG_INPUT,
        REST_TRIG_INPUT,
        CLEAR_TRIG_INPUT,
        RESET_TRIG_INPUT,
        STEP_FWD_TRIG_INPUT,
        STEP_BACK_TRIG_INPUT,
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
        STEP_FWD_LIGHT_R,
        STEP_FWD_LIGHT_G,
        STEP_FWD_LIGHT_B,
        STEP_BACK_LIGHT_R,
        STEP_BACK_LIGHT_G,
        STEP_BACK_LIGHT_B,
        NUM_LIGHTS
    };

    RaEndlessModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TRACK_SELECT_PARAM, 0.f, 1.f, 0.f, "Track select");
        configParam(WRITE_PARAM, 0.f, 1.f, 0.f, "Write");
        configParam(REST_PARAM, 0.f, 1.f, 0.f, "Rest");
        configParam(CLEAR_PARAM, 0.f, 1.f, 0.f, "Clear");
        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset position");
        configParam(STEP_FWD_PARAM, 0.f, 1.f, 0.f, "Step forward");
        configParam(STEP_BACK_PARAM, 0.f, 1.f, 0.f, "Step back");
        configInput(CV_INPUT, "Pitch CV (1V/oct)");
        configInput(POSITION_INPUT, "Position CV (0-10V)");
        configInput(WRITE_TRIG_INPUT, "Write trigger");
        configInput(REST_TRIG_INPUT, "Rest trigger");
        configInput(CLEAR_TRIG_INPUT, "Clear trigger");
        configInput(RESET_TRIG_INPUT, "Reset trigger");
        configInput(STEP_FWD_TRIG_INPUT, "Step forward trigger");
        configInput(STEP_BACK_TRIG_INPUT, "Step back trigger");
        configParam(RUN_PARAM, 0.f, 1.f, 0.f, "Run");
        configSwitch(PASSTHROUGH_PARAM, 0.f, 1.f, 0.f, "Passthrough", {"Off", "On"});
        configInput(RUN_INPUT, "Run");
        configLight(RUN_LIGHT, "Run");
        for (int c = 0; c < 3; c++) {
            configLight(STEP_FWD_LIGHT_R + c, "Step forward light");
            configLight(STEP_BACK_LIGHT_R + c, "Step back light");
        }
        configOutput(TRACK_A_CV_OUTPUT, "Track A pitch CV");
        configOutput(TRACK_A_TRIG_OUTPUT, "Track A step trigger");
        configOutput(TRACK_A_END_OUTPUT, "Track A sequence end");
        configOutput(TRACK_B_CV_OUTPUT, "Track B pitch CV");
        configOutput(TRACK_B_TRIG_OUTPUT, "Track B step trigger");
        configOutput(TRACK_B_END_OUTPUT, "Track B sequence end");
    }

    void writeStep(int t) {
        float v = inputs[CV_INPUT].getVoltage();
        lastCvValue[t] = v;
        if (currentPos[t] >= (int)steps[t].size())
            steps[t].push_back(v);
        else
            steps[t][currentPos[t]] = v;
        currentPos[t]++;
        trigPulse[t].trigger(1e-3f);
    }

    void insertRest(int t) {
        if (currentPos[t] >= (int)steps[t].size())
            steps[t].push_back(REST_VALUE);
        else
            steps[t][currentPos[t]] = REST_VALUE;
        currentPos[t]++;
    }

    void stepFwd(int t) {
        if (steps[t].empty()) return;
        currentPos[t]++;
        trigPulse[t].trigger(1e-3f);
        if (currentPos[t] >= (int)steps[t].size()) {
            currentPos[t] = 0;
            endPulse[t].trigger(1e-3f);
        }
    }

    void stepBack(int t) {
        if (steps[t].empty()) return;
        currentPos[t]--;
        if (currentPos[t] < 0)
            currentPos[t] = (int)steps[t].size() - 1;
        trigPulse[t].trigger(1e-3f);
    }

    void process(const ProcessArgs &args) override {
        if (trackSelTrigger.process(params[TRACK_SELECT_PARAM].getValue() * 10.f))
            selectedTrack = 1 - selectedTrack;
        int t = selectedTrack;

        if (writeBtnTrigger.process(params[WRITE_PARAM].getValue() * 10.f) ||
            writeExtTrigger.process(inputs[WRITE_TRIG_INPUT].getVoltage()))
            writeStep(t);

        if (restBtnTrigger.process(params[REST_PARAM].getValue() * 10.f) ||
            restExtTrigger.process(inputs[REST_TRIG_INPUT].getVoltage()))
            insertRest(t);

        if (clearBtnTrigger.process(params[CLEAR_PARAM].getValue() * 10.f) ||
            clearExtTrigger.process(inputs[CLEAR_TRIG_INPUT].getVoltage())) {
            steps[t].clear();
            currentPos[t] = 0;
            running = false;
        }

        // Reset: both tracks
        if (resetBtnTrigger.process(params[RESET_PARAM].getValue() * 10.f) ||
            resetExtTrigger.process(inputs[RESET_TRIG_INPUT].getVoltage())) {
            for (int i = 0; i < NUM_TRACKS; i++) {
                currentPos[i] = 0;
                endPulse[i].trigger(1e-3f);
            }
        }

        if (runBtnTrigger.process(params[RUN_PARAM].getValue() * 10.f))
            running = !running;
        bool runActive = running || inputs[RUN_INPUT].getVoltage() > 1.f;
        lights[RUN_LIGHT].setBrightness(runActive ? 1.f : 0.f);

        float glow = !runActive ? 1.f : 0.f;
        lights[STEP_FWD_LIGHT_R].setBrightness(glow);
        lights[STEP_FWD_LIGHT_G].setBrightness(glow);
        lights[STEP_FWD_LIGHT_B].setBrightness(0.f);
        lights[STEP_BACK_LIGHT_R].setBrightness(glow);
        lights[STEP_BACK_LIGHT_G].setBrightness(glow);
        lights[STEP_BACK_LIGHT_B].setBrightness(0.f);

        float posVoltage = inputs[POSITION_INPUT].getVoltage();
        bool posActive = inputs[POSITION_INPUT].isConnected() && fabsf(posVoltage) >= 0.001f && runActive;

        // Step fwd/back only when position CV is not active
        if (!posActive) {
            // Step fwd button: both tracks
            if (stepFwdBtnTrigger.process(params[STEP_FWD_PARAM].getValue() * 10.f)) {
                stepFwd(0);
                stepFwd(1);
            }

            // Step back: both tracks
            if (stepBackBtnTrigger.process(params[STEP_BACK_PARAM].getValue() * 10.f)) {
                stepBack(0);
                stepBack(1);
            }

            // Step fwd/back trigger inputs: both tracks (only when run is active)
            if (runActive) {
                if (stepFwdExtTrigger.process(inputs[STEP_FWD_TRIG_INPUT].getVoltage())) {
                    stepFwd(0);
                    stepFwd(1);
                }
                if (stepBackExtTrigger.process(inputs[STEP_BACK_TRIG_INPUT].getVoltage())) {
                    stepBack(0);
                    stepBack(1);
                }
            }
        }

        // Position CV overrides forward/back
        if (posActive) {
            float posNorm = clamp(posVoltage / 10.f, 0.f, 1.f);
            for (int i = 0; i < NUM_TRACKS; i++) {
                if (!steps[i].empty()) {
                    int newPos = (int)roundf(posNorm * (steps[i].size() - 1));
                    if (newPos != currentPos[i]) {
                        currentPos[i] = newPos;
                        trigPulse[i].trigger(1e-3f);
                    }
                }
            }
        }

        bool passActive = params[PASSTHROUGH_PARAM].getValue() > 0.5f && !runActive;

        for (int i = 0; i < NUM_TRACKS; i++) {
            int cvOutId = (i == 0) ? TRACK_A_CV_OUTPUT : TRACK_B_CV_OUTPUT;
            int trigOutId = (i == 0) ? TRACK_A_TRIG_OUTPUT : TRACK_B_TRIG_OUTPUT;
            int endOutId = (i == 0) ? TRACK_A_END_OUTPUT : TRACK_B_END_OUTPUT;

            if (passActive && i == selectedTrack) {
                outputs[cvOutId].setVoltage(inputs[CV_INPUT].getVoltage());
                outputs[trigOutId].setVoltage(10.f);
                outputs[endOutId].setVoltage(0.f);
            } else {
                int pos = currentPos[i];
                if (!steps[i].empty() && pos < (int)steps[i].size()) {
                    float v = steps[i][pos];
                    if (v == REST_VALUE) {
                        outputs[cvOutId].setVoltage(lastCvValue[i]);
                        trigPulse[i].reset();
                    } else {
                        outputs[cvOutId].setVoltage(v);
                    }
                } else {
                    outputs[cvOutId].setVoltage(0.f);
                }
                outputs[trigOutId].setVoltage(trigPulse[i].process(args.sampleTime) ? 10.f : 0.f);
                outputs[endOutId].setVoltage(endPulse[i].process(args.sampleTime) ? 10.f : 0.f);
            }
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
            json_t *stepsJ = json_array();
            for (float v : steps[t])
                json_array_append_new(stepsJ, json_real(v));
            json_object_set_new(trackJ, "steps", stepsJ);
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
                steps[t].clear();
                json_t *stepsJ = json_object_get(trackJ, "steps");
                if (stepsJ) {
                    size_t i;
                    json_t *vJ;
                    json_array_foreach(stepsJ, i, vJ)
                        steps[t].push_back((float)json_real_value(vJ));
                }
            }
        }
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
        int t = module->selectedTrack;

        auto trackInfo = [&](int i) -> std::string {
            int sc = (int)module->steps[i].size();
            int pos = module->currentPos[i];
            int dp = (sc > 0) ? clamp(pos, 0, sc - 1) + 1 : 0;
            char label = (i == 0) ? 'A' : 'B';
            int cvId = (i == 0) ? RaEndlessModule::TRACK_A_CV_OUTPUT : RaEndlessModule::TRACK_B_CV_OUTPUT;
            float outV = module->outputs[cvId].getVoltage();
            if (sc == 0) return rack::string::f("%c  0.00  0", label);
            return rack::string::f("%c %+.2f %d/%d", label, outV, dp, sc);
        };

        if (font) {
            nvgFontFaceId(args.vg, font->handle);
            nvgFontSize(args.vg, 16);
            nvgFillColor(args.vg, nvgRGB(0xaa, 0xcc, 0x88));
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            std::string line1 = rack::string::f("IN  %+.2fV", inV);
            std::string line2 = trackInfo(0);
            std::string line3 = trackInfo(1);

            nvgText(args.vg, box.size.x / 2, box.size.y * 0.2, line1.c_str(), nullptr);
            nvgFillColor(args.vg, t == 0 ? nvgRGB(0xaa, 0xcc, 0x88) : nvgRGB(0x55, 0x77, 0x44));
            nvgText(args.vg, box.size.x / 2, box.size.y * 0.5, line2.c_str(), nullptr);
            nvgFillColor(args.vg, t == 1 ? nvgRGB(0xaa, 0xcc, 0x88) : nvgRGB(0x55, 0x77, 0x44));
            nvgText(args.vg, box.size.x / 2, box.size.y * 0.8, line3.c_str(), nullptr);
        }
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

        // 4x taller display spanning nearly full width
        auto *display = new EndlessDisplay();
        display->box.pos = Vec(12, 21);
        display->box.size = Vec(126, 96);
        display->module = module;
        addChild(display);

        // CV, Passthrough, Run trigger, Run button, Track select at y=140
        addInput(createInputCentered<RaPort>(Vec(24, 140), module, RaEndlessModule::CV_INPUT));
        addParam(createParamCentered<RaSwitch2>(Vec(52, 140), module, RaEndlessModule::PASSTHROUGH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(80, 140), module, RaEndlessModule::RUN_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(106, 140), module, RaEndlessModule::RUN_PARAM, RaEndlessModule::RUN_LIGHT));
        addParam(createParamCentered<RaButton>(Vec(130, 140), module, RaEndlessModule::TRACK_SELECT_PARAM));

        // Function controls — 3 rows, each with Btn (x=23|91) + Trig (x=57|125)
        // Row 1: WRT / REST  at y=177
        addParam(createParamCentered<RaButton>(Vec(23, 177), module, RaEndlessModule::WRITE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 177), module, RaEndlessModule::WRITE_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(91, 177), module, RaEndlessModule::REST_PARAM));
        addInput(createInputCentered<RaPort>(Vec(125, 177), module, RaEndlessModule::REST_TRIG_INPUT));

        // Row 2: BACK / FWD  at y=217
        addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(23, 217), module, RaEndlessModule::STEP_BACK_PARAM, RaEndlessModule::STEP_BACK_LIGHT_R));
        addInput(createInputCentered<RaPort>(Vec(57, 217), module, RaEndlessModule::STEP_BACK_TRIG_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(91, 217), module, RaEndlessModule::STEP_FWD_PARAM, RaEndlessModule::STEP_FWD_LIGHT_R));
        addInput(createInputCentered<RaPort>(Vec(125, 217), module, RaEndlessModule::STEP_FWD_TRIG_INPUT));

        // Row 3: CLR / RST  at y=257
        addParam(createParamCentered<RaButton>(Vec(23, 257), module, RaEndlessModule::CLEAR_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 257), module, RaEndlessModule::CLEAR_TRIG_INPUT));
        addParam(createParamCentered<RaButton>(Vec(91, 257), module, RaEndlessModule::RESET_PARAM));
        addInput(createInputCentered<RaPort>(Vec(125, 257), module, RaEndlessModule::RESET_TRIG_INPUT));

        // Position input above Track A trigger output
        addInput(createInputCentered<RaPort>(Vec(75, 237), module, RaEndlessModule::POSITION_INPUT));

        // Outputs — Track A at y=305, Track B at y=345
        // Each row: CV | TRIG | END  spaced 45 units apart
        float outX[] = {30, 75, 120};
        addOutput(createOutputCentered<RaPort>(Vec(outX[0], 305), module, RaEndlessModule::TRACK_A_CV_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(outX[1], 305), module, RaEndlessModule::TRACK_A_TRIG_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(outX[2], 305), module, RaEndlessModule::TRACK_A_END_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(outX[0], 345), module, RaEndlessModule::TRACK_B_CV_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(outX[1], 345), module, RaEndlessModule::TRACK_B_TRIG_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(outX[2], 345), module, RaEndlessModule::TRACK_B_END_OUTPUT));
    }
};

Model *modelRaEndless = createModel<RaEndlessModule, RaEndlessWidget>("ra-endless");
