# ra-repeater — Trigger Repeater

Repeats an incoming trigger on 4 outputs, each with independent delay and chance controls, plus a global all-triggers output.

## Controls
- **Del n** knob + **Del CV n**: delay for output n (0–1 s).
- **Cha n** knob + **Cha CV n**: chance for output n (0–100%). Each repeat has this probability of firing.
- **Mode** switch: selects passthrough behavior.
- **Push** button: manually trigger the repeater.

## Inputs
- **In**: trigger input.
- **Clk**: clock input for syncing delays to an external clock.

## Outputs
- **Out n**: repeated trigger output n (with delay and chance applied).
- **All**: global output that fires on every trigger, regardless of chance.

## Notes
- Each output has independent delay and chance controls.
- The chance control allows probabilistic trigger repetition.
- The global output fires on every input trigger.
