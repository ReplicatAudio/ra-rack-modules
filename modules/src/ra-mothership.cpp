#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

// Parameter/input indices for quantity classes
static constexpr int RA_MOTHERSHIP_FREQ_CV_INPUT = 1;
static constexpr int RA_MOTHERSHIP_FM_INPUT = 3;
static constexpr int RA_MOTHERSHIP_FM_ATTN_PARAM = 2;

struct RaMothershipFreqQuantity : ParamQuantity {
    float getDisplayValue() override {
        float v = getValue();
        float freq;
        if (module && module->params[0].getValue() > 0.5f) {
            freq = 20.f * powf(1000.f, v);
        } else {
            freq = 0.01f * powf(1000.f, v);
        }
        if (module) {
            if (module->inputs[RA_MOTHERSHIP_FREQ_CV_INPUT].isConnected())
                freq *= powf(2.f, module->inputs[RA_MOTHERSHIP_FREQ_CV_INPUT].getVoltage() * v);
            if (module->inputs[RA_MOTHERSHIP_FM_INPUT].isConnected())
                freq *= powf(2.f, module->inputs[RA_MOTHERSHIP_FM_INPUT].getVoltage() * module->params[RA_MOTHERSHIP_FM_ATTN_PARAM].getValue());
        }
        return std::max(freq, 0.f);
    }
};

struct RaMothershipModule : Module {
    static constexpr int NUM_OSC = 8;
    static constexpr int GLOBAL_PARAMS = 5;
    static constexpr int PARAMS_PER_OSC = 6; // shape, phase, filter, fm, invert, detune
    static constexpr int GLOBAL_INPUTS = 4;
    static constexpr int INPUTS_PER_OSC = 5; // shape, phase, filter, fm, detune

    enum ParamIds {
        MODE_PARAM,
        FREQ_PARAM,
        FREQ_ATTN_PARAM,
        FM_ATTN_PARAM,
        PHASE_PARAM,
        NUM_PARAMS_BASE,
        NUM_PARAMS = NUM_PARAMS_BASE + NUM_OSC * PARAMS_PER_OSC
    };

    // Per-osc param offsets
    static constexpr int OSC_SHAPE = 0;
    static constexpr int OSC_PHASE = 1;
    static constexpr int OSC_FILTER = 2;
    static constexpr int OSC_FM = 3;
    static constexpr int OSC_INVERT = 4;
    static constexpr int OSC_DETUNE = 5;

    static int oscParam(int i, int offset) { return GLOBAL_PARAMS + i * PARAMS_PER_OSC + offset; }

    enum InputIds {
        FREQ_CV_INPUT,
        FREQ_ATTN_CV_INPUT,
        FM_INPUT,
        PHASE_CV_INPUT,
        NUM_INPUTS_BASE,
        NUM_INPUTS = NUM_INPUTS_BASE + NUM_OSC * INPUTS_PER_OSC
    };

    // Per-osc input offsets
    static constexpr int OSC_SHAPE_CV = 0;
    static constexpr int OSC_PHASE_CV = 1;
    static constexpr int OSC_FILTER_CV = 2;
    static constexpr int OSC_FM_CV = 3;
    static constexpr int OSC_DETUNE_CV = 4;

    static int oscInput(int i, int offset) { return GLOBAL_INPUTS + i * INPUTS_PER_OSC + offset; }

    enum OutputIds {
        NUM_OUTPUTS = NUM_OSC
    };

    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr float LFO_MIN_FREQ = 0.01f;
    static constexpr float LFO_MAX_FREQ = 10.f;
    static constexpr float VCO_MIN_FREQ = 20.f;
    static constexpr float VCO_MAX_FREQ = 20000.f;
    // Detune range in octaves (full knob sweep = ±2 semitones).
    static constexpr float DETUNE_OCT = 2.f / 12.f;

    float phase[NUM_OSC] = {};
    float filterState[NUM_OSC] = {};

    RaMothershipModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"LFO", "VCO"});
        configParam<RaMothershipFreqQuantity>(FREQ_PARAM, 0.f, 1.f, 0.3722f, "Frequency");
        configParam(FREQ_ATTN_PARAM, 0.f, 1.f, 1.f, "Frequency CV attenuator", "%", 0.f, 100.f);
        configParam(FM_ATTN_PARAM, 0.f, 1.f, 0.f, "FM attenuation", "%", 0.f, 100.f);
        configParam(PHASE_PARAM, 0.f, 1.f, 0.f, "Global phase offset", "%", 0.f, 100.f);

        for (int i = 0; i < 8; i++) {
            configParam(oscParam(i, RaMothershipModule::OSC_SHAPE), 0.f, 1.f, 0.f, "Shape", "%", 0.f, 100.f);
            configParam(oscParam(i, RaMothershipModule::OSC_PHASE), 0.f, 1.f, 0.f, "Phase offset", "%", 0.f, 100.f);
            configParam(oscParam(i, RaMothershipModule::OSC_FILTER), 0.f, 1.f, 1.f, "Filter cutoff", "%", 0.f, 100.f);
            configParam(oscParam(i, RaMothershipModule::OSC_FM), 0.f, 1.f, 0.f, "FM attenuation", "%", 0.f, 100.f);
            configSwitch(oscParam(i, RaMothershipModule::OSC_INVERT), 0.f, 1.f, 0.f, "Invert", {"Off", "Invert"});
            configParam(oscParam(i, RaMothershipModule::OSC_DETUNE), 0.f, 1.f, 0.5f, "Detune", "%", 0.f, 100.f);
        }

        configInput(FREQ_CV_INPUT, "Frequency CV");
        configInput(FREQ_ATTN_CV_INPUT, "Frequency CV attenuator");
        configInput(FM_INPUT, "FM");
        configInput(PHASE_CV_INPUT, "Phase CV");

        for (int i = 0; i < 8; i++) {
            configInput(oscInput(i, RaMothershipModule::OSC_SHAPE_CV), "Shape CV");
            configInput(oscInput(i, RaMothershipModule::OSC_PHASE_CV), "Phase CV");
            configInput(oscInput(i, RaMothershipModule::OSC_FILTER_CV), "Filter CV");
            configInput(oscInput(i, RaMothershipModule::OSC_FM_CV), "FM CV");
            configInput(oscInput(i, RaMothershipModule::OSC_DETUNE_CV), "Detune CV");
        }

        for (int i = 0; i < 8; i++)
            configOutput(i, "Oscillator");
    }

    void process(const ProcessArgs &args) override {
        bool vcoMode = params[MODE_PARAM].getValue() > 0.5f;
        float minFreq = vcoMode ? VCO_MIN_FREQ : LFO_MIN_FREQ;
        float maxFreq = vcoMode ? VCO_MAX_FREQ : LFO_MAX_FREQ;

        float freq = minFreq * powf(maxFreq / minFreq, params[FREQ_PARAM].getValue());

        if (inputs[FREQ_CV_INPUT].isConnected()) {
            float attn = params[FREQ_ATTN_PARAM].getValue();
            if (inputs[FREQ_ATTN_CV_INPUT].isConnected())
                attn *= inputs[FREQ_ATTN_CV_INPUT].getVoltage() / 10.f;
            freq *= powf(2.f, inputs[FREQ_CV_INPUT].getVoltage() * attn);
        }

        if (inputs[FM_INPUT].isConnected())
            freq *= powf(2.f, inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue());

        freq = std::max(freq, 0.001f);

        float globalPhase = params[PHASE_PARAM].getValue();
        if (inputs[PHASE_CV_INPUT].isConnected())
            globalPhase += inputs[PHASE_CV_INPUT].getVoltage() / 10.f;
        globalPhase = clamp(globalPhase, 0.f, 1.f);

        for (int i = 0; i < NUM_OSC; i++) {
            float oscFreq = freq;
            if (inputs[oscInput(i, RaMothershipModule::OSC_FM_CV)].isConnected()) {
                float fmAttn = params[oscParam(i, RaMothershipModule::OSC_FM)].getValue();
                oscFreq *= powf(2.f, inputs[oscInput(i, RaMothershipModule::OSC_FM_CV)].getVoltage() * fmAttn);
            }

            // Detune — knob is an attenuator when CV is connected, otherwise sets the offset.
            float detune;
            if (inputs[oscInput(i, RaMothershipModule::OSC_DETUNE_CV)].isConnected())
                detune = clamp(inputs[oscInput(i, RaMothershipModule::OSC_DETUNE_CV)].getVoltage() / 5.f, -1.f, 1.f) * params[oscParam(i, RaMothershipModule::OSC_DETUNE)].getValue();
            else
                detune = params[oscParam(i, RaMothershipModule::OSC_DETUNE)].getValue() - 0.5f;
            oscFreq *= powf(2.f, detune * DETUNE_OCT);

            phase[i] += oscFreq * args.sampleTime;
            if (phase[i] >= 1.f)
                phase[i] -= 1.f;

            float shape = params[oscParam(i, RaMothershipModule::OSC_SHAPE)].getValue();
            if (inputs[oscInput(i, RaMothershipModule::OSC_SHAPE_CV)].isConnected())
                shape *= clamp(inputs[oscInput(i, RaMothershipModule::OSC_SHAPE_CV)].getVoltage() / 5.f, 0.f, 1.f);
            shape = clamp(shape, 0.f, 1.f);

            float phaseOffset = params[oscParam(i, RaMothershipModule::OSC_PHASE)].getValue();
            if (inputs[oscInput(i, RaMothershipModule::OSC_PHASE_CV)].isConnected())
                phaseOffset *= clamp(inputs[oscInput(i, RaMothershipModule::OSC_PHASE_CV)].getVoltage() / 5.f, 0.f, 1.f);
            phaseOffset = clamp(phaseOffset, 0.f, 1.f);

            float p = fmodf(phase[i] + phaseOffset + globalPhase, 1.f);

            float sawUp = 2.f * p - 1.f;
            float square = (p < 0.5f) ? 1.f : -1.f;
            float wave = sawUp + (square - sawUp) * shape;

            if (params[oscParam(i, RaMothershipModule::OSC_INVERT)].getValue() > 0.5f)
                wave = -wave;

            wave *= 5.f;

            float cutoffNorm = params[oscParam(i, RaMothershipModule::OSC_FILTER)].getValue();
            if (inputs[oscInput(i, RaMothershipModule::OSC_FILTER_CV)].isConnected())
                cutoffNorm *= clamp(inputs[oscInput(i, RaMothershipModule::OSC_FILTER_CV)].getVoltage() / 5.f, 0.f, 1.f);
            cutoffNorm = clamp(cutoffNorm, 0.f, 1.f);

            float filterFreq = 20.f * powf(10000.f, cutoffNorm);
            float coeff = 1.f - expf(-2.f * M_PI * filterFreq * args.sampleTime);
            coeff = clamp(coeff, 0.001f, 1.f);
            filterState[i] += coeff * (wave - filterState[i]);

            outputs[i].setVoltage(filterState[i]);
        }
    }

    void onReset() override {
        for (int i = 0; i < NUM_OSC; i++) {
            phase[i] = 0.f;
            filterState[i] = 0.f;
        }
    }
};

struct RaMothershipWidget : ModuleWidget {
    RaMothershipWidget(RaMothershipModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-mothership.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Global controls
        addParam(createParamCentered<RaSwitch2>(Vec(20, 30), module, RaMothershipModule::MODE_PARAM));
        addParam(createParamCentered<RaKnobLarge>(Vec(55, 38), module, RaMothershipModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(90, 38), module, RaMothershipModule::FREQ_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(55, 72), module, RaMothershipModule::FREQ_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(90, 72), module, RaMothershipModule::FREQ_ATTN_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(130, 38), module, RaMothershipModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(165, 38), module, RaMothershipModule::FM_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(130, 72), module, RaMothershipModule::PHASE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(165, 72), module, RaMothershipModule::PHASE_CV_INPUT));

        // Per-voice horizontal rows. Each voice is on its own row, with its
        // output jack at the right end of the row.
        float rowY[8] = {104.f, 138.f, 172.f, 206.f, 240.f, 274.f, 308.f, 342.f};

        for (int i = 0; i < 8; i++) {
            float y = rowY[i];

            // Shape
            addParam(createParamCentered<RaKnobSmall>(Vec(20, y), module, RaMothershipModule::oscParam(i, RaMothershipModule::OSC_SHAPE)));
            addInput(createInputCentered<RaPort>(Vec(50, y), module, RaMothershipModule::oscInput(i, RaMothershipModule::OSC_SHAPE_CV)));

            // Phase
            addParam(createParamCentered<RaKnobSmall>(Vec(80, y), module, RaMothershipModule::oscParam(i, RaMothershipModule::OSC_PHASE)));
            addInput(createInputCentered<RaPort>(Vec(110, y), module, RaMothershipModule::oscInput(i, RaMothershipModule::OSC_PHASE_CV)));

            // Invert
            addParam(createParamCentered<RaSwitch2>(Vec(140, y), module, RaMothershipModule::oscParam(i, RaMothershipModule::OSC_INVERT)));

            // Filter
            addParam(createParamCentered<RaKnobSmall>(Vec(170, y), module, RaMothershipModule::oscParam(i, RaMothershipModule::OSC_FILTER)));
            addInput(createInputCentered<RaPort>(Vec(200, y), module, RaMothershipModule::oscInput(i, RaMothershipModule::OSC_FILTER_CV)));

            // FM
            addParam(createParamCentered<RaKnobSmall>(Vec(230, y), module, RaMothershipModule::oscParam(i, RaMothershipModule::OSC_FM)));
            addInput(createInputCentered<RaPort>(Vec(260, y), module, RaMothershipModule::oscInput(i, RaMothershipModule::OSC_FM_CV)));

            // Detune
            addParam(createParamCentered<RaKnobSmall>(Vec(290, y), module, RaMothershipModule::oscParam(i, RaMothershipModule::OSC_DETUNE)));
            addInput(createInputCentered<RaPort>(Vec(320, y), module, RaMothershipModule::oscInput(i, RaMothershipModule::OSC_DETUNE_CV)));

            // Output
            addOutput(createOutputCentered<RaPort>(Vec(350, y), module, i));
        }
    }
};

Model *modelRaMothership = createModel<RaMothershipModule, RaMothershipWidget>("ra-mothership");