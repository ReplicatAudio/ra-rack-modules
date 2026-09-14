# ra-karplus-strong — Karplus-Strong String

Physical-modelling plucked string synthesis with sympathetic string resonators, damping, brightness, pick position, and stiffness. Polyphonic — one voice per 1V/oct channel.

## Controls
- **Frequency**: base string frequency (~20 Hz–4 kHz, logarithmic).
- **FM attenuation**: amount of linear FM from the FM input (0–100%).
- **Damping**: high-frequency damping of the string loop (0–100%).
- **Feedback**: loop feedback gain (0–200%).
- **Brightness**: classic Karplus-Strong loop filter — 0 = no filtering, 0.5 = classic average, 1 = maximum filtering.
- **Pick position**: where the string is plucked, producing comb-filter notches.
- **Stiffness**: string stiffness, which stretches upper partials upward (inharmonicity).
- **Level**: output level (0–100%).
- **Loop limit**: in-loop saturation ceiling (1–64) that prevents runaway modes.
- **Excitation mode**: the pluck burst shape — Noise, Chirp, Saw, Square, or Sine.
- **Sympathetic strings**: number of resonator strings (0–8).
- **Sympathetic detune**: detune spread of the resonator strings.
- **Sympathetic level**: how loudly the resonator strings ring.

## Inputs
- **Trigger**: plucks the string (per voice).
- **1V/Oct**: pitch input (1 V per octave).
- **FM**: linear frequency modulation.
- **Damping / Feedback / Brightness / Pick position / Stiffness / Level / Loop limit**: CVs added to the corresponding knobs (0–10 V spans the full range).
- **Excitation mode CV / Sympathetic count CV / Sympathetic detune CV / Sympathetic level CV**: CVs controlling those parameters.

## Outputs
- **Audio**: the plucked string output (polyphonic).