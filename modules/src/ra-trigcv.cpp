// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: TRIG1_INPUT "TR1"
// fname: CV1_INPUT "CV1"
// fname: KNOB1_PARAM "K1"
// fname: LED1_R "L1"
// fname: LED1_G "L1"
// fname: LED1_B "L1"
// fname: TRIG2_INPUT "TR2"
// fname: CV2_INPUT "CV2"
// fname: KNOB2_PARAM "K2"
// fname: LED2_R "L2"
// fname: LED2_G "L2"
// fname: LED2_B "L2"
// fname: TRIG3_INPUT "TR3"
// fname: CV3_INPUT "CV3"
// fname: KNOB3_PARAM "K3"
// fname: LED3_R "L3"
// fname: LED3_G "L3"
// fname: LED3_B "L3"
// fname: TRIG4_INPUT "TR4"
// fname: CV4_INPUT "CV4"
// fname: KNOB4_PARAM "K4"
// fname: LED4_R "L4"
// fname: LED4_G "L4"
// fname: LED4_B "L4"
// fname: TRIG5_INPUT "TR5"
// fname: CV5_INPUT "CV5"
// fname: KNOB5_PARAM "K5"
// fname: LED5_R "L5"
// fname: LED5_G "L5"
// fname: LED5_B "L5"
// fname: TRIG6_INPUT "TR6"
// fname: CV6_INPUT "CV6"
// fname: KNOB6_PARAM "K6"
// fname: LED6_R "L6"
// fname: LED6_G "L6"
// fname: LED6_B "L6"
// fname: TRIG7_INPUT "TR7"
// fname: CV7_INPUT "CV7"
// fname: KNOB7_PARAM "K7"
// fname: LED7_R "L7"
// fname: LED7_G "L7"
// fname: LED7_B "L7"
// fname: TRIG8_INPUT "TR8"
// fname: CV8_INPUT "CV8"
// fname: KNOB8_PARAM "K8"
// fname: LED8_R "L8"
// fname: LED8_G "L8"
// fname: LED8_B "L8"
// fname: OUTPUT1 "OUT"
// fname: MODE_PARAM "Mode"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaTrigcvModule : Module {
    enum ParamIds {
        KNOB1_PARAM,
        KNOB2_PARAM,
        KNOB3_PARAM,
        KNOB4_PARAM,
        KNOB5_PARAM,
        KNOB6_PARAM,
        KNOB7_PARAM,
        KNOB8_PARAM,
        MODE_PARAM,
        SLEW_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG1_INPUT,
        TRIG2_INPUT,
        TRIG3_INPUT,
        TRIG4_INPUT,
        TRIG5_INPUT,
        TRIG6_INPUT,
        TRIG7_INPUT,
        TRIG8_INPUT,
        CV1_INPUT,
        CV2_INPUT,
        CV3_INPUT,
        CV4_INPUT,
        CV5_INPUT,
        CV6_INPUT,
        CV7_INPUT,
        CV8_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT1,
        NUM_OUTPUTS
    };
    enum LightIds {
        LED1_LIGHT_R,
        LED1_LIGHT_G,
        LED1_LIGHT_B,
        LED2_LIGHT_R,
        LED2_LIGHT_G,
        LED2_LIGHT_B,
        LED3_LIGHT_R,
        LED3_LIGHT_G,
        LED3_LIGHT_B,
        LED4_LIGHT_R,
        LED4_LIGHT_G,
        LED4_LIGHT_B,
        LED5_LIGHT_R,
        LED5_LIGHT_G,
        LED5_LIGHT_B,
        LED6_LIGHT_R,
        LED6_LIGHT_G,
        LED6_LIGHT_B,
        LED7_LIGHT_R,
        LED7_LIGHT_G,
        LED7_LIGHT_B,
        LED8_LIGHT_R,
        LED8_LIGHT_G,
        LED8_LIGHT_B,
        NUM_LIGHTS
    };

    int activeTrigger = 1; // 1-8 = trigger index of the active (last-triggered) group; defaults to group 1
    float lastOutput = 0.f;

    RaTrigcvModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 8; i++) {
            configParam(KNOB1_PARAM + i, 0.f, 1.f, 0.5f, string::f("Knob %d", i + 1));
            configInput(TRIG1_INPUT + i, string::f("Trigger %d", i + 1));
            configInput(CV1_INPUT + i, string::f("CV %d", i + 1));
            configLight(LED1_LIGHT_R + i * 3, string::f("LED %d R", i + 1));
            configLight(LED1_LIGHT_G + i * 3, string::f("LED %d G", i + 1));
            configLight(LED1_LIGHT_B + i * 3, string::f("LED %d B", i + 1));
        }
        configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"0–10V", "±5V", "0–1V"});
        configParam(SLEW_PARAM, 0.f, 1.f, 1.f, "Slew");
        configOutput(OUTPUT1, "Output");
    }

    void process(const ProcessArgs &args) override {
        int mode = (int)std::round(params[MODE_PARAM].getValue());
        float fullScale;
        float offset;
        switch (mode) {
            default:
            case 0: fullScale = 10.f; offset = 0.f; break;
            case 1: fullScale = 10.f; offset = -5.f; break;
            case 2: fullScale = 1.f; offset = 0.f; break;
        }

        // Update the active (last-triggered) group. When a trigger is high, the
        // highest-numbered one becomes the active group. Otherwise the previous
        // active group is held, so its LED stays lit and its value is output.
        for (int i = 7; i >= 0; i--) {
            if (inputs[TRIG1_INPUT + i].getVoltage() > 1.f) {
                activeTrigger = i + 1;
                break;
            }
        }

        // Compute output voltage from the active trigger's value
        float out = lastOutput;
        if (activeTrigger > 0) {
            int idx = activeTrigger - 1;
            float knobVal = params[KNOB1_PARAM + idx].getValue();
            float cvVal = inputs[CV1_INPUT + idx].getVoltage();
            float value;
            if (inputs[CV1_INPUT + idx].isConnected()) {
                // CV controls the value, knob acts as 0-1 attenuation
                value = clamp(knobVal * cvVal / 10.f, 0.f, 1.f);
            } else {
                value = knobVal;
            }
            out = value * fullScale + offset;
        }
        // When no trigger is active, hold the last output (out = lastOutput)

        // Slew limiting (exponential smoothing, matching ra-reflectingpool)
        // At slew = 0 the output snaps instantly to the target (never frozen).
        float slewKnob = params[SLEW_PARAM].getValue();
        float slewMs = 10000.f * slewKnob * slewKnob * slewKnob * slewKnob;
        float factor = (slewMs < 0.1f) ? 1.f : 1.f - expf(-args.sampleTime / (slewMs * 0.001f));
        lastOutput += (out - lastOutput) * factor;
        out = lastOutput;
        outputs[OUTPUT1].setVoltage(clamp(out, -10.f, 10.f));

        // Update LED lights — only the latest trigger light is on (instant, no fade)
        for (int i = 0; i < 8; i++) {
            float r = 0.0f, g = 0.0f, b = 0.0f;
            if (i + 1 == activeTrigger) {
                // Purple
                r = 0.8f;
                g = 0.1f;
                b = 1.0f;
            }
            lights[LED1_LIGHT_R + i * 3 + 0].setBrightness(r);
            lights[LED1_LIGHT_R + i * 3 + 1].setBrightness(g);
            lights[LED1_LIGHT_R + i * 3 + 2].setBrightness(b);
        }
    }
};

struct RaTrigcvWidget : ModuleWidget {
    RaTrigcvWidget(RaTrigcvModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-trigcv.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // 8 trigger groups arranged horizontally (one row per group)
        // Each row: trig input | cv input | knob | led
        // Vertical spacing ~30 ru per group, total ~240 ru = ~16 HP (still compact)
        float groupSpacing = 30.f;
        float startY = 42.f;
        float xTrig = 25.f;
        float xCv = xTrig + 30.f;
        float xKnob = xCv + 30.f;
        float xLed = xKnob + 30.f;

        for (int i = 0; i < 8; i++) {
            float y = startY + i * groupSpacing;
            addInput(createInputCentered<RaPort>(Vec(xTrig, y), module, RaTrigcvModule::TRIG1_INPUT + i));
            addInput(createInputCentered<RaPort>(Vec(xCv, y), module, RaTrigcvModule::CV1_INPUT + i));
            addParam(createParamCentered<RaKnobTrim>(Vec(xKnob, y), module, RaTrigcvModule::KNOB1_PARAM + i));
            addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(xLed, y), module, RaTrigcvModule::LED1_LIGHT_R + i * 3));
        }

        // Output jack
        float outX = box.size.x / 2.f;
        // Below all 8 groups
        float bottomY = 42.f + 8 * 30.f + 20.f;

        // Output jack (bottom left-ish, centered)
        addOutput(createOutputCentered<RaPort>(Vec(outX - 20.f, bottomY), module, RaTrigcvModule::OUTPUT1));

        // Slew control knob (center-bottom)
        addParam(createParamCentered<RaKnobTrim>(Vec(outX, bottomY + 15.f), module, RaTrigcvModule::SLEW_PARAM));

        // 3-way mode switch (bottom-right)
        addParam(createParamCentered<RaSwitch3>(Vec(outX + 25.f, bottomY + 15.f), module, RaTrigcvModule::MODE_PARAM));
    }
};

Model *modelRaTrigcv = createModel<RaTrigcvModule, RaTrigcvWidget>("ra-trigcv");
