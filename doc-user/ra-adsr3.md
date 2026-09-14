# ra-adsr3 — ADSR Envelope Generator

3 hp ADSR envelope generator with attack, decay, sustain, and release knobs plus gate, retrigger, and trigger inputs.

## Controls
- **Attack**: attack time (1 ms–10 s).
- **Decay**: decay time (1 ms–10 s).
- **Sustain**: sustain level (0–100%).
- **Release**: release time (1 ms–10 s).

## Inputs
- **Gate**: held gate signal that sustains the envelope while high.
- **Retrigger**: retriggers the envelope from the start on each rising edge.
- **Trigger**: on a rising edge, latches the gate high and restarts the attack.

## Outputs
- **Envelope**: the ADSR envelope signal (0–10 V).