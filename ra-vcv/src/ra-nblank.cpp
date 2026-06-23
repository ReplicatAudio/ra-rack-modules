#include "ra-components.hpp"
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

#define BLANK_BUFFER 2048
#define BLANK_PRECISION 128
#define BLANK_PRECISION_SCOPE 256
#define BLANK_CABLES 256
#define BLANK_SCOPE_LABEL_BUFFER 128
#define BLANK_SCOPE_LABEL 55

#define BLANK_CABLE_POLY_FIRST 0
#define BLANK_CABLE_POLY_SUM 1
#define BLANK_CABLE_POLY_SUM_DIVIDED 2

#define BLANK_SCOPE_TOP_LEFT 0
#define BLANK_SCOPE_TOP_RIGHT 1
#define BLANK_SCOPE_BOTTOM_LEFT 2
#define BLANK_SCOPE_BOTTOM_RIGHT 3
#define BLANK_SCOPE_CENTER 4
#define BLANK_SCOPE_CIRCULAR 0
#define BLANK_SCOPE_LINEAR 1

#define BLANK_CABLE_INCOMPLETE_OFF 0
#define BLANK_CABLE_INCOMPLETE_IN 1
#define BLANK_CABLE_INCOMPLETE_OUT 2


struct RaNblankModule;
struct RaNblankWidget;
struct BlankCablesWidget;
struct BlankScopeWidget;

struct BlankCable {
    int id;
    math::Vec pos_in;
    math::Vec pos_out;
    NVGcolor color;
    bool thick;
    float buffer[BLANK_BUFFER];
};

static RaNblankModule* g_nblank = NULL;


struct RaNblankModule : Module {
    enum ParamIds {
        PARAM_CABLE_ENABLED,
        PARAM_CABLE_BRIGHTNESS,
        PARAM_CABLE_LIGHT,
        PARAM_CABLE_POLY_THICK,
        PARAM_CABLE_POLY_MODE,
        PARAM_CABLE_FAST,
        PARAM_CABLE_SLEW,
        PARAM_CABLE_SCALE,

        PARAM_SCOPE_ENABLED,
        PARAM_SCOPE_MAJ,
        PARAM_SCOPE_MODE,
        PARAM_SCOPE_POSITION,
        PARAM_SCOPE_SCALE,
        PARAM_SCOPE_THICKNESS,
        PARAM_SCOPE_BACK_ALPHA,
        PARAM_SCOPE_VOLT_ALPHA,
        PARAM_SCOPE_LABEL_ALPHA,
        PARAM_SCOPE_ALPHA,

        PARAM_PANEL,
        PARAM_COUNT
    };
    enum InputIds { INPUT_COUNT };
    enum OutputIds { OUTPUT_COUNT };
    enum LightIds { LIGHT_COUNT };

    int width = 10;
    int cable_count;
    int cable_incomplete;
    BlankCable cables[BLANK_CABLES + 1];
    int buffer_i;
    int scope_index;
    BlankScopeWidget* scope;
    BlankCablesWidget* display;
    char scope_label[BLANK_SCOPE_LABEL_BUFFER];

    RaNblankModule();
    ~RaNblankModule();
    void processBypass(const ProcessArgs& args) override;
    void process(const ProcessArgs& args) override;

    void fromJson(json_t* rootJ) override {
        Module::fromJson(rootJ);
        json_t* widthJ = json_object_get(rootJ, "width");
        if (widthJ)
            width = std::round(json_number_value(widthJ) / RACK_GRID_WIDTH);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "width", json_integer(width));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* widthJ = json_object_get(rootJ, "width");
        if (widthJ)
            width = json_integer_value(widthJ);
    }
};


struct RaNblankPanel : Widget {
    Widget* panelBorder;
    std::shared_ptr<window::Svg> svg;

    RaNblankPanel() {
        panelBorder = new PanelBorder;
        addChild(panelBorder);
        svg = window::Svg::load(asset::plugin(pluginInstance, "res/ra-nblank.svg"));
    }

    void step() override {
        panelBorder->box.size = box.size;
        Widget::step();
    }

    void draw(const DrawArgs& args) override {
        Vec svgSize = svg->getSize();
        for (float x = 0.f; x < box.size.x; x += svgSize.x) {
            nvgSave(args.vg);
            nvgTranslate(args.vg, x, 0.f);
            svg->draw(args.vg);
            nvgRestore(args.vg);
        }
        Widget::draw(args);
    }
};


struct RaNblankResizeHandle : OpaqueWidget {
    bool right = false;
    Vec dragPos;
    Rect originalBox;
    RaNblankModule* module;

    RaNblankResizeHandle() {
        box.size = Vec(RACK_GRID_WIDTH * 1, RACK_GRID_HEIGHT);
    }

    void onDragStart(const DragStartEvent& e) override {
        if (e.button != GLFW_MOUSE_BUTTON_LEFT)
            return;
        dragPos = APP->scene->rack->getMousePos();
        ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
        assert(mw);
        originalBox = mw->box;
    }

    void onDragMove(const DragMoveEvent& e) override {
        ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
        assert(mw);

        Vec newDragPos = APP->scene->rack->getMousePos();
        float deltaX = newDragPos.x - dragPos.x;

        Rect newBox = originalBox;
        Rect oldBox = mw->box;
        const float minWidth = 3 * RACK_GRID_WIDTH;
        if (right) {
            newBox.size.x += deltaX;
            newBox.size.x = std::fmax(newBox.size.x, minWidth);
            newBox.size.x = std::round(newBox.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
        } else {
            newBox.size.x -= deltaX;
            newBox.size.x = std::fmax(newBox.size.x, minWidth);
            newBox.size.x = std::round(newBox.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
            newBox.pos.x = originalBox.pos.x + originalBox.size.x - newBox.size.x;
        }

        mw->box = newBox;
        if (!APP->scene->rack->requestModulePos(mw, newBox.pos)) {
            mw->box = oldBox;
        }
        module->width = std::round(mw->box.size.x / RACK_GRID_WIDTH);
    }

    void draw(const DrawArgs& args) override {
        for (float x = 5.0; x <= 10.0; x += 5.0) {
            nvgBeginPath(args.vg);
            const float margin = 5.0;
            nvgMoveTo(args.vg, x + 0.5, margin + 0.5);
            nvgLineTo(args.vg, x + 0.5, box.size.y - margin + 0.5);
            nvgStrokeWidth(args.vg, 1.0);
            nvgStrokeColor(args.vg, nvgRGBAf(0.5, 0.5, 0.5, 0.5));
            nvgStroke(args.vg);
        }
    }
};


struct BlankCablesWidget : Widget {
    RaNblankModule* module;

    BlankCablesWidget() {}

    void draw(const DrawArgs& args) override {}

    void drawLayer(const DrawArgs& args, int layer) override {
        BlankCable* cable;
        Vec pos_in, pos_out, pos_slump;
        Vec pos_point, pos_angle;
        NVGcolor color, color_light;
        NVGcolor col_in, col_out;
        bool brightness;
        bool fast;
        bool light;
        float t, angle, amp, length, scale, slew;
        float radius, radius_out;
        float voltage, voltage_prev;
        float voltage_diff, voltage_diff_max, voltage_max;
        float orientation;
        int buffer_phase, buffer_phase_prev;
        int i, j;

        if (layer != 1)
            return;
        if (!module)
            return;
        if (g_nblank != module)
            return;

        brightness = module->params[RaNblankModule::PARAM_CABLE_BRIGHTNESS].getValue();
        light = module->params[RaNblankModule::PARAM_CABLE_LIGHT].getValue();
        scale = module->params[RaNblankModule::PARAM_CABLE_SCALE].getValue();
        fast = module->params[RaNblankModule::PARAM_CABLE_FAST].getValue();
        slew = module->params[RaNblankModule::PARAM_CABLE_SLEW].getValue();
        slew = (slew * slew) * 0.8f;

        nvgLineCap(args.vg, 1);
        nvgLineJoin(args.vg, 1);

        for (i = 0; i < module->cable_count; ++i) {
            cable = &(module->cables[i]);

            pos_in = cable->pos_in;
            pos_out = cable->pos_out;

            float dist = pos_in.minus(pos_out).norm();
            pos_slump = pos_in.plus(pos_out).div(2);
            pos_slump.y += (1.0f - settings::cableTension) * (150.0f + 1.0f * dist);

            orientation = (pos_in.x > pos_out.x) ? 1.0f : -1.0f;

            length = std::sqrt(
                (pos_in.x - pos_out.x) * (pos_in.x - pos_out.x) +
                (pos_in.y - pos_out.y) * (pos_in.y - pos_out.y));
            if (length > 300.0f) length = 300.0f;
            if (length < 1.0f) length = 1.0f;
            length = length / 300.0f;

            if (brightness) {
                color_light = color::mult(cable->color, settings::rackBrightness);
                nvgStrokeColor(args.vg, color_light);
                nvgFillColor(args.vg, color_light);
            } else {
                nvgStrokeColor(args.vg, cable->color);
                nvgFillColor(args.vg, cable->color);
            }

            nvgGlobalAlpha(args.vg, 1.0f);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cable->pos_in.x, cable->pos_in.y, 8.5);
            nvgStrokeWidth(args.vg, 4.0);
            nvgStroke(args.vg);
            if (!light)
                nvgFill(args.vg);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cable->pos_out.x, cable->pos_out.y, 8.5);
            nvgStrokeWidth(args.vg, 4.0);
            nvgStroke(args.vg);
            if (!light)
                nvgFill(args.vg);

            nvgGlobalAlpha(args.vg, settings::cableOpacity);
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, pos_in.x, pos_in.y);
            buffer_phase_prev = module->buffer_i;
            voltage_prev = 0.0f;
            for (j = 0; j < BLANK_PRECISION; ++j) {
                t = (float)(j + 1) / (float)BLANK_PRECISION;

                pos_angle.x = 2.0f * (1.0f - t) * (pos_slump.x - pos_in.x)
                    + 2.0f * t * (pos_out.x - pos_slump.x);
                pos_angle.y = 2.0f * (1.0f - t) * (pos_slump.y - pos_in.y)
                    + 2.0f * t * (pos_out.y - pos_slump.y);
                angle = atan2(pos_angle.y, pos_angle.x);

                pos_point.x =
                    (1.0f - t) * (1.0f - t) * pos_in.x
                    + 2.0f * (1.0f - t) * t * pos_slump.x
                    + t * t * pos_out.x;
                pos_point.y =
                    (1.0f - t) * (1.0f - t) * pos_in.y
                    + 2.0f * (1.0f - t) * t * pos_slump.y
                    + t * t * pos_out.y;

                buffer_phase = module->buffer_i
                    - t * ((float)BLANK_BUFFER * length);
                if (buffer_phase < 0)
                    buffer_phase += BLANK_BUFFER;

                if (fast) {
                    voltage = cable->buffer[buffer_phase];
                } else {
                    voltage_diff_max = 0;
                    voltage_max = voltage_prev;
                    while (buffer_phase_prev != buffer_phase) {
                        voltage = cable->buffer[buffer_phase_prev];
                        voltage_diff = voltage_prev - voltage;
                        if (voltage_diff < 0)
                            voltage_diff = -voltage_diff;
                        if (voltage_diff > voltage_diff_max) {
                            voltage_diff_max = voltage_diff;
                            voltage_max = voltage;
                        }
                        buffer_phase_prev -= 1;
                        if (buffer_phase_prev < 0)
                            buffer_phase_prev += BLANK_BUFFER;
                    }
                    voltage = voltage_max;
                }
                voltage = voltage * (1.0f - slew) + voltage_prev * slew;
                voltage_prev = voltage;

                angle += (float)M_PI * 0.5f;
                if (t < 0.2f)
                    amp = t * 5.0f;
                else if (t > 0.8f)
                    amp = (1.0f - t) * 5.0f;
                else
                    amp = 1.0f;
                amp *= scale * orientation;
                pos_point.x += std::cos(angle) * voltage * amp;
                pos_point.y += std::sin(angle) * voltage * amp;

                nvgLineTo(args.vg, pos_point.x, pos_point.y);
            }
            nvgStrokeWidth(args.vg, (cable->thick) ? 9.0 : 6.0);
            nvgStroke(args.vg);

            if (light) {
                buffer_phase = module->buffer_i - 1;
                if (buffer_phase < 0)
                    buffer_phase += BLANK_BUFFER;
                voltage = cable->buffer[buffer_phase];
                if (voltage > 10.0f) voltage = 10.0f;
                else if (voltage < -10.0f) voltage = -10.0f;
                if (voltage > 0)
                    color = color::mult(nvgRGBAf(0.6274f, 0.8235f, 0.2862f, 1.0f), voltage * 0.1f);
                else
                    color = color::mult(nvgRGBAf(0.9450f, 0.2078f, 0.1725f, 1.0f), voltage * -0.1f);
                color.a = 1.0f;
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, cable->pos_in.x, cable->pos_in.y, 5.75);
                nvgCircle(args.vg, cable->pos_out.x, cable->pos_out.y, 5.75);
                nvgFillColor(args.vg, color);
                nvgFill(args.vg);
                if (brightness) {
                    nvgStrokeColor(args.vg,
                        color::mult(nvgRGBAf(0.9019f, 0.8823f, 0.8823f, 1.0f), settings::rackBrightness));
                } else {
                    nvgStrokeColor(args.vg, nvgRGBAf(0.9019f, 0.8823f, 0.8823f, 1.0f));
                }
                nvgStrokeWidth(args.vg, 1.5);
                nvgStroke(args.vg);

                if (settings::haloBrightness > 0) {
                    nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
                    radius = 5.0;
                    radius_out = radius + std::min(radius * 4.f, 15.f);
                    col_in = color::mult(color, settings::haloBrightness);
                    col_out = nvgRGBAf(0, 0, 0, 0);

                    pos_in = cable->pos_in;
                    nvgBeginPath(args.vg);
                    nvgRect(args.vg, pos_in.x - radius_out, pos_in.y - radius_out,
                        2 * radius_out, 2 * radius_out);
                    NVGpaint paint = nvgRadialGradient(args.vg,
                        pos_in.x, pos_in.y, radius, radius_out, col_in, col_out);
                    nvgFillPaint(args.vg, paint);
                    nvgFill(args.vg);

                    pos_out = cable->pos_out;
                    nvgBeginPath(args.vg);
                    nvgRect(args.vg, pos_out.x - radius_out, pos_out.y - radius_out,
                        2 * radius_out, 2 * radius_out);
                    paint = nvgRadialGradient(args.vg,
                        pos_out.x, pos_out.y, radius, radius_out, col_in, col_out);
                    nvgFillPaint(args.vg, paint);
                    nvgFill(args.vg);

                    nvgGlobalCompositeOperation(args.vg, NVG_SOURCE_OVER);
                }
            }
        }

        if (module->cable_incomplete != BLANK_CABLE_INCOMPLETE_OFF) {
            cable = &(module->cables[BLANK_CABLES]);
            pos_in = cable->pos_in;
            pos_out = cable->pos_out;

            float dist = pos_in.minus(pos_out).norm();
            pos_slump = pos_in.plus(pos_out).div(2);
            pos_slump.y += (1.0f - settings::cableTension) * (150.0f + 1.0f * dist);

            if (brightness) {
                color_light = color::mult(cable->color, settings::rackBrightness);
                nvgStrokeColor(args.vg, color_light);
                nvgFillColor(args.vg, color_light);
            } else {
                nvgStrokeColor(args.vg, cable->color);
                nvgFillColor(args.vg, cable->color);
            }
            nvgGlobalAlpha(args.vg, 1.0f);

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cable->pos_in.x, cable->pos_in.y, 8.5);
            nvgStrokeWidth(args.vg, 4.0);
            nvgStroke(args.vg);
            nvgFill(args.vg);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, cable->pos_out.x, cable->pos_out.y, 8.5);
            nvgStrokeWidth(args.vg, 4.0);
            nvgStroke(args.vg);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, pos_in.x, pos_in.y);
            nvgQuadTo(args.vg, pos_slump.x, pos_slump.y, pos_out.x, pos_out.y);
            nvgStrokeWidth(args.vg, 6.0);
            nvgStroke(args.vg);
        }
    }
};

struct BlankScopeWidget : Widget {
    RaNblankModule* module;
    std::string font_path;

    BlankScopeWidget() {
        font_path = asset::system("res/fonts/ShareTechMono-Regular.ttf");
    }

    void draw(const DrawArgs& args) override {
        std::shared_ptr<Font> font;
        Vec pos_point;
        BlankCable* cable;
        Rect box;
        bool mode;
        float label_alpha;
        float details;
        float background;
        float scale;
        float alpha;
        float thickness;
        float t;
        float voltage, voltage_prev;
        float voltage_diff, voltage_diff_max, voltage_max;
        int buffer_phase, buffer_phase_prev;
        int position;
        int i;

        if (g_nblank != module)
            return;
        if (module->params[RaNblankModule::PARAM_SCOPE_ENABLED].getValue() == 0.0)
            return;
        if (module->scope_index < 0)
            return;

        cable = &(module->cables[module->scope_index]);
        scale = module->params[RaNblankModule::PARAM_SCOPE_SCALE].getValue();
        position = module->params[RaNblankModule::PARAM_SCOPE_POSITION].getValue();
        mode = module->params[RaNblankModule::PARAM_SCOPE_MODE].getValue();
        thickness = module->params[RaNblankModule::PARAM_SCOPE_THICKNESS].getValue();
        details = module->params[RaNblankModule::PARAM_SCOPE_VOLT_ALPHA].getValue();
        background = module->params[RaNblankModule::PARAM_SCOPE_BACK_ALPHA].getValue();
        label_alpha = module->params[RaNblankModule::PARAM_SCOPE_LABEL_ALPHA].getValue();
        alpha = module->params[RaNblankModule::PARAM_SCOPE_ALPHA].getValue();

        box.size.x = scale * APP->scene->box.size.x;
        box.size.y = scale * APP->scene->box.size.x * 0.5f;

        switch (position) {
            case BLANK_SCOPE_TOP_LEFT:
                box.pos = Vec(10.0f, 40.0f);
                break;
            case BLANK_SCOPE_TOP_RIGHT:
                box.pos = Vec(APP->scene->box.size.x - (box.size.x + 10.0f), 40.0f);
                break;
            case BLANK_SCOPE_BOTTOM_LEFT:
                box.pos = Vec(10.0f, APP->scene->box.size.y - (box.size.y + 10.0f));
                break;
            case BLANK_SCOPE_BOTTOM_RIGHT:
                box.pos = Vec(APP->scene->box.size.x - (box.size.x + 10.0f),
                    APP->scene->box.size.y - (box.size.y + 10.0f));
                break;
            default:
                box.pos = Vec(APP->scene->box.size.x * 0.5f - box.size.x * 0.5f,
                    APP->scene->box.size.y * 0.5f - box.size.y * 0.5f);
                break;
        }

        nvgGlobalAlpha(args.vg, alpha);

        if (background >= 0) {
            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, nvgRGBAf(0, 0, 0, background));
            nvgRect(args.vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
            nvgFill(args.vg);
        }

        if (details >= 0) {
            nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 1, details));
            nvgStrokeWidth(args.vg, 1.0);
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, box.pos.x, box.pos.y + box.size.y * 0.5f);
            nvgLineTo(args.vg, box.pos.x + box.size.x, box.pos.y + box.size.y * 0.5f);
            nvgMoveTo(args.vg, box.pos.x, box.pos.y + box.size.y * 0.3f);
            nvgLineTo(args.vg, box.pos.x + box.size.x, box.pos.y + box.size.y * 0.3f);
            nvgMoveTo(args.vg, box.pos.x, box.pos.y + box.size.y * 0.7f);
            nvgLineTo(args.vg, box.pos.x + box.size.x, box.pos.y + box.size.y * 0.7f);
            nvgMoveTo(args.vg, box.pos.x, box.pos.y + box.size.y * 0.1f);
            nvgLineTo(args.vg, box.pos.x + box.size.x, box.pos.y + box.size.y * 0.1f);
            nvgMoveTo(args.vg, box.pos.x, box.pos.y + box.size.y * 0.9f);
            nvgLineTo(args.vg, box.pos.x + box.size.x, box.pos.y + box.size.y * 0.9f);
            nvgStroke(args.vg);
        }

        nvgScissor(args.vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
        nvgBeginPath(args.vg);
        buffer_phase_prev = module->buffer_i;
        voltage_prev = 0.0f;
        for (i = 0; i < BLANK_PRECISION_SCOPE; ++i) {
            t = (float)i / (float)BLANK_PRECISION_SCOPE;

            if (mode == BLANK_SCOPE_CIRCULAR) {
                buffer_phase = module->buffer_i - 1 - t * (float)BLANK_BUFFER;
                if (buffer_phase < 0)
                    buffer_phase += BLANK_BUFFER;
                voltage_diff_max = 0;
                voltage_max = voltage_prev;
                while (buffer_phase_prev != buffer_phase) {
                    voltage = cable->buffer[buffer_phase_prev];
                    voltage_diff = voltage_prev - voltage;
                    if (voltage_diff < 0)
                        voltage_diff = -voltage_diff;
                    if (voltage_diff > voltage_diff_max) {
                        voltage_diff_max = voltage_diff;
                        voltage_max = voltage;
                    }
                    buffer_phase_prev -= 1;
                    if (buffer_phase_prev < 0)
                        buffer_phase_prev += BLANK_BUFFER;
                }
                voltage_prev = voltage_max;
                voltage = voltage_max;
            } else {
                buffer_phase = t * (float)BLANK_BUFFER;
                voltage = cable->buffer[buffer_phase];
            }

            pos_point.x = box.pos.x + t * box.size.x;
            pos_point.y = box.pos.y + box.size.y * 0.5f
                - voltage * 0.05f * box.size.y * 0.8f;
            if (i == 0)
                nvgMoveTo(args.vg, pos_point.x, pos_point.y);
            else
                nvgLineTo(args.vg, pos_point.x, pos_point.y);
        }
        nvgStrokeColor(args.vg, cable->color);
        nvgStrokeWidth(args.vg, thickness);
        nvgStroke(args.vg);
        nvgResetScissor(args.vg);

        if (label_alpha > 0) {
            font = APP->window->loadFont(font_path);
            if (font) {
                nvgFontSize(args.vg, 12.0f * scale * 5.0f);
                nvgFontFaceId(args.vg, font->handle);
                nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgFillColor(args.vg, nvgRGBAf(1, 1, 1, label_alpha));
                nvgText(args.vg, box.pos.x + box.size.x * 0.5f,
                    box.pos.y + box.size.y * 0.905f, module->scope_label, NULL);
            }
        }
    }
};


// Out-of-line definitions for RaNblankModule destructor/process
// (must be after BlankCablesWidget and BlankScopeWidget are fully defined)

RaNblankModule::RaNblankModule() {
    config(PARAM_COUNT, INPUT_COUNT, OUTPUT_COUNT, LIGHT_COUNT);

    configSwitch(PARAM_CABLE_ENABLED, 0, 1, 1, "Cable animation", {"Off", "On"});
    configSwitch(PARAM_CABLE_BRIGHTNESS, 0, 1, 1, "Cable brightness", {"Off", "On"});
    configSwitch(PARAM_CABLE_LIGHT, 0, 1, 1, "Cable LED", {"Off", "On"});
    configSwitch(PARAM_CABLE_POLY_THICK, 0, 1, 1, "Polyphonic cable thickness", {"Off", "On"});
    configSwitch(PARAM_CABLE_POLY_MODE, 0, 2, 0, "Polyphonic mode", {"1st channel", "Sum", "Sum / channels"});
    configSwitch(PARAM_CABLE_FAST, 0, 1, 0, "CPU fast mode", {"Off", "On"});
    configParam(PARAM_CABLE_SLEW, 0.0, 1.0, 0.0, "Cable slew", "%", 0, 100);
    configParam(PARAM_CABLE_SCALE, 0.0, 2.0, 1.0, "Cable scale", "%", 0, 100);

    configSwitch(PARAM_SCOPE_ENABLED, 0, 1, 1, "Scope", {"Off", "On"});
    configSwitch(PARAM_SCOPE_MAJ, 0, 1, 0, "Scope on Shift only", {"Off", "On"});
    configSwitch(PARAM_SCOPE_MODE, 0, 1, 0, "Scope mode", {"Circular", "Linear"});
    configSwitch(PARAM_SCOPE_POSITION, 0, 4, 0, "Scope position", {"Top left", "Top right", "Bottom left", "Bottom right", "Center"});
    configParam(PARAM_SCOPE_SCALE, 0.02, 1, 0.2, "Scope scale", "%", 0, 100);
    configParam(PARAM_SCOPE_THICKNESS, 1, 10, 2, "Scope thickness", "");
    configParam(PARAM_SCOPE_BACK_ALPHA, 0, 1, 0.6, "Scope background alpha", "%", 0, 100);
    configParam(PARAM_SCOPE_VOLT_ALPHA, 0, 1, 0.3, "Scope voltage alpha", "%", 0, 100);
    configParam(PARAM_SCOPE_LABEL_ALPHA, 0, 1, 1, "Scope label alpha", "%", 0, 100);
    configParam(PARAM_SCOPE_ALPHA, 0, 1, 1, "Scope alpha", "%", 0, 100);

    configSwitch(PARAM_PANEL, 0, 3, 0, "Panel", {"City pigeon", "Wild pigeon", "Pigeon gang", "Pigeon Army"});

    buffer_i = 0;
    display = NULL;
    scope = NULL;
    std::memset(scope_label, 0, sizeof(scope_label));
}

RaNblankModule::~RaNblankModule() {
    if (display) {
        if (APP->scene->rack->hasChild(display))
            APP->scene->rack->removeChild(display);
        delete display;
    }
    if (scope) {
        if (APP->scene->hasChild(scope))
            APP->scene->removeChild(scope);
        delete scope;
    }
    if (this == g_nblank) {
        g_nblank = NULL;
        APP->scene->rack->getCableContainer()->show();
    }
}

void RaNblankModule::processBypass(const ProcessArgs& args) {
    if (this == g_nblank) {
        APP->scene->rack->getCableContainer()->show();
        if (display)
            display->hide();
    }
}

void RaNblankModule::process(const ProcessArgs& args) {
    if (args.frame % 32 != 0)
        return;
    if (g_nblank == NULL)
        g_nblank = this;
    if (g_nblank != this)
        return;

    bool scope_enabled;
    bool poly_thick;
    int poly_mode;

    if (params[PARAM_SCOPE_MAJ].getValue() && APP->window)
        scope_enabled = ((APP->window->getMods() & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT);
    else
        scope_enabled = true;

    poly_thick = params[PARAM_CABLE_POLY_THICK].getValue();
    poly_mode = params[PARAM_CABLE_POLY_MODE].getValue();

    if (params[PARAM_CABLE_ENABLED].getValue()) {
        APP->scene->rack->getCableContainer()->hide();
        if (display)
            display->show();
    } else {
        APP->scene->rack->getCableContainer()->show();
        if (display)
            display->hide();
    }

    PortWidget* hovered = dynamic_cast<PortWidget*>(APP->event->hoveredWidget);
    scope_label[0] = 0;

    Widget* container = APP->scene->rack->getCableContainer();
    int i = 0;
    scope_index = -1;

    for (Widget* child : container->children) {
        CableWidget* widget = dynamic_cast<CableWidget*>(child);
        if (!widget || !widget->isComplete())
            continue;

        Cable* cable = widget->cable;
        cables[i].pos_out = widget->getInputPos();
        cables[i].pos_in = widget->getOutputPos();
        cables[i].color = widget->color;

        if (cable && cable->outputModule && cable->outputId >= 0) {
            Output* output = &(cable->outputModule->outputs[cable->outputId]);
            int channels = output->getChannels();
            if (channels == 0)
                channels = 1;
            cables[i].thick = (channels > 1 && poly_thick);
            if (poly_mode == BLANK_CABLE_POLY_FIRST) {
                cables[i].buffer[buffer_i] = output->getVoltage();
            } else if (poly_mode == BLANK_CABLE_POLY_SUM) {
                cables[i].buffer[buffer_i] = output->getVoltageSum();
            } else {
                cables[i].buffer[buffer_i] = output->getVoltageSum() / channels;
            }
        }

        if (scope_enabled &&
            (widget->outputPort == hovered || widget->inputPort == hovered)) {
            scope_index = i;
            PortInfo* port_info = widget->outputPort->getPortInfo();
            if (port_info) {
                strncpy(scope_label, port_info->name.c_str(), BLANK_SCOPE_LABEL);
            }
            strcat(scope_label, " output to ");
            port_info = widget->inputPort->getPortInfo();
            if (port_info) {
                strncat(scope_label, port_info->name.c_str(), BLANK_SCOPE_LABEL);
            }
            strcat(scope_label, " input");
        }

        ++i;
        if (i >= BLANK_CABLES)
            break;
    }
    cable_count = i;

    if (scope_index < 0 && hovered && scope_enabled) {
        PortWidget* port_widget = dynamic_cast<PortWidget*>(hovered);
        if (port_widget && port_widget->type == engine::Port::OUTPUT) {
            Port* port = port_widget->getPort();
            if (port) {
                scope_index = BLANK_CABLES;
                cables[BLANK_CABLES].color = nvgRGBAf(1, 1, 1, 1);
                cables[BLANK_CABLES].buffer[buffer_i] = port->voltages[0];
                PortInfo* port_info = port_widget->getPortInfo();
                if (port_info) {
                    strncpy(scope_label, port_info->name.c_str(), BLANK_SCOPE_LABEL - 1);
                }
            }
        }
    }

    CableWidget* incomplete = APP->scene->rack->getIncompleteCable();
    cable_incomplete = BLANK_CABLE_INCOMPLETE_OFF;
    if (incomplete) {
        if (incomplete->inputPort)
            cable_incomplete = BLANK_CABLE_INCOMPLETE_IN;
        else
            cable_incomplete = BLANK_CABLE_INCOMPLETE_OUT;
        scope_index = -1;
        cables[BLANK_CABLES].pos_out = incomplete->getInputPos();
        cables[BLANK_CABLES].pos_in = incomplete->getOutputPos();
        cables[BLANK_CABLES].color = incomplete->color;
    }

    buffer_i += 1;
    if (buffer_i >= BLANK_BUFFER)
        buffer_i = 0;
}


struct RaNblankWidget : ModuleWidget {
    Widget* topRightScrew;
    Widget* bottomRightScrew;
    Widget* rightHandle;
    RaNblankPanel* blankPanel;
    RaNblankModule* nmodule;

    RaNblankWidget(RaNblankModule* module) {
        setModule(module);
        nmodule = module;
        box.size = Vec(RACK_GRID_WIDTH * 10, RACK_GRID_HEIGHT);

        blankPanel = new RaNblankPanel;
        addChild(blankPanel);

        RaNblankResizeHandle* leftHandle = new RaNblankResizeHandle;
        leftHandle->module = module;
        addChild(leftHandle);

        RaNblankResizeHandle* rightHandle = new RaNblankResizeHandle;
        rightHandle->right = true;
        this->rightHandle = rightHandle;
        rightHandle->module = module;
        addChild(rightHandle);

        addChild(createWidget<RaScrew>(Vec(15, 0)));
        addChild(createWidget<RaScrew>(Vec(15, 365)));
        topRightScrew = createWidget<RaScrew>(Vec(box.size.x - 30, 0));
        bottomRightScrew = createWidget<RaScrew>(Vec(box.size.x - 30, 365));
        addChild(topRightScrew);
        addChild(bottomRightScrew);

        if (module) {
            box.size.x = module->width * RACK_GRID_WIDTH;

            BlankCablesWidget* display = createWidget<BlankCablesWidget>(Vec(0, 0));
            display->box.size = Vec(INFINITY, INFINITY);
            display->module = module;
            module->display = display;
            APP->scene->rack->addChild(display);

            BlankScopeWidget* scope = createWidget<BlankScopeWidget>(Vec(0, 0));
            scope->box.size = Vec(INFINITY, INFINITY);
            scope->module = module;
            module->scope = scope;
            APP->scene->addChild(scope);
        }
    }

    void step() override {
        RaNblankModule* module = dynamic_cast<RaNblankModule*>(this->module);
        if (module) {
            box.size.x = module->width * RACK_GRID_WIDTH;
        }

        blankPanel->box.size = box.size;
        topRightScrew->box.pos.x = box.size.x - 30;
        bottomRightScrew->box.pos.x = box.size.x - 30;
        if (box.size.x < RACK_GRID_WIDTH * 6) {
            topRightScrew->hide();
            bottomRightScrew->hide();
        } else {
            topRightScrew->show();
            bottomRightScrew->show();
        }
        rightHandle->box.pos.x = box.size.x - rightHandle->box.size.x;
        ModuleWidget::step();
    }

    void appendContextMenu(Menu* menu) override {
        if (!nmodule)
            return;

        menu->addChild(new ui::MenuSeparator);

        menu->addChild(createSubmenuItem("Panel", "",
            [=](ui::Menu* menu) {
                menu->addChild(createCheckMenuItem("City pigeon", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_PANEL].getValue() == 0; },
                    [=]() { this->set_panel(0); }
                ));
                menu->addChild(createCheckMenuItem("Wild pigeon", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_PANEL].getValue() == 1; },
                    [=]() { this->set_panel(1); }
                ));
                menu->addChild(createCheckMenuItem("Pigeon gang", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_PANEL].getValue() == 2; },
                    [=]() { this->set_panel(2); }
                ));
                menu->addChild(createCheckMenuItem("Pigeon Army (loops)", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_PANEL].getValue() == 3; },
                    [=]() { this->set_panel(3); }
                ));
            }
        ));

        menu->addChild(new ui::MenuSeparator);
        ui::MenuLabel* label = new ui::MenuLabel;
        label->text = "Cables";
        menu->addChild(label);

        menu->addChild(createCheckMenuItem("Cable animation enabled", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_ENABLED].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_ENABLED].setValue(!(int)nmodule->params[RaNblankModule::PARAM_CABLE_ENABLED].getValue()); }
        ));

        menu->addChild(createCheckMenuItem("Cable LED", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_LIGHT].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_LIGHT].setValue(!(int)nmodule->params[RaNblankModule::PARAM_CABLE_LIGHT].getValue()); }
        ));

        menu->addChild(createCheckMenuItem("Cable brightness", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_BRIGHTNESS].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_BRIGHTNESS].setValue(!(int)nmodule->params[RaNblankModule::PARAM_CABLE_BRIGHTNESS].getValue()); }
        ));

        menu->addChild(createCheckMenuItem("Cable polyphonic thickness", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_POLY_THICK].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_POLY_THICK].setValue(!(int)nmodule->params[RaNblankModule::PARAM_CABLE_POLY_THICK].getValue()); }
        ));

        menu->addChild(createSubmenuItem("Cable polyphonic", "",
            [=](ui::Menu* menu) {
                menu->addChild(createCheckMenuItem("1st channel", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_POLY_MODE].getValue() == BLANK_CABLE_POLY_FIRST; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_POLY_MODE].setValue(BLANK_CABLE_POLY_FIRST); }
                ));
                menu->addChild(createCheckMenuItem("Sum", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_POLY_MODE].getValue() == BLANK_CABLE_POLY_SUM; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_POLY_MODE].setValue(BLANK_CABLE_POLY_SUM); }
                ));
                menu->addChild(createCheckMenuItem("Sum / channel count", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_POLY_MODE].getValue() == BLANK_CABLE_POLY_SUM_DIVIDED; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_POLY_MODE].setValue(BLANK_CABLE_POLY_SUM_DIVIDED); }
                ));
            }
        ));

        menu->addChild(createCheckMenuItem("Cable CPU fast", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_CABLE_FAST].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_CABLE_FAST].setValue(!(int)nmodule->params[RaNblankModule::PARAM_CABLE_FAST].getValue()); }
        ));

        menu->addChild(new ui::MenuSeparator);
        label = new ui::MenuLabel;
        label->text = "Scope";
        menu->addChild(label);

        menu->addChild(createCheckMenuItem("Scope enabled", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_ENABLED].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_ENABLED].setValue(!(int)nmodule->params[RaNblankModule::PARAM_SCOPE_ENABLED].getValue()); }
        ));

        menu->addChild(createCheckMenuItem("Scope on Shift only", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_MAJ].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_MAJ].setValue(!(int)nmodule->params[RaNblankModule::PARAM_SCOPE_MAJ].getValue()); }
        ));

        menu->addChild(createCheckMenuItem("Scope circular display", "",
            [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_MODE].getValue() == 1; },
            [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_MODE].setValue(!(int)nmodule->params[RaNblankModule::PARAM_SCOPE_MODE].getValue()); }
        ));

        menu->addChild(createSubmenuItem("Scope position", "",
            [=](ui::Menu* menu) {
                menu->addChild(createCheckMenuItem("Top left", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].getValue() == BLANK_SCOPE_TOP_LEFT; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].setValue(BLANK_SCOPE_TOP_LEFT); }
                ));
                menu->addChild(createCheckMenuItem("Top right", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].getValue() == BLANK_SCOPE_TOP_RIGHT; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].setValue(BLANK_SCOPE_TOP_RIGHT); }
                ));
                menu->addChild(createCheckMenuItem("Bottom left", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].getValue() == BLANK_SCOPE_BOTTOM_LEFT; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].setValue(BLANK_SCOPE_BOTTOM_LEFT); }
                ));
                menu->addChild(createCheckMenuItem("Bottom right", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].getValue() == BLANK_SCOPE_BOTTOM_RIGHT; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].setValue(BLANK_SCOPE_BOTTOM_RIGHT); }
                ));
                menu->addChild(createCheckMenuItem("Center", "",
                    [=]() { return nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].getValue() == BLANK_SCOPE_CENTER; },
                    [=]() { nmodule->params[RaNblankModule::PARAM_SCOPE_POSITION].setValue(BLANK_SCOPE_CENTER); }
                ));
            }
        ));
    }

    void set_panel(int id) {
        if (id == 1) {
            setPanel(createPanel(asset::plugin(pluginInstance, "res/Blank-Wild.svg")));
        } else if (id == 2) {
            setPanel(createPanel(asset::plugin(pluginInstance, "res/Blank-Gang.svg")));
        } else if (id == 3) {
            setPanel(createPanel(asset::plugin(pluginInstance, "res/Blank-Army.svg")));
        } else {
            id = 0;
        }
        if (id == 0) {
            setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-nblank.svg")));
        }
        if (nmodule)
            nmodule->params[RaNblankModule::PARAM_PANEL].setValue(id);
    }
};

Model* modelRaNblank = createModel<RaNblankModule, RaNblankWidget>("ra-nblank");
