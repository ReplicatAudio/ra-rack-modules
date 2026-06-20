#include "ra-widgets.hpp"
#include "s7.h"
#include <mutex>
#include <cmath>
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RaSchemeModule : Module {
    enum ParamIds {
        A_PARAM,
        B_PARAM,
        C_PARAM,
        D_PARAM,
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
        STATUS_GREEN_LIGHT,
        STATUS_RED_LIGHT,
        NUM_LIGHTS
    };

    s7_scheme *sc = nullptr;
    std::string exprText;
    std::mutex textMutex;
    bool textChanged = true;

    std::atomic<float> outputValue{0.f};
    bool evalError = false;
    int expressionVersion = 0;
    float lastA = 0.f, lastB = 0.f, lastC = 0.f, lastD = 0.f;
    int evalCounter = 0;
    static const int EVAL_INTERVAL = 64;

    RaSchemeModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(A_PARAM, -10.f, 10.f, 0.f, "A", " V");
        configParam(B_PARAM, -10.f, 10.f, 0.f, "B", " V");
        configParam(C_PARAM, -10.f, 10.f, 0.f, "C", " V");
        configParam(D_PARAM, -10.f, 10.f, 0.f, "D", " V");
        configInput(A_INPUT, "A");
        configInput(B_INPUT, "B");
        configInput(C_INPUT, "C");
        configInput(D_INPUT, "D");
        configOutput(OUT_OUTPUT, "Out");
        configLight(STATUS_GREEN_LIGHT, "Status green");
        configLight(STATUS_RED_LIGHT, "Status red");

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
            expressionVersion++;
        }
    }

    std::string getExpression() {
        std::lock_guard<std::mutex> lock(textMutex);
        return exprText;
    }

    void process(const ProcessArgs &args) override {
        if (++evalCounter >= EVAL_INTERVAL) {
            evalCounter = 0;

            float a = inputs[A_INPUT].isConnected() ? inputs[A_INPUT].getVoltage() : params[A_PARAM].getValue();
            float b = inputs[B_INPUT].isConnected() ? inputs[B_INPUT].getVoltage() : params[B_PARAM].getValue();
            float c = inputs[C_INPUT].isConnected() ? inputs[C_INPUT].getVoltage() : params[C_PARAM].getValue();
            float d = inputs[D_INPUT].isConnected() ? inputs[D_INPUT].getVoltage() : params[D_PARAM].getValue();

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

                bool error = false;
                outputValue = 0.f;
                if (!expr.empty()) {
                    std::string evalExpr = expr;
                    size_t start = evalExpr.find_first_not_of(" \t\n\r");
                    if (start != std::string::npos
                        && evalExpr[start] != '('
                        && evalExpr.find_first_of(" \t\n\r", start + 1) != std::string::npos) {
                        evalExpr = "(" + evalExpr + ")";
                    }
                    s7_pointer result = s7_eval_c_string(sc, evalExpr.c_str());
                    if (s7_is_real(result)) {
                        float v = (float)s7_real(result);
                        if (std::isnan(v)) {
                            error = true;
                        } else {
                            outputValue = v;
                        }
                    } else if (s7_is_integer(result)) {
                        outputValue = (float)s7_integer(result);
                    } else {
                        error = true;
                    }
                } else {
                    error = true;
                }
                evalError = error;
            }
        }

        outputs[OUT_OUTPUT].setVoltage(outputValue);

        lights[STATUS_GREEN_LIGHT].setBrightness(evalError ? 0.f : 1.f);
        lights[STATUS_RED_LIGHT].setBrightness(evalError ? 1.f : 0.f);
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

};


struct OutputValueDisplay : LedDisplay {
    RaSchemeModule *module;

    void draw(const DrawArgs &args) override {
        if (!module) return;

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgStroke(args.vg);

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFontSize(args.vg, 14);

        float v = module->outputValue.load(std::memory_order_relaxed);
        char text[16];
        snprintf(text, sizeof(text), "%.2fV", v);

        nvgFillColor(args.vg, nvgRGB(0x88, 0xcc, 0x44));
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, text, NULL);
    }
};


struct ActionButton : Widget {
    std::string label;
    std::function<void()> onClick;

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, nvgRGB(0x55, 0x55, 0x55));
        nvgStroke(args.vg);

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFontSize(args.vg, 10);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, nvgRGB(0xbb, 0xbb, 0xbb));
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, label.c_str(), NULL);
    }

    void onButton(const ButtonEvent &e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
            if (onClick) onClick();
            e.consume(this);
        }
        Widget::onButton(e);
    }
};


struct RaSchemeWidget : ModuleWidget {
    SchemeTextField *textField;
    int lastExpressionVersion = -1;

    RaSchemeWidget(RaSchemeModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-scheme.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[] = {26, 67, 108, 149};
        float inY = 38;
        for (int i = 0; i < 4; i++) {
            addInput(createInputCentered<RaPort>(Vec(colX[i], inY), module, RaSchemeModule::A_INPUT + i));
        }

        float knobY = 66;
        addParam(createParamCentered<RaKnobSmall>(Vec(colX[0], knobY), module, RaSchemeModule::A_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(colX[1], knobY), module, RaSchemeModule::B_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(colX[2], knobY), module, RaSchemeModule::C_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(colX[3], knobY), module, RaSchemeModule::D_PARAM));

        textField = new SchemeTextField();
        textField->box.pos = Vec(12, 92);
        textField->box.size = Vec(186, 130);
        textField->schemeModule = module;
        if (module) {
            textField->text = module->getExpression();
        }
        addChild(textField);

        addOutput(createOutputCentered<RaPort>(Vec(105, 250), module, RaSchemeModule::OUT_OUTPUT));

        addChild(createLightCentered<TinyLight<GreenLight>>(
            Vec(185, 250), module, RaSchemeModule::STATUS_GREEN_LIGHT));
        addChild(createLightCentered<TinyLight<RedLight>>(
            Vec(185, 250), module, RaSchemeModule::STATUS_RED_LIGHT));

        auto *display = new OutputValueDisplay();
        display->box.pos = Vec(68, 272);
        display->box.size = Vec(74, 28);
        display->module = module;
        addChild(display);

        auto *writeBtn = new ActionButton();
        writeBtn->box.pos = Vec(12, 275);
        writeBtn->box.size = Vec(44, 22);
        writeBtn->label = "Write";
        writeBtn->onClick = [this]() {
            if (this->module) ((RaSchemeModule*)this->module)->setExpression(textField->text);
        };
        addChild(writeBtn);

        auto *clearBtn = new ActionButton();
        clearBtn->box.pos = Vec(154, 275);
        clearBtn->box.size = Vec(44, 22);
        clearBtn->label = "Clear";
        clearBtn->onClick = [this]() {
            textField->text = "";
        };
        addChild(clearBtn);
    }

    void step() override {
        if (module && textField) {
            auto *m = (RaSchemeModule*)module;
            if (m->expressionVersion != lastExpressionVersion) {
                lastExpressionVersion = m->expressionVersion;
                textField->text = m->getExpression();
            }
        }
        ModuleWidget::step();
    }
};

Model *modelRaScheme = createModel<RaSchemeModule, RaSchemeWidget>("ra-scheme");
