# ra-adsr — Polyphonic ADSR Envelope

Polyphonic ADSR envelope generator with CV control over all stages and a visual envelope display.

## Controls
- **Attack / Decay / Sustain / Release**: envelope stage times and sustain level.
- **Attack CV / Decay CV / Sustain CV / Release CV**: bipolar amount of external CV applied to each stage.
- **Push**: button to trigger the envelope.

## Inputs
- **Attack / Decay / Sustain / Release**: CV inputs for each stage.
- **Gate**: held gate signal that sustains the envelope.
- **Trigger**: retriggers the envelope from the start.
- **Retrigger**: also retriggers the envelope.
- **Position**: CV to scrub through the envelope shape.

## Outputs
- **Envelope**: the ADSR envelope signal (0–10 V, polyphonic).
- **End of cycle**: trigger output that fires when the envelope completes.
