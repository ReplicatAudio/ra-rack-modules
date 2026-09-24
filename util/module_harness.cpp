// Standalone harness: instantiate the REAL RaReverbirModule and drive its
// actual process() per sample, like VCV does. Measures output peak/RMS.
#include <cstdio>
#include <cmath>
#include <vector>
#include <cstring>

// Pull in the real module definition (module portion only).
#include "/tmp/module_body.cpp"

// pluginInstance is referenced by ra-components.hpp; provide a dummy.
Plugin *pluginInstance = nullptr;

int main(int argc, char** argv) {
	if (argc < 2) { fprintf(stderr,"usage: %s <ir.wav> [sampleRate]\n", argv[0]); return 1; }
	int sampleRate = argc >= 3 ? atoi(argv[2]) : 48000;

	RaReverbirModule m;
	m.loadIR(argv[1]);

	m.inputs[RaReverbirModule::L_AUDIO_INPUT].setChannels(1);
	m.inputs[RaReverbirModule::R_AUDIO_INPUT].setChannels(1);

	m.params[RaReverbirModule::MIX_PARAM].setValue(0.5f);
	m.params[RaReverbirModule::LEVEL_PARAM].setValue(1.0f);
	m.params[RaReverbirModule::LENGTH_PARAM].setValue(1.0f);
	m.params[RaReverbirModule::GATE_PARAM].setValue(0.0f);
	m.params[RaReverbirModule::PREDELAY_PARAM].setValue(0.f);
	m.params[RaReverbirModule::ATTACK_PARAM].setValue(0.01f);
	m.params[RaReverbirModule::DECAY_PARAM].setValue(0.5f);
	m.params[RaReverbirModule::DAMP_PARAM].setValue(0.5f);

	float t = 0.f;
	const float dt = 1.f / (float)sampleRate;
	const float scale = 5.0f; // volts peak drive
	double sumAbs=0, sumSq=0, outCount=0; float peak=0;
	int N = sampleRate * 6;
	int skip = (int)(sampleRate * 1.0f); // skip convolver latency + fade-in
	rack::engine::Module::ProcessArgs args;
	args.sampleRate = (float)sampleRate;
	args.sampleTime = dt;
	for (int i = 0; i < N; i++) {
		float s = scale * (0.5f*sinf(2*M_PI*997.0f*t) + 0.3f*sinf(2*M_PI*1433.0f*t) + 0.2f*sinf(2*M_PI*2111.0f*t));
		t += dt;
		m.inputs[RaReverbirModule::L_AUDIO_INPUT].setVoltage(s);
		m.inputs[RaReverbirModule::R_AUDIO_INPUT].setVoltage(s);
		args.frame = i;
		m.process(args);
		float o = m.outputs[RaReverbirModule::L_AUDIO_OUTPUT].getVoltage();
		if (i >= skip) {
			float a = fabsf(o); if (a > peak) peak = a;
			sumAbs += a; sumSq += (double)o*(double)o; outCount++;
		}
	}
	float dryPeak = 2.0f * (0.5f+0.3f+0.2f); // ~2.0V peak drive
	double rms = outCount>0 ? sqrt(sumSq/outCount) : 0.0;
	printf("DRIVE peak=%.3fV  (module sees this as dry input)\n", dryPeak);
	printf("OUTPUT peak=%.3fV  RMS=%.3fV   (drawing direct 1:1 would be peak=%.3f)\n", peak, (double)rms, dryPeak);
	printf("=> output is %+.1f dB vs drive peak\n", 20.0*log10((double)peak/dryPeak));
	return 0;
}