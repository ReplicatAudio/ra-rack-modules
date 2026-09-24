// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: MIX_PARAM "Mix"
// fname: LEVEL_PARAM "Level"
// fname: PREDELAY_PARAM "Predelay"
// fname: LENGTH_PARAM "Length"
// fname: DAMP_PARAM "Damp"
// fname: ATTACK_PARAM "Attack"
// fname: DECAY_PARAM "Decay"
// fname: GATE_PARAM "Gate"
// fname: LOAD_PARAM "Load IR"
// fname: L_AUDIO_INPUT "In L"
// fname: R_AUDIO_INPUT "In R"
// fname: MIX_CV_INPUT "Mix CV"
// fname: LEVEL_CV_INPUT "Level CV"
// fname: GATE_CV_INPUT "Gate CV"
// fname: PREDELAY_CV_INPUT "Predelay CV"
// fname: ATTACK_CV_INPUT "Attack CV"
// fname: DECAY_CV_INPUT "Decay CV"
// fname: DAMP_CV_INPUT "Damp CV"
// fname: L_AUDIO_OUTPUT "Out L"
// fname: R_AUDIO_OUTPUT "Out R"
// fname: GATE_LIGHT "Gate"
#include "ra-components.hpp"

#include <osdialog.h>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <cmath>
#include <mutex>

using namespace rack;
using namespace rack::system;

extern Plugin *pluginInstance;

// Convolution engine: uniformly partitioned convolution via RealTimeConvolver.
// Each KW channel runs its own instance so L and R process independently.
static constexpr int BLOCK_SIZE = 2048; // FFT block size (power of two)
static constexpr float MAX_PREDELAY_S = 0.5f; // seconds

// IR screen (right-hand column of the widget): a waveform + spectrogram.
// The display data is precomputed at load time into fixed-resolution arrays.
// Both plots run vertically (time top->bottom) to match the tall, narrow module.
static const int DISP_WAVE_ROWS = 128; // waveform time rows (top->bottom)
static const int SPEC_XBINS = 96;      // spectrogram freq bins across (left->right)
static const int SPEC_YROWS = 128;     // spectrogram time rows (top->bottom)
static const int SPEC_FFT = 256;       // FFT size used for the spectrogram

// ---------------------------------------------------------------------------
// Minimal WAV reader. Returns mono samples (channel 0) from a RIFF/WAVE file.
// Supports: 16/24-bit PCM (format 1) and 32-bit IEEE float (format 3).
// ---------------------------------------------------------------------------
static std::vector<float> parseWav(const std::vector<uint8_t> &data, int &outRate) {
	outRate = 0;
	if (data.size() < 44) return {};
	if (memcmp(data.data(), "RIFF", 4) != 0) return {};
	if (memcmp(data.data() + 8, "WAVE", 4) != 0) return {};

	const uint8_t *d = data.data();
	size_t size = data.size();
	size_t pos = 12;

	int fmtFormat = 0, fmtChannels = 0, fmtRate = 0, fmtBits = 0;

	while (pos + 8 <= size) {
		char id[5];
		memcpy(id, &d[pos], 4);
		id[4] = 0;
		pos += 4;
		uint32_t cksz = ((uint32_t)d[pos]) | ((uint32_t)d[pos+1] << 8)
			| ((uint32_t)d[pos+2] << 16) | ((uint32_t)d[pos+3] << 24);
		pos += 4;

		if (memcmp(id, "fmt ", 4) == 0) {
			const uint8_t *f = &d[pos];
			if (cksz >= 16) {
				fmtFormat  = (int)((uint16_t)f[0] | ((uint16_t)f[1] << 8));
				fmtChannels = (int)((uint16_t)f[2] | ((uint16_t)f[3] << 8));
				fmtRate    = (int)((uint32_t)f[4] | ((uint32_t)f[5] << 8) | ((uint32_t)f[6] << 16) | ((uint32_t)f[7] << 24));
				fmtBits    = (int)((uint16_t)f[14] | ((uint16_t)f[15] << 8));
			}
		}
		else if (memcmp(id, "data", 4) == 0) {
			if (fmtRate <= 0 || fmtChannels <= 0 || fmtBits <= 0) return {};
			int bytesPerSample = fmtBits / 8;
			int bytesPerFrame = bytesPerSample * fmtChannels;
			if (bytesPerFrame <= 0) return {};
			int frameCount = (int)(cksz / bytesPerFrame);
			const uint8_t *p = &d[pos];
			std::vector<float> mono(frameCount);
			for (int i = 0; i < frameCount; i++) {
				const uint8_t *s = p + (size_t)i * bytesPerFrame; // channel 0
				float v = 0.f;
				if (fmtFormat == 3) {
					// IEEE float (32-bit)
					memcpy(&v, s, sizeof(float));
				}
				else if (fmtFormat == 1) {
					if (bytesPerSample == 2) {
						int16_t raw = (int16_t)((uint16_t)s[0] | ((uint16_t)s[1] << 8));
						v = (float)raw / 32768.f;
					}
					else if (bytesPerSample == 3) {
						int32_t raw = ((int32_t)s[0]) | ((int32_t)s[1] << 8) | ((int32_t)s[2] << 16);
						if (raw & 0x800000) raw |= 0xFF000000;
						v = (float)raw / 8388608.f;
					}
					else if (bytesPerSample == 4) {
						int32_t raw = ((int32_t)s[0]) | ((int32_t)s[1] << 8) | ((int32_t)s[2] << 16) | ((int32_t)s[3] << 24);
						v = (float)raw / 2147483648.f;
					}
				}
				mono[i] = v;
			}
			outRate = fmtRate;
			return mono;
		}

		pos += cksz + (cksz & 1); // pad to word boundary
	}
	return {};
}

// ---------------------------------------------------------------------------
// Linear-interpolation resampler (fine for IRs).
// ---------------------------------------------------------------------------
static std::vector<float> resampleIR(const std::vector<float> &in, int inRate, int outRate) {
	size_t n = in.size();
	if (n == 0 || inRate <= 0 || outRate <= 0 || inRate == outRate)
		return in;
	double ratio = (double)outRate / (double)inRate;
	size_t outN = (size_t)std::ceil((double)n * ratio);
	std::vector<float> out(outN);
	for (size_t i = 0; i < outN; i++) {
		double posD = (double)i / ratio;
		size_t i0 = (size_t)posD;
		if (i0 >= n) i0 = n - 1;
		float frac = (float)(posD - (double)i0);
		size_t i1 = (i0 + 1 < n) ? i0 + 1 : i0;
		out[i] = in[i0] + (in[i1] - in[i0]) * frac;
	}
	return out;
}

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------
struct RaReverbirModule : Module {
	enum ParamIds {
		MIX_PARAM,
		LEVEL_PARAM,
		PREDELAY_PARAM,
		LENGTH_PARAM,
		DAMP_PARAM,
		ATTACK_PARAM,
		DECAY_PARAM,
		GATE_PARAM,
		LOAD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		L_AUDIO_INPUT,
		R_AUDIO_INPUT,
		MIX_CV_INPUT,
		LEVEL_CV_INPUT,
		PREDELAY_CV_INPUT,
		ATTACK_CV_INPUT,
		DECAY_CV_INPUT,
		DAMP_CV_INPUT,
		GATE_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		L_AUDIO_OUTPUT,
		R_AUDIO_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LOAD_LIGHT,
		GATE_LIGHT,
		NUM_LIGHTS
	};

	// Uniformly partitioned convolution engine, one per stereo channel.
	std::unique_ptr<dsp::RealTimeConvolver> conv[2];
	// Block input/output accumulation.
	float inBuf[2][BLOCK_SIZE] = {};
	float outBuf[2][BLOCK_SIZE] = {};
	int blockPos[2] = {0, 0};
	// Guards kernel swaps against concurrent block processing.
	std::mutex dspMutex;

	// Predelay (integer-sample) per channel.
	std::vector<float> predelayBuf[2];
	int predelayWpos[2] = {0, 0};
	int predelayLen = 0;

	// Wet-path shaping.
	float env = 0.f;          // attack/decay envelope (0..1)
	float levelSmooth = 0.f;  // smoothed input level for envelope follower
	float dampState[2] = {0.f, 0.f};
	int fadeRemain = 0;       // crossfade count after kernel swap

	// IR state. `srcIR` holds the file at its original sample rate so it can
	// be re-resampled if the engine sample rate ever changes.
	// `srcIRLen` is an atomic mirror of srcIR.size() so the audio thread can
	// cheaply detect an IR swap without locking.
	std::string irPath;
	std::vector<float> srcIR;
	int srcIRRate = 0;
	std::atomic<int> srcIRLen{0};
	float builtRate = 0.f;
	int builtLen = 0;
	int builtTotal = 0;      // resampled sample count at build time
	int builtSrcIRLen = 0;   // srcIRLen value the current kernel was built from

	// Cached fully-resampled, peak-normalized kernel. Reused across Length
	// changes so only the trim + FFT are redone (not the resample + normalize).
	std::vector<float> fullKernel;
	int fullKernelRate = 0;      // engine rate fullKernel was built at
	int fullKernelSrcIRLen = 0;  // srcIRLen it was built from

	// UI<->audio coordination for the load dialog.
	std::atomic<bool> loadRequested{false};
	dsp::SchmittTrigger loadTrigger;

	float loadLight = 0.f;

	// IR screen data, precomputed on the UI thread when an IR is loaded.
	bool displayReady = false;
	std::vector<float> dispWaveMin, dispWaveMax; // waveform envelope (peak-normalized -1..1)
	std::vector<uint8_t> dispImage;              // RGBA spectrogram framebuffer
	int dispImageW = 0, dispImageH = 0;

	RaReverbirModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix", "%", 0.f, 100.f);
		configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0.f, 100.f);
		configParam(PREDELAY_PARAM, 0.f, MAX_PREDELAY_S, 0.f, "Predelay", " ms", 0.f, 1000.f);
		configParam(LENGTH_PARAM, 0.f, 1.f, 0.5f, "Length", "%", 0.f, 100.f);
		configParam(DAMP_PARAM, 0.f, 1.f, 0.5f, "Damp", "%", 0.f, 100.f);
		configParam(ATTACK_PARAM, 0.f, 1.f, 0.f, "Attack", "%", 0.f, 100.f);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay", "%", 0.f, 100.f);
		configParam(GATE_PARAM, 0.f, 1.f, 0.1f, "Gate", "%", 0.f, 100.f);
		configButton(LOAD_PARAM, "Load IR");

		configInput(L_AUDIO_INPUT, "Audio L");
		configInput(R_AUDIO_INPUT, "Audio R");
		configInput(MIX_CV_INPUT, "Mix CV");
		configInput(LEVEL_CV_INPUT, "Level CV");
		configInput(PREDELAY_CV_INPUT, "Predelay CV");
		configInput(ATTACK_CV_INPUT, "Attack CV");
		configInput(DECAY_CV_INPUT, "Decay CV");
		configInput(DAMP_CV_INPUT, "Damp CV");
		configInput(GATE_CV_INPUT, "Gate CV");

		configOutput(L_AUDIO_OUTPUT, "Audio L");
		configOutput(R_AUDIO_OUTPUT, "Audio R");

		configLight(LOAD_LIGHT, "Loading");
		configLight(GATE_LIGHT, "Gate");

		conv[0].reset(new dsp::RealTimeConvolver(BLOCK_SIZE));
		conv[1].reset(new dsp::RealTimeConvolver(BLOCK_SIZE));
	}

	~RaReverbirModule() {
		// RealTimeConvolver destructor frees pffft buffers.
	}

	// Rebuild the convolution kernel from the loaded IR. The full IR is
	// resampled and peak-normalized once and cached; a Length change only
	// re-trims the cached kernel. Caller must already hold dspMutex (protects
	// srcIR/fullKernel access and the convolver kernel swap against the
	// UI-thread load).
	void rebuildKernel(float sampleRate, float lengthParam) {
		if (srcIRRate <= 0 || srcIR.empty())
			return;

		// Resample + normalize only when the source IR or engine rate changed.
		int irLen = srcIRLen.load(std::memory_order_relaxed);
		if (fullKernelRate != (int)sampleRate || fullKernelSrcIRLen != irLen) {
			std::vector<float> k = resampleIR(srcIR, srcIRRate, (int)sampleRate);
			// Power/energy normalize so the wet tail is level-matched with the dry
			// signal: a unit-RMS (unit-power) dry input through the reverb yields a
			// unit-RMS wet output. Peak-normalization is wrong for convolution reverb
			// — a dense IR sums many overlapping taps, so a sustained dry signal
			// comes back several times hotter than source. Normalizing so
			// sum(k^2) == 1 keeps wet ~ dry for every IR (sparse or dense, short or
			// long). Level knob then controls the final trim.
			double sumsq = 0.0;
			for (float x : k)
				sumsq += (double)x * (double)x;
			double g = (sumsq > 1e-12) ? 1.0 / std::sqrt(sumsq) : 1.0;
			if (g != 1.0)
				for (float &x : k) x = (float)((double)x * g);
			fullKernel = std::move(k);
			fullKernelRate = (int)sampleRate;
			fullKernelSrcIRLen = irLen;
		}

		int total = (int)fullKernel.size();
		int len = (int)std::ceil(total * clamp(lengthParam, 0.f, 1.f));
		len = std::max(1, std::min(len, total));

		conv[0]->setKernel(fullKernel.data(), len);
		conv[1]->setKernel(fullKernel.data(), len);
		builtLen = len;
		builtTotal = total;
		builtRate = sampleRate;
		builtSrcIRLen = fullKernelSrcIRLen;
		fadeRemain = BLOCK_SIZE;
	}

	void process(const ProcessArgs &args) override {
		float sampleRate = args.sampleRate;

		// Button -> request the file dialog.
		if (loadTrigger.process(params[LOAD_PARAM].getValue())) {
			loadRequested.store(true, std::memory_order_relaxed);
			loadLight = 1.f;
		}
		loadLight = std::max(0.f, loadLight - args.sampleTime * 2.f);
		lights[LOAD_LIGHT].setBrightness(loadLight);

		// Rebuild the kernel only when needed: IR swap, sample-rate change, or
		// Length change. The length basis is `builtTotal` (the resampled sample
		// count), NOT the raw srcIR length, so the wantLen/builtLen comparison
		// is consistent even when the engine rate differs from the IR file rate.
		int irLen = srcIRLen.load(std::memory_order_relaxed);
		if (irLen > 0) {
			if (sampleRate != builtRate || irLen != builtSrcIRLen) {
				std::lock_guard<std::mutex> lock(dspMutex);
				int r0 = srcIRLen.load(std::memory_order_relaxed);
				if (r0 > 0 && (sampleRate != builtRate || r0 != builtSrcIRLen)) {
					rebuildKernel(sampleRate, params[LENGTH_PARAM].getValue());
				}
			}
			else {
				float lengthParam = params[LENGTH_PARAM].getValue();
				int wantLen = std::max(1, (int)std::ceil((double)builtTotal * clamp(lengthParam, 0.f, 1.f)));
				if (wantLen != builtLen) {
					std::lock_guard<std::mutex> lock(dspMutex);
					if (srcIRLen.load(std::memory_order_relaxed) == builtSrcIRLen) {
						int wl = std::max(1, (int)std::ceil((double)builtTotal * clamp(params[LENGTH_PARAM].getValue(), 0.f, 1.f)));
						if (wl != builtLen)
							rebuildKernel(sampleRate, params[LENGTH_PARAM].getValue());
					}
				}
			}
		}

		// Params / CV.
		float mix = params[MIX_PARAM].getValue();
		if (inputs[MIX_CV_INPUT].isConnected())
			mix = clamp(mix + inputs[MIX_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		float level = params[LEVEL_PARAM].getValue();
		if (inputs[LEVEL_CV_INPUT].isConnected())
			level = clamp(level * inputs[LEVEL_CV_INPUT].getVoltage() / 10.f, 0.f, 2.f);

		// Predelay (seconds). CV offsets the 0..1 knob fraction.
		float predelayNorm = params[PREDELAY_PARAM].getValue() / MAX_PREDELAY_S;
		if (inputs[PREDELAY_CV_INPUT].isConnected())
			predelayNorm = clamp(predelayNorm + inputs[PREDELAY_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		float predelayTime = predelayNorm * MAX_PREDELAY_S;

		// Attack / Decay (0..1, CV offsets on top).
		float attack = params[ATTACK_PARAM].getValue();
		if (inputs[ATTACK_CV_INPUT].isConnected())
			attack = clamp(attack + inputs[ATTACK_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		float decay = params[DECAY_PARAM].getValue();
		if (inputs[DECAY_CV_INPUT].isConnected())
			decay = clamp(decay + inputs[DECAY_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		float damp = params[DAMP_PARAM].getValue();
		if (inputs[DAMP_CV_INPUT].isConnected())
			damp = clamp(damp + inputs[DAMP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

		// Gate threshold (Volts of rectified dry input). Knob maps 0..1 across
		// a 0..5V range; CV adds on top.
		float gate = params[GATE_PARAM].getValue();
		if (inputs[GATE_CV_INPUT].isConnected())
			gate = clamp(gate + inputs[GATE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		float gateThreshold = gate * 5.f;

		// Predelay buffer sizing (integer samples).
		predelayLen = (int)std::round(predelayTime * sampleRate);
		int cap = (int)std::ceil(sampleRate * MAX_PREDELAY_S);
		for (int ch = 0; ch < 2; ch++) {
			if ((int)predelayBuf[ch].size() != cap) {
				predelayBuf[ch].resize(cap, 0.f);
				predelayWpos[ch] = 0;
			}
		}

		// Attack/decay envelope rates (set from params each block).
		float attackTime = 0.001f + attack * 2.0f;
		float decayTime  = 0.05f  + decay * 19.95f;
		float attackRate = 1.f / attackTime;
		float decayRate  = 1.f / decayTime;

		// Damp one-pole coefficient (wet lowpass). Maps DAMP 0->1 so cutoff
		// runs ~20kHz down to ~57Hz.
		float fc = 20000.f * std::exp(-damp * 6.f);
		float dampCoeff = 1.f - std::exp(-2.f * M_PI * fc / sampleRate);
		dampCoeff = clamp(dampCoeff, 0.f, 0.9999f);

		// Fade-in gain after a kernel swap.
		float fadeGain = 1.f;
		if (fadeRemain > 0) {
			fadeGain = 1.f - (float)fadeRemain / (float)BLOCK_SIZE;
			fadeRemain--;
		}

		for (int ch = 0; ch < 2; ch++) {
			float dryIn = inputs[ch == 0 ? L_AUDIO_INPUT : R_AUDIO_INPUT].getVoltage();

			// Predelay: integer-sample delay before the convolver.
			if (predelayLen > 0 && (int)predelayBuf[ch].size() > 0) {
				predelayBuf[ch][predelayWpos[ch]] = dryIn;
				int readPos = predelayWpos[ch] - predelayLen;
				if (readPos < 0) readPos += (int)predelayBuf[ch].size();
				dryIn = predelayBuf[ch][readPos];
				predelayWpos[ch]++;
				if (predelayWpos[ch] >= (int)predelayBuf[ch].size()) predelayWpos[ch] = 0;
			}

			// Accumulate into the input block.
			inBuf[ch][blockPos[ch]] = dryIn;
			float wetRaw = outBuf[ch][blockPos[ch]];
			blockPos[ch]++;
			if (blockPos[ch] == BLOCK_SIZE) {
				std::lock_guard<std::mutex> lock(dspMutex);
				conv[ch]->processBlock(inBuf[ch], outBuf[ch]);
				blockPos[ch] = 0;
			}

			// Wet damp (one-pole lowpass).
			dampState[ch] += dampCoeff * (wetRaw - dampState[ch]);
			wetRaw = dampState[ch];

			// Output mix (dry is the pre-predelay input so the direct signal
			// is ungated; wet is the convolved, envelope-shaped tail).
			float dry = inputs[ch == 0 ? L_AUDIO_INPUT : R_AUDIO_INPUT].getVoltage();
			float wet = wetRaw * env * fadeGain;
			float out = (1.f - mix) * dry + mix * wet;
			out = std::isfinite(out) ? out * level : 0.f;

			outputs[ch == 0 ? L_AUDIO_OUTPUT : R_AUDIO_OUTPUT].setVoltage(out);
		}

		// Envelope follower driven by the (left) dry input level -> attack/decay.
		float lvlIn = inputs[L_AUDIO_INPUT].getVoltage();
		float dryAbs = std::fabs(lvlIn);
		float followerCoeff = 1.f - std::exp(-(1.f / 0.002f) * args.sampleTime);
		levelSmooth += followerCoeff * (dryAbs - levelSmooth);
		bool hot = levelSmooth > gateThreshold;
		float rate = hot ? attackRate : decayRate;
		float target = hot ? 1.f : 0.f;
		float k = 1.f - std::exp(-rate * args.sampleTime);
		env += (target - env) * k;
		env = clamp(env, 0.f, 1.f);

		// Gate indicator: lit while the gate is held open. Brightness tapers a
		// touch with how far the envelope has decayed so it's visibly distinct
		// from the steady-state "load" light.
		lights[GATE_LIGHT].setBrightness(hot ? 1.f : env);

		outputs[L_AUDIO_OUTPUT].setChannels(1);
		outputs[R_AUDIO_OUTPUT].setChannels(1);
	}

	// Called from the UI thread (via the widget). Reads the WAV and swaps IR.
	void loadIR(const std::string &path) {
		std::vector<uint8_t> data = readFile(path);
		int wavRate = 0;
		std::vector<float> mono = parseWav(data, wavRate);
		if (mono.empty() || wavRate <= 0) {
			// Failed to parse — keep the previous IR (if any) loaded.
			return;
		}
		{
			std::lock_guard<std::mutex> lock(dspMutex);
			srcIR = std::move(mono);
			srcIRRate = wavRate;
			srcIRLen.store((int)srcIR.size(), std::memory_order_relaxed);
			irPath = path;
			builtLen = 0; // force rebuild with current rate / length
		}
		updateIRDisplay();
	}

		// Precompute the waveform envelope + spectrogram for the IR screen.
	// Runs on the UI thread only (called from loadIR) and only touches the
	// UI-side display buffers, so it needs no lock.
	void updateIRDisplay() {
		dispImageW = SPEC_XBINS;
		dispImageH = SPEC_YROWS;
		dispImage.assign((size_t)SPEC_XBINS * SPEC_YROWS * 4, 0);
		dispWaveMin.assign(DISP_WAVE_ROWS, 0.f);
		dispWaveMax.assign(DISP_WAVE_ROWS, 0.f);
		displayReady = false;

		if (srcIR.empty() || srcIRRate <= 0)
			return;

		const std::vector<float> &ir = srcIR;
		int len = (int)ir.size();
		float sr = (float)srcIRRate;

		// ---- Waveform envelope (min/max per time row, peak normalized).
		// Time runs down the screen, so row 0 = IR start.
		int per = std::max(1, len / DISP_WAVE_ROWS);
		for (int r = 0; r < DISP_WAVE_ROWS; r++) {
			int i0 = r * per;
			int i1 = std::min(len, i0 + per);
			if (i1 <= i0) {
				dispWaveMin[r] = 0.f;
				dispWaveMax[r] = 0.f;
				continue;
			}
			float mn = 1e30f, mx = -1e30f;
			for (int i = i0; i < i1; i++) {
				float v = ir[i];
				if (v < mn) mn = v;
				if (v > mx) mx = v;
			}
			dispWaveMin[r] = mn;
			dispWaveMax[r] = mx;
		}
		float peak = 1e-9f;
		for (int r = 0; r < DISP_WAVE_ROWS; r++)
			peak = std::max(peak, std::max(std::fabs(dispWaveMin[r]), std::fabs(dispWaveMax[r])));
		if (peak > 1e-9f) {
			float g = 1.f / peak;
			for (int r = 0; r < DISP_WAVE_ROWS; r++) {
				dispWaveMin[r] *= g;
				dispWaveMax[r] *= g;
			}
		}

		// ---- Spectrogram (short-time FFT, log frequency axis).
		// Time runs down the screen (SPEC_YROWS rows), frequency across it
		// (SPEC_XBINS columns, low->high left->right).
		int fftN = 32;
		while (fftN < std::min(SPEC_FFT, len))
			fftN <<= 1;
		if (fftN < 32)
			return;
		int numBins = fftN / 2;

		dsp::RealFFT fft(fftN);
		float* fftIn  = (float*)pffft_aligned_malloc(fftN * sizeof(float));
		float* fftOut = (float*)pffft_aligned_malloc(2 * fftN * sizeof(float));
		std::vector<float> hann(fftN);
		for (int i = 0; i < fftN; i++)
			hann[i] = 0.5f * (1.f - std::cosf(2.f * M_PI * i / (fftN - 1)));

		float binSpacing = sr / (float)fftN;
		float logMin = std::logf(30.f);
		float logRange = std::logf(std::max(0.5f * sr, 31.f)) - logMin;

		int nFrames = std::max(0, len - fftN);
		std::vector<float> mag(numBins);
		for (int ty = 0; ty < SPEC_YROWS; ty++) {
			int start = nFrames > 0 ? (int)((float)ty * (float)nFrames / (float)(SPEC_YROWS - 1)) : 0;
			for (int i = 0; i < fftN; i++)
				fftIn[i] = ir[start + i] * hann[i];
			fft.rfft(fftIn, fftOut);

			mag[0] = std::fabs(fftOut[0]);
			for (int i = 1; i < numBins; i++) {
				float re = fftOut[2 * i];
				float im = fftOut[2 * i + 1];
				mag[i] = std::sqrt(re * re + im * im);
			}

			for (int fx = 0; fx < SPEC_XBINS; fx++) {
				float xNorm = (float)fx / (float)(SPEC_XBINS - 1); // low->high
				float freq = std::expf(logMin + xNorm * logRange);
				int bin = clamp((int)(freq / binSpacing), 0, numBins - 1);
				float db = 20.f * std::log10f(mag[bin] + 1e-6f);
				float norm = clamp(rescale(db, -60.f, 0.f, 0.f, 1.f), 0.f, 1.f);

				// purple -> green colormap: dim purple, mid purple->green, bright green -> white
				float v = std::pow(norm, 0.45f);
				int r, g, b;
				if (norm <= 0.f) { r = g = b = 0; }
				else if (norm < 0.33f) {
					float a = v * norm / 0.33f;
					r = (int)(a * 90.f); g = (int)(a * 20.f); b = (int)(a * 170.f);
				}
				else if (norm < 0.66f) {
					float a = (norm - 0.33f) / 0.33f;
					r = (int)(v * (150.f * (1.f - a) + 30.f * a));
					g = (int)(v * (20.f + 220.f * a));
					b = (int)(v * (150.f * (1.f - a)));
				}
				else {
					float a = (norm - 0.66f) / 0.34f;
					r = (int)(v * (30.f + 210.f * a));
					g = (int)(v * 255.f);
					b = (int)(v * (60.f + 190.f * a));
				}
				r = clamp(r, 0, 255); g = clamp(g, 0, 255); b = clamp(b, 0, 255);

				int idx = (ty * SPEC_XBINS + fx) * 4;
				dispImage[idx + 0] = (uint8_t)r;
				dispImage[idx + 1] = (uint8_t)g;
				dispImage[idx + 2] = (uint8_t)b;
				dispImage[idx + 3] = 255;
			}
		}

		pffft_aligned_free(fftIn);
		pffft_aligned_free(fftOut);

		displayReady = true;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "irPath", json_string(irPath.c_str()));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// Note: do NOT call Module::fromJson(rootJ) here. dataFromJson receives
		// the "data" object and Module::fromJson is the base dispatcher that
		// loads params and routes to dataFromJson; calling it here re-runs that
		// dispatch during deserialization, which breaks the module restore.
		json_t *p = json_object_get(rootJ, "irPath");
		if (p && json_is_string(p)) {
			std::string path = json_string_value(p);
			if (!path.empty())
				loadIR(path);
		}
	}
};

// ---------------------------------------------------------------------------
// Widget
// ---------------------------------------------------------------------------

// Right-hand IR display: waveform envelope (top) + spectrogram (bottom), with
// a filename label. All data is precomputed on the module at load time.
struct RaReverbirDisplay : LedDisplay {
	RaReverbirModule* module = nullptr;
	int imageId = -1;

	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer != 1)
			return;

		if (!module)
			return;

		Rect b = getBox().zeroPos();

		// ---- Spectrogram fills the whole display, darkened so the waveform
		// (and filename) remain visible on top. ----
		bool hasImage = module->displayReady &&
			module->dispImage.size() == (size_t)module->dispImageW * module->dispImageH * 4 &&
			module->dispImageW > 0 && module->dispImageH > 0;

		if (hasImage) {
			if (imageId >= 0)
				nvgDeleteImage(args.vg, imageId);
			imageId = nvgCreateImageRGBA(args.vg, module->dispImageW, module->dispImageH,
				NVG_IMAGE_NEAREST, (const uint8_t*)module->dispImage.data());
			nvgSave(args.vg);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0, 0, b.size.x, b.size.y);
			nvgFillPaint(args.vg, nvgImagePattern(args.vg, 0, 0, b.size.x, b.size.y, 0.f, imageId, 1.f));
			nvgFill(args.vg);
			nvgRestore(args.vg);
			// Dim layer over the spectrogram to make the waveform pop.
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0, 0, b.size.x, b.size.y);
			nvgFillColor(args.vg, nvgRGBAf(0.f, 0.f, 0.f, 0.45f));
			nvgFill(args.vg);
		}
		else {
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0, 0, b.size.x, b.size.y);
			nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
			nvgFill(args.vg);
		}

		// ---- Waveform envelope overlaid on the spectrogram: time runs down,
		// amplitude across, centered on the display. ----
		nvgSave(args.vg);
		nvgScissor(args.vg, RECT_ARGS(Rect(Vec(0, 0), b.size)));
		{
			float cx = b.size.x * 0.5f;
			float halfW = b.size.x * 0.5f - 1.f;
			nvgBeginPath(args.vg);
			for (int r = 0; r < DISP_WAVE_ROWS && r < (int)module->dispWaveMax.size(); r++) {
				float y = ((float)r + 0.5f) / (float)DISP_WAVE_ROWS * b.size.y;
				float x0 = cx + module->dispWaveMin[r] * halfW;
				float x1 = cx + module->dispWaveMax[r] * halfW;
				x0 = clamp(x0, 0.f, b.size.x);
				x1 = clamp(x1, 0.f, b.size.x);
				nvgMoveTo(args.vg, x0, y);
				nvgLineTo(args.vg, x1, y);
			}
			nvgStrokeWidth(args.vg, 1.f);
			nvgStrokeColor(args.vg, nvgRGB(0xd9, 0xd0, 0xf0));
			nvgStroke(args.vg);
		}
		nvgRestore(args.vg);

		// ---- Filename, vertical (top->bottom, one UPPERCASE character per row)
		// centered on the display. Uses the basename with the .wav suffix
		// stripped. ----
		std::string base = module->irPath;
		if (module->displayReady) {
			std::size_t s1 = base.rfind('/');
			std::size_t s2 = base.rfind('\\');
			std::size_t sl = std::string::npos;
			if (s1 != std::string::npos) sl = s1;
			if (s2 != std::string::npos && (sl == std::string::npos || s2 > sl)) sl = s2;
			if (sl != std::string::npos)
				base = base.substr(sl + 1);
			// Strip a trailing .wav/.WAV extension.
			std::size_t dot = base.rfind('.');
			if (dot != std::string::npos && dot > 0 &&
				(dot == base.size() - 4) &&
				(base[dot + 1] == 'w' || base[dot + 1] == 'W'))
				base = base.substr(0, dot);
			for (std::size_t i = 0; i < base.size(); i++)
				if (base[i] >= 'a' && base[i] <= 'z')
					base[i] = (char)(base[i] - 'a' + 'A');
		}
		else {
			base = "NO IR";
		}
		const char* name = base.c_str();
		int chars = (int)base.size();
		float lineH = 10.f;
		int maxRows = (int)(b.size.y / lineH);
		if (maxRows < 1) maxRows = 1;
		int begin = 0; // anchor at top; long names truncate at the bottom
		char ch[2] = {0, 0};
		nvgFontFaceId(args.vg, APP->window->uiFont->handle);
		nvgFontSize(args.vg, 9);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, nvgRGB(0xc9, 0xd4, 0xe6));
		float textCx = 6.f; // center the character column near the left edge (away from the waveform)
		for (int i = 0; i < maxRows; i++) {
			int ci = begin + i;
			if (ci >= chars) break;
			ch[0] = name[ci];
			nvgText(args.vg, textCx, 4.f + (float)i * lineH, ch, NULL);
		}

		LedDisplay::drawLayer(args, layer);
	}
};

struct RaReverbirWidget : ModuleWidget {
	RaReverbirWidget(RaReverbirModule *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-reverbir.svg")));

		// The module is a 12HP-wide panel: the left 8HP hold the controls, the
		// right 4HP hold the IR display. box.size is derived from the panel SVG
		// and snapped to the rack grid by the framework.
		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		// Knob (left) and CV-jack (right) columns.
		const float leftX = 30;
		const float rightX = 90;

		// Audio inputs
		addInput(createInputCentered<RaPort>(Vec(leftX, 40), module, RaReverbirModule::L_AUDIO_INPUT));
		addInput(createInputCentered<RaPort>(Vec(rightX, 40), module, RaReverbirModule::R_AUDIO_INPUT));

		// Mix
		addParam(createParamCentered<RaKnob>(Vec(leftX, 72), module, RaReverbirModule::MIX_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, 72), module, RaReverbirModule::MIX_CV_INPUT));

		// Level
		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 104), module, RaReverbirModule::LEVEL_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, 104), module, RaReverbirModule::LEVEL_CV_INPUT));

		// Predelay
		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 136), module, RaReverbirModule::PREDELAY_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, 136), module, RaReverbirModule::PREDELAY_CV_INPUT));

		// Attack
		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 168), module, RaReverbirModule::ATTACK_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, 168), module, RaReverbirModule::ATTACK_CV_INPUT));

		// Decay
		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 200), module, RaReverbirModule::DECAY_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, 200), module, RaReverbirModule::DECAY_CV_INPUT));

		// Damp
		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 232), module, RaReverbirModule::DAMP_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, 232), module, RaReverbirModule::DAMP_CV_INPUT));

		// Gate
		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 264), module, RaReverbirModule::GATE_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, 264), module, RaReverbirModule::GATE_CV_INPUT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(60, 264), module, RaReverbirModule::GATE_LIGHT));

		// Length (no CV input) — right below Damp
		addParam(createParamCentered<RaKnobSmall>(Vec(leftX, 296), module, RaReverbirModule::LENGTH_PARAM));

		// Load IR button
		addParam(createLightParamCentered<VCVLightBezel<RedLight>>(Vec(60, 328), module, RaReverbirModule::LOAD_PARAM, RaReverbirModule::LOAD_LIGHT));

		// Audio outputs
		addOutput(createOutputCentered<RaPort>(Vec(leftX, 360), module, RaReverbirModule::L_AUDIO_OUTPUT));
		addOutput(createOutputCentered<RaPort>(Vec(rightX, 360), module, RaReverbirModule::R_AUDIO_OUTPUT));

		// IR display (right column of the 12HP panel)
		RaReverbirDisplay* display = createWidget<RaReverbirDisplay>(Vec(box.size.x - 58.f, 8.f));
		display->box.size = Vec(54.f, box.size.y - 16.f);
		display->module = module;
		addChild(display);
	}

	// Poll for a load request and open the file dialog on the UI thread.
	void step() override {
		ModuleWidget::step();
		if (!module)
			return;
		auto *m = static_cast<RaReverbirModule *>(module);
		if (m->loadRequested.exchange(false, std::memory_order_relaxed)) {
			char *pathC = osdialog_file(OSDIALOG_OPEN, nullptr, nullptr,
				osdialog_filters_parse("WAV IR Files:wav,WAV"));
			if (pathC) {
				m->loadIR(pathC);
				free(pathC);
			}
		}
	}
};

Model *modelRaReverbir = createModel<RaReverbirModule, RaReverbirWidget>("ra-reverbir");