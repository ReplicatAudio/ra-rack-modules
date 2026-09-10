// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: RECORD_PARAM "Rec"
// fname: RECORD_INPUT "Rec tr"
// fname: PLAY_PARAM "Play"
// fname: PLAY_INPUT "Play tr"
// fname: RATE_PARAM "Rate"
// fname: RATE_INPUT "Rate CV"
// fname: POSITION_PARAM "Pos"
// fname: POSITION_INPUT "Pos CV"
// fname: IN1_INPUT "In 1"
// fname: IN2_INPUT "In 2"
// fname: IN3_INPUT "In 3"
// fname: IN4_INPUT "In 4"
// fname: OUT1_OUTPUT "Out 1"
// fname: OUT2_OUTPUT "Out 2"
// fname: OUT3_OUTPUT "Out 3"
// fname: OUT4_OUTPUT "Out 4"
#include "ra-components.hpp"

#include <cstdio>
#include <cstring>
#include <cmath>

using namespace rack;
using namespace rack::system;

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 4;
static constexpr float MAX_RECORD_SECONDS = 480.f; // 8 minutes

// Write a 32-bit little-endian value to a buffer
static void writeLE32(uint8_t *buf, uint32_t val) {
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
    buf[2] = (val >> 16) & 0xff;
    buf[3] = (val >> 24) & 0xff;
}

// Write a 16-bit little-endian value to a buffer
static void writeLE16(uint8_t *buf, uint16_t val) {
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
}

// Write a recording file with float32 PCM data
// Format: [4 bytes: magic "RAREC"][4 bytes: sampleRate][4 bytes: numSamples][numSamples * 4 bytes: float samples]
// Returns true on success
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
// Format: [4 bytes: magic "RAREC"][4 bytes: sampleRate][4 bytes: numSamples][numSamples * 4 bytes: float samples]
// Returns empty vector on failure
static std::vector<float> readRecording(const std::string &path, int &sampleRate) {
    std::vector<float> samples;
    sampleRate = 44100;

    auto data = readFile(path);
    if (data.size() < 12)
        return samples;

    // Check magic
    if (memcmp(data.data(), "RAREC", 4) != 0)
        return samples;

    // Parse header
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
        RECORD_PARAM,
        PLAY_PARAM,
        RATE_PARAM,
        POSITION_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        RECORD_INPUT,
        PLAY_INPUT,
        RATE_INPUT,
        POSITION_INPUT,
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
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
        RECORD_LIGHT,
        PLAY_LIGHT,
        NUM_LIGHTS
    };

    // Per-channel recording buffers (dynamic allocation)
    std::vector<float> buffers[NUM_CHANNELS];
    int writePositions[NUM_CHANNELS] = {0, 0, 0, 0};
    int bufferSizes[NUM_CHANNELS] = {0, 0, 0, 0};

    // Playback state
    float readPositions[NUM_CHANNELS] = {0.f, 0.f, 0.f, 0.f};
    bool recording = false;
    bool playing = false;
    float lastRecordedValue[NUM_CHANNELS] = {0.f, 0.f, 0.f, 0.f};

    // File paths for persistence
    std::string filePaths[NUM_CHANNELS];

    // Triggers
    dsp::SchmittTrigger recordTrigger;
    dsp::SchmittTrigger playTrigger;
    dsp::SchmittTrigger recordExtTrigger;
    dsp::SchmittTrigger playExtTrigger;

    RaRecModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configButton(RECORD_PARAM, "Record");
        configButton(PLAY_PARAM, "Play");
        configParam(RATE_PARAM, 0.f, 4.f, 1.f, "Rate", "x");
        configParam(POSITION_PARAM, 0.f, 1.f, 0.f, "Position");
        configInput(RECORD_INPUT, "Record");
        configInput(PLAY_INPUT, "Play");
        configInput(RATE_INPUT, "Rate CV");
        configInput(POSITION_INPUT, "Position CV");
        configInput(IN1_INPUT, "In 1");
        configInput(IN2_INPUT, "In 2");
        configInput(IN3_INPUT, "In 3");
        configInput(IN4_INPUT, "In 4");
        configOutput(OUT1_OUTPUT, "Out 1");
        configOutput(OUT2_OUTPUT, "Out 2");
        configOutput(OUT3_OUTPUT, "Out 3");
        configOutput(OUT4_OUTPUT, "Out 4");
        configLight(RECORD_LIGHT, "Record");
        configLight(PLAY_LIGHT, "Play");

        for (int i = 0; i < NUM_CHANNELS; i++) {
            allocateBuffer(i, APP->engine->getSampleRate());
            // Set up file path for this channel
            filePaths[i] = "";
        }
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
        for (int i = 0; i < NUM_CHANNELS; i++) {
            allocateBuffer(i, APP->engine->getSampleRate());
        }
    }

    float interpRead(int channel, float pos) const {
        const auto &buf = buffers[channel];
        int size = bufferSizes[channel];
        if (size == 0)
            return 0.f;
        // Wrap position
        while (pos < 0.f)
            pos += (float)size;
        while (pos >= (float)size)
            pos -= (float)size;
        int i0 = (int)pos;
        int i1 = (i0 + 1) % size;
        float frac = pos - (float)i0;
        return buf[i0] + frac * (buf[i1] - buf[i0]);
    }

    // Get the recording directory path
    std::string getRecordingsDir() {
        return asset::user("ra-recordings");
    }

    // Get the file path for a channel
    std::string getFilePath(int channel) {
        if (!filePaths[channel].empty())
            return filePaths[channel];
        // Generate a unique file path based on module instance and channel
        std::string dir = getRecordingsDir();
        // Use module pointer as unique identifier
        char path[512];
        snprintf(path, sizeof(path), "%sra-rec-%p-%d.rarec", dir.c_str(), this, channel);
        return std::string(path);
    }

    // Save a channel recording to file
    void saveChannelToFile(int channel) {
        std::string path = getFilePath(channel);
        // Create the directory if it doesn't exist
        std::string dir = getRecordingsDir();
        createDirectories(dir);
        int sr = (int)APP->engine->getSampleRate();
        int numSamples = writePositions[channel];
        if (numSamples <= 0)
            return;
        writeRecording(path, buffers[channel].data(), numSamples, sr);
        filePaths[channel] = path;
    }

    // Load a channel recording from file into the buffer
    void loadChannelFromFile(int channel) {
        std::string path = getFilePath(channel);
        int sr = (int)APP->engine->getSampleRate();
        int fileSampleRate = 0;
        auto samples = readRecording(path, fileSampleRate);
        if (samples.empty())
            return;

        // Resample if sample rates differ
        int targetSize = (int)(MAX_RECORD_SECONDS * sr);
        buffers[channel].assign(targetSize, 0.f);
        bufferSizes[channel] = targetSize;

        if (fileSampleRate == sr) {
            // Direct copy
            int copyLen = std::min((int)samples.size(), targetSize);
            for (int i = 0; i < copyLen; i++) {
                buffers[channel][i] = samples[i];
            }
            writePositions[channel] = copyLen;
        } else {
            // Resample
            float ratio = (float)fileSampleRate / (float)sr;
            int writePos = 0;
            for (float srcPos = 0.f; srcPos < (float)samples.size() && writePos < targetSize;
                 srcPos += ratio) {
                int i0 = (int)srcPos;
                int i1 = std::min(i0 + 1, (int)samples.size() - 1);
                float frac = srcPos - (float)i0;
                buffers[channel][writePos] = samples[i0] + frac * (samples[i1] - samples[i0]);
                writePos++;
            }
            writePositions[channel] = writePos;
        }

        readPositions[channel] = 0.f;
        filePaths[channel] = path;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        // Save recording state
        json_object_set_new(rootJ, "recording", json_boolean(recording));
        json_object_set_new(rootJ, "playing", json_boolean(playing));

        // Save file paths for each channel
        json_t *pathsJ = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (!filePaths[i].empty()) {
                json_array_append_new(pathsJ, json_string(filePaths[i].c_str()));
            } else {
                json_array_append_new(pathsJ, NULL);
            }
        }
        json_object_set_new(rootJ, "filePaths", pathsJ);

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        // Restore recording state
        json_t *recJ = json_object_get(rootJ, "recording");
        if (recJ)
            recording = json_boolean_value(recJ);

        json_t *playJ = json_object_get(rootJ, "playing");
        if (playJ)
            playing = json_boolean_value(playJ);

        // Restore file paths and load audio data
        json_t *pathsJ = json_object_get(rootJ, "filePaths");
        if (pathsJ) {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                json_t *pathJ = json_array_get(pathsJ, i);
                if (pathJ && json_is_string(pathJ)) {
                    filePaths[i] = json_string_value(pathJ);
                    loadChannelFromFile(i);
                }
            }
        }
    }

    void process(const ProcessArgs &args) override {

        // Rate
        float rate = params[RATE_PARAM].getValue();
        if (inputs[RATE_INPUT].isConnected())
            rate += inputs[RATE_INPUT].getVoltage() / 10.f * 4.f;
        rate = clamp(rate, 0.f, 4.f);
        if (rate < 0.001f)
            rate = 0.001f;

        // Position scrub
        float position = params[POSITION_PARAM].getValue();
        if (inputs[POSITION_INPUT].isConnected())
            position += inputs[POSITION_INPUT].getVoltage() / 10.f;
        position = clamp(position, 0.f, 1.f);

        // Record toggle (button + external trigger)
        bool recordBtn = recordTrigger.process(params[RECORD_PARAM].getValue());
        bool recordExt = recordExtTrigger.process(inputs[RECORD_INPUT].getVoltage());
        if (recordBtn || recordExt) {
            if (recording) {
                // Stopping recording — save all channels to files
                for (int i = 0; i < NUM_CHANNELS; i++) {
                    saveChannelToFile(i);
                }
            }
            recording = !recording;
            if (recording) {
                for (int i = 0; i < NUM_CHANNELS; i++)
                    writePositions[i] = 0;
            }
        }

        // Play toggle (button + external trigger)
        bool playBtn = playTrigger.process(params[PLAY_PARAM].getValue());
        bool playExt = playExtTrigger.process(inputs[PLAY_INPUT].getVoltage());
        if (playBtn || playExt) {
            if (playing) {
                // Stopping playback — nothing to save
            }
            playing = !playing;
            if (playing) {
                for (int i = 0; i < NUM_CHANNELS; i++)
                    readPositions[i] = 0.f;
            }
        }

        // Record light
        lights[RECORD_LIGHT].setBrightness(recording ? 1.f : 0.f);

        // Play light
        lights[PLAY_LIGHT].setBrightness(playing ? 1.f : 0.f);

        // Process each channel
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            int inId = IN1_INPUT + ch;
            int outId = OUT1_OUTPUT + ch;

            float in = inputs[inId].getVoltage();

            // Record: write input to buffer
            if (recording) {
                buffers[ch][writePositions[ch]] = in;
                writePositions[ch]++;
                if (writePositions[ch] >= bufferSizes[ch])
                    writePositions[ch] = 0;
                lastRecordedValue[ch] = in;
            }

            // Playback or pass-through
            float out = 0.f;
            if (playing) {
                out = interpRead(ch, readPositions[ch]);
                readPositions[ch] += rate;
                // Loop within the recorded range
                float recordEnd = (float)writePositions[ch];
                if (recordEnd > 0.f && readPositions[ch] >= recordEnd)
                    readPositions[ch] = fmodf(readPositions[ch], recordEnd);
            } else {
                // Pass through input when not playing
                out = in;
            }

            // Scrub position: sets the playhead. When playing, playback continues from the new position.
            if (inputs[POSITION_INPUT].isConnected() && fabsf(inputs[POSITION_INPUT].getVoltage()) > 0.001f) {
                readPositions[ch] = position * (float)bufferSizes[ch];
            }

            outputs[outId].setVoltage(out);
        }
    }
};

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

        float cx = box.size.x / 2;

        // Tight 2x2 grid: two columns flanking cx, fixed row step
        float colA = cx - 15;
        float colB = cx + 15;
        float topY = 76;
        float rowStep = 38;

        // ---- Top: 4 inputs in a tight 2x2 grid (1 2 / 3 4) ----
        for (int i = 0; i < 2; i++) {
            float x = colA + i * (colB - colA);
            addInput(createInputCentered<RaPort>(Vec(x, topY), module, RaRecModule::IN1_INPUT + i));
        }
        for (int i = 0; i < 2; i++) {
            float x = colA + i * (colB - colA);
            addInput(createInputCentered<RaPort>(Vec(x, topY + rowStep), module, RaRecModule::IN3_INPUT + i));
        }

        // ---- Controls: Rec/Play buttons, Rate/Pos knobs, each with its CV jack below ----
        float buttonY = topY + 2 * rowStep;
        addParam(createLightParamCentered<VCVLightBezel<RedLight>>(Vec(colA, buttonY), module, RaRecModule::RECORD_PARAM, RaRecModule::RECORD_LIGHT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(colB, buttonY), module, RaRecModule::PLAY_PARAM, RaRecModule::PLAY_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(colA, buttonY + rowStep), module, RaRecModule::RECORD_INPUT));
        addInput(createInputCentered<RaPort>(Vec(colB, buttonY + rowStep), module, RaRecModule::PLAY_INPUT));
        float knobY = buttonY + 2 * rowStep;
        addParam(createParamCentered<RaKnob>(Vec(colA, knobY), module, RaRecModule::RATE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(colB, knobY), module, RaRecModule::POSITION_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colA, knobY + rowStep), module, RaRecModule::RATE_INPUT));
        addInput(createInputCentered<RaPort>(Vec(colB, knobY + rowStep), module, RaRecModule::POSITION_INPUT));

        // ---- Bottom: 4 outputs in a tight 2x2 grid (1 2 / 3 4) ----
        float outY0 = knobY + 2 * rowStep;
        for (int i = 0; i < 2; i++) {
            float x = colA + i * (colB - colA);
            addOutput(createOutputCentered<RaPort>(Vec(x, outY0), module, RaRecModule::OUT1_OUTPUT + i));
        }
        for (int i = 0; i < 2; i++) {
            float x = colA + i * (colB - colA);
            addOutput(createOutputCentered<RaPort>(Vec(x, outY0 + rowStep), module, RaRecModule::OUT3_OUTPUT + i));
        }
    }
};

Model *modelRaRec = createModel<RaRecModule, RaRecWidget>("ra-rec");
