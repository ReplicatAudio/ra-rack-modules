#include "ra-components.hpp"
#include <string.h>
#include <cmath>
#include <dsp/fft.hpp>

using namespace rack;

extern Plugin *pluginInstance;

static const int BUFFER_SIZE = 256;
static const int FFT_SIZE = 2048;

struct DScopeModule : Module {
	enum ParamIds {
		X_SCALE_PARAM,
		X_POS_PARAM,
		Y_SCALE_PARAM,
		Y_POS_PARAM,
		TIME_PARAM,
		MODE_PARAM,
		THRESH_PARAM,
		TRIG_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		X_INPUT,
		Y_INPUT,
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		X_OUTPUT,
		Y_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		MODE_LIGHT_R,
		MODE_LIGHT_G,
		MODE_LIGHT_B,
		TRIG_LIGHT,
		NUM_LIGHTS
	};

	struct Point {
		float min = INFINITY;
		float max = -INFINITY;
	};
	Point pointBuffer[BUFFER_SIZE][2][PORT_MAX_CHANNELS];
	Point currentPoint[2][PORT_MAX_CHANNELS];
	int channelsX = 0;
	int channelsY = 0;
	int bufferIndex = 0;
	int frameIndex = 0;

	dsp::SchmittTrigger triggers[16];

	int mode = 0;
	dsp::SchmittTrigger modeCycleTrigger;
	bool disableCableColors = false;

	dsp::RealFFT* fft = nullptr;
	float* fftInputL = nullptr;
	float* fftOutputL = nullptr;
	float* spectrumMagL = nullptr;
	float* fftInputR = nullptr;
	float* fftOutputR = nullptr;
	float* spectrumMagR = nullptr;
	int fftIndex = 0;
	float* hannWindow = nullptr;
	bool spectrumReady = false;
	float sampleRate = 44100.f;

	static const int SONO_NUM_COLS = 400;
	float sonoBuffer[SONO_NUM_COLS * (FFT_SIZE / 2)];
	int sonoCount = 0;
	float sonoRingBuffer[FFT_SIZE];
	int sonoRingPos = 0;
	int sonoHopCounter = 0;

	DScopeModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(X_SCALE_PARAM, 0.f, 8.f, 0.f, "Gain 1", " V/screen", 1 / 2.f, 20);
		getParamQuantity(X_SCALE_PARAM)->snapEnabled = true;
		configParam(X_POS_PARAM, -10.f, 10.f, 0.f, "Offset 1", " V");
		configParam(Y_SCALE_PARAM, 0.f, 8.f, 0.f, "Gain 2", " V/screen", 1 / 2.f, 20);
		getParamQuantity(Y_SCALE_PARAM)->snapEnabled = true;
		configParam(Y_POS_PARAM, -10.f, 10.f, 0.f, "Offset 2", " V");
		const float maxTime = -std::log2(5e1f);
		const float minTime = -std::log2(5e-3f);
		const float defaultTime = -std::log2(5e-1f);
		configParam(TIME_PARAM, maxTime, minTime, defaultTime, "Time", " ms/screen", 1 / 2.f, 1000);
		configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Cycle mode", {"Cycle"});
		configParam(THRESH_PARAM, -10.f, 10.f, 0.f, "Trigger threshold", " V");
		configSwitch(TRIG_PARAM, 0.f, 1.f, 1.f, "Trigger", {"Enabled", "Disabled"});

		configInput(X_INPUT, "Ch 1");
		configInput(Y_INPUT, "Ch 2");
		configInput(TRIG_INPUT, "External trigger");

		configOutput(X_OUTPUT, "Ch 1");
		configOutput(Y_OUTPUT, "Ch 2");

		configLight(MODE_LIGHT_R, "Mode indicator");
		configLight(MODE_LIGHT_G, "");
		configLight(MODE_LIGHT_B, "");

		fft = new dsp::RealFFT(FFT_SIZE);
		fftInputL = (float*) pffft_aligned_malloc(FFT_SIZE * sizeof(float));
		fftOutputL = (float*) pffft_aligned_malloc(2 * FFT_SIZE * sizeof(float));
		spectrumMagL = new float[FFT_SIZE / 2];
		fftInputR = (float*) pffft_aligned_malloc(FFT_SIZE * sizeof(float));
		fftOutputR = (float*) pffft_aligned_malloc(2 * FFT_SIZE * sizeof(float));
		spectrumMagR = new float[FFT_SIZE / 2];
		hannWindow = new float[FFT_SIZE];
		for (int i = 0; i < FFT_SIZE; i++) {
			hannWindow[i] = 0.5f * (1.f - cosf(2.f * M_PI * i / (FFT_SIZE - 1)));
		}
		memset(sonoBuffer, 0, sizeof(sonoBuffer));
	}

	~DScopeModule() {
		delete fft;
		pffft_aligned_free(fftInputL);
		pffft_aligned_free(fftOutputL);
		delete[] spectrumMagL;
		pffft_aligned_free(fftInputR);
		pffft_aligned_free(fftOutputR);
		delete[] spectrumMagR;
		delete[] hannWindow;
	}

	void onReset() override {
		for (int i = 0; i < BUFFER_SIZE; i++) {
			for (int w = 0; w < 2; w++) {
				for (int c = 0; c < 16; c++) {
					pointBuffer[i][w][c] = Point();
				}
			}
		}
	}

	int getMode() {
		return mode;
	}

	void process(const ProcessArgs& args) override {
		sampleRate = args.sampleRate;
		if (modeCycleTrigger.process(params[MODE_PARAM].getValue())) {
			mode = (mode + 1) % 4;
		}

		float lr = 0.f, lg = 0.f, lb = 0.f;
		switch (mode) {
			case 0: lr = lg = lb = 1.0f; break;
			case 1: lr = 1.0f; lg = 1.0f; break;
			case 2: lg = 1.0f; break;
			case 3: lb = 1.0f; break;
		}
		lights[MODE_LIGHT_R].setBrightness(lr);
		lights[MODE_LIGHT_G].setBrightness(lg);
		lights[MODE_LIGHT_B].setBrightness(lb);

		bool trigEnabled = !params[TRIG_PARAM].getValue();
		lights[TRIG_LIGHT].setBrightness(trigEnabled);

		int channelsX = inputs[X_INPUT].getChannels();
		if (channelsX != this->channelsX) {
			this->channelsX = channelsX;
		}
		int channelsY = inputs[Y_INPUT].getChannels();
		if (channelsY != this->channelsY) {
			this->channelsY = channelsY;
		}

		outputs[X_OUTPUT].setChannels(channelsX);
		outputs[X_OUTPUT].writeVoltages(inputs[X_INPUT].getVoltages());
		outputs[Y_OUTPUT].setChannels(channelsY);
		outputs[Y_OUTPUT].writeVoltages(inputs[Y_INPUT].getVoltages());

		if (mode == 2) {
			fftInputL[fftIndex] = inputs[X_INPUT].getVoltageSum();
			fftInputR[fftIndex] = inputs[Y_INPUT].getVoltageSum();
			fftIndex++;

			if (fftIndex >= FFT_SIZE) {
				for (int i = 0; i < FFT_SIZE; i++)
					fftInputL[i] *= hannWindow[i];
				fft->rfft(fftInputL, fftOutputL);
				spectrumMagL[0] = fabsf(fftOutputL[0]);
				for (int i = 1; i < FFT_SIZE / 2; i++) {
					float re = fftOutputL[2 * i];
					float im = fftOutputL[2 * i + 1];
					spectrumMagL[i] = sqrtf(re * re + im * im);
				}

				for (int i = 0; i < FFT_SIZE; i++)
					fftInputR[i] *= hannWindow[i];
				fft->rfft(fftInputR, fftOutputR);
				spectrumMagR[0] = fabsf(fftOutputR[0]);
				for (int i = 1; i < FFT_SIZE / 2; i++) {
					float re = fftOutputR[2 * i];
					float im = fftOutputR[2 * i + 1];
					spectrumMagR[i] = sqrtf(re * re + im * im);
				}

				spectrumReady = true;
				fftIndex = 0;
			}
			return;
		}

		if (mode == 3) {
			float in = inputs[X_INPUT].getVoltageSum();
			sonoRingBuffer[sonoRingPos] = in;
			sonoRingPos = (sonoRingPos + 1) % FFT_SIZE;

			float hopFloat = (float)FFT_SIZE * dsp::exp2_taylor5(1.f - params[TIME_PARAM].getValue());
			int hop = clamp((int)hopFloat, FFT_SIZE / 8, FFT_SIZE * 8);

			sonoHopCounter++;
			if (sonoHopCounter >= (int)hop) {
				sonoHopCounter = 0;

				for (int i = 0; i < FFT_SIZE; i++)
					fftInputL[i] = sonoRingBuffer[(sonoRingPos + i) % FFT_SIZE] * hannWindow[i];
				fft->rfft(fftInputL, fftOutputL);

				int numBins = FFT_SIZE / 2;
				memmove(&sonoBuffer[numBins], &sonoBuffer[0], (SONO_NUM_COLS - 1) * numBins * sizeof(float));
				sonoBuffer[0] = fabsf(fftOutputL[0]) / (float)(FFT_SIZE / 2);
				for (int i = 1; i < numBins; i++) {
					float re = fftOutputL[2 * i];
					float im = fftOutputL[2 * i + 1];
					sonoBuffer[i] = sqrtf(re * re + im * im) / (float)(FFT_SIZE / 2);
				}
				if (sonoCount < SONO_NUM_COLS)
					sonoCount++;
			}
			return;
		}

		if (bufferIndex >= BUFFER_SIZE) {
			bool triggered = false;

			if (mode == 1 || !trigEnabled) {
				triggered = true;
			}
			else {
				float trigThreshold = params[THRESH_PARAM].getValue();
				Input& trigInput = inputs[TRIG_INPUT].isConnected() ? inputs[TRIG_INPUT] : inputs[X_INPUT];

				int trigChannels = trigInput.getChannels();
				for (int c = 0; c < trigChannels; c++) {
					float trigVoltage = trigInput.getVoltage(c);
					if (triggers[c].process(rescale(trigVoltage, trigThreshold, trigThreshold + 0.001f, 0.f, 1.f))) {
						triggered = true;
					}
				}
			}

			if (triggered) {
				for (int c = 0; c < 16; c++) {
					triggers[c].reset();
				}
				bufferIndex = 0;
				frameIndex = 0;
			}
		}

		if (bufferIndex < BUFFER_SIZE) {
			float deltaTime = dsp::exp2_taylor5(-params[TIME_PARAM].getValue()) / BUFFER_SIZE;
			int frameCount = (int) std::ceil(deltaTime * args.sampleRate);

			for (int c = 0; c < channelsX; c++) {
				float x = inputs[X_INPUT].getVoltage(c);
				currentPoint[0][c].min = std::min(currentPoint[0][c].min, x);
				currentPoint[0][c].max = std::max(currentPoint[0][c].max, x);
			}
			for (int c = 0; c < channelsY; c++) {
				float y = inputs[Y_INPUT].getVoltage(c);
				currentPoint[1][c].min = std::min(currentPoint[1][c].min, y);
				currentPoint[1][c].max = std::max(currentPoint[1][c].max, y);
			}

			if (++frameIndex >= frameCount) {
				frameIndex = 0;
				for (int w = 0; w < 2; w++) {
					for (int c = 0; c < 16; c++) {
						pointBuffer[bufferIndex][w][c] = currentPoint[w][c];
					}
				}
				for (int w = 0; w < 2; w++) {
					for (int c = 0; c < 16; c++) {
						currentPoint[w][c] = Point();
					}
				}
				bufferIndex++;
			}
		}
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "mode");
		if (modeJ) {
			mode = json_integer_value(modeJ);
		} else {
			json_t* lissajousJ = json_object_get(rootJ, "lissajous");
			if (lissajousJ && json_integer_value(lissajousJ))
				mode = 1;
		}

		json_t* externalJ = json_object_get(rootJ, "external");
		if (externalJ) {
			if (json_integer_value(externalJ))
				params[TRIG_PARAM].setValue(1.f);
		}

		json_t* disableColorsJ = json_object_get(rootJ, "disableCableColors");
		if (disableColorsJ)
			disableCableColors = json_is_true(disableColorsJ);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "mode", json_integer(mode));
		json_object_set_new(rootJ, "disableCableColors", json_boolean(disableCableColors));
		return rootJ;
	}
};


DScopeModule::Point DEMO_POINT_BUFFER[BUFFER_SIZE];

void demoPointBufferInit() {
	static bool init = false;
	if (init)
		return;
	init = true;

	for (size_t i = 0; i < BUFFER_SIZE; i++) {
		float phase = float(i) / BUFFER_SIZE;
		DScopeModule::Point point;
		point.min = point.max = 4.f * std::sin(2 * M_PI * phase * 2.f);
		DEMO_POINT_BUFFER[i] = point;
	}
}


struct DScopeDisplay : LedDisplay {
	DScopeModule* module;
	ModuleWidget* moduleWidget;
	int statsFrame = 0;
	std::string fontPath;
	int sonoImageId = -1;

	struct Stats {
		float min = INFINITY;
		float max = -INFINITY;
	};
	Stats statsX;
	Stats statsY;

	DScopeDisplay() {
		fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
		demoPointBufferInit();
	}

	void calculateStats(Stats& stats, int wave, int channels) {
		if (!module) {
			stats.min = -5.f;
			stats.max = 5.f;
			return;
		}

		stats = Stats();
		for (int i = 0; i < BUFFER_SIZE; i++) {
			for (int c = 0; c < channels; c++) {
				DScopeModule::Point point = module->pointBuffer[i][wave][c];
				stats.max = std::fmax(stats.max, point.max);
				stats.min = std::fmin(stats.min, point.min);
			}
		}
	}

	void drawWave(const DrawArgs& args, int wave, int channel, float offset, float gain) {
		DScopeModule::Point pointBuffer[BUFFER_SIZE];
		for (int i = 0; i < BUFFER_SIZE; i++) {
			pointBuffer[i] = module ? module->pointBuffer[i][wave][channel] : DEMO_POINT_BUFFER[i];
		}

		nvgSave(args.vg);
		Rect b = box.zeroPos().shrink(Vec(0, 15));
		nvgScissor(args.vg, RECT_ARGS(b));
		nvgBeginPath(args.vg);
		for (int i = 0; i < BUFFER_SIZE; i++) {
			const DScopeModule::Point& point = pointBuffer[i];
			float max = point.max;
			if (!std::isfinite(max))
				max = 0.f;

			Vec p;
			p.x = (float) i / (BUFFER_SIZE - 1);
			p.y = (max + offset) * gain * -0.5f + 0.5f;
			p = b.interpolate(p);
			p.y -= 1.0;
			if (i == 0)
				nvgMoveTo(args.vg, p.x, p.y);
			else
				nvgLineTo(args.vg, p.x, p.y);
		}
		for (int i = BUFFER_SIZE - 1; i >= 0; i--) {
			const DScopeModule::Point& point = pointBuffer[i];
			float min = point.min;
			if (!std::isfinite(min))
				min = 0.f;

			Vec p;
			p.x = (float) i / (BUFFER_SIZE - 1);
			p.y = (min + offset) * gain * -0.5f + 0.5f;
			p = b.interpolate(p);
			p.y += 1.0;
			nvgLineTo(args.vg, p.x, p.y);
		}
		nvgClosePath(args.vg);
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
		nvgFill(args.vg);
		nvgResetScissor(args.vg);
		nvgRestore(args.vg);
	}

	void drawLissajous(const DrawArgs& args, int channel, float offsetX, float gainX, float offsetY, float gainY, NVGcolor colorOld, NVGcolor colorNew) {
		if (!module)
			return;

		DScopeModule::Point pointBufferX[BUFFER_SIZE];
		DScopeModule::Point pointBufferY[BUFFER_SIZE];
		for (int i = 0; i < BUFFER_SIZE; i++) {
			pointBufferX[i] = module->pointBuffer[i][0][channel];
			pointBufferY[i] = module->pointBuffer[i][1][channel];
		}

		nvgSave(args.vg);
		Rect b = box.zeroPos().shrink(Vec(0, 15));
		nvgScissor(args.vg, RECT_ARGS(b));

		Vec points[BUFFER_SIZE];
		int numValid = 0;
		int bufferIndex = module->bufferIndex;
		for (int i = 0; i < BUFFER_SIZE; i++) {
			const DScopeModule::Point& pointX = pointBufferX[(i + bufferIndex) % BUFFER_SIZE];
			const DScopeModule::Point& pointY = pointBufferY[(i + bufferIndex) % BUFFER_SIZE];
			float avgX = (pointX.min + pointX.max) / 2;
			float avgY = (pointY.min + pointY.max) / 2;
			if (!std::isfinite(avgX) || !std::isfinite(avgY))
				continue;

			Vec p;
			p.x = (avgX + offsetX) * gainX * 0.5f + 0.5f;
			p.y = (avgY + offsetY) * gainY * -0.5f + 0.5f;
			p = b.interpolate(p);
			points[numValid++] = p;
		}

		nvgLineCap(args.vg, NVG_ROUND);
		nvgMiterLimit(args.vg, 2.f);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);

		if (module && module->disableCableColors) {
			nvgStrokeColor(args.vg, colorNew);
			nvgBeginPath(args.vg);
			for (int i = 0; i < numValid; i++) {
				if (i == 0)
					nvgMoveTo(args.vg, points[i].x, points[i].y);
				else
					nvgLineTo(args.vg, points[i].x, points[i].y);
			}
			nvgStroke(args.vg);
		} else {
			for (int i = 0; i < numValid - 1; i++) {
				float t = (float)(i + 1) / numValid;
				NVGcolor c;
				c.r = colorOld.r + (colorNew.r - colorOld.r) * t;
				c.g = colorOld.g + (colorNew.g - colorOld.g) * t;
				c.b = colorOld.b + (colorNew.b - colorOld.b) * t;
				c.a = colorOld.a + (colorNew.a - colorOld.a) * t;
				nvgStrokeColor(args.vg, c);

				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, points[i].x, points[i].y);
				nvgLineTo(args.vg, points[i + 1].x, points[i + 1].y);
				nvgStroke(args.vg);
			}
		}

		nvgResetScissor(args.vg);
		nvgRestore(args.vg);
	}

	void drawTrig(const DrawArgs& args, float value) {
		Rect b = Rect(Vec(0, 15), box.size.minus(Vec(0, 15 * 2)));
		nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);

		value = value / 2.f + 0.5f;
		Vec p = Vec(box.size.x, b.pos.y + b.size.y * (1.f - value));

		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
		{
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, p.x - 13, p.y);
			nvgLineTo(args.vg, 0, p.y);
		}
		nvgStroke(args.vg);

		nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x60));
		{
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, p.x - 2, p.y - 4);
			nvgLineTo(args.vg, p.x - 9, p.y - 4);
			nvgLineTo(args.vg, p.x - 13, p.y);
			nvgLineTo(args.vg, p.x - 9, p.y + 4);
			nvgLineTo(args.vg, p.x - 2, p.y + 4);
			nvgClosePath(args.vg);
		}
		nvgFill(args.vg);

		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		if (font) {
			nvgFontSize(args.vg, 9);
			nvgFontFaceId(args.vg, font->handle);
			nvgFillColor(args.vg, nvgRGBA(0x1e, 0x28, 0x2b, 0xff));
			nvgText(args.vg, p.x - 8, p.y + 3, "T", NULL);
		}
		nvgResetScissor(args.vg);
	}

	void drawStats(const DrawArgs& args, Vec pos, const char* title, const Stats& stats) {
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		if (!font)
			return;
		nvgFontSize(args.vg, 13);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -1);

		nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x40));
		nvgText(args.vg, pos.x + 6, pos.y + 11, title, NULL);

		nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x80));
		pos = pos.plus(Vec(20, 11));

		std::string text;
		text = "pp ";
		float pp = stats.max - stats.min;
		text += isNear(pp, 0.f, 100.f) ? string::f("% 6.2f", pp) : "  ---";
		nvgText(args.vg, pos.x, pos.y, text.c_str(), NULL);
		text = "max";
		text += isNear(stats.max, 0.f, 100.f) ? string::f("% 6.2f", stats.max) : "  ---";
		nvgText(args.vg, pos.x + 60 * 1, pos.y, text.c_str(), NULL);
		text = "min";
		text += isNear(stats.min, 0.f, 100.f) ? string::f("% 6.2f", stats.min) : "  ---";
		nvgText(args.vg, pos.x + 60 * 2, pos.y, text.c_str(), NULL);
	}

	void drawBackground(const DrawArgs& args) {
		// Screen backdrop — painted slightly larger than the box to cover the
		// SVG bezel outline, recolored with a muted purple border to match the accent
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
		nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
		nvgStroke(args.vg);

		Rect b = box.zeroPos().shrink(Vec(0, 15));

		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
		for (int i = 0; i < 5; i++) {
			nvgBeginPath(args.vg);

			Vec p;
			p.x = 0.0;
			p.y = float(i) / (5 - 1);
			nvgMoveTo(args.vg, VEC_ARGS(b.interpolate(p)));

			p.x = 1.0;
			nvgLineTo(args.vg, VEC_ARGS(b.interpolate(p)));
			nvgStroke(args.vg);
		}
	}

	void drawSpectrum(const DrawArgs& args) {
		if (!module || !module->spectrumReady)
			return;

		float marginL = 12.f;
		float marginR = 0.f;
		float marginT = 6.f;
		float marginB = 20.f;
		Rect plot = box.zeroPos().grow(Vec(-marginL, -marginT)).grow(Vec(-marginR, -marginB));

		int numBins = FFT_SIZE / 2;
		float binSpacingHz = module->sampleRate / FFT_SIZE;
		float minLogFreq = 20.f;
		float maxLogFreq = module->sampleRate / 2.f;
		float logMin = logf(minLogFreq);
		float logRange = logf(maxLogFreq) - logMin;

		auto logX = [&](float freq) -> float {
			float norm = (logf(fmaxf(freq, minLogFreq)) - logMin) / logRange;
			return plot.pos.x + norm * plot.size.x;
		};

		nvgSave(args.vg);
		nvgScissor(args.vg, RECT_ARGS(plot));

		PortWidget* inputX = moduleWidget->getInput(DScopeModule::X_INPUT);
		PortWidget* inputY = moduleWidget->getInput(DScopeModule::Y_INPUT);
		CableWidget* inputXCable = APP->scene->rack->getTopCable(inputX);
		CableWidget* inputYCable = APP->scene->rack->getTopCable(inputY);
		NVGcolor colorL, colorR;
		if (module->disableCableColors) {
			colorL = SCHEME_PURPLE;
			colorR = nvgRGB(0xff, 0x66, 0xaa);
		} else {
			colorL = inputXCable ? inputXCable->color : SCHEME_YELLOW;
			colorR = inputYCable ? inputYCable->color : SCHEME_YELLOW;
		}

		struct ChannelSpec { float* mag; NVGcolor color; };
		ChannelSpec chs[] = {
			{module->spectrumMagL, colorL},
			{module->spectrumMagR, colorR},
		};

		for (auto& ch : chs) {
			float gainY = module ? powf(2.f, roundf(module->params[DScopeModule::Y_SCALE_PARAM].getValue())) / 10.f : 0.1f;
			float offsetY = module ? module->params[DScopeModule::Y_POS_PARAM].getValue() : 0.f;

			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, logX(0.f), plot.pos.y + plot.size.y);

			for (int i = 0; i < numBins; i++) {
				float freq = (float)i * binSpacingHz;
				float mag = ch.mag[i] / (float)(FFT_SIZE / 2) * gainY;
				float db = 20.f * log10f(mag + 1e-6f) + offsetY;
				float yNorm = clamp(rescale(db, -60.f, 0.f, 0.f, 1.f), 0.f, 1.f);
				float y = plot.pos.y + plot.size.y * (1.f - yNorm);
				nvgLineTo(args.vg, logX(freq), y);
			}

			nvgLineTo(args.vg, logX(maxLogFreq), plot.pos.y + plot.size.y);
			nvgClosePath(args.vg);

			nvgFillColor(args.vg, ch.color);
			nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
			nvgFill(args.vg);
		}

		nvgResetScissor(args.vg);

		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		nvgFontFaceId(args.vg, font ? font->handle : -1);
		nvgTextLetterSpacing(args.vg, 0);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);

		struct FreqMark { int freq; const char* label; };
		FreqMark freqMarks[] = {{100, "100"}, {500, "500"}, {1000, "1k"}, {5000, "5k"}, {10000, "10k"}, {20000, "20k"}};
		for (auto& fm : freqMarks) {
			if (fm.freq > maxLogFreq) break;
			float x = logX((float)fm.freq);

			nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, x, plot.pos.y);
			nvgLineTo(args.vg, x, plot.pos.y + plot.size.y);
			nvgStroke(args.vg);

			if (x - plot.pos.x < 20.f) continue;

			nvgFontSize(args.vg, 10);
			nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x50));
			nvgText(args.vg, x, plot.pos.y + plot.size.y + 4, fm.label, NULL);
		}

		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
		nvgFontSize(args.vg, 10);

		int dbMarks[] = {0, -20, -40, -60};
		for (int db : dbMarks) {
			float norm = rescale((float)db, -60.f, 0.f, 0.f, 1.f);
			float y = plot.pos.y + plot.size.y * (1.f - norm);

			nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, plot.pos.x, y);
			nvgLineTo(args.vg, plot.pos.x + plot.size.x, y);
			nvgStroke(args.vg);

			if (db > -60) {
				nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x50));
				nvgText(args.vg, plot.pos.x + 4, y, string::f("%d", db).c_str(), NULL);
			}
		}

		nvgRestore(args.vg);
	}

	void drawSonograph(const DrawArgs& args) {
		if (!module)
			return;

		float marginL = 12.f;
		float marginR = 0.f;
		float marginT = 6.f;
		float marginB = 20.f;
		Rect plot = box.zeroPos().grow(Vec(-marginL, -marginT)).grow(Vec(-marginR, -marginB));

		int plotW = (int)plot.size.x;
		int plotH = (int)plot.size.y;
		if (plotW <= 0 || plotH <= 0)
			return;

		float sampleRate = module->sampleRate;
		float binSpacingHz = sampleRate / FFT_SIZE;
		int numBins = FFT_SIZE / 2;
		float minLogFreq = 20.f;
		float maxLogFreq = sampleRate / 2.f;
		float logMin = logf(minLogFreq);
		float logRange = logf(maxLogFreq) - logMin;

		float gainY = powf(2.f, roundf(module->params[DScopeModule::Y_SCALE_PARAM].getValue())) / 10.f;
		float offsetY = module->params[DScopeModule::Y_POS_PARAM].getValue();

		uint8_t* pixels = new uint8_t[plotW * plotH * 4];

		auto normToRGB = [](float norm, uint8_t& r, uint8_t& g, uint8_t& b) {
			float hue = (1.f - clamp(norm, 0.f, 1.f)) * 270.f;
			float s = 1.f;
			float v = powf(clamp(norm, 0.f, 1.f), 0.4f);
			int hi = ((int)(hue / 60.f)) % 6;
			float f = hue / 60.f - floorf(hue / 60.f);
			float p = v * (1.f - s);
			float q = v * (1.f - s * f);
			float t = v * (1.f - s * (1.f - f));
			switch (hi) {
				case 0: r = v*255; g = t*255; b = p*255; break;
				case 1: r = q*255; g = v*255; b = p*255; break;
				case 2: r = p*255; g = v*255; b = t*255; break;
				case 3: r = p*255; g = q*255; b = v*255; break;
				case 4: r = t*255; g = p*255; b = v*255; break;
				default: r = v*255; g = p*255; b = q*255; break;
			}
		};

		for (int px = 0; px < plotW; px++) {
			int frameAge = plotW - 1 - px;
			if (frameAge >= module->sonoCount) {
				for (int py = 0; py < plotH; py++) {
					int idx = (py * plotW + px) * 4;
					pixels[idx+0] = 0;
					pixels[idx+1] = 0;
					pixels[idx+2] = 0;
					pixels[idx+3] = 0;
				}
				continue;
			}
			float* frame = &module->sonoBuffer[frameAge * numBins];

			for (int py = 0; py < plotH; py++) {
				float yNorm = 1.f - (float)py / plotH;
				float freq = expf(logMin + yNorm * logRange);
				int bin = clamp((int)(freq / binSpacingHz), 0, numBins - 1);

				float mag = frame[bin] * gainY;
				float db = 20.f * log10f(mag + 1e-6f) + offsetY;
				float norm = clamp(rescale(db, -60.f, 0.f, 0.f, 1.f), 0.f, 1.f);

				uint8_t r, g, b;
				normToRGB(norm, r, g, b);

				int idx = (py * plotW + px) * 4;
				pixels[idx+0] = r;
				pixels[idx+1] = g;
				pixels[idx+2] = b;
				pixels[idx+3] = 255;
			}
		}

		if (sonoImageId >= 0)
			nvgDeleteImage(args.vg, sonoImageId);
		sonoImageId = nvgCreateImageRGBA(args.vg, plotW, plotH, NVG_IMAGE_NEAREST, pixels);
		delete[] pixels;

		nvgSave(args.vg);
		nvgBeginPath(args.vg);
		nvgRect(args.vg, plot.pos.x, plot.pos.y, plotW, plotH);
		nvgFillPaint(args.vg, nvgImagePattern(args.vg, plot.pos.x, plot.pos.y, plotW, plotH, 0.f, sonoImageId, 1.f));
		nvgFill(args.vg);
		nvgRestore(args.vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return;

		drawBackground(args);

		int displayMode = module ? module->getMode() : 0;

		if (displayMode == 2) {
			drawSpectrum(args);
			return;
		}

		if (displayMode == 3) {
			drawSonograph(args);
			return;
		}

		float gainX = module ? module->params[DScopeModule::X_SCALE_PARAM].getValue() : 0.f;
		gainX = std::pow(2.f, std::round(gainX)) / 10.f;
		float gainY = module ? module->params[DScopeModule::Y_SCALE_PARAM].getValue() : 0.f;
		gainY = std::pow(2.f, std::round(gainY)) / 10.f;
		float offsetX = module ? module->params[DScopeModule::X_POS_PARAM].getValue() : 5.f;
		float offsetY = module ? module->params[DScopeModule::Y_POS_PARAM].getValue() : -5.f;

		PortWidget* inputX = moduleWidget->getInput(DScopeModule::X_INPUT);
		PortWidget* inputY = moduleWidget->getInput(DScopeModule::Y_INPUT);
		CableWidget* inputXCable = APP->scene->rack->getTopCable(inputX);
		CableWidget* inputYCable = APP->scene->rack->getTopCable(inputY);
		NVGcolor inputXColor, inputYColor;
		if (module && module->disableCableColors) {
			inputXColor = SCHEME_PURPLE;
			inputYColor = nvgRGB(0xff, 0x66, 0xaa);
		} else {
			inputXColor = inputXCable ? inputXCable->color : SCHEME_YELLOW;
			inputYColor = inputYCable ? inputYCable->color : SCHEME_YELLOW;
		}

		int channelsY = module ? module->channelsY : 1;
		int channelsX = module ? module->channelsX : 1;
		if (displayMode == 1) {
			int lissajousChannels = std::min(channelsX, channelsY);
			for (int c = 0; c < lissajousChannels; c++) {
				drawLissajous(args, c, offsetX, gainX, offsetY, gainY, inputYColor, inputXColor);
			}
		}
		else {
			for (int c = 0; c < channelsY; c++) {
				nvgFillColor(args.vg, inputYColor);
				drawWave(args, 1, c, offsetY, gainY);
			}

			for (int c = 0; c < channelsX; c++) {
				nvgFillColor(args.vg, inputXColor);
				drawWave(args, 0, c, offsetX, gainX);
			}

			float trigThreshold = module ? module->params[DScopeModule::THRESH_PARAM].getValue() : 0.f;
			trigThreshold = (trigThreshold + offsetX) * gainX;
			drawTrig(args, trigThreshold);
		}

		if (statsFrame == 0) {
			calculateStats(statsX, 0, channelsX);
			calculateStats(statsY, 1, channelsY);
		}
		statsFrame = (statsFrame + 1) % 4;

		drawStats(args, Vec(0, 0 + 1), "1", statsX);
		drawStats(args, Vec(0, box.size.y - 15 - 1), "2", statsY);
	}
};


struct DScopeWidget : ModuleWidget {
	DScopeWidget(DScopeModule* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-dscope.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(30, 24), module, DScopeModule::MODE_PARAM, DScopeModule::MODE_LIGHT_R));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(30, 72), module, DScopeModule::TRIG_PARAM, DScopeModule::TRIG_LIGHT));
		addParam(createParamCentered<RaKnob>(Vec(30, 120), module, DScopeModule::THRESH_PARAM));
		addInput(createInputCentered<RaPort>(Vec(30, 168), module, DScopeModule::TRIG_INPUT));

		addParam(createParamCentered<RaKnob>(Vec(30, 216), module, DScopeModule::TIME_PARAM));

		addInput(createInputCentered<RaPort>(Vec(30, 264), module, DScopeModule::X_INPUT));
		addInput(createInputCentered<RaPort>(Vec(30, 312), module, DScopeModule::Y_INPUT));

		addParam(createParamCentered<RaKnob>(Vec(480, 24), module, DScopeModule::X_SCALE_PARAM));
		addParam(createParamCentered<RaKnob>(Vec(480, 72), module, DScopeModule::Y_SCALE_PARAM));

		addParam(createParamCentered<RaKnob>(Vec(480, 120), module, DScopeModule::X_POS_PARAM));
		addParam(createParamCentered<RaKnob>(Vec(480, 168), module, DScopeModule::Y_POS_PARAM));

		addOutput(createOutputCentered<RaPort>(Vec(480, 264), module, DScopeModule::X_OUTPUT));
		addOutput(createOutputCentered<RaPort>(Vec(480, 312), module, DScopeModule::Y_OUTPUT));

		DScopeDisplay* display = createWidget<DScopeDisplay>(Vec(60, 4));
		display->box.size = Vec(390, 380 - 8);
		display->module = module;
		display->moduleWidget = this;
		addChild(display);
	}

	void appendContextMenu(ui::Menu* menu) override {
		DScopeModule* mod = dynamic_cast<DScopeModule*>(module);

		menu->addChild(new ui::MenuSeparator);

		struct CableColorsItem : ui::MenuItem {
			DScopeModule* mod;
			void onAction(const event::Action& e) override {
				mod->disableCableColors = !mod->disableCableColors;
			}
			void step() override {
				rightText = mod->disableCableColors ? "✔" : "";
				MenuItem::step();
			}
		};

		CableColorsItem* item = new CableColorsItem;
		item->text = "Disable cable colors";
		item->mod = mod;
		menu->addChild(item);
	}
};


Model* modelRaDscope = createModel<DScopeModule, DScopeWidget>("ra-dscope");
