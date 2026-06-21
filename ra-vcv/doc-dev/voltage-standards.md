Voltage Standards

In Rack, input and output values carried by patch cables are in voltage units (V). You can measure absolute voltage levels using modules like VCV Scope.

Rack attempts to model Eurorack standards as accurately as possible, but this is a problem for two reasons: there are very few actual “standards” in Eurorack (The only rule is that you can always find a module which breaks the rule), and there are a few differences between digital (finite sample rate) and analog (infinite sample rate).
Levels ¶

Oscillators and CV generators should typically produce 10 Vpp10Vpp​ (peak-to-peak) signals. This means that audio outputs should typically be ±5 V (before bandlimiting is applied), and CV modulation sources should typically be 0 to 10 V (unipolar CV) or ±5 V (bipolar CV).

Absolute decibel measurements should use dBFS (decibels relative to full scale) with full scale being -10 to 10 V, although they can be labeled as simply “dB”.

    0 dBFS: 10 V
    0 dBV (voltage, rarely used): 1 V
    0 dbVU (for VU meters): -18 dBFS by hardware convention

Output Saturation ¶

In Eurorack, power supplies supply -12 to 12 V. No voltage should be generated beyond this range, since it would be mostly impossible to obtain in Eurorack. Additionally, protection diodes on the ±12 V rails usually drop the range to about ±11.7 V.

However, if you do not want to model analog output saturation for simplicity or performance reasons, that is perfectly fine. It is much better to allow voltages outside this range rather than use hard clipping with clamp(out, -1.f, 1.f) because in the best case they will be attenuated by a module downstream, and in the worst case, they will be hard clipped by the Audio module from Core.

If your module is capable of applying >1x gain to an input, it is a good idea to saturate the output.
Triggers and Gates ¶

In Eurorack, many modules are triggered by reaching a particular rising slope threshold. However, because of the Gibbs phenomenon, a digital emulation will falsely retrigger many times if the trigger source is bandlimited (e.g. by using a virtual VCO square wave as a trigger input or a hardware trigger through an audio interface.)

Therefore, trigger inputs in Rack should be triggered by a Schmitt trigger with a low threshold of about 0.1 V and a high threshold of around 1 to 2 V. Rack plugins can implement this using dsp::SchmittTrigger with schmittTrigger.process(x, 0.1f, 1.f)

Trigger sources should produce 10 V with a duration of 1 ms. An easy way to hold a trigger for this duration is to use dsp::PulseGenerator with pulseGenerator.trigger(1e-3f).

Gates should produce 10 V when active.
Timing ¶

Each cable in Rack induces a 1-sample delay of its carried signal from the output port to the input port. This means that it is not guaranteed that two signals generated simultaneously will arrive at their destinations at the same time if the number of cables in each signal’s chain is different. For example, a pulse sent through a utility module and then to a sequencer’s CLOCK input will arrive one sample later than the same pulse sent directly to the sequencer’s RESET input. This will cause the sequencer to reset to step 1, and one sample later, advance to step 2, which is undesired behavior.

Therefore, modules with a CLOCK and RESET input, or similar variants, should ignore CLOCK triggers up to 1 ms after receiving a RESET trigger. You can use dsp::Timer for keeping track of time.
Pitch and Frequencies ¶

Modules should use the 1 V/oct (volt per octave) standard for CV control of frequency information. In this standard, the relationship between frequency ff and voltage VV is f=f0⋅2Vf=f0​⋅2V, where f0f0​ is the baseline frequency. Your module might have a frequency knob which may offset VV. At its default position, audio-rate oscillators should use a baseline of the note C4 defined by International Pitch Notation (“middle C”, MIDI note 60, f0f0​ = 261.6256 Hz = dsp::FREQ_C4). Low-frequency oscillators and clock generators should use 120 BPM (f0f0​ = 2 Hz).
NaNs and Infinity ¶

If your module might produce NaNs or infinite output values when given only finite input, e.g. an unstable IIR filter or reverb, it should check and return 0 when this happens: std::isfinite(out) ? out : 0.f.
Polyphony ¶

If your module supports polyphonic inputs or has polyphonic outputs, then it can be considered a “polyphonic module”, so add the “Polyphonic” tag to the module’s manifest. It is recommended to support up to 16 channels, which is the maximum that Rack allows.

Typically each voice in your module can be abstracted into an “engine”. The number of active engines NN should be defined by the number of channels of the “primary” input (e.g. 1 V/oct input for VCOs, audio input for filters, gate input for envelope generators, etc). All other secondary inputs with MM channels should follow these rules:

    If monophonic (M=1M=1), its voltage should be copied to all engines.
    If polyphonic with enough channels (M≥NM≥N), each channel voltage should be used in its respective engine.
    If polyphonic but not enough channels (1<M<N1<M<N), 0 V should be copied to out-of-bounds engines.

All of this behavior is provided by Port::getPolyVoltage(c).

Monophonic modules should handle polyphonic inputs gracefully:

    For inputs intended to be used solely for audio, sum the voltages of all channels (e.g. with Port::getVoltageSum())
    For inputs intended to be used for CV or hybrid audio/CV, use the first channel’s voltage (e.g. with Port::getVoltage())
