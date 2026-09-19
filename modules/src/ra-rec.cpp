// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: REC1_PARAM "Rec 1"
// fname: REC1_INPUT "Tr g 1"
// fname: CLEAR1_PARAM "Clr 1"
// fname: CLEAR1_INPUT "Tr c 1"
// fname: REC2_PARAM "Rec 2"
// fname: REC2_INPUT "Tr g 2"
// fname: CLEAR2_PARAM "Clr 2"
// fname: CLEAR2_INPUT "Tr c 2"
// fname: REC3_PARAM "Rec 3"
// fname: REC3_INPUT "Tr g 3"
// fname: CLEAR3_PARAM "Clr 3"
// fname: CLEAR3_INPUT "Tr c 3"
// fname: REC4_PARAM "Rec 4"
// fname: REC4_INPUT "Tr g 4"
// fname: CLEAR4_PARAM "Clr 4"
// fname: CLEAR4_INPUT "Tr c 4"
// fname: IN1_INPUT "In 1"
// fname: IN2_INPUT "In 2"
// fname: IN3_INPUT "In 3"
// fname: IN4_INPUT "In 4"
// fname: OUT1_OUTPUT "Out 1"
// fname: OUT2_OUTPUT "Out 2"
// fname: OUT3_OUTPUT "Out 3"
// fname: OUT4_OUTPUT "Out 4"
// fname: GLOBAL_REC_PARAM "All Rec"
// fname: GLOBAL_REC_INPUT "Tr g"
// fname: GLOBAL_CLEAR_PARAM "All Clr"
// fname: GLOBAL_CLEAR_INPUT "Tr c"
// fname: PLAY_PARAM "Play"
// fname: PLAY_INPUT "Tr p"
// fname: RESET_PARAM "Reset"
// fname: RESET_INPUT "Tr r"
#include "ra-components.hpp"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <mutex>
#include <atomic>
#include <algorithm>

#include <osdialog.h>

using namespace rack;
using namespace rack::system;

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 4;
static constexpr int MAX_RECORD_SECONDS = 480.f; // 8 minutes

// Write a recording file with float32 PCM data
// Format: [4 bytes: magic "RAREC"][4 bytes: sampleRate][4 bytes: numSamples][numSamples * 4 bytes: float samples]
static bool writeRecording(const std::string &path, const float *samples, int numSamples, int sampleRate) {
    std::vector<uint8_t> data;
    // Header
    data.insert(data.end(), (uint8_t*)"RAREC", (uint8_t*)"RAREC" + 4);
    data.push_back((sampleRate >> 24) & 0xff);
    data.push_back((sampleRate >> 16) & 0xff);
    data.push_back((sampleRate >> 8) & 0xff);
    data.push_back(sampleRate & 0xff);
    data.push_back((numSamples >> 24) & 0xff);
    data.push_back((numSamples >> 16) & 0xff);
    data.push_back((numSamples >> 8) & 0xff);
    data.push_back(numSamples & 0xff);
    // Samples
    for (int i = 0; i < numSamples; i++) {
        float s = clamp(samples[i], -1.f, 1.f);
        int32_t val = static_cast<int32_t>(s * 2147483647.f);
        data.push_back((val >> 24) & 0xff);
        data.push_back((val >> 16) & 0xff);
        data.push_back((val >> 8) & 0xff);
        data.push_back(val & 0xff);
    }
    writeFile(path, data);
    return true;
}

// Read a recording file and return the samples as float vector
// Returns empty vector on failure
static std::vector<float> readRecording(const std::string &path, int &sampleRate) {
    std::vector<float> samples;
    sampleRate = 44100;

    auto data = readFile(path);
    if (data.size() < 12)
        return samples;

    if (memcmp(data.data(), "RAREC", 4) != 0)
        return samples;

    sampleRate = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    int numSamples = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

    if ((int)data.size() < 12 + numSamples * 4)
        return samples;

    samples.resize(numSamples);
    for (int i = 0; i < numSamples; i++) {
        int idx = 12 + i * 4;
        int32_t val = (data[idx] << 24) | (data[idx+1] << 16) | (data[idx+2] << 8) | data[idx+3];
        samples[i] = (float)val / 2147483647.f;
    }

    return samples;
}

struct RaRecModule : Module {
    enum ParamIds {
        REC1_PARAM,
        REC2_PARAM,
        REC3_PARAM,
        REC4_PARAM,
        CLEAR1_PARAM,
        CLEAR2_PARAM,
        CLEAR3_PARAM,
        CLEAR4_PARAM,
        GLOBAL_REC_PARAM,
        GLOBAL_CLEAR_PARAM,
        PLAY_PARAM,
        RESET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        REC1_INPUT,
        REC2_INPUT,
        REC3_INPUT,
        REC4_INPUT,
        CLEAR1_INPUT,
        CLEAR2_INPUT,
        CLEAR3_INPUT,
        CLEAR4_INPUT,
        GLOBAL_REC_INPUT,
        GLOBAL_CLEAR_INPUT,
        PLAY_INPUT,
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        REC1_LIGHT,
        REC2_LIGHT,
        REC3_LIGHT,
        REC4_LIGHT,
        CLEAR1_LIGHT,
        CLEAR2_LIGHT,
        CLEAR3_LIGHT,
        CLEAR4_LIGHT,
        GLOBAL_REC_LIGHT,
        GLOBAL_CLEAR_LIGHT,
        PLAY_LIGHT,
        RESET_LIGHT,
        NUM_LIGHTS
    };

    // Per-track recording buffers and playback state
    std::vector<float> buffers[NUM_CHANNELS];
    int writePositions[NUM_CHANNELS] = {0, 0, 0, 0};   // recorded sample length
    int bufferSizes[NUM_CHANNELS] = {0, 0, 0, 0};
    float readPositions[NUM_CHANNELS] = {0.f, 0.f, 0.f, 0.f};
    bool recording[NUM_CHANNELS] = {false, false, false, false};
    bool playing = false;

    // Base path shared by all 4 tracks, each suffixed _<n>. Guarded by a mutex.
    std::string basePath;
    mutable std::mutex pathMutex;

    // Pending record start waiting on the file dialog (UI thread).
    // pendingStart: -1 = none, 0..3 = single track, 4 = all tracks.
    std::atomic<int> pendingStart{-1};
    std::atomic<bool> pathRequested{false};

    // Triggers
    dsp::SchmittTrigger recTriggers[NUM_CHANNELS];
    dsp::SchmittTrigger clearTriggers[NUM_CHANNELS];
    dsp::SchmittTrigger globalRecTrigger;
    dsp::SchmittTrigger globalClearTrigger;
    dsp::SchmittTrigger playTrigger;
    dsp::SchmittTrigger resetTrigger;

    RaRecModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configButton(REC1_PARAM, "Record 1");
        configButton(REC2_PARAM, "Record 2");
        configButton(REC3_PARAM, "Record 3");
        configButton(REC4_PARAM, "Record 4");
        configButton(CLEAR1_PARAM, "Clear 1");
        configButton(CLEAR2_PARAM, "Clear 2");
        configButton(CLEAR3_PARAM, "Clear 3");
        configButton(CLEAR4_PARAM, "Clear 4");
        configButton(GLOBAL_REC_PARAM, "Record all");
        configButton(GLOBAL_CLEAR_PARAM, "Clear all");
        configButton(PLAY_PARAM, "Play");
        configButton(RESET_PARAM, "Reset");

        configInput(IN1_INPUT, "In 1");
        configInput(IN2_INPUT, "In 2");
        configInput(IN3_INPUT, "In 3");
        configInput(IN4_INPUT, "In 4");
        configInput(REC1_INPUT, "Record 1 trigger");
        configInput(REC2_INPUT, "Record 2 trigger");
        configInput(REC3_INPUT, "Record 3 trigger");
        configInput(REC4_INPUT, "Record 4 trigger");
        configInput(CLEAR1_INPUT, "Clear 1 trigger");
        configInput(CLEAR2_INPUT, "Clear 2 trigger");
        configInput(CLEAR3_INPUT, "Clear 3 trigger");
        configInput(CLEAR4_INPUT, "Clear 4 trigger");
        configInput(GLOBAL_REC_INPUT, "Record all trigger");
        configInput(GLOBAL_CLEAR_INPUT, "Clear all trigger");
        configInput(PLAY_INPUT, "Play trigger");
        configInput(RESET_INPUT, "Reset trigger");

        configOutput(OUT1_OUTPUT, "Out 1");
        configOutput(OUT2_OUTPUT, "Out 2");
        configOutput(OUT3_OUTPUT, "Out 3");
        configOutput(OUT4_OUTPUT, "Out 4");

        configLight(REC1_LIGHT, "Record 1");
        configLight(REC2_LIGHT, "Record 2");
        configLight(REC3_LIGHT, "Record 3");
        configLight(REC4_LIGHT, "Record 4");
        configLight(CLEAR1_LIGHT, "Clear 1");
        configLight(CLEAR2_LIGHT, "Clear 2");
        configLight(CLEAR3_LIGHT, "Clear 3");
        configLight(CLEAR4_LIGHT, "Clear 4");
        configLight(GLOBAL_REC_LIGHT, "Record all");
        configLight(GLOBAL_CLEAR_LIGHT, "Clear all");
        configLight(PLAY_LIGHT, "Play");
        configLight(RESET_LIGHT, "Reset");

        int sr = (int)APP->engine->getSampleRate();
        for (int i = 0; i < NUM_CHANNELS; i++)
            allocateBuffer(i, sr);
    }

    void allocateBuffer(int channel, float sr) {
        int newSize = (int)(MAX_RECORD_SECONDS * sr);
        if (newSize == bufferSizes[channel]) return;
        buffers[channel].assign(newSize, 0.f);
        bufferSizes[channel] = newSize;
        writePositions[channel] = 0;
        readPositions[channel] = 0.f;
    }

    void onSampleRateChange() override {
        int sr = (int)APP->engine->getSampleRate();
        for (int i = 0; i < NUM_CHANNELS; i++)
            allocateBuffer(i, sr);
    }

    // ---- File dialog / base path helpers (UI thread access) ----

    // Insert _<n> before the file extension, e.g. "foo.rarec" -> "foo_1.rarec"
    static std::string suffixedPath(const std::string &base, int n) {
        size_t dot = base.find_last_of('.');
        size_t slash = base.find_last_of('/');
        if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
            return base.substr(0, dot) + "_" + std::to_string(n) + base.substr(dot);
        }
        return base + "_" + std::to_string(n);
    }

    std::string getBasePath() {
        std::lock_guard<std::mutex> lock(pathMutex);
        return basePath;
    }

    void setBasePath(const std::string &p) {
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            basePath = p;
        }
        // Make sure any pending start is honored after the path is set
        int start = pendingStart.exchange(-1);
        if (start >= 0)
            beginRecording(start);
    }

    // Discard a pending record start that was cancelled in the file dialog
    void clearPendingStart() {
        pendingStart = -1;
        pathRequested = false;
    }

    // Start recording the given scope (called only on the UI thread after dialog)
    void beginRecording(int scope) {
        if (scope == 4) {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                recording[i] = true;
                writePositions[i] = 0;
            }
        } else if (scope >= 0 && scope < NUM_CHANNELS) {
            recording[scope] = true;
            writePositions[scope] = 0;
        }
        pathRequested = false;
    }

    void clearTrack(int channel) {
        writePositions[channel] = 0;
        readPositions[channel] = 0.f;
        recording[channel] = false;
    }

    void clearAll() {
        for (int i = 0; i < NUM_CHANNELS; i++)
            clearTrack(i);
        {
            // Fresh take -> new base path requested next time
            std::lock_guard<std::mutex> lock(pathMutex);
            basePath.clear();
        }
    }

    void saveTrack(int channel) {
        std::string base;
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            base = basePath;
        }
        if (base.empty())
            return;
        std::string path = suffixedPath(base, channel);
        int numSamples = writePositions[channel];
        if (numSamples <= 0)
            return;
        // Ensure parent directory exists
        std::string dir = getDirectory(base);
        if (!dir.empty())
            createDirectories(dir);
        int sr = (int)APP->engine->getSampleRate();
        writeRecording(path, buffers[channel].data(), numSamples, sr);
    }

    bool loadTrack(int channel, const std::string &base) {
        std::string path = suffixedPath(base, channel);
        int sr = (int)APP->engine->getSampleRate();
        int fileSampleRate = 0;
        auto samples = readRecording(path, fileSampleRate);
        if (samples.empty())
            return false;

        int targetSize = bufferSizes[channel];
        buffers[channel].assign(targetSize, 0.f);

        if (fileSampleRate == sr) {
            int copyLen = std::min((int)samples.size(), targetSize);
            for (int i = 0; i < copyLen; i++)
                buffers[channel][i] = samples[i];
            writePositions[channel] = copyLen;
        } else {
            float ratio = (float)fileSampleRate / (float)sr;
            int writePos = 0;
            for (float srcPos = 0.f; srcPos < (float)samples.size() && writePos < targetSize; srcPos += ratio) {
                int i0 = (int)srcPos;
                int i1 = std::min(i0 + 1, (int)samples.size() - 1);
                float frac = srcPos - (float)i0;
                buffers[channel][writePos] = samples[i0] + frac * (samples[i1] - samples[i0]);
                writePos++;
            }
            writePositions[channel] = writePos;
        }
        readPositions[channel] = 0.f;
        return true;
    }

    float interpRead(int channel, float pos) const {
        const auto &buf = buffers[channel];
        int size = bufferSizes[channel];
        if (size == 0)
            return 0.f;
        while (pos < 0.f)
            pos += (float)size;
        while (pos >= (float)size)
            pos -= (float)size;
        int i0 = (int)pos;
        int i1 = (i0 + 1) % size;
        float frac = pos - (float)i0;
        return buf[i0] + frac * (buf[i1] - buf[i0]);
    }

    // ---- Record trigger handling ----
    // Handles a record-on request from the audio thread. If a base path is set,
    // starts immediately; otherwise requests the file dialog from the UI thread.
    void requestRecordStart(int scope) {
        std::string base = getBasePath();
        if (!base.empty()) {
            beginRecording(scope);
        } else {
            // Defer until the dialog supplies a base path
            pendingStart = scope;
            pathRequested = true;
        }
    }

    void toggleRecord(int scope) {
        if (scope == 4) {
            bool any = recording[0] || recording[1] || recording[2] || recording[3];
            if (any) {
                // Stop all recording and save each
                for (int i = 0; i < NUM_CHANNELS; i++) {
                    if (recording[i]) {
                        saveTrack(i);
                        recording[i] = false;
                    }
                }
            } else {
                requestRecordStart(4);
            }
        } else if (scope >= 0 && scope < NUM_CHANNELS) {
            if (recording[scope]) {
                saveTrack(scope);
                recording[scope] = false;
            } else {
                requestRecordStart(scope);
            }
        }
    }

    void process(const ProcessArgs &args) override {

        // ---- Per-track record toggles (button + trigger) ----
        for (int i = 0; i < NUM_CHANNELS; i++) {
            float recSig = std::max(params[REC1_PARAM + i].getValue(), inputs[REC1_INPUT + i].getVoltage());
            if (recTriggers[i].process(recSig))
                toggleRecord(i);

            float clrSig = std::max(params[CLEAR1_PARAM + i].getValue(), inputs[CLEAR1_INPUT + i].getVoltage());
            if (clearTriggers[i].process(clrSig))
                clearTrack(i);

            // Record current sample
            float in = inputs[IN1_INPUT + i].getVoltage();
            if (recording[i]) {
                buffers[i][writePositions[i]] = in;
                writePositions[i]++;
                if (writePositions[i] >= bufferSizes[i])
                    writePositions[i] = 0;
            }
        }

        // ---- Global record ----
        float gRec = std::max(params[GLOBAL_REC_PARAM].getValue(), inputs[GLOBAL_REC_INPUT].getVoltage());
        if (globalRecTrigger.process(gRec))
            toggleRecord(4);

        // ---- Global clear ----
        float gClr = std::max(params[GLOBAL_CLEAR_PARAM].getValue(), inputs[GLOBAL_CLEAR_INPUT].getVoltage());
        if (globalClearTrigger.process(gClr))
            clearAll();

        // ---- Global play / pause ----
        float playSig = std::max(params[PLAY_PARAM].getValue(), inputs[PLAY_INPUT].getVoltage());
        if (playTrigger.process(playSig))
            playing = !playing;

        // ---- Reset playheads ----
        float resetSig = std::max(params[RESET_PARAM].getValue(), inputs[RESET_INPUT].getVoltage());
        if (resetTrigger.process(resetSig)) {
            for (int i = 0; i < NUM_CHANNELS; i++)
                readPositions[i] = 0.f;
        }

        // ---- Lights ----
        for (int i = 0; i < NUM_CHANNELS; i++)
            lights[REC1_LIGHT + i].setBrightness(recording[i] ? 1.f : 0.f);
        lights[GLOBAL_REC_LIGHT].setBrightness((recording[0] || recording[1] || recording[2] || recording[3]) ? 1.f : 0.f);
        lights[GLOBAL_CLEAR_LIGHT].setBrightness(0.f);
        lights[PLAY_LIGHT].setBrightness(playing ? 1.f : 0.f);
        lights[RESET_LIGHT].setBrightness(0.f);

        // ---- Outputs ----
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            float in = inputs[IN1_INPUT + ch].getVoltage();
            float out = in; // pass-through

            if (recording[ch]) {
                // Monitor the live input while recording
                out = in;
            } else if (playing) {
                // Playback, looping within this track's own recorded length
                if (writePositions[ch] > 0) {
                    out = interpRead(ch, readPositions[ch]);
                    readPositions[ch] += 1.f;
                    float recordEnd = (float)writePositions[ch];
                    if (readPositions[ch] >= recordEnd)
                        readPositions[ch] = fmodf(readPositions[ch], recordEnd);
                } else {
                    out = 0.f;
                }
            }

            outputs[OUT1_OUTPUT + ch].setVoltage(out);
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "playing", json_boolean(playing));
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            if (!basePath.empty())
                json_object_set_new(rootJ, "basePath", json_string(basePath.c_str()));
        }
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *playJ = json_object_get(rootJ, "playing");
        if (playJ)
            playing = json_boolean_value(playJ);

        json_t *pathJ = json_object_get(rootJ, "basePath");
        if (pathJ && json_is_string(pathJ)) {
            std::string base = json_string_value(pathJ);
            {
                std::lock_guard<std::mutex> lock(pathMutex);
                basePath = base;
            }
            for (int i = 0; i < NUM_CHANNELS; i++)
                loadTrack(i, base);
        }
    }
};

// ------------------------------------------------------------------
// UI — scope display
// ------------------------------------------------------------------

struct TrackScopeDisplay : LedDisplay {
    RaRecModule *module;

    TrackScopeDisplay() {}

    void draw(const DrawArgs &args) override {
        if (!module)
            return;

        // Backdrop
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x3c, 0x3c, 0x44));
        nvgStroke(args.vg);

        // Draw 4 lanes
        float laneH = box.size.y / NUM_CHANNELS;
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);

        for (int i = 0; i < NUM_CHANNELS; i++) {
            float top = i * laneH;
            float midY = top + laneH / 2.f;

            // Lane divider
            if (i > 0) {
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, 0, top);
                nvgLineTo(args.vg, box.size.x, top);
                nvgStrokeWidth(args.vg, 1.f);
                nvgStrokeColor(args.vg, nvgRGBA(0x55, 0x55, 0x66, 120));
                nvgStroke(args.vg);
            }

            // Horizontal center marker (subtle)
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, midY);
            nvgLineTo(args.vg, box.size.x, midY);
            nvgStrokeWidth(args.vg, 0.5f);
            nvgStrokeColor(args.vg, nvgRGBA(0x55, 0x55, 0x66, 60));
            nvgStroke(args.vg);

            // Track label
            char label[8];
            snprintf(label, sizeof(label), "%d", i + 1);
            nvgFontSize(args.vg, 10);
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(args.vg, nvgRGB(0x77, 0x77, 0x88));
            nvgText(args.vg, 4, midY, label, NULL);

            drawWaveform(args.vg, i, top, midY, laneH);
        }
    }

    void drawWaveform(NVGcontext *vg, int channel, float top, float midY, float laneH) {
        // Always draw the full recorded waveform, stretched/shrunk to fit the
        // lane width. As the recording grows the waveform compresses to fit.
        int len = module->writePositions[channel];
        int cap = module->bufferSizes[channel];
        if (cap <= 0)
            return;
        const auto &buf = module->buffers[channel];

        // Vertical bounds — never let the line leave the lane
        const float plotTop = top + 3;
        const float plotBot = top + laneH - 3;
        // Map ±5V to the lane height; anything beyond is clamped to the edge
        const float scale = (plotBot - plotTop) / 2.f / 5.f;

        nvgBeginPath(vg);
        int n = (int)box.size.x;
        for (int x = 0; x < n; x++) {
            float t = (float)x / (float)n;
            int idx = (int)(t * (len > 0 ? len : cap));
            if (idx >= cap) idx = cap - 1;
            float v = buf[idx];
            float y = midY - v * scale;
            if (y < plotTop) y = plotTop;
            if (y > plotBot) y = plotBot;
            if (x == 0)
                nvgMoveTo(vg, (float)x, y);
            else
                nvgLineTo(vg, (float)x, y);
        }

        nvgStrokeWidth(vg, 1.2f);
        nvgStrokeColor(vg, nvgRGB(0x99, 0x6d, 0xd2)); // purple
        nvgStroke(vg);
    }
};

// ------------------------------------------------------------------
// UI — module widget
// ------------------------------------------------------------------

struct PurpleLight : GrayModuleLightWidget {
    PurpleLight() {
        addBaseColor(nvgRGB(0x99, 0x6d, 0xd2));
    }
};

struct RaRecWidget : ModuleWidget {
    RaRecWidget(RaRecModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-rec.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // ---- Global controls across the top ----
        float gbtnY = 42;
        float gtrigY = 62;
        float gx[4] = {144.f, 302.f, 461.f, 619.f};

        addParam(createLightParamCentered<VCVLightBezel<RedLight>>(Vec(gx[0], gbtnY), module, RaRecModule::GLOBAL_REC_PARAM, RaRecModule::GLOBAL_REC_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(gx[0], gtrigY), module, RaRecModule::GLOBAL_REC_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<YellowLight>>(Vec(gx[1], gbtnY), module, RaRecModule::GLOBAL_CLEAR_PARAM, RaRecModule::GLOBAL_CLEAR_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(gx[1], gtrigY), module, RaRecModule::GLOBAL_CLEAR_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(gx[2], gbtnY), module, RaRecModule::PLAY_PARAM, RaRecModule::PLAY_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(gx[2], gtrigY), module, RaRecModule::PLAY_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(gx[3], gbtnY), module, RaRecModule::RESET_PARAM, RaRecModule::RESET_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(gx[3], gtrigY), module, RaRecModule::RESET_INPUT));

        // ---- Display ----
        auto *display = new TrackScopeDisplay();
        display->box.pos = Vec(150, 117);
        display->box.size = Vec(420, 220);
        display->module = module;
        addChild(display);

        // ---- Per-track controls ----
        // Left column elements per lane (x-centers)
        float inX = 28;
        float recBtX = 62;
        float recTrX = 86;
        float clrBtX = 110;
        float clrTrX = 132;
        float outX = 665;

        float laneYs[4] = {145.f, 200.f, 255.f, 310.f};

        for (int i = 0; i < 4; i++) {
            float y = laneYs[i];
            addInput(createInputCentered<RaPort>(Vec(inX, y), module, RaRecModule::IN1_INPUT + i));
            addParam(createLightParamCentered<VCVLightBezel<RedLight>>(Vec(recBtX, y), module, RaRecModule::REC1_PARAM + i, RaRecModule::REC1_LIGHT + i));
            addInput(createInputCentered<RaPort>(Vec(recTrX, y), module, RaRecModule::REC1_INPUT + i));
            addParam(createLightParamCentered<VCVLightBezel<YellowLight>>(Vec(clrBtX, y), module, RaRecModule::CLEAR1_PARAM + i, RaRecModule::CLEAR1_LIGHT + i));
            addInput(createInputCentered<RaPort>(Vec(clrTrX, y), module, RaRecModule::CLEAR1_INPUT + i));
            addOutput(createOutputCentered<RaPort>(Vec(outX, y), module, RaRecModule::OUT1_OUTPUT + i));
        }
    }

    // Handle the file dialog on the UI thread when a record start requests a path.
    void step() override {
        ModuleWidget::step();
        if (!module)
            return;
        auto *m = (RaRecModule*)module;
        if (m->pathRequested.exchange(false)) {
            // Ask the user where/what to save via the system file browser
            std::string existing = m->getBasePath();
            std::string dir = existing.empty() ? "" : getDirectory(existing);
            std::string fname = existing.empty() ? "recording.rarec" : getFilename(existing);

            char *pathC = osdialog_file(OSDIALOG_SAVE,
                                        dir.empty() ? nullptr : dir.c_str(),
                                        fname.empty() ? nullptr : fname.c_str(),
                                        osdialog_filters_parse("RA-Rec Recording:rarec"));
            if (pathC) {
                m->setBasePath(pathC);
                free(pathC);
            } else {
                // Cancelled — discard the pending start
                m->clearPendingStart();
            }
        }
    }
};

Model *modelRaRec = createModel<RaRecModule, RaRecWidget>("ra-rec");
