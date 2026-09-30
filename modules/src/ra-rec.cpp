// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: REC1_PARAM "Rec 1"
// fname: REC1_INPUT "Tr g 1"
// fname: PLAY1_INPUT "Tr p 1"
// fname: CLEAR1_PARAM "Clr 1"
// fname: CLEAR1_INPUT "Tr c 1"
// fname: WRITE1_PARAM "Wr 1"
// fname: READ1_PARAM "Rd 1"
// fname: PLAY1_PARAM "▶ 1"
// fname: RESET1_PARAM "Rst 1"
// fname: RESET1_INPUT "Tr r 1"
// fname: POSITION1_INPUT "Pos 1"
// fname: REC2_PARAM "Rec 2"
// fname: REC2_INPUT "Tr g 2"
// fname: PLAY2_INPUT "Tr p 2"
// fname: CLEAR2_PARAM "Clr 2"
// fname: CLEAR2_INPUT "Tr c 2"
// fname: WRITE2_PARAM "Wr 2"
// fname: READ2_PARAM "Rd 2"
// fname: PLAY2_PARAM "▶ 2"
// fname: RESET2_PARAM "Rst 2"
// fname: RESET2_INPUT "Tr r 2"
// fname: POSITION2_INPUT "Pos 2"
// fname: REC3_PARAM "Rec 3"
// fname: REC3_INPUT "Tr g 3"
// fname: PLAY3_INPUT "Tr p 3"
// fname: CLEAR3_PARAM "Clr 3"
// fname: CLEAR3_INPUT "Tr c 3"
// fname: WRITE3_PARAM "Wr 3"
// fname: READ3_PARAM "Rd 3"
// fname: PLAY3_PARAM "▶ 3"
// fname: RESET3_PARAM "Rst 3"
// fname: RESET3_INPUT "Tr r 3"
// fname: POSITION3_INPUT "Pos 3"
// fname: REC4_PARAM "Rec 4"
// fname: REC4_INPUT "Tr g 4"
// fname: PLAY4_INPUT "Tr p 4"
// fname: CLEAR4_PARAM "Clr 4"
// fname: CLEAR4_INPUT "Tr c 4"
// fname: WRITE4_PARAM "Wr 4"
// fname: READ4_PARAM "Rd 4"
// fname: PLAY4_PARAM "▶ 4"
// fname: RESET4_PARAM "Rst 4"
// fname: RESET4_INPUT "Tr r 4"
// fname: SPEED1_PARAM "Spd 1"
// fname: SPEED1_INPUT "CV s 1"
// fname: SPEED2_PARAM "Spd 2"
// fname: SPEED2_INPUT "CV s 2"
// fname: SPEED3_PARAM "Spd 3"
// fname: SPEED3_INPUT "CV s 3"
// fname: SPEED4_PARAM "Spd 4"
// fname: SPEED4_INPUT "CV s 4"
// fname: POSITION4_INPUT "Pos 4"
// fname: REVERSE1_PARAM "◀ 1"
// fname: REVERSE1_INPUT "Tr v 1"
// fname: REVERSE2_PARAM "◀ 2"
// fname: REVERSE2_INPUT "Tr v 2"
// fname: REVERSE3_PARAM "◀ 3"
// fname: REVERSE3_INPUT "Tr v 3"
// fname: REVERSE4_PARAM "◀ 4"
// fname: REVERSE4_INPUT "Tr v 4"
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
// fname: GLOBAL_PLAY_PARAM "▶ All"
// fname: GLOBAL_PLAY_INPUT "Tr p"
// fname: GLOBAL_RESET_PARAM "Rst All"
// fname: GLOBAL_RESET_INPUT "Tr r"
#include "ra-components.hpp"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <random>

#include <osdialog.h>

using namespace rack;
using namespace rack::system;

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 4;
static constexpr int MAX_RECORD_SECONDS = 480.f; // 8 minutes

// Write a recording file as 16-bit mono WAV
static bool writeRecording(const std::string &path, const float *samples, int numSamples, int sampleRate) {
    const int numChannels = 1;
    const int bitsPerSample = 16;
    const int bytesPerSample = bitsPerSample / 8;
    int dataSize = numSamples * numChannels * bytesPerSample;
    int fileSize = 36 + dataSize; // Total file size minus 8 (RIFF header includes this)

    std::vector<uint8_t> data;
    data.reserve(44 + dataSize);

    // RIFF header
    data.insert(data.end(), (uint8_t*)"RIFF", (uint8_t*)"RIFF" + 4);
    data.push_back(fileSize & 0xff);
    data.push_back((fileSize >> 8) & 0xff);
    data.push_back((fileSize >> 16) & 0xff);
    data.push_back((fileSize >> 24) & 0xff);
    data.insert(data.end(), (uint8_t*)"WAVE", (uint8_t*)"WAVE" + 4);

    // fmt chunk
    data.insert(data.end(), (uint8_t*)"fmt ", (uint8_t*)"fmt " + 4);
    int fmtChunkSize = 16; // PCM format
    data.push_back(fmtChunkSize & 0xff);
    data.push_back((fmtChunkSize >> 8) & 0xff);
    data.push_back((fmtChunkSize >> 16) & 0xff);
    data.push_back((fmtChunkSize >> 24) & 0xff);
    int16_t audioFormat = 1; // PCM
    data.push_back(audioFormat & 0xff);
    data.push_back((audioFormat >> 8) & 0xff);
    int16_t channels = numChannels;
    data.push_back(channels & 0xff);
    data.push_back((channels >> 8) & 0xff);
    data.push_back(sampleRate & 0xff);
    data.push_back((sampleRate >> 8) & 0xff);
    data.push_back((sampleRate >> 16) & 0xff);
    data.push_back((sampleRate >> 24) & 0xff);
    int byteRate = sampleRate * numChannels * bytesPerSample;
    data.push_back(byteRate & 0xff);
    data.push_back((byteRate >> 8) & 0xff);
    data.push_back((byteRate >> 16) & 0xff);
    data.push_back((byteRate >> 24) & 0xff);
    int blockAlign = numChannels * bytesPerSample;
    data.push_back(blockAlign & 0xff);
    data.push_back((blockAlign >> 8) & 0xff);
    data.push_back(bitsPerSample & 0xff);
    data.push_back((bitsPerSample >> 8) & 0xff);

    // data chunk
    data.insert(data.end(), (uint8_t*)"data", (uint8_t*)"data" + 4);
    data.push_back(dataSize & 0xff);
    data.push_back((dataSize >> 8) & 0xff);
    data.push_back((dataSize >> 16) & 0xff);
    data.push_back((dataSize >> 24) & 0xff);

    // Audio samples (16-bit mono)
    for (int i = 0; i < numSamples; i++) {
        float s = clamp(samples[i], -1.f, 1.f);
        int16_t val = static_cast<int16_t>(s * 32767.f);
        data.push_back(val & 0xff);
        data.push_back((val >> 8) & 0xff);
    }

    writeFile(path, data);
    return true;
}

// Read a WAV file and return the samples as float vector
// Returns empty vector on failure
static std::vector<float> readRecording(const std::string &path, int &sampleRate) {
    std::vector<float> samples;
    sampleRate = 44100;

    auto data = readFile(path);
    if (data.size() < 44)
        return samples;

    // Check RIFF header
    if (memcmp(data.data(), "RIFF", 4) != 0)
        return samples;
    if (memcmp(data.data() + 8, "WAVE", 4) != 0)
        return samples;

    // Find fmt chunk
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
        if (chunkSize % 2) offset++; // Pad byte
    }

    // Find data chunk
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
                // Normalize to -1..1 range
                if (bitsPerSample == 16) samples[i] /= 32768.f;
                else if (bitsPerSample == 24) samples[i] /= 8388608.f;
                else if (bitsPerSample == 32) samples[i] /= 2147483648.f;
            }
            return samples;
        }
        offset += 8 + chunkSize;
        if (chunkSize % 2) offset++; // Pad byte
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
        WRITE1_PARAM,
        WRITE2_PARAM,
        WRITE3_PARAM,
        WRITE4_PARAM,
        READ1_PARAM,
        READ2_PARAM,
        READ3_PARAM,
        READ4_PARAM,
        PLAY1_PARAM,
        PLAY2_PARAM,
        PLAY3_PARAM,
        PLAY4_PARAM,
        RESET1_PARAM,
        RESET2_PARAM,
        RESET3_PARAM,
        RESET4_PARAM,
        SPEED1_PARAM,
        SPEED2_PARAM,
        SPEED3_PARAM,
        SPEED4_PARAM,
        REVERSE1_PARAM,
        REVERSE2_PARAM,
        REVERSE3_PARAM,
        REVERSE4_PARAM,
        GLOBAL_REC_PARAM,
        GLOBAL_CLEAR_PARAM,
        GLOBAL_PLAY_PARAM,
        GLOBAL_RESET_PARAM,
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
        PLAY1_INPUT,
        PLAY2_INPUT,
        PLAY3_INPUT,
        PLAY4_INPUT,
        RESET1_INPUT,
        RESET2_INPUT,
        RESET3_INPUT,
        RESET4_INPUT,
        SPEED1_INPUT,
        SPEED2_INPUT,
        SPEED3_INPUT,
        SPEED4_INPUT,
        REVERSE1_INPUT,
        REVERSE2_INPUT,
        REVERSE3_INPUT,
        REVERSE4_INPUT,
        POSITION1_INPUT,
        POSITION2_INPUT,
        POSITION3_INPUT,
        POSITION4_INPUT,
        GLOBAL_REC_INPUT,
        GLOBAL_CLEAR_INPUT,
        GLOBAL_PLAY_INPUT,
        GLOBAL_RESET_INPUT,
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
        PLAY1_LIGHT,
        PLAY2_LIGHT,
        PLAY3_LIGHT,
        PLAY4_LIGHT,
        RESET1_LIGHT,
        RESET2_LIGHT,
        RESET3_LIGHT,
        RESET4_LIGHT,
        REVERSE1_LIGHT,
        REVERSE2_LIGHT,
        REVERSE3_LIGHT,
        REVERSE4_LIGHT,
        GLOBAL_REC_LIGHT,
        GLOBAL_CLEAR_LIGHT,
        GLOBAL_PLAY_LIGHT,
        GLOBAL_RESET_LIGHT,
        NUM_LIGHTS
    };

    // Per-track recording buffers and playback state
    std::vector<float> buffers[NUM_CHANNELS];
    int writePositions[NUM_CHANNELS] = {0, 0, 0, 0};   // recorded sample length
    int bufferSizes[NUM_CHANNELS] = {0, 0, 0, 0};
    float readPositions[NUM_CHANNELS] = {0.f, 0.f, 0.f, 0.f};
    bool recording[NUM_CHANNELS] = {false, false, false, false};
    bool playing[NUM_CHANNELS] = {false, false, false, false};
    float lastPositionVoltage[NUM_CHANNELS] = {100.f, 100.f, 100.f, 100.f};  // Start with out-of-range value

    // Track which channels have valid waveform data loaded
    bool loaded[NUM_CHANNELS] = {false, false, false, false};
    bool reversePlaying[NUM_CHANNELS] = {false, false, false, false};

    // Flag to trigger loading from storage after JSON load
    bool needsStorageLoad = false;

    // Base path shared by all 4 tracks, each suffixed _<n>. Guarded by a mutex.
    std::string basePath;
    mutable std::mutex pathMutex;

    // Pending record start waiting on the file dialog (UI thread).
    // pendingStart: -1 = none, 0..3 = single track, 4 = all tracks.
    std::atomic<int> pendingStart{-1};
    std::atomic<bool> pathRequested{false};

    // Pending write/read per channel. -1 = none, 0..3 = channel index.
    std::atomic<int> pendingWrite{-1};
    std::atomic<int> pendingRead{-1};
    std::atomic<bool> writeRequested{false};
    std::atomic<bool> readRequested{false};

    // Triggers
    dsp::SchmittTrigger recTriggers[NUM_CHANNELS];
    dsp::SchmittTrigger clearTriggers[NUM_CHANNELS];
    dsp::SchmittTrigger playTriggers[NUM_CHANNELS];
    dsp::SchmittTrigger resetTriggers[NUM_CHANNELS];
    dsp::SchmittTrigger reverseTriggers[NUM_CHANNELS];
    dsp::SchmittTrigger globalRecTrigger;
    dsp::SchmittTrigger globalClearTrigger;
    dsp::SchmittTrigger globalPlayTrigger;
    dsp::SchmittTrigger globalResetTrigger;

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
        configButton(WRITE1_PARAM, "Write 1");
        configButton(WRITE2_PARAM, "Write 2");
        configButton(WRITE3_PARAM, "Write 3");
        configButton(WRITE4_PARAM, "Write 4");
        configButton(READ1_PARAM, "Read 1");
        configButton(READ2_PARAM, "Read 2");
        configButton(READ3_PARAM, "Read 3");
        configButton(READ4_PARAM, "Read 4");
        configButton(PLAY1_PARAM, "Play 1");
        configButton(PLAY2_PARAM, "Play 2");
        configButton(PLAY3_PARAM, "Play 3");
        configButton(PLAY4_PARAM, "Play 4");
        configButton(RESET1_PARAM, "Reset 1");
        configButton(RESET2_PARAM, "Reset 2");
        configButton(RESET3_PARAM, "Reset 3");
        configButton(RESET4_PARAM, "Reset 4");
        configParam(SPEED1_PARAM, 0.f, 1.f, 0.125f, "Speed 1", "x", 8, 0);
        configParam(SPEED2_PARAM, 0.f, 1.f, 0.125f, "Speed 2", "x", 8, 0);
        configParam(SPEED3_PARAM, 0.f, 1.f, 0.125f, "Speed 3", "x", 8, 0);
        configParam(SPEED4_PARAM, 0.f, 1.f, 0.125f, "Speed 4", "x", 8, 0);
        configButton(REVERSE1_PARAM, "Reverse 1");
        configButton(REVERSE2_PARAM, "Reverse 2");
        configButton(REVERSE3_PARAM, "Reverse 3");
        configButton(REVERSE4_PARAM, "Reverse 4");
        configButton(GLOBAL_REC_PARAM, "Record all");
        configButton(GLOBAL_CLEAR_PARAM, "Clear all");
        configButton(GLOBAL_PLAY_PARAM, "Play all");
        configButton(GLOBAL_RESET_PARAM, "Reset all");

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
        configInput(PLAY1_INPUT, "Play 1 trigger");
        configInput(PLAY2_INPUT, "Play 2 trigger");
        configInput(PLAY3_INPUT, "Play 3 trigger");
        configInput(PLAY4_INPUT, "Play 4 trigger");
        configInput(RESET1_INPUT, "Reset 1 trigger");
        configInput(RESET2_INPUT, "Reset 2 trigger");
        configInput(RESET3_INPUT, "Reset 3 trigger");
        configInput(RESET4_INPUT, "Reset 4 trigger");
        configInput(SPEED1_INPUT, "Speed 1");
        configInput(SPEED2_INPUT, "Speed 2");
        configInput(SPEED3_INPUT, "Speed 3");
        configInput(SPEED4_INPUT, "Speed 4");
        configInput(REVERSE1_INPUT, "Reverse 1 trigger");
        configInput(REVERSE2_INPUT, "Reverse 2 trigger");
        configInput(REVERSE3_INPUT, "Reverse 3 trigger");
        configInput(REVERSE4_INPUT, "Reverse 4 trigger");
        configInput(POSITION1_INPUT, "Position 1");
        configInput(POSITION2_INPUT, "Position 2");
        configInput(POSITION3_INPUT, "Position 3");
        configInput(POSITION4_INPUT, "Position 4");
        configInput(GLOBAL_REC_INPUT, "Record all trigger");
        configInput(GLOBAL_CLEAR_INPUT, "Clear all trigger");
        configInput(GLOBAL_PLAY_INPUT, "Play all trigger");
        configInput(GLOBAL_RESET_INPUT, "Reset all trigger");

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
        configLight(PLAY1_LIGHT, "Play 1");
        configLight(PLAY2_LIGHT, "Play 2");
        configLight(PLAY3_LIGHT, "Play 3");
        configLight(PLAY4_LIGHT, "Play 4");
        configLight(RESET1_LIGHT, "Reset 1");
        configLight(RESET2_LIGHT, "Reset 2");
        configLight(RESET3_LIGHT, "Reset 3");
        configLight(RESET4_LIGHT, "Reset 4");
        configLight(REVERSE1_LIGHT, "Reverse 1");
        configLight(REVERSE2_LIGHT, "Reverse 2");
        configLight(REVERSE3_LIGHT, "Reverse 3");
        configLight(REVERSE4_LIGHT, "Reverse 4");
        configLight(GLOBAL_REC_LIGHT, "Record all");
        configLight(GLOBAL_CLEAR_LIGHT, "Clear all");
        configLight(GLOBAL_PLAY_LIGHT, "Play all");
        configLight(GLOBAL_RESET_LIGHT, "Reset all");

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

    // Insert _<n> before the file extension, e.g. "foo.wav" -> "foo_1.wav"
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

    // Clear pending write request
    void clearPendingWrite() {
        pendingWrite = -1;
        writeRequested = false;
    }

    // Clear pending read request
    void clearPendingRead() {
        pendingRead = -1;
        readRequested = false;
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

    // Generate a unique temp path for the given channel
    std::string generateTempPath(int channel) {
        std::string tempDir = system::getTempDirectory();
        // Use random hex to avoid collisions
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        char hex[9];
        for (int i = 0; i < 8; i++)
            hex[i] = "0123456789abcdef"[dis(gen)];
        hex[8] = '\0';
        return system::join(tempDir, "ra-rec-ch" + std::to_string(channel + 1) + "-" + hex + ".wav");
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
        return loadTrackDirect(channel, path);
    }

    bool loadTrackDirect(int channel, const std::string &path) {
        if (!system::isFile(path))
            return false;
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
    // starts immediately; otherwise uses a temp directory.
    void requestRecordStart(int scope) {
        std::string base = getBasePath();
        if (!base.empty()) {
            beginRecording(scope);
        } else {
            // Use temp directory
            std::string tempPath = generateTempPath(0);
            setBasePath(tempPath);
            beginRecording(scope);
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

            // Per-channel play toggle
            float playSig = std::max(params[PLAY1_PARAM + i].getValue(), inputs[PLAY1_INPUT + i].getVoltage());
            if (playTriggers[i].process(playSig))
                playing[i] = !playing[i];

            // Per-channel reset
            float resetSig = std::max(params[RESET1_PARAM + i].getValue(), inputs[RESET1_INPUT + i].getVoltage());
            if (resetTriggers[i].process(resetSig))
                readPositions[i] = 0.f;

            // Per-channel reverse toggle
            float revSig = std::max(params[REVERSE1_PARAM + i].getValue(), inputs[REVERSE1_INPUT + i].getVoltage());
            if (reverseTriggers[i].process(revSig))
                reversePlaying[i] = !reversePlaying[i];

            // Per-channel scrub/position (continuous CV)
            if (inputs[POSITION1_INPUT + i].isConnected() && writePositions[i] > 0) {
                float posVoltage = inputs[POSITION1_INPUT + i].getVoltage();
                // Only update position if changed by more than 0.05V threshold
                if (std::abs(posVoltage - lastPositionVoltage[i]) > 0.05f) {
                    lastPositionVoltage[i] = posVoltage;
                    // 0V = start, 10V = end
                    float pos = clamp(posVoltage / 10.f, 0.f, 1.f);
                    readPositions[i] = pos * (float)writePositions[i];
                    // When not playing, output audio at the scrubbed position for audible scrubbing
                    if (!playing[i]) {
                        outputs[OUT1_OUTPUT + i].setVoltage(interpRead(i, readPositions[i]));
                    }
                }
            }

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

        // ---- Global play (toggle all) ----
        float gPlay = std::max(params[GLOBAL_PLAY_PARAM].getValue(), inputs[GLOBAL_PLAY_INPUT].getVoltage());
        if (globalPlayTrigger.process(gPlay)) {
            bool anyPlaying = playing[0] || playing[1] || playing[2] || playing[3];
            for (int i = 0; i < NUM_CHANNELS; i++)
                playing[i] = !anyPlaying;
        }

        // ---- Global reset ----
        float gReset = std::max(params[GLOBAL_RESET_PARAM].getValue(), inputs[GLOBAL_RESET_INPUT].getVoltage());
        if (globalResetTrigger.process(gReset)) {
            for (int i = 0; i < NUM_CHANNELS; i++)
                readPositions[i] = 0.f;
        }

        // ---- Lights ----
        for (int i = 0; i < NUM_CHANNELS; i++)
            lights[REC1_LIGHT + i].setBrightness(recording[i] ? 1.f : 0.f);
        for (int i = 0; i < NUM_CHANNELS; i++)
            lights[PLAY1_LIGHT + i].setBrightness(playing[i] ? 1.f : 0.f);
        for (int i = 0; i < NUM_CHANNELS; i++)
            lights[REVERSE1_LIGHT + i].setBrightness(reversePlaying[i] ? 1.f : 0.f);
        lights[GLOBAL_REC_LIGHT].setBrightness((recording[0] || recording[1] || recording[2] || recording[3]) ? 1.f : 0.f);
        lights[GLOBAL_PLAY_LIGHT].setBrightness((playing[0] || playing[1] || playing[2] || playing[3]) ? 1.f : 0.f);
        lights[GLOBAL_CLEAR_LIGHT].setBrightness(0.f);

        // ---- Write/Read button handling ----
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (params[WRITE1_PARAM + i].getValue() > 0.f) {
                pendingWrite = i;
                writeRequested = true;
            }
            if (params[READ1_PARAM + i].getValue() > 0.f) {
                pendingRead = i;
                readRequested = true;
            }
        }

        // ---- Outputs ----
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            float in = inputs[IN1_INPUT + ch].getVoltage();
            float out = in; // pass-through

            if (recording[ch]) {
                // Monitor the live input while recording
                out = in;
            } else if (playing[ch]) {
                // Playback, looping within this track's own recorded length
                if (writePositions[ch] > 0) {
                    // Calculate speed: knob (0-1) * 8 = base speed, CV adds up to same amount again
                    float baseSpeed = params[SPEED1_PARAM + ch].getValue() * 8.f;
                    if (inputs[SPEED1_INPUT + ch].isConnected()) {
                        float cvMod = inputs[SPEED1_INPUT + ch].getVoltage() / 10.f;
                        baseSpeed *= (1.f + cvMod);
                    }
                    out = interpRead(ch, readPositions[ch]);
                    // Direction: positive speed for forward, negative for reverse
                    float dir = reversePlaying[ch] ? -1.f : 1.f;
                    readPositions[ch] += baseSpeed * dir;
                    float recordEnd = (float)writePositions[ch];
                    // Handle wrap-around for both directions
                    if (reversePlaying[ch]) {
                        if (readPositions[ch] < 0.f)
                            readPositions[ch] += recordEnd;
                    } else {
                        if (readPositions[ch] >= recordEnd)
                            readPositions[ch] -= recordEnd;
                    }
                } else {
                    out = 0.f;
                }
            }

            outputs[OUT1_OUTPUT + ch].setVoltage(out);
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_t *playingJ = json_array();
        json_t *reversePlayingJ = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            json_array_append_new(playingJ, json_boolean(playing[i]));
            json_array_append_new(reversePlayingJ, json_boolean(reversePlaying[i]));
        }
        json_object_set_new(rootJ, "playing", playingJ);
        json_object_set_new(rootJ, "reversePlaying", reversePlayingJ);
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            if (!basePath.empty())
                json_object_set_new(rootJ, "basePath", json_string(basePath.c_str()));
        }
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *playJ = json_object_get(rootJ, "playing");
        if (playJ && json_is_array(playJ)) {
            for (int i = 0; i < NUM_CHANNELS; i++)
                playing[i] = json_boolean_value(json_array_get(playJ, i));
        }

        json_t *pathJ = json_object_get(rootJ, "basePath");
        if (pathJ && json_is_string(pathJ)) {
            std::string base = json_string_value(pathJ);
            {
                std::lock_guard<std::mutex> lock(pathMutex);
                basePath = base;
            }
            for (int i = 0; i < NUM_CHANNELS; i++) {
                if (loadTrack(i, base))
                    loaded[i] = true;
            }
        }

        json_t *revJ = json_object_get(rootJ, "reversePlaying");
        if (revJ && json_is_array(revJ)) {
            for (int i = 0; i < NUM_CHANNELS; i++)
                reversePlaying[i] = json_boolean_value(json_array_get(revJ, i));
        }

        // Mark that we need to load from storage
        needsStorageLoad = true;
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

            // Playhead marker (show when playing, flash when paused if has content)
            if (module->writePositions[i] > 0) {
                float playPos = module->readPositions[i] / (float)module->writePositions[i];
                float playX = playPos * box.size.x;
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, playX, top + 2);
                nvgLineTo(args.vg, playX, top + laneH - 2);
                nvgStrokeWidth(args.vg, 2.f);
                if (module->playing[i]) {
                    // Solid white when playing
                    nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 200));
                } else {
                    // Flash when paused
                    float t = system::getTime();
                    float flash = (fmodf(t, 1.f) < 0.5f) ? 1.f : 0.2f;
                    nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, (uint8_t)(255.f * flash)));
                }
                nvgStroke(args.vg);
            }
        }
    }

    void drawWaveform(NVGcontext *vg, int channel, float top, float midY, float laneH) {
        // Always draw the full recorded waveform, stretched/shrunk to fit the
        // lane width. As the recording grows the waveform compresses to fit.
        int len = module->writePositions[channel];
        int cap = module->bufferSizes[channel];
        if (cap <= 0 || len <= 0)
            return;
        const auto &buf = module->buffers[channel];

        // Vertical bounds — never let the line leave the lane
        const float plotTop = top + 3;
        const float plotBot = top + laneH - 3;
        const float plotHeight = plotBot - plotTop;

        // Auto-scale: find the peak value in the recording
        float peak = 0.1f;  // Minimum to avoid division by zero
        int samplesToCheck = std::min(len, 1000);  // Sample up to 1000 points for performance
        int step = len / samplesToCheck;
        if (step < 1) step = 1;
        for (int i = 0; i < len; i += step) {
            float v = std::abs(buf[i]);
            if (v > peak) peak = v;
        }
        // Add 10% headroom
        peak *= 1.1f;

        // Map ±peak to the lane height
        const float scale = plotHeight / 2.f / peak;

        nvgBeginPath(vg);
        int n = (int)box.size.x;
        for (int x = 0; x < n; x++) {
            float t = (float)x / (float)n;
            int idx = (int)(t * (float)len);
            if (idx >= len) idx = len - 1;
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

struct CyanLight : GrayModuleLightWidget {
    CyanLight() {
        addBaseColor(nvgRGB(0x33, 0x88, 0xdd));
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
        // Positions from SVG: four groups at x=22.35, 48.77, 75.18, 101.60, y=14.22 (lights), y=21 (ports)
        float gx[4] = {22.35f, 48.77f, 75.18f, 101.60f};
        float gLightY = 14.22f;
        float gTrigY = 21.00f;

        addParam(createLightParamCentered<VCVLightBezel<RedLight>>(mm2px(Vec(gx[0], gLightY)), module, RaRecModule::GLOBAL_REC_PARAM, RaRecModule::GLOBAL_REC_LIGHT));
        addInput(createInputCentered<RaPort>(mm2px(Vec(gx[0], gTrigY)), module, RaRecModule::GLOBAL_REC_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<YellowLight>>(mm2px(Vec(gx[1], gLightY)), module, RaRecModule::GLOBAL_CLEAR_PARAM, RaRecModule::GLOBAL_CLEAR_LIGHT));
        addInput(createInputCentered<RaPort>(mm2px(Vec(gx[1], gTrigY)), module, RaRecModule::GLOBAL_CLEAR_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(mm2px(Vec(gx[2], gLightY)), module, RaRecModule::GLOBAL_PLAY_PARAM, RaRecModule::GLOBAL_PLAY_LIGHT));
        addInput(createInputCentered<RaPort>(mm2px(Vec(gx[2], gTrigY)), module, RaRecModule::GLOBAL_PLAY_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(mm2px(Vec(gx[3], gLightY)), module, RaRecModule::GLOBAL_RESET_PARAM, RaRecModule::GLOBAL_RESET_LIGHT));
        addInput(createInputCentered<RaPort>(mm2px(Vec(gx[3], gTrigY)), module, RaRecModule::GLOBAL_RESET_INPUT));

        // ---- Display ----
        // Display: y=31.5, height=84mm gives 21mm lanes with centers at 42, 63, 84, 105mm
        // This aligns with the per-channel control rows (yRow1 at 42/63/84/105, yRow2 at 48/69/90/111)
        auto *display = new TrackScopeDisplay();
        display->box.pos = mm2px(Vec(88.f, 31.5f));
        display->box.size = mm2px(Vec(125.f, 84.f));
        display->module = module;
        addChild(display);

        // ---- Per-track controls ----
        // Left side: input jack + buttons (Rec, Clr, Play, Rst, Speed knob)
        // Left triggers: Rec, Clr, Play, Rst, Speed CV, Pos
        // Right side: output jack + Wr/Rd buttons
        float colIn = 8.f;
        float colRec = 19.f;
        float colClr = 30.f;
        float colPly = 41.f;
        float colRst = 52.f;
        float colSpeedKnob = 63.f;
        float colSpeedCV = 63.f;
        float colRev = 74.f;
        float colPos = 8.f;
        float colTrigRec = 19.f;
        float colTrigClr = 30.f;
        float colTrigPly = 41.f;
        float colTrigRst = 52.f;
        float colWr = 233.f;
        float colOut = 244.f;

        for (int i = 0; i < 4; i++) {
            float yRow1 = 42.f + i * 21.f;
            float yRow2 = 48.f + i * 21.f;

            // Row 1: Input jack + buttons (Rec, Clr, Play, Rst, Speed knob)
            addInput(createInputCentered<RaPort>(mm2px(Vec(colIn, yRow1)), module, RaRecModule::IN1_INPUT + i));
            addParam(createLightParamCentered<VCVLightBezel<RedLight>>(mm2px(Vec(colRec, yRow1)), module, RaRecModule::REC1_PARAM + i, RaRecModule::REC1_LIGHT + i));
            addParam(createParamCentered<RaButton>(mm2px(Vec(colClr, yRow1)), module, RaRecModule::CLEAR1_PARAM + i));
            addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(mm2px(Vec(colPly, yRow1)), module, RaRecModule::PLAY1_PARAM + i, RaRecModule::PLAY1_LIGHT + i));
            addParam(createParamCentered<RaButton>(mm2px(Vec(colRst, yRow1)), module, RaRecModule::RESET1_PARAM + i));
            addParam(createParamCentered<RaKnobSmall>(mm2px(Vec(colSpeedKnob, yRow1)), module, RaRecModule::SPEED1_PARAM + i));

            // Row 2: Triggers (Rec, Clr, Play, Rst) + Speed CV + Rev trig + Pos + Wr/Rd + Out
            addInput(createInputCentered<RaPort>(mm2px(Vec(colTrigRec, yRow2 + 5.f)), module, RaRecModule::REC1_INPUT + i));
            addInput(createInputCentered<RaPort>(mm2px(Vec(colTrigClr, yRow2 + 5.f)), module, RaRecModule::CLEAR1_INPUT + i));
            addInput(createInputCentered<RaPort>(mm2px(Vec(colTrigPly, yRow2 + 5.f)), module, RaRecModule::PLAY1_INPUT + i));
            addInput(createInputCentered<RaPort>(mm2px(Vec(colTrigRst, yRow2 + 5.f)), module, RaRecModule::RESET1_INPUT + i));
            addInput(createInputCentered<RaPort>(mm2px(Vec(colSpeedCV, yRow2 + 5.f)), module, RaRecModule::SPEED1_INPUT + i));
            addParam(createLightParamCentered<VCVLightBezel<CyanLight>>(mm2px(Vec(colRev, yRow1)), module, RaRecModule::REVERSE1_PARAM + i, RaRecModule::REVERSE1_LIGHT + i));
            addInput(createInputCentered<RaPort>(mm2px(Vec(colRev, yRow2 + 5.f)), module, RaRecModule::REVERSE1_INPUT + i));
            addInput(createInputCentered<RaPort>(mm2px(Vec(colPos, yRow2 + 5.f)), module, RaRecModule::POSITION1_INPUT + i));
            addParam(createParamCentered<RaButton>(mm2px(Vec(colWr, yRow1)), module, RaRecModule::WRITE1_PARAM + i));
            addParam(createParamCentered<RaButton>(mm2px(Vec(colWr, yRow2 + 3.f)), module, RaRecModule::READ1_PARAM + i));
            addOutput(createOutputCentered<RaPort>(mm2px(Vec(colOut, yRow2)), module, RaRecModule::OUT1_OUTPUT + i));
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
            std::string fname = existing.empty() ? "recording.wav" : getFilename(existing);

            char *pathC = osdialog_file(OSDIALOG_SAVE,
                                        dir.empty() ? nullptr : dir.c_str(),
                                        fname.empty() ? nullptr : fname.c_str(),
                                        osdialog_filters_parse("WAV Audio:wav"));
            if (pathC) {
                m->setBasePath(pathC);
                free(pathC);
            } else {
                // Cancelled — discard the pending start
                m->clearPendingStart();
            }
        }

        // Handle write dialog
        if (m->writeRequested.exchange(false)) {
            int ch = m->pendingWrite;
            if (ch >= 0 && ch < NUM_CHANNELS) {
                char *pathC = osdialog_file(OSDIALOG_SAVE,
                                            nullptr, nullptr,
                                            osdialog_filters_parse("WAV Audio:wav"));
                if (pathC) {
                    std::string path(pathC);
                    free(pathC);
                    // Save the channel's recording to the chosen path
                    int numSamples = m->writePositions[ch];
                    if (numSamples > 0) {
                        int sr = (int)APP->engine->getSampleRate();
                        writeRecording(path, m->buffers[ch].data(), numSamples, sr);
                    }
                }
                m->clearPendingWrite();
            }
        }

        // Handle read dialog
        if (m->readRequested.exchange(false)) {
            int ch = m->pendingRead;
            if (ch >= 0 && ch < NUM_CHANNELS) {
                char *pathC = osdialog_file(OSDIALOG_OPEN,
                                            nullptr, nullptr,
                                            osdialog_filters_parse("WAV Audio:wav"));
                if (pathC) {
                    std::string path(pathC);
                    free(pathC);
                    // Load the file into the channel
                    int fileSampleRate = 0;
                    auto samples = readRecording(path, fileSampleRate);
                    if (!samples.empty()) {
                        int targetSize = m->bufferSizes[ch];
                        m->buffers[ch].assign(targetSize, 0.f);

                        if (fileSampleRate == (int)APP->engine->getSampleRate()) {
                            int copyLen = std::min((int)samples.size(), targetSize);
                            for (int i = 0; i < copyLen; i++)
                                m->buffers[ch][i] = samples[i];
                            m->writePositions[ch] = copyLen;
                        } else {
                            float ratio = (float)fileSampleRate / (float)APP->engine->getSampleRate();
                            int writePos = 0;
                            for (float srcPos = 0.f; srcPos < (float)samples.size() && writePos < targetSize; srcPos += ratio) {
                                int i0 = (int)srcPos;
                                int i1 = std::min(i0 + 1, (int)samples.size() - 1);
                                float frac = srcPos - (float)i0;
                                m->buffers[ch][writePos] = samples[i0] + frac * (samples[i1] - samples[i0]);
                                writePos++;
                            }
                            m->writePositions[ch] = writePos;
                        }
                        m->readPositions[ch] = 0.f;
                        m->loaded[ch] = true;

                        // Save to storage for persistence
                        std::string storageDir = system::getTempDirectory();
                        std::string storagePath = system::join(storageDir, "ra-rec-track_" + std::to_string(ch) + ".wav");
                        int sr = (int)APP->engine->getSampleRate();
                        writeRecording(storagePath, m->buffers[ch].data(), m->writePositions[ch], sr);
                    }
                }
                m->clearPendingRead();
            }
        }

        // Load from storage if needed
        if (m->needsStorageLoad) {
            m->needsStorageLoad = false;
            std::string storageDir = system::getTempDirectory();
            for (int i = 0; i < NUM_CHANNELS; i++) {
                std::string path = system::join(storageDir, "ra-rec-track_" + std::to_string(i) + ".wav");
                if (system::isFile(path)) {
                    if (m->loadTrackDirect(i, path))
                        m->loaded[i] = true;
                }
            }
        }
    }
};

Model *modelRaRec = createModel<RaRecModule, RaRecWidget>("ra-rec");
