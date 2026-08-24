#include "ra-components.hpp"
#include <cmath>

using namespace rack;

extern Plugin *pluginInstance;

static const char* TUNER_NOTE_NAMES[12] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

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
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
		nvgFillColor(args.vg, nvgRGB(0x08, 0x0c, 0x08));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1);
		nvgStrokeColor(args.vg, nvgRGB(0x44, 0x44, 0x44));
		nvgStroke(args.vg);

		if (!module || !font)
			return;

		bool audio = module->params[RaTunerModule::MODE_PARAM].getValue() > 0.5f;

		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		// Western note notation (MIDI octave convention, 69 = A4)
		char noteBuf[16];
		if (module->valid) {
			int semitone = (int)std::lround(module->semitone);
			int pc = eucMod(semitone, 12);
			int oct = semitone / 12 - 1;
			snprintf(noteBuf, sizeof(noteBuf), "%s%d", TUNER_NOTE_NAMES[pc], oct);
		}
		else {
			snprintf(noteBuf, sizeof(noteBuf), "--");
		}

		nvgFontSize(args.vg, 36);
		nvgFillColor(args.vg, module->valid ? nvgRGB(0xcc, 0xee, 0x88) : nvgRGB(0x4c, 0x5a, 0x3c));
		nvgText(args.vg, box.size.x / 2, 46, noteBuf, NULL);

		// Frequency
		char freqBuf[32];
		if (module->valid) {
			if (module->freq >= 1000.f)
				snprintf(freqBuf, sizeof(freqBuf), "%.3f kHz", module->freq / 1000.f);
			else
				snprintf(freqBuf, sizeof(freqBuf), "%.1f Hz", module->freq);
		}
		else {
			snprintf(freqBuf, sizeof(freqBuf), audio ? "NO SIGNAL" : "NO CABLE");
		}

		nvgFontSize(args.vg, 16);
		nvgFillColor(args.vg, module->valid ? nvgRGB(0x88, 0xaa, 0x66) : nvgRGB(0x4c, 0x5a, 0x3c));
		nvgText(args.vg, box.size.x / 2, 80, freqBuf, NULL);

		// Mode readout
		nvgFontSize(args.vg, 11);
		nvgFillColor(args.vg, nvgRGB(0x66, 0x77, 0x55));
		nvgText(args.vg, box.size.x / 2, 112, audio ? "AUDIO" : "1V/OCT", NULL);
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

		TunerDisplay* display = createWidget<TunerDisplay>(Vec(6, 24));
		display->box.size = Vec(78, 150);
		display->module = module;
		addChild(display);

		addInput(createInputCentered<RaPort>(Vec(30, 234), module, RaTunerModule::IN_INPUT));
		addParam(createParamCentered<RaSwitch2>(Vec(60, 234), module, RaTunerModule::MODE_PARAM));
	}
};

Model* modelRaTuner = createModel<RaTunerModule, RaTunerWidget>("ra-tuner");