#include "ra-widgets.hpp"
#include "s7.h"
#include <mutex>
#include <cmath>

using namespace rack;

extern Plugin *pluginInstance;

struct RaSchemeModule : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        A_INPUT,
        B_INPUT,
        C_INPUT,
        D_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    s7_scheme *sc = nullptr;
    std::string exprText;
    std::mutex textMutex;
    bool textChanged = true;

    float outputValue = 0.f;
    float lastA = 0.f, lastB = 0.f, lastC = 0.f, lastD = 0.f;
    int evalCounter = 0;
    static const int EVAL_INTERVAL = 64;

    RaSchemeModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(A_INPUT, "A");
        configInput(B_INPUT, "B");
        configInput(C_INPUT, "C");
        configInput(D_INPUT, "D");
        configOutput(OUT_OUTPUT, "Out");

        sc = s7_init();
        exprText = "(+ a b c d)";
    }

    ~RaSchemeModule() {
        if (sc) {
            s7_free(sc);
            sc = nullptr;
        }
    }

    void setExpression(const std::string &text) {
        std::lock_guard<std::mutex> lock(textMutex);
        if (exprText != text) {
            exprText = text;
            textChanged = true;
        }
    }

    std::string getExpression() {
        std::lock_guard<std::mutex> lock(textMutex);
        return exprText;
    }

    void process(const ProcessArgs &args) override {
        if (++evalCounter >= EVAL_INTERVAL) {
            evalCounter = 0;

            float a = inputs[A_INPUT].getVoltage();
            float b = inputs[B_INPUT].getVoltage();
            float c = inputs[C_INPUT].getVoltage();
            float d = inputs[D_INPUT].getVoltage();

            bool inputsChanged = (std::abs(a - lastA) > 0.001f) ||
                                 (std::abs(b - lastB) > 0.001f) ||
                                 (std::abs(c - lastC) > 0.001f) ||
                                 (std::abs(d - lastD) > 0.001f);

            bool changed = false;
            {
                std::lock_guard<std::mutex> lock(textMutex);
                if (textChanged) {
                    textChanged = false;
                    changed = true;
                }
            }

            if (inputsChanged || changed) {
                lastA = a; lastB = b; lastC = c; lastD = d;

                s7_define_variable(sc, "a", s7_make_real(sc, a));
                s7_define_variable(sc, "b", s7_make_real(sc, b));
                s7_define_variable(sc, "c", s7_make_real(sc, c));
                s7_define_variable(sc, "d", s7_make_real(sc, d));

                std::string expr;
                {
                    std::lock_guard<std::mutex> lock(textMutex);
                    expr = exprText;
                }

                if (!expr.empty()) {
                    s7_pointer result = s7_eval_c_string(sc, expr.c_str());
                    if (s7_is_real(result)) {
                        outputValue = (float)s7_real(result);
                    } else if (s7_is_integer(result)) {
                        outputValue = (float)s7_integer(result);
                    }
                }
            }
        }

        outputs[OUT_OUTPUT].setVoltage(outputValue);
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "expression", json_string(getExpression().c_str()));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *j = json_object_get(rootJ, "expression");
        if (j) {
            setExpression(json_string_value(j));
        }
    }
};


struct SchemeTextField : LedDisplayTextField {
    RaSchemeModule *schemeModule;

    SchemeTextField() {
        textOffset = Vec(8, 8);
        color = nvgRGB(0x88, 0xcc, 0x44);
        bgColor = nvgRGBA(0x0a, 0x0a, 0x0a, 220);
        placeholder = "Enter Scheme expression...";
        multiline = true;
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 4);
        nvgFillColor(args.vg, nvgRGBA(0x0a, 0x0a, 0x0a, 220));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 4);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgStroke(args.vg);

        LedDisplayTextField::draw(args);
    }

    int getTextPosition(math::Vec mousePos) override {
        return LedDisplayTextField::getTextPosition(mousePos);
    }

    void onSelectKey(const SelectKeyEvent &e) override {
        if (e.action == GLFW_PRESS && (e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER)) {
            if (e.mods & RACK_MOD_MASK) {
                if (schemeModule) {
                    schemeModule->setExpression(text);
                }
                e.consume(this);
                return;
            }
        }
        LedDisplayTextField::onSelectKey(e);
    }

    void onDeselect(const DeselectEvent &e) override {
        if (schemeModule) {
            schemeModule->setExpression(text);
        }
        LedDisplayTextField::onDeselect(e);
    }
};


struct RaSchemeWidget : ModuleWidget {
    SchemeTextField *textField;

    RaSchemeWidget(RaSchemeModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-scheme.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float inX[] = {26, 67, 108, 149};
        float inY = 45;
        addInput(createInputCentered<RaPort>(Vec(inX[0], inY), module, RaSchemeModule::A_INPUT));
        addInput(createInputCentered<RaPort>(Vec(inX[1], inY), module, RaSchemeModule::B_INPUT));
        addInput(createInputCentered<RaPort>(Vec(inX[2], inY), module, RaSchemeModule::C_INPUT));
        addInput(createInputCentered<RaPort>(Vec(inX[3], inY), module, RaSchemeModule::D_INPUT));

        textField = new SchemeTextField();
        textField->box.pos = Vec(12, 70);
        textField->box.size = Vec(186, 145);
        textField->schemeModule = module;
        if (module) {
            textField->text = module->getExpression();
        }
        addChild(textField);

        addOutput(createOutputCentered<RaPort>(Vec(105, 250), module, RaSchemeModule::OUT_OUTPUT));
    }

    void step() override {
        if (module && textField && (APP->event->getSelectedWidget() != textField)) {
            std::string expr = ((RaSchemeModule*)module)->getExpression();
            if (textField->text != expr) {
                textField->text = expr;
            }
        }
        ModuleWidget::step();
    }
};

Model *modelRaScheme = createModel<RaSchemeModule, RaSchemeWidget>("ra-scheme");
