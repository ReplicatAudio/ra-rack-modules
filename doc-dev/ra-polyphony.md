# Polyphony in audio FX modules

Survey of how popular VCV Rack FX modules handle polyphonic input, and what ra-krush should do.

## Findings

| Module | Poly handling |
|---|---|
| VCV Delay (stock) | Always per-channel. Separate delay line per voice. |
| SurgeXT FX (Distortion, Chorus, Flanger, Reverb, etc.) | User-selectable `polyphonicMode` toggle (context menu). Mono mode: `getVoltageSum()` into one effect instance. Poly mode: one full effect instance per channel. |
| Bogaudio CmpDist | Per-channel engine pool (`addChannel`/`removeChannel` keyed to input channel count). CV read per-channel via `getPolyVoltage(c)`. |
| Valley Plateau | Sums all channels into one signal: `inputs[IN].getVoltageSum()`. Tagged "Polyphonic" but is effectively sum-to-mono. |
| NYSTHI FX line | Mono only (channel 0). |
| Sha-Bang QubitCrusher | Mono only. |

## Patterns

- Time/voiced effects (delay, chorus, flanger): per-channel is the standard.
- Bus/spectral effects (reverb, phaser): often sum.
- Poly-tagged modules from major open-source devs (Surge, Bogaudio) process per-channel.
- Reading only channel 0 with `getVoltage()` on a poly cable silently drops channels 1-15. Do not do this.

## Options for an FX module

1. Per-channel processing. Each voice gets independent DSP state. Matches stock delay and Bogaudio behavior. Mono cables unaffected.
2. Channel summing (`getVoltageSum()`). Crush/filter acts on the combined waveform. Plateau's approach.
3. Both, via a MONO/POLY toggle (Surge's approach).

## Sources

- SurgeXT for Rack: https://github.com/surge-synthesizer/surge-rack (src/FX.h)
- Bogaudio Modules: https://github.com/bogaudio/BogaudioModules (src/CmpDist.cpp)
- Valley Rack Free: https://github.com/ValleyAudio/ValleyRackFree (src/Plateau/Plateau.cpp)
