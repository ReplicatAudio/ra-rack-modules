#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

// Byte position of the next UTF-8 codepoint (same semantics as
// rack::string::UTF8NextCodepoint, which is unavailable in older Rack pins).
static size_t raUtf8NextCodepoint(const std::string& s, size_t pos) {
    if (pos >= s.size()) return s.size();
    unsigned char c = s[pos];
    size_t len = 1;
    if ((c & 0xE0) == 0xC0) len = 2;
    else if ((c & 0xF0) == 0xE0) len = 3;
    else if ((c & 0xF8) == 0xF0) len = 4;
    return std::min(pos + len, s.size());
}

struct RaVnoteModule : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    std::string text;

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "text", json_string(text.c_str()));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* textJ = json_object_get(rootJ, "text");
        if (textJ)
            text = json_string_value(textJ);
    }
};

// Vertical text field: the text stream is laid out in a rotated frame
// (wrap along the strip length, lines stacking across the strip width) and
// rendered top-to-bottom / bottom-to-top as chosen below. Mouse events are
// mapped into that rotated frame so clicking places the cursor where you see
// the text. Editing (keys, clipboard, insertion) is handled by ui::TextField;
// only layout/rendering/hit-testing are custom.
struct VerticalTextField : ui::TextField {
    std::shared_ptr<Font> font;
    int scrollTop = 0;
    float textX0 = 0.f;   // centering offsets in the rotated frame
    float textY0 = 0.f;
    float lineBaseline = -1.f;   // exact baseline for the single centered line
    bool centered = false;

    static constexpr float FONT_SIZE = 28.f;   // glyph block (asc+desc) fits the strip width
    static constexpr float LINE_H = 35.f;      // >= strip width, so only one line fits
    static constexpr float WRAP_MARGIN = 4.f;

    struct LayoutLine {
        std::string str;
        int start = 0;              // byte offset of first char
        int end = 0;                // byte offset after last char
        float width = 0.f;
        std::vector<float> charX;   // screen-x of each char (and trailing end)
        std::vector<int> charOff;   // byte offset of each char
    };
    std::vector<LayoutLine> lines;

    VerticalTextField() {
        font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
    }

    // Wrap `text` into lines in the rotated frame (wrap width = strip length)
    void layoutText(const DrawArgs& args) {
        lines.clear();
        if (!font || !APP) return;
        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, FONT_SIZE);
        const float wrapW = std::max(FONT_SIZE, box.size.y - WRAP_MARGIN);

        LayoutLine cur;
        float lineW = 0.f;
        int lineStart = 0;
        int n = (int) text.size();
        int i = 0;
        while (i < n) {
            int begin = i;
            i = raUtf8NextCodepoint(text, i);
            std::string ch = text.substr(begin, i - begin);
            if (ch[0] == '\n') {
                lines.push_back(cur);
                LayoutLine& L = lines.back();
                L.start = lineStart;
                L.end = (cur.str.empty()) ? lineStart : lineStart + (int) cur.str.size();
                L.width = lineW;
                L.charX.push_back(lineW);
                cur = LayoutLine();
                lineW = 0.f;
                lineStart = i;
                continue;
            }
            float cw = nvgTextBounds(args.vg, 0.f, 0.f, ch.c_str(), NULL, NULL);
            if (!cur.str.empty() && lineW + cw > wrapW) {
                lines.push_back(cur);
                LayoutLine& L = lines.back();
                L.start = lineStart;
                L.end = lineStart + (int) cur.str.size();
                L.width = lineW;
                L.charX.push_back(lineW);
                cur = LayoutLine();
                lineW = 0.f;
                lineStart = begin;
            }
            cur.charX.push_back(lineW);
            cur.charOff.push_back(begin);
            cur.str += ch;
            lineW += cw;
        }
        lines.push_back(cur);
        LayoutLine& last = lines.back();
        last.start = lineStart;
        last.end = (cur.str.empty()) ? lineStart : lineStart + (int) cur.str.size();
        last.width = lineW;
        last.charX.push_back(lineW);
    }

    // Line index containing the given byte offset
    int lineOf(int byte) {
        for (int i = 0; i < (int) lines.size(); i++) {
            if (byte >= lines[i].start && byte < lines[i].end)
                return i;
        }
        return std::max(0, (int) lines.size() - 1);
    }

    // x within a line for a given byte offset (the caret x)
    float caretX(const LayoutLine& L, int byte) {
        int k = 0;
        for (int c = 0; c < (int) L.charOff.size(); c++) {
            if (L.charOff[c] < byte)
                k = c + 1;
        }
        return L.charX[clamp(k, 0, (int) L.charX.size() - 1)];
    }

    // Map a strip-relative pointer position into the rotated text frame
    Vec localize(Vec pos) {
        return Vec(box.size.y - pos.y, pos.x);
    }

    // Hit-test: text-space position -> byte offset (uses the layout from the
    // last draw, which is current between edits)
    int getTextPosition(math::Vec pos) override {
        if (lines.empty())
            return 0;
        float px = pos.x - textX0;
        float py = pos.y - textY0;
        int lineIdx;
        if (centered)
            lineIdx = clamp((int) std::floor(py / LINE_H), 0, (int) lines.size() - 1);
        else
            lineIdx = clamp((int) std::floor(pos.y / LINE_H) + scrollTop, 0, (int) lines.size() - 1);
        const LayoutLine& L = lines[lineIdx];
        // upper_bound: first char-x strictly greater than the click
        int k = (int) (std::upper_bound(L.charX.begin(), L.charX.end(), px) - L.charX.begin()) - 1;
        if (k < 0)
            return L.start;
        if (k >= (int) L.charOff.size())
            return L.end;
        return L.charOff[k];
    }

    void draw(const DrawArgs& args) override {
        // Visible full-height textarea: dark fill + border over the whole strip.
        // Painted slightly larger than the box to cover the SVG bezel outline,
        // recolored with a muted purple border to match the accent.
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
        nvgFillColor(args.vg, nvgRGB(0x11, 0x11, 0x11));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
        nvgStroke(args.vg);
        if (!font)
            return;

        nvgSave(args.vg);
        // Clip all content to the textarea so nothing can hang past its edges
        nvgScissor(args.vg, 0.f, 0.f, box.size.x, box.size.y);
        // Rotate the drawing plane about the BOX CENTRE using the classic Rack
        // idiom (translate-to-centre, rotate, translate-back) — the centre is
        // an invariant under any correct rotation, so the text anchor cannot
        // drift regardless of transform composition semantics.
        nvgTranslate(args.vg, box.size.x / 2.f, box.size.y / 2.f);
        nvgRotate(args.vg, -M_PI / 2.f);
        nvgTranslate(args.vg, -box.size.x / 2.f, -box.size.y / 2.f);

        layoutText(args);
        if (lines.empty()) {
            nvgRestore(args.vg);
            return;
        }

        bool focused = (APP->event->getSelectedWidget() == this);
        float maxW = 0.f;
        for (const LayoutLine& l : lines)
            maxW = std::max(maxW, l.width);

        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, FONT_SIZE);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
        float asc = 0.f, desc = 0.f, lh = 0.f;
        nvgTextMetrics(args.vg, &asc, &desc, &lh);

        // The text is drawn with nvg's own CENTER|MIDDLE alignment AT the box
        // centre. Because the plane rotates about exactly that centre (an
        // invariant), the rendered text centre lands on the box centre with no
        // manual offset that any transform semantics could move.
        textX0 = box.size.x / 2.f - maxW / 2.f;   // run-left: caret/selection/hit-test base
        textY0 = box.size.y / 2.f - lh / 2.f;
        lineBaseline = box.size.y / 2.f;
        centered = ((int) lines.size() == 1);
        scrollTop = 0;

        // Selection highlight (rendered behind the text)
        if (focused && selection != cursor) {
            int s1 = std::min(selection, cursor);
            int s2 = std::max(selection, cursor);
            for (int li = scrollTop; li < (int) lines.size(); li++) {
                const LayoutLine& L = lines[li];
                int a = std::max(s1, L.start);
                int b = std::min(s2, L.end);
                if (a >= b || L.charOff.empty())
                    continue;
                float xa = caretX(L, a);
                float xb = caretX(L, b);
                float y = textY0 + (li - scrollTop) * LINE_H;
                if (y > box.size.x)
                    break;
                nvgBeginPath(args.vg);
                nvgRect(args.vg, textX0 + xa, y, xb - xa, lh);
                nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, 60));
                nvgFill(args.vg);
            }
        }

        // Text lines
        for (int li = scrollTop; li < (int) lines.size(); li++) {
            const LayoutLine& L = lines[li];
            if (L.str.empty()) {
                // Placeholder, centred like real text
                if (text.empty() && li == 0) {
                    nvgFillColor(args.vg, nvgRGB(0x55, 0x3d, 0x74));
                    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, placeholder.c_str(), NULL);
                }
                continue;
            }
            nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
            if (centered && (int) lines.size() == 1) {
                // Renderer-centred at the rotation-invariant box centre
                nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, L.str.c_str(), NULL);
            }
            else {
                nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
                float y = textY0 + (li - scrollTop) * LINE_H;
                nvgText(args.vg, textX0, y + asc, L.str.c_str(), NULL);
            }
        }

        // Caret
        if (focused) {
            int li = (text.empty()) ? 0 : lineOf(cursor);
            const LayoutLine& L = lines[li];
            float x = textX0 + caretX(L, cursor);
            float y = textY0 + (li - scrollTop) * LINE_H;
            nvgBeginPath(args.vg);
            nvgRect(args.vg, x, y, 2.f, lh);
            nvgFillColor(args.vg, nvgRGB(0xc0, 0x9a, 0xe8));
            nvgFill(args.vg);
        }

        nvgRestore(args.vg);
    }

    void onButton(const event::Button& e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT)
            APP->event->setSelectedWidget(this);
        event::Button e2 = e;
        e2.pos = localize(e.pos);
        ui::TextField::onButton(e2);
    }

    void onDragHover(const event::DragHover& e) override {
        event::DragHover e2 = e;
        e2.pos = localize(e.pos);
        ui::TextField::onDragHover(e2);
    }
};

struct RaVnoteWidget : ModuleWidget {
    RaVnoteModule* module;
    VerticalTextField* textField;

    RaVnoteWidget(RaVnoteModule* module) {
        this->module = module;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-vnote.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Vertical text editor, padded clear of the corner screws on top and
        // bottom (symmetric padding keeps the box centred on the module, so
        // the centred text does not move)
        textField = new VerticalTextField;
        textField->box.pos = Vec(5, 18);
        textField->box.size = Vec(box.size.x - 10, box.size.y - 36);
        textField->multiline = true;
        textField->placeholder = "ReplicatAudio";
        // The browser renders a preview widget with a null module instance
        if (module)
            textField->text = module->text;
        addChild(textField);
    }

    void step() override {
        // Keep the module's text current so it is saved with the patch
        if (module && textField)
            module->text = textField->getText();
        ModuleWidget::step();
    }
};

Model* modelRaVnote = createModel<RaVnoteModule, RaVnoteWidget>("ra-vnote");