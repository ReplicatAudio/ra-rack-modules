#include "ra-components.hpp"
#include <cmath>
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

static const char* TUNER_NOTE_NAMES[12] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// Project purple palette (see ra-klock, ra-reflectingpool displays)
static const NVGcolor TUNER_PURPLE = nvgRGB(0x99, 0x6d, 0xd2);
static const NVGcolor TUNER_PURPLE_DIM = nvgRGB(0x55, 0x3d, 0x74);
static const NVGcolor TUNER_PURPLE_INACTIVE = nvgRGB(0x2a, 0x1d, 0x33);
static const NVGcolor TUNER_GREEN = nvgRGB(0x66, 0xee, 0x88);

struct RaTunerModule : Module {
	enum ParamIds {
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// Display state
	float freq = 0.f;
	float semitone = 0.f;
	bool valid = false;

	// Audio-mode pitch detection
	float simTime = 0.f;
	float prevSample = 0.f;
	float lastRiseTime = -1.f;
	float holdTime = 0.f;
	static const int MED_N = 9;
	float medBuf[MED_N] = {};
	int medIndex = 0;
	int medCount = 0;

	RaTunerModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"1V/oct", "Audio"});
		configInput(IN_INPUT, "Input");
		configOutput(OUT_OUTPUT, "Out");
	}

	void onReset() override {
		freq = 0.f;
		semitone = 0.f;
		valid = false;
		simTime = 0.f;
		prevSample = 0.f;
		lastRiseTime = -1.f;
		holdTime = 0.f;
		medIndex = 0;
		medCount = 0;
	}

	void process(const ProcessArgs& args) override {
		// Passthrough
		int channels = inputs[IN_INPUT].getChannels();
		outputs[OUT_OUTPUT].setChannels(channels);
		outputs[OUT_OUTPUT].writeVoltages(inputs[IN_INPUT].getVoltages());

		bool audio = params[MODE_PARAM].getValue() > 0.5f;
		float v = inputs[IN_INPUT].getVoltage(0);

		if (audio) {
			simTime += args.sampleTime;

			// Rising zero crossings with hysteresis span one full period
			if (prevSample < -0.01f && v > 0.01f) {
				if (lastRiseTime >= 0.f) {
					float period = simTime - lastRiseTime;
					if (period > 0.f) {
						float detected = 1.f / period;
						if (detected >= 1.f && detected <= 20000.f) {
							medBuf[medIndex] = detected;
							medIndex = (medIndex + 1) % MED_N;
							if (medCount < MED_N)
								medCount++;
							holdTime = 0.f;
						}
					}
				}
				lastRiseTime = simTime;
			}
			prevSample = v;

			// Drop the measurement if the signal goes away
			holdTime += args.sampleTime;
			if (holdTime > 0.5f && medCount > 0) {
				medCount = 0;
				medIndex = 0;
			}

			if (medCount > 0) {
				// Median filter for jitter rejection
				float sorted[MED_N];
				for (int i = 0; i < medCount; i++)
					sorted[i] = medBuf[i];
				for (int i = 1; i < medCount; i++) {
					float key = sorted[i];
					int j = i - 1;
					while (j >= 0 && sorted[j] > key) {
						sorted[j + 1] = sorted[j];
						j--;
					}
					sorted[j + 1] = key;
				}
				freq = sorted[medCount / 2];
				valid = true;
				semitone = 69.f + 12.f * std::log2(freq / 440.f);
			}
			else {
				valid = false;
				freq = 0.f;
			}
		}
		else {
			freq = dsp::FREQ_C4 * std::pow(2.f, v);
			semitone = 60.f + 12.f * v;
			valid = inputs[IN_INPUT].isConnected();
		}
	}
};

struct TunerDisplay : LedDisplay {
	RaTunerModule* module;
	std::shared_ptr<Font> font;

	TunerDisplay() {
		font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
	}

	void draw(const DrawArgs& args) override {
		// Screen backdrop — painted slightly larger than the box to cover the
		// SVG bezel outline, recolored with a muted purple border to match the accent
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
		nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
		nvgStroke(args.vg);

		if (!module || !font)
			return;

		bool audio = module->params[RaTunerModule::MODE_PARAM].getValue() > 0.5f;
		bool valid = module->valid;

		// Deviation from the nearest semitone in cents (-50..+50)
		float cents = 0.f;
		if (valid) {
			float nearest = std::round(module->semitone);
			cents = (module->semitone - nearest) * 100.f;
		}
		bool inTune = valid && std::abs(cents) <= 3.f;

		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		// Western note notation (MIDI octave convention, 69 = A4)
		char noteBuf[16];
		if (valid) {
			int semitone = (int)std::lround(module->semitone);
			snprintf(noteBuf, sizeof(noteBuf), "%s%d",
				TUNER_NOTE_NAMES[eucMod(semitone, 12)], semitone / 12 - 1);
		}
		else {
			snprintf(noteBuf, sizeof(noteBuf), "--");
		}
		nvgFontSize(args.vg, (int)strlen(noteBuf) >= 4 ? 34.f : 46.f);
		nvgFillColor(args.vg, valid ? TUNER_PURPLE : TUNER_PURPLE_DIM);
		nvgText(args.vg, box.size.x / 2, 60, noteBuf, NULL);

		// Frequency
		char freqBuf[32];
		if (valid) {
			if (module->freq >= 1000.f)
				snprintf(freqBuf, sizeof(freqBuf), "%.2f kHz", module->freq / 1000.f);
			else
				snprintf(freqBuf, sizeof(freqBuf), "%.1f Hz", module->freq);
		}
		else {
			snprintf(freqBuf, sizeof(freqBuf), audio ? "NO SIGNAL" : "NO CABLE");
		}
		nvgFontSize(args.vg, 15);
		nvgFillColor(args.vg, valid ? TUNER_PURPLE : TUNER_PURPLE_DIM);
		nvgText(args.vg, box.size.x / 2, 96, freqBuf, NULL);

		// Cents meter — 21 segments, 5 cents each, -50..+50
		const int N = 21;
		const float barX = 10.f;
		const float barW = box.size.x - 2 * barX;
		const float slot = barW / N;
		const float segGap = 0.8f;
		const float segW = slot - segGap;
		const float segH = 10.f;
		const float barY = 124.f;

		int idx = N / 2;
		if (valid) {
			idx = (int)std::lround(clamp(cents, -50.f, 50.f) / 5.f) + N / 2;
			idx = clamp(idx, 0, N - 1);
		}

		for (int i = 0; i < N; i++) {
			bool active = valid && (i == idx);
			bool center = (i == N / 2);
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, barX + i * slot, barY, segW, segH, 1);
			if (active && inTune)
				nvgFillColor(args.vg, TUNER_GREEN);
			else if (active)
				nvgFillColor(args.vg, TUNER_PURPLE);
			else if (center)
				nvgFillColor(args.vg, TUNER_PURPLE_DIM);
			else
				nvgFillColor(args.vg, TUNER_PURPLE_INACTIVE);
			nvgFill(args.vg);
		}

		// Center reference marker
		float cx = barX + (N / 2) * slot + slot / 2;
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, cx, barY - 5);
		nvgLineTo(args.vg, cx, barY);
		nvgStrokeWidth(args.vg, 1.5);
		nvgStrokeColor(args.vg, inTune ? TUNER_GREEN : TUNER_PURPLE_DIM);
		nvgStroke(args.vg);

		// Flat / sharp indicators
		nvgFontSize(args.vg, 9);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, (valid && cents < -3.f) ? TUNER_PURPLE : TUNER_PURPLE_DIM);
		nvgText(args.vg, 4, barY + segH + 14, "FLAT", NULL);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, (valid && cents > 3.f) ? TUNER_PURPLE : TUNER_PURPLE_DIM);
		nvgText(args.vg, box.size.x - 4, barY + segH + 14, "SHARP", NULL);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, inTune ? TUNER_GREEN : TUNER_PURPLE_DIM);
		nvgText(args.vg, box.size.x / 2, barY + segH + 14, "0", NULL);

		// Cents readout (green when in tune)
		char centsBuf[16];
		if (valid)
			snprintf(centsBuf, sizeof(centsBuf), "%+.0fc", cents);
		else
			snprintf(centsBuf, sizeof(centsBuf), "--");
		nvgFontSize(args.vg, 14);
		nvgFillColor(args.vg, inTune ? TUNER_GREEN : (valid ? TUNER_PURPLE : TUNER_PURPLE_DIM));
		nvgText(args.vg, box.size.x / 2, 176, centsBuf, NULL);

		// Mode readout
		nvgFontSize(args.vg, 11);
		nvgFillColor(args.vg, TUNER_PURPLE_DIM);
		nvgText(args.vg, box.size.x / 2, 202, audio ? "AUDIO" : "1V/OCT", NULL);
	}
};

struct RaTunerWidget : ModuleWidget {
	RaTunerWidget(RaTunerModule* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-tuner.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		TunerDisplay* display = createWidget<TunerDisplay>(Vec(4, 24));
		display->box.size = Vec(82, 224);
		display->module = module;
		addChild(display);

		addInput(createInputCentered<RaPort>(Vec(30, 328), module, RaTunerModule::IN_INPUT));
		addParam(createParamCentered<RaSwitch2>(Vec(45, 268), module, RaTunerModule::MODE_PARAM));
		addOutput(createOutputCentered<RaPort>(Vec(60, 328), module, RaTunerModule::OUT_OUTPUT));
	}
};

Model* modelRaTuner = createModel<RaTunerModule, RaTunerWidget>("ra-tuner");