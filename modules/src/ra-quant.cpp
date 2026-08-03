#include "ra-components.hpp"
#include <algorithm>
#include <cstring>
#include <climits>

using namespace rack;

extern Plugin *pluginInstance;

struct RaQuantModule : Module {
    enum ParamIds {
        OFFSET_PARAM,
        QUANTIZE_OFFSET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        OFFSET_CV_INPUT,
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        IN5_INPUT,
        IN6_INPUT,
        IN7_INPUT,
        IN8_INPUT,
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

    // Enabled notes of the scale (pitch class 0 = C)
    bool enabledNotes[12];
    // 24 half-semitone ranges mapping to the closest enabled note
    int ranges[24];
    // Notes currently played, for the display
    bool playingNotes[12];

    RaQuantModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(OFFSET_PARAM, -1.f, 1.f, 0.f, "Pre-offset", " semitones", 0.f, 12.f);
        configSwitch(QUANTIZE_OFFSET_PARAM, 0.f, 1.f, 0.f, "Quantize offset", {"Off", "On"});
        configInput(OFFSET_CV_INPUT, "Offset CV");
        for (int i = 0; i < 8; i++) {
            configInput(IN1_INPUT + i, string::f("Pitch %d", i + 1));
            configOutput(OUT1_OUTPUT + i, string::f("Pitch %d", i + 1));
            configBypass(IN1_INPUT + i, OUT1_OUTPUT + i);
        }
        onReset();
    }

    void onReset() override {
        for (int i = 0; i < 12; i++) {
            enabledNotes[i] = true;
        }
        updateRanges();
    }

    void onRandomize() override {
        for (int i = 0; i < 12; i++) {
            enabledNotes[i] = (random::uniform() < 0.5f);
        }
        updateRanges();
    }

    void process(const ProcessArgs &args) override {
        bool playingNotes[12] = {};
        float offsetParam = clamp(params[OFFSET_PARAM].getValue() + inputs[OFFSET_CV_INPUT].getVoltage() / 10.f, -1.f, 1.f);
        if (params[QUANTIZE_OFFSET_PARAM].getValue() > 0.5f) {
            offsetParam = std::round(offsetParam * 12.f) / 12.f;
        }

        for (int i = 0; i < 8; i++) {
            int channels = std::max(inputs[IN1_INPUT + i].getChannels(), 1);
            outputs[OUT1_OUTPUT + i].setChannels(channels);
            for (int c = 0; c < channels; c++) {
                float pitch = inputs[IN1_INPUT + i].getVoltage(c);
                pitch += offsetParam;
                int range = std::floor(pitch * 24);
                int octave = eucDiv(range, 24);
                range -= octave * 24;
                int note = ranges[range] + octave * 12;
                playingNotes[eucMod(note, 12)] = true;
                outputs[OUT1_OUTPUT + i].setVoltage(float(note) / 12, c);
            }
        }

        std::memcpy(this->playingNotes, playingNotes, sizeof(playingNotes));
    }

    void updateRanges() {
        // Check if no notes are enabled
        bool anyEnabled = false;
        for (int note = 0; note < 12; note++) {
            if (enabledNotes[note]) {
                anyEnabled = true;
                break;
            }
        }

        // Find closest notes for each range
        for (int i = 0; i < 24; i++) {
            int closestNote = 0;
            int closestDist = INT_MAX;
            for (int note = -12; note <= 24; note++) {
                int dist = std::abs((i + 1) / 2 - note);
                // Ignore enabled state if no notes are enabled
                if (anyEnabled && !enabledNotes[eucMod(note, 12)]) {
                    continue;
                }
                if (dist < closestDist) {
                    closestNote = note;
                    closestDist = dist;
                } else {
                    // If dist increases, we won't find a better one. break.
                    break;
                }
            }
            ranges[i] = closestNote;
        }
    }

    void rotateNotes(int delta) {
        delta = eucMod(-delta, 12);
        std::rotate(&enabledNotes[0], &enabledNotes[delta], &enabledNotes[12]);
        updateRanges();
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_t *enabledNotesJ = json_array();
        for (int i = 0; i < 12; i++) {
            json_array_insert_new(enabledNotesJ, i, json_boolean(enabledNotes[i]));
        }
        json_object_set_new(rootJ, "enabledNotes", enabledNotesJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *enabledNotesJ = json_object_get(rootJ, "enabledNotes");
        if (enabledNotesJ) {
            for (int i = 0; i < 12; i++) {
                json_t *enabledNoteJ = json_array_get(enabledNotesJ, i);
                if (enabledNoteJ)
                    enabledNotes[i] = json_boolean_value(enabledNoteJ);
            }
        }
        updateRanges();
    }
};

struct QuantizerButton : OpaqueWidget {
    int note;
    RaQuantModule *module;

    void drawLayer(const DrawArgs &args, int layer) override {
        if (layer != 1) return;
        Rect r = box.zeroPos();
        const float margin = mm2px(1.0);
        Rect rMargin = r.grow(Vec(margin, margin));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, RECT_ARGS(rMargin));
        nvgFillColor(args.vg, nvgRGB(0x12, 0x12, 0x12));
        nvgFill(args.vg);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, RECT_ARGS(r));
        if (module ? module->playingNotes[note] : (note == 0)) {
            nvgFillColor(args.vg, componentlibrary::SCHEME_YELLOW);
        } else if (module ? module->enabledNotes[note] : true) {
            nvgFillColor(args.vg, nvgRGB(0x7f, 0x6b, 0x0a));
        } else {
            nvgFillColor(args.vg, nvgRGB(0x40, 0x40, 0x40));
        }
        nvgFill(args.vg);
    }

    void onDragStart(const event::DragStart &e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
            module->enabledNotes[note] ^= true;
            module->updateRanges();
        }
        OpaqueWidget::onDragStart(e);
    }

    void onDragEnter(const event::DragEnter &e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
            QuantizerButton *origin = dynamic_cast<QuantizerButton *>(e.origin);
            if (origin) {
                module->enabledNotes[note] = module->enabledNotes[origin->note];
                module->updateRanges();
            }
        }
        OpaqueWidget::onDragEnter(e);
    }
};

struct QuantizerDisplay : LedDisplay {
    void setModule(RaQuantModule *module) {
        // Original 4hp key layout at full size (same keyboard width as the VCV module)
        const float s = 1.0f;

        // Display-local key positions/sizes (mm, derived from the original panel art)
        std::vector<Vec> notePositions = {
            mm2px(Vec(2.242, 47.501) * s),  // C
            mm2px(Vec(2.242, 45.377) * s),  // C#
            mm2px(Vec(2.242, 39.004) * s),  // D
            mm2px(Vec(2.242, 36.880) * s),  // D#
            mm2px(Vec(2.242, 32.631) * s),  // E
            mm2px(Vec(2.242, 26.259) * s),  // F
            mm2px(Vec(2.242, 24.134) * s),  // F#
            mm2px(Vec(2.242, 17.762) * s),  // G
            mm2px(Vec(2.242, 15.638) * s),  // G#
            mm2px(Vec(2.242, 9.265) * s),   // A
            mm2px(Vec(2.242, 7.141) * s),   // A#
            mm2px(Vec(2.242, 2.892) * s),   // B
        };
        std::vector<Vec> noteSizes = {
            mm2px(Vec(10.734, 5.644) * s),  // C
            mm2px(Vec(8.231, 3.520) * s),   // C#
            mm2px(Vec(10.734, 7.769) * s),  // D
            mm2px(Vec(8.231, 3.520) * s),   // D#
            mm2px(Vec(10.734, 5.644) * s),  // E
            mm2px(Vec(10.734, 5.644) * s),  // F
            mm2px(Vec(8.231, 3.520) * s),   // F#
            mm2px(Vec(10.734, 7.769) * s),  // G
            mm2px(Vec(8.231, 3.520) * s),   // G#
            mm2px(Vec(10.734, 7.768) * s),  // A
            mm2px(Vec(8.231, 3.520) * s),   // A#
            mm2px(Vec(10.734, 5.644) * s),  // B
        };

        // White notes
        static const std::vector<int> whiteNotes = {0, 2, 4, 5, 7, 9, 11};
        for (int note : whiteNotes) {
            QuantizerButton *quantizerButton = new QuantizerButton();
            quantizerButton->box.pos = notePositions[note];
            quantizerButton->box.size = noteSizes[note];
            quantizerButton->module = module;
            quantizerButton->note = note;
            addChild(quantizerButton);
        }
        // Black notes
        static const std::vector<int> blackNotes = {1, 3, 6, 8, 10};
        for (int note : blackNotes) {
            QuantizerButton *quantizerButton = new QuantizerButton();
            quantizerButton->box.pos = notePositions[note];
            quantizerButton->box.size = noteSizes[note];
            quantizerButton->module = module;
            quantizerButton->note = note;
            addChild(quantizerButton);
        }
    }
};

struct RaQuantWidget : ModuleWidget {
    RaQuantWidget(RaQuantModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-quant.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Column 1: piano-key note display at full VCV keyboard size
        QuantizerDisplay *quantizerDisplay = createWidget<QuantizerDisplay>(Vec(0, 101.5));
        quantizerDisplay->box.size = mm2px(Vec(15.24, 55.88));
        quantizerDisplay->setModule(module);
        addChild(quantizerDisplay);

        addParam(createParamCentered<RaKnob>(Vec(22.5, 292), module, RaQuantModule::OFFSET_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(22.5, 320), module, RaQuantModule::QUANTIZE_OFFSET_PARAM));
        addInput(createInputCentered<RaPort>(Vec(22.5, 344), module, RaQuantModule::OFFSET_CV_INPUT));

        // Columns 2 and 3: 8 inputs, 8 outputs
        float rows[] = {100, 124, 148, 172, 196, 220, 244, 268};
        for (int i = 0; i < 8; i++) {
            addInput(createInputCentered<RaPort>(Vec(60, rows[i]), module, RaQuantModule::IN1_INPUT + i));
            addOutput(createOutputCentered<RaPort>(Vec(90, rows[i]), module, RaQuantModule::OUT1_OUTPUT + i));
        }
    }

    void appendContextMenu(Menu *menu) override {
        RaQuantModule *module = getModule<RaQuantModule>();
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Shift notes up", "", [=]() { module->rotateNotes(1); }));
        menu->addChild(createMenuItem("Shift notes down", "", [=]() { module->rotateNotes(-1); }));
    }
};

Model *modelRaQuant = createModel<RaQuantModule, RaQuantWidget>("ra-quant");
