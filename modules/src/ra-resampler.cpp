// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: LOAD_PARAM "Load"
// fname: SPEED_PARAM "Speed"
// fname: POSITION_PARAM "Position"
// fname: LEVEL_PARAM "Level"
// fname: LOOP_PARAM "Loop"
// fname: PITCH_INPUT "1V/Oct"
// fname: GATE_INPUT "Gate"
// fname: SPEED_INPUT "Spd CV"
// fname: POSITION_INPUT "Pos CV"
// fname: LEVEL_INPUT "Lvl CV"
// fname: AUDIO_OUTPUT "Audio"
// fname: LOOP_LIGHT "Loop"
#include "ra-components.hpp"

#include <vector>
#include <cmath>
#include <cstring>
#include <atomic>
#include <osdialog.h>

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int MAX_VOICES = 16;

// Read a WAV file and return samples as float vector
static std::vector<float> readWavFile(const std::string &path, int &sampleRate) {
    std::vector<float> samples;
    sampleRate = 44100;

    auto data = rack::system::readFile(path);
    if (data.size() < 44)
        return samples;

    if (memcmp(data.data(), "RIFF", 4) != 0)
        return samples;
    if (memcmp(data.data() + 8, "WAVE", 4) != 0)
        return samples;

    size_t offset = 12;
    int channels = 1;
    int bitsPerSample = 16;
    while (offset + 8 < data.size()) {
        uint32_t chunkId = *(uint32_t*)(data.data() + offset);
        uint32_t chunkSize = data[offset+4] | (data[offset+5] << 8) | (data[offset+6] << 16) | (data[offset+7] << 24);

        if (memcmp(&chunkId, "fmt ", 4) == 0) {
            channels = data[offset+10] | (data[offset+11] << 8);
            sampleRate = data[offset+12] | (data[offset+13] << 8) | (data[offset+14] << 16) | (data[offset+15] << 24);
            bitsPerSample = data[offset+22] | (data[offset+23] << 8);
            break;
        }
        offset += 8 + chunkSize;
        if (chunkSize % 2) offset++;
    }

    offset = 12;
    while (offset + 8 < data.size()) {
        uint32_t chunkId = *(uint32_t*)(data.data() + offset);
        uint32_t chunkSize = data[offset+4] | (data[offset+5] << 8) | (data[offset+6] << 16) | (data[offset+7] << 24);

        if (memcmp(&chunkId, "data", 4) == 0) {
            size_t audioOffset = offset + 8;
            int bytesPerSample = bitsPerSample / 8;
            int numSamples = chunkSize / (channels * bytesPerSample);

            samples.resize(numSamples);
            for (int i = 0; i < numSamples; i++) {
                float sum = 0.f;
                for (int ch = 0; ch < channels; ch++) {
                    size_t idx = audioOffset + (i * channels + ch) * bytesPerSample;
                    if (idx + bytesPerSample <= data.size()) {
                        int32_t val;
                        if (bitsPerSample == 16) {
                            val = (int16_t)(data[idx] | (data[idx+1] << 8));
                        } else if (bitsPerSample == 24) {
                            val = data[idx] | (data[idx+1] << 8) | ((int8_t)data[idx+2] << 16);
                        } else if (bitsPerSample == 32) {
                            val = data[idx] | (data[idx+1] << 8) | (data[idx+2] << 16) | ((int8_t)data[idx+3] << 24);
                        } else {
                            val = 0;
                        }
                        sum += (float)val;
                    }
                }
                samples[i] = sum / (float)channels;
                if (bitsPerSample == 16) samples[i] /= 32768.f;
                else if (bitsPerSample == 24) samples[i] /= 8388608.f;
                else if (bitsPerSample == 32) samples[i] /= 2147483648.f;
            }
            return samples;
        }
        offset += 8 + chunkSize;
        if (chunkSize % 2) offset++;
    }

    return samples;
}

struct RaResamplerModule : Module {
    enum ParamIds {
        LOAD_PARAM,
        SPEED_PARAM,
        POSITION_PARAM,
        LEVEL_PARAM,
        LOOP_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        GATE_INPUT,
        SPEED_INPUT,
        POSITION_INPUT,
        LEVEL_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LOOP_LIGHT,
        NUM_LIGHTS
    };

    std::vector<float> sample;
    int sampleRate = 44100;
    bool sampleLoaded = false;

    float playPos[MAX_VOICES] = {};
    bool playing[MAX_VOICES] = {};

    dsp::SchmittTrigger loadTrigger;
    dsp::SchmittTrigger loopTrigger;

    std::atomic<bool> loadRequested{false};

    RaResamplerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configButton(LOAD_PARAM, "Load");
        configParam(SPEED_PARAM, 0.1f, 10.f, 1.f, "Speed", "x");
        configParam(POSITION_PARAM, 0.f, 1.f, 0.f, "Position", "%", 0.f, 100.f);
        configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0.f, 100.f);
        configSwitch(LOOP_PARAM, 0.f, 1.f, 0.f, "Loop", {"Off", "On"});
        configInput(PITCH_INPUT, "1V/Oct");
        configInput(GATE_INPUT, "Gate");
        configInput(SPEED_INPUT, "Speed CV");
        configInput(POSITION_INPUT, "Position CV");
        configInput(LEVEL_INPUT, "Level CV");
        configOutput(AUDIO_OUTPUT, "Audio");
        configLight(LOOP_LIGHT, "Loop");
    }

    float interp(float pos) const {
        if (sample.empty()) return 0.f;
        int size = (int)sample.size();
        int i = (int)pos;
        float f = pos - (float)i;
        int i0 = i % size;
        int i1 = (i0 + 1) % size;
        return sample[i0] + f * (sample[i1] - sample[i0]);
    }

    void process(const ProcessArgs &args) override {
        int channels = std::max(1, inputs[PITCH_INPUT].getChannels());

        if (loadTrigger.process(params[LOAD_PARAM].getValue()))
            loadRequested = true;

        if (loopTrigger.process(params[LOOP_PARAM].getValue()))
            params[LOOP_PARAM].setValue(params[LOOP_PARAM].getValue() > 0.5f ? 0.f : 1.f);
        bool loop = params[LOOP_PARAM].getValue() > 0.5f;
        lights[LOOP_LIGHT].setBrightness(loop ? 1.f : 0.f);

        float speed = clamp(params[SPEED_PARAM].getValue() + inputs[SPEED_INPUT].getVoltage() / 10.f * 9.f, 0.1f, 10.f);
        float position = clamp(params[POSITION_PARAM].getValue() + inputs[POSITION_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float level = clamp(params[LEVEL_PARAM].getValue() + inputs[LEVEL_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        for (int c = 0; c < channels; c++) {
            float pitch = inputs[PITCH_INPUT].getVoltage(c);
            float gate = inputs[GATE_INPUT].getVoltage(c);

            if (gate > 1.f) {
                if (!playing[c]) {
                    playing[c] = true;
                    playPos[c] = position * (float)(sample.size() - 1);
                }
            } else if (!loop && playing[c]) {
                playing[c] = false;
            }

            float out = 0.f;
            if (playing[c] && sampleLoaded && sample.size() > 1) {
                float rate = speed * powf(2.f, pitch);
                out = interp(playPos[c]);
                playPos[c] += rate;

                if (playPos[c] >= (float)(sample.size() - 1)) {
                    if (loop) {
                        playPos[c] = 0.f;
                    } else {
                        playing[c] = false;
                        out = 0.f;
                    }
                }
            }

            outputs[AUDIO_OUTPUT].setVoltage(out * level * 5.f, c);
        }

        outputs[AUDIO_OUTPUT].setChannels(channels);
    }

    void loadSample(const std::string &path) {
        int sr = 0;
        auto samples = readWavFile(path, sr);
        if (!samples.empty()) {
            sample = std::move(samples);
            sampleRate = sr;
            sampleLoaded = true;
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "sampleLoaded", json_boolean(sampleLoaded));
        json_object_set_new(rootJ, "sampleRate", json_integer(sampleRate));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *loadedJ = json_object_get(rootJ, "sampleLoaded");
        if (loadedJ)
            sampleLoaded = json_boolean_value(loadedJ);
        json_t *srJ = json_object_get(rootJ, "sampleRate");
        if (srJ)
            sampleRate = json_integer_value(srJ);
    }
};

struct RaResamplerWidget : ModuleWidget {
    RaResamplerWidget(RaResamplerModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-resampler.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[3] = {22.f, 90.f, 158.f};
        float rowY[4] = {40.f, 100.f, 160.f, 220.f};

        // Column 1: Load, Gate, 1V/Oct
        addParam(createParamCentered<RaButton>(Vec(colX[0], rowY[0]), module, RaResamplerModule::LOAD_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[0], rowY[1]), module, RaResamplerModule::GATE_INPUT));
        addInput(createInputCentered<RaPort>(Vec(colX[0], rowY[2]), module, RaResamplerModule::PITCH_INPUT));

        // Column 2: Speed knob, Speed CV, Position knob, Position CV
        addParam(createParamCentered<RaKnobTrim>(Vec(colX[1], rowY[0]), module, RaResamplerModule::SPEED_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[1], rowY[1]), module, RaResamplerModule::SPEED_INPUT));
        addParam(createParamCentered<RaKnobTrim>(Vec(colX[1], rowY[2]), module, RaResamplerModule::POSITION_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[1], rowY[3]), module, RaResamplerModule::POSITION_INPUT));

        // Column 3: Level knob, Level CV, Loop, Output
        addParam(createParamCentered<RaKnobTrim>(Vec(colX[2], rowY[0]), module, RaResamplerModule::LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[2], rowY[1]), module, RaResamplerModule::LEVEL_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(colX[2], rowY[2]), module, RaResamplerModule::LOOP_PARAM, RaResamplerModule::LOOP_LIGHT));
        addOutput(createOutputCentered<RaPort>(Vec(colX[2], rowY[3]), module, RaResamplerModule::AUDIO_OUTPUT));
    }

    void step() override {
        ModuleWidget::step();
        if (!module)
            return;
        auto *m = (RaResamplerModule*)module;
        if (m->loadRequested.exchange(false)) {
            char *pathC = osdialog_file(OSDIALOG_OPEN,
                                        nullptr, nullptr,
                                        osdialog_filters_parse("WAV Audio:wav"));
            if (pathC) {
                m->loadSample(pathC);
                free(pathC);
            }
        }
    }
};

Model *modelRaResampler = createModel<RaResamplerModule, RaResamplerWidget>("ra-resampler");
