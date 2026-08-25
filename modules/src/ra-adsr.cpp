#include "ra-components.hpp"

using namespace rack;
using simd::float_4;

extern Plugin *pluginInstance;

// Envelope curve constants matching RaAdsrDisplay curve rendering
static constexpr float ENV_TARGET = 1.1f;
static constexpr float ENV_LAMBDA = 2.3978952727983702f;
// Repo accent purple — matches the other modules' envelope/filter accents
static const NVGcolor ADSR_PURPLE = nvgRGB(0x99, 0x6d, 0xd2);
static float envPhaseToEnv(float phase) {
	return (1 - std::exp(-ENV_LAMBDA * phase)) * ENV_TARGET;
}
static float envEnvToPhase(float env) {
	return -std::log(1 - env / ENV_TARGET) / ENV_LAMBDA;
}

struct RaAdsrModule : Module {
	enum ParamIds {
		ATTACK_PARAM,
		DECAY_PARAM,
		SUSTAIN_PARAM,
		RELEASE_PARAM,
		ATTACK_CV_PARAM,
		DECAY_CV_PARAM,
		SUSTAIN_CV_PARAM,
		RELEASE_CV_PARAM,
		PUSH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ATTACK_INPUT,
		DECAY_INPUT,
		SUSTAIN_INPUT,
		RELEASE_INPUT,
		GATE_INPUT,
		RETRIG_INPUT,
		TRIGGER_INPUT,
		POSITION_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENVELOPE_OUTPUT,
		EOC_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ATTACK_LIGHT,
		DECAY_LIGHT,
		SUSTAIN_LIGHT,
		RELEASE_LIGHT,
		PUSH_LIGHT,
		NUM_LIGHTS
	};

	static constexpr float MIN_TIME = 1e-3f;
	static constexpr float MAX_TIME = 10.f;
	static constexpr float LAMBDA_BASE = MAX_TIME / MIN_TIME;
	static constexpr float ATT_TARGET = 1.2f;

	int channels = 1;
	float_4 gate[4] = {};
	float_4 attacking[4] = {};
	float_4 env[4] = {};
	dsp::TSchmittTrigger<float_4> trigger[4];
	dsp::TSchmittTrigger<float_4> triggerInput[4];
	float_4 triggerActive[4] = {};
	dsp::ClockDivider cvDivider;
	float_4 attackLambda[4] = {};
	float_4 decayLambda[4] = {};
	float_4 releaseLambda[4] = {};
	float_4 sustain[4] = {};
	float_4 attProp[4] = {};
	float_4 decProp[4] = {};
	float_4 relProp[4] = {};
	float_4 wasResting[4] = {float_4::mask(), float_4::mask(), float_4::mask(), float_4::mask()};
	bool positionConnected = false;
	float positionCv = 0.f;
	dsp::ClockDivider lightDivider;

	RaAdsrModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ATTACK_PARAM, 0.f, 1.f, 0.5f, "Attack", " ms", LAMBDA_BASE, MIN_TIME * 1000);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay", " ms", LAMBDA_BASE, MIN_TIME * 1000);
		configParam(SUSTAIN_PARAM, 0.f, 1.f, 0.5f, "Sustain", "%", 0, 100);
		configParam(RELEASE_PARAM, 0.f, 1.f, 0.5f, "Release", " ms", LAMBDA_BASE, MIN_TIME * 1000);

		configParam(ATTACK_CV_PARAM, -1.f, 1.f, 0.f, "Attack CV", "%", 0, 100);
		configParam(DECAY_CV_PARAM, -1.f, 1.f, 0.f, "Decay CV", "%", 0, 100);
		configParam(SUSTAIN_CV_PARAM, -1.f, 1.f, 0.f, "Sustain CV", "%", 0, 100);
		configParam(RELEASE_CV_PARAM, -1.f, 1.f, 0.f, "Release CV", "%", 0, 100);

		configButton(PUSH_PARAM, "Push");

		configInput(ATTACK_INPUT, "Attack");
		configInput(DECAY_INPUT, "Decay");
		configInput(SUSTAIN_INPUT, "Sustain");
		configInput(RELEASE_INPUT, "Release");
		configInput(GATE_INPUT, "Gate");
		configInput(RETRIG_INPUT, "Retrigger");
		configInput(TRIGGER_INPUT, "Trigger");
		configInput(POSITION_INPUT, "Position");

		configOutput(ENVELOPE_OUTPUT, "Envelope");
		configOutput(EOC_OUTPUT, "End of cycle");

		cvDivider.setDivision(16);
		lightDivider.setDivision(128);
	}

	void process(const ProcessArgs& args) override {
		channels = std::max(1, inputs[GATE_INPUT].getChannels());

		positionConnected = inputs[POSITION_INPUT].isConnected();
		positionCv = inputs[POSITION_INPUT].getVoltage() / 10.f;

		if (cvDivider.process()) {
			float attackParam = params[ATTACK_PARAM].getValue();
			float decayParam = params[DECAY_PARAM].getValue();
			float sustainParam = params[SUSTAIN_PARAM].getValue();
			float releaseParam = params[RELEASE_PARAM].getValue();

			float attackCvParam = params[ATTACK_CV_PARAM].getValue();
			float decayCvParam = params[DECAY_CV_PARAM].getValue();
			float sustainCvParam = params[SUSTAIN_CV_PARAM].getValue();
			float releaseCvParam = params[RELEASE_CV_PARAM].getValue();

			for (int c = 0; c < channels; c += 4) {
				float_4 attack = attackParam + inputs[ATTACK_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f * attackCvParam;
				float_4 decay = decayParam + inputs[DECAY_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f * decayCvParam;
				float_4 sustain = sustainParam + inputs[SUSTAIN_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f * sustainCvParam;
				float_4 release = releaseParam + inputs[RELEASE_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f * releaseCvParam;

				attack = simd::clamp(attack, 0.f, 1.f);
				decay = simd::clamp(decay, 0.f, 1.f);
				sustain = simd::clamp(sustain, 0.f, 1.f);
				release = simd::clamp(release, 0.f, 1.f);

				attackLambda[c / 4] = simd::pow(LAMBDA_BASE, -attack) / MIN_TIME;
				decayLambda[c / 4] = simd::pow(LAMBDA_BASE, -decay) / MIN_TIME;
				releaseLambda[c / 4] = simd::pow(LAMBDA_BASE, -release) / MIN_TIME;
				this->sustain[c / 4] = sustain;

				float_4 invAtt = 1.f / attackLambda[c / 4];
				float_4 invDec = 1.f / decayLambda[c / 4];
				float_4 invRel = 1.f / releaseLambda[c / 4];
				float_4 total = invAtt + invDec + invRel;
				attProp[c / 4] = invAtt / total;
				decProp[c / 4] = invDec / total;
				relProp[c / 4] = invRel / total;
			}
		}

		bool push = (params[PUSH_PARAM].getValue() > 0.f);

		for (int c = 0; c < channels; c += 4) {
			if (positionConnected) {
				float_4 pos = inputs[POSITION_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f;
				pos = simd::clamp(pos, 0.f, 1.f);
				float_4 result;
				for (int j = 0; j < 4 && (c + j) < channels; j++) {
					result[j] = computeEnvelope(pos[j], attProp[c / 4][j], decProp[c / 4][j], relProp[c / 4][j], sustain[c / 4][j]);
				}
				env[c / 4] = result;
				gate[c / 4] = float_4::mask();
				attacking[c / 4] = float_4::zero();
			}
			else {
				float_4 oldGate = gate[c / 4];

				// Trigger input: rising edge forces gate high, restarts attack
				float_4 trigEdge = triggerInput[c / 4].process(inputs[TRIGGER_INPUT].getPolyVoltageSimd<float_4>(c));
				triggerActive[c / 4] |= trigEdge;

				if (push) {
					gate[c / 4] = float_4::mask();
				}
				else {
					gate[c / 4] = inputs[GATE_INPUT].getVoltageSimd<float_4>(c) >= 1.f;
				}
				gate[c / 4] |= triggerActive[c / 4];

				attacking[c / 4] |= (gate[c / 4] & ~oldGate);
				attacking[c / 4] |= trigEdge;

				float_4 triggered = trigger[c / 4].process(inputs[RETRIG_INPUT].getPolyVoltageSimd<float_4>(c));
				attacking[c / 4] |= triggered;

				attacking[c / 4] &= gate[c / 4];

				float_4 decayTarget = simd::ifelse(triggerActive[c / 4], float_4(0.f), sustain[c / 4]);
				float_4 target = simd::ifelse(attacking[c / 4], ATT_TARGET, simd::ifelse(gate[c / 4], decayTarget, 0.f));
				float_4 lambda = simd::ifelse(attacking[c / 4], attackLambda[c / 4], simd::ifelse(gate[c / 4], decayLambda[c / 4], releaseLambda[c / 4]));

				env[c / 4] += (target - env[c / 4]) * lambda * args.sampleTime;

				attacking[c / 4] &= (env[c / 4] < 1.f);

				triggerActive[c / 4] &= trigEdge | attacking[c / 4] | (env[c / 4] > 0.01f);
			}

			outputs[ENVELOPE_OUTPUT].setVoltageSimd(10.f * env[c / 4], c);

			float_4 resting = env[c / 4] < 0.01f;
			float_4 eocTriggered = simd::ifelse(wasResting[c / 4], float_4(0.f), resting);
			float_4 eoc = simd::ifelse(eocTriggered, float_4(10.f), float_4(0.f));
			wasResting[c / 4] = resting;
			outputs[EOC_OUTPUT].setVoltageSimd(eoc, c);
		}

		outputs[ENVELOPE_OUTPUT].setChannels(positionConnected ? std::max(1, inputs[POSITION_INPUT].getChannels()) : channels);
		outputs[EOC_OUTPUT].setChannels(positionConnected ? std::max(1, inputs[POSITION_INPUT].getChannels()) : channels);

		if (lightDivider.process()) {
			lights[ATTACK_LIGHT].setBrightness(0);
			lights[DECAY_LIGHT].setBrightness(0);
			lights[SUSTAIN_LIGHT].setBrightness(0);
			lights[RELEASE_LIGHT].setBrightness(0);

		for (int c = 0; c < channels; c += 4) {
			const float epsilon = 0.01f;
			float_4 resting = (env[c / 4] < epsilon);

			float_4 attackLight, decayLight, sustainLight, releaseLight;

			if (positionConnected) {
				float_4 pos = inputs[POSITION_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f;
				pos = simd::clamp(pos, 0.f, 1.f);
				float_4 attP = attProp[c / 4];
				float_4 decEnd = attP + decProp[c / 4];

				attackLight = pos <= attP;
				float_4 inDecay = (pos > attP) & (pos <= decEnd);
				float_4 inRelease = pos > decEnd;

				sustainLight = inDecay & (env[c / 4] >= sustain[c / 4]) & (env[c / 4] < sustain[c / 4] + epsilon);
				decayLight = inDecay & ~sustainLight;
				releaseLight = inRelease;
			}
			else {
				attackLight = gate[c / 4] & attacking[c / 4];

				float_4 trigRelease = triggerActive[c / 4] & ~attacking[c / 4] & (env[c / 4] < sustain[c / 4]) & ~resting;
				releaseLight = (~gate[c / 4] & ~resting) | trigRelease;

				float_4 gateHighNotAttack = gate[c / 4] & ~attacking[c / 4] & ~trigRelease;
				float_4 sustaining = (sustain[c / 4] <= env[c / 4]) & (env[c / 4] < sustain[c / 4] + epsilon);
				sustainLight = gateHighNotAttack & sustaining;
				decayLight = gateHighNotAttack & ~sustaining;
			}

			if (simd::movemask(attackLight))
				lights[ATTACK_LIGHT].setBrightness(1);
			if (simd::movemask(decayLight))
				lights[DECAY_LIGHT].setBrightness(1);
			if (simd::movemask(sustainLight))
				lights[SUSTAIN_LIGHT].setBrightness(1);
			if (simd::movemask(releaseLight))
				lights[RELEASE_LIGHT].setBrightness(1);
		}

			bool anyGate = false;
			for (int c = 0; c < channels; c += 4)
				anyGate = anyGate || simd::movemask(gate[c / 4]);
			lights[PUSH_LIGHT].setBrightness(anyGate);
		}
	}

	static float computeEnvelope(float t, float attP, float decP, float relP, float sus) {
		if (t <= attP) {
			return envPhaseToEnv(t / attP);
		}
		t -= attP;
		if (t <= decP) {
			return 1.f - envPhaseToEnv(t / decP) * (1.f - sus);
		}
		t -= decP;
		return (1.f - envPhaseToEnv(t / relP)) * sus;
	}

	void paramsFromJson(json_t* rootJ) override {
		params[ATTACK_CV_PARAM].setValue(1.f);
		params[DECAY_CV_PARAM].setValue(1.f);
		params[SUSTAIN_CV_PARAM].setValue(1.f);
		params[RELEASE_CV_PARAM].setValue(1.f);

		Module::paramsFromJson(rootJ);
	}
};


struct RaAdsrDisplay : LedDisplay {
	RaAdsrModule* module;

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			// Screen backdrop — painted slightly larger than the box to cover the
			// SVG bezel outline, recolored with a muted purple border to match the accent
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
			nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
			nvgFill(args.vg);
			nvgStrokeWidth(args.vg, 1.5f);
			nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
			nvgStroke(args.vg);

			nvgScissor(args.vg, RECT_ARGS(args.clipBox));

			Rect gridBox = getBox().zeroPos().shrink(Vec(0, 6.5));
			Rect r = gridBox;
			r.pos.x += 4.5;
			r.size.x -= 4.5;
			Vec p;

			float attTime = module ? 1 / module->attackLambda[0][0] : 1.f;
			float decTime = module ? 1 / module->decayLambda[0][0] : 1.f;
			float relTime = module ? 1 / module->releaseLambda[0][0] : 1.f;
			float totalTime = attTime + decTime + relTime;
			attTime /= totalTime;
			decTime /= totalTime;
			relTime /= totalTime;
			float sustain = module ? module->sustain[0][0] : 0.5f;
			int channels = module ? module->channels : 1;

			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 1, 0.20));
			nvgBeginPath(args.vg);

			p = r.getTopLeft();
			nvgMoveTo(args.vg, VEC_ARGS(p));
			p = r.getBottomLeft();
			nvgLineTo(args.vg, VEC_ARGS(p));

			p = gridBox.getTopLeft();
			nvgMoveTo(args.vg, VEC_ARGS(p));
			p = gridBox.getTopRight();
			nvgLineTo(args.vg, VEC_ARGS(p));

			p = gridBox.getBottomLeft();
			nvgMoveTo(args.vg, VEC_ARGS(p));
			p = gridBox.getBottomRight();
			nvgLineTo(args.vg, VEC_ARGS(p));

			p = r.interpolate(Vec(attTime, 0));
			nvgMoveTo(args.vg, VEC_ARGS(p));
			p = r.interpolate(Vec(attTime, 1));
			nvgLineTo(args.vg, VEC_ARGS(p));

			p = r.interpolate(Vec(attTime + decTime, 1 - sustain));
			nvgMoveTo(args.vg, VEC_ARGS(p));
			p = r.interpolate(Vec(attTime + decTime, 1));
			nvgLineTo(args.vg, VEC_ARGS(p));

			p = r.interpolate(Vec(attTime, 1 - sustain));
			nvgMoveTo(args.vg, VEC_ARGS(p));
			p = r.interpolate(Vec(1, 1 - sustain));
			nvgLineTo(args.vg, VEC_ARGS(p));

			nvgStroke(args.vg);

			nvgStrokeColor(args.vg, ADSR_PURPLE);
			nvgBeginPath(args.vg);

			p = r.getBottomLeft();
			nvgMoveTo(args.vg, VEC_ARGS(p));

			const int I = 10;
			for (int i = 1; i <= I; i++) {
				float phase = float(i) / I;
				phase = std::pow(phase, 2);
				float env = envPhaseToEnv(phase);
				p = r.interpolate(Vec(attTime * phase, 1 - env));
				nvgLineTo(args.vg, VEC_ARGS(p));
			}

			for (int i = 1; i <= I; i++) {
				float phase = float(i) / I;
				phase = std::pow(phase, 2);
				float env = 1 - envPhaseToEnv(phase) * (1 - sustain);
				p = r.interpolate(Vec(attTime + decTime * phase, 1 - env));
				nvgLineTo(args.vg, VEC_ARGS(p));
			}

			for (int i = 1; i <= I; i++) {
				float phase = float(i) / I;
				phase = std::pow(phase, 2);
				float env = (1 - envPhaseToEnv(phase)) * sustain;
				p = r.interpolate(Vec(attTime + decTime + relTime * phase, 1 - env));
				nvgLineTo(args.vg, VEC_ARGS(p));
			}

			nvgStroke(args.vg);

			{
				nvgStrokeColor(args.vg, ADSR_PURPLE);
				nvgBeginPath(args.vg);
				p = r.interpolate(Vec(attTime + decTime, 1 - sustain));
				nvgCircle(args.vg, VEC_ARGS(p), 2.5);
				nvgFillColor(args.vg, nvgRGB(0x22, 0x22, 0x22));
				nvgFill(args.vg);
				nvgStroke(args.vg);
			}

			bool positionMode = module ? module->positionConnected : false;

		if (positionMode) {
			float cv = clamp(module->positionCv, 0.f, 1.f);
			p = r.interpolate(Vec(cv, 0));
			nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 1, 0.25));
			nvgStrokeWidth(args.vg, 1.0);
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, VEC_ARGS(p));
			p = r.interpolate(Vec(cv, 1));
			nvgLineTo(args.vg, VEC_ARGS(p));
			nvgStroke(args.vg);
		}

		for (int c = 0; c < channels; c++) {
			float env = module ? module->env[c / 4][c % 4] : 0.f;
			if (env > 0.01f) {
				if (positionMode) {
					float cv = clamp(module->positionCv, 0.f, 1.f);
					p = r.interpolate(Vec(cv, 1 - env));
				} else {
				bool attacking = module ? (simd::movemask(module->attacking[c / 4]) & (1 << (c % 4))) : false;
				bool gate = module ? module->gate[c / 4][c % 4] : false;
				bool trigActive = module ? (simd::movemask(module->triggerActive[c / 4]) & (1 << (c % 4))) : false;

				if (attacking) {
					float phase = envEnvToPhase(env);
					p = r.interpolate(Vec(attTime * phase, 1 - env));
				}
				else if (trigActive) {
					float displayEnv = std::min(env, sustain);
					float phase = sustain > 0.f ? envEnvToPhase(1.f - displayEnv / sustain) : 0.f;
					p = r.interpolate(Vec(attTime + decTime + relTime * phase, 1.f - displayEnv));
				}
				else if (gate) {
						float phase = envEnvToPhase(1 - (env - sustain) / (1 - sustain));
						p = r.interpolate(Vec(attTime + decTime * phase, 1 - env));
					}
					else {
						env = std::min(env, sustain);
						float phase = envEnvToPhase(1 - env / sustain);
						p = r.interpolate(Vec(attTime + decTime + relTime * phase, 1 - env));
					}
				}
				nvgBeginPath(args.vg);
				nvgCircle(args.vg, VEC_ARGS(p), 2.5);
				nvgFillColor(args.vg, nvgRGBAf(1, 1, 1, 0.66));
				nvgFill(args.vg);
			}
		}

			nvgResetScissor(args.vg);
		}

		LedDisplay::drawLayer(args, layer);
	}
};


struct RaAdsrWidget : ModuleWidget {
	RaAdsrWidget(RaAdsrModule* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-adsr.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		addParam(createLightParamCentered<VCVLightSlider<YellowLight>>(mm2px(Vec(6.604, 61.0)), module, RaAdsrModule::ATTACK_PARAM, RaAdsrModule::ATTACK_LIGHT));
		addParam(createLightParamCentered<VCVLightSlider<YellowLight>>(mm2px(Vec(17.441, 61.0)), module, RaAdsrModule::DECAY_PARAM, RaAdsrModule::DECAY_LIGHT));
		addParam(createLightParamCentered<VCVLightSlider<YellowLight>>(mm2px(Vec(28.279, 61.0)), module, RaAdsrModule::SUSTAIN_PARAM, RaAdsrModule::SUSTAIN_LIGHT));
		addParam(createLightParamCentered<VCVLightSlider<YellowLight>>(mm2px(Vec(39.116, 61.0)), module, RaAdsrModule::RELEASE_PARAM, RaAdsrModule::RELEASE_LIGHT));
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(6.604, 80.603)), module, RaAdsrModule::ATTACK_CV_PARAM));
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(17.441, 80.63)), module, RaAdsrModule::DECAY_CV_PARAM));
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(28.279, 80.603)), module, RaAdsrModule::SUSTAIN_CV_PARAM));
		addParam(createParamCentered<RaKnobTrim>(mm2px(Vec(39.119, 80.603)), module, RaAdsrModule::RELEASE_CV_PARAM));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(mm2px(Vec(22.86, 45.0)), module, RaAdsrModule::PUSH_PARAM, RaAdsrModule::PUSH_LIGHT));

		addInput(createInputCentered<RaPort>(mm2px(Vec(6.604, 96.882)), module, RaAdsrModule::ATTACK_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(17.441, 96.859)), module, RaAdsrModule::DECAY_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(28.279, 96.886)), module, RaAdsrModule::SUSTAIN_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(39.119, 96.89)), module, RaAdsrModule::RELEASE_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(6.604, 113.115)), module, RaAdsrModule::GATE_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(17.441, 113.115)), module, RaAdsrModule::RETRIG_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(28.279, 113.115)), module, RaAdsrModule::TRIGGER_INPUT));
		addInput(createInputCentered<RaPort>(mm2px(Vec(39.116, 113.115)), module, RaAdsrModule::POSITION_INPUT));

		addOutput(createOutputCentered<RaPort>(mm2px(Vec(17.441, 124.5)), module, RaAdsrModule::ENVELOPE_OUTPUT));
		addOutput(createOutputCentered<RaPort>(mm2px(Vec(28.279, 124.5)), module, RaAdsrModule::EOC_OUTPUT));

		RaAdsrDisplay* display = createWidget<RaAdsrDisplay>(mm2px(Vec(0.0, 13.039)));
		display->box.size = mm2px(Vec(45.72, 21.219));
		display->module = module;
		addChild(display);
	}
};


Model* modelRaAdsr = createModel<RaAdsrModule, RaAdsrWidget>("ra-adsr");
