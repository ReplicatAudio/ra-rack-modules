# ra-countdown — Trigger Countdown

Counts trigger events, then fires a trigger or a gate. Each increment advances
the running count by one; when the count reaches the `Count` target, the output
fires.

## Controls
- **Screen**: shows the current running count.
- **Count**: target number of increments (0–256, quantized). The output fires
  once the running count reaches this value.
- **Inc**: button to add one to the running count.
- **Dec**: button to subtract one from the running count.
- **Zero**: button to force the output to the Zero level and restart the
  countdown (this releases a held gate).
- **Max**: button to set the running count straight to the **Count** target,
  firing the output immediately.
- **Zero lvl**: output voltage when not fired (default 0 V).
- **Max lvl**: output voltage when fired (default 10 V).
- **Mode**: output mode — **Trig** (short trigger pulse) or **Gate** (stays
  high until a Zero).
- **Auto**: auto reset — **Off** (one-shot until Zero) or **Auto** (restart the
  countdown immediately after firing; the gate is still held until a Zero).

## Inputs
- **Count CV**: overrides the Count knob when connected.
- **Inc trig / Dec trig**: trigger inputs for increment and decrement.
- **Zero trig / Max trig**: trigger inputs for the Zero and Max overrides.
- **Zero output CV / Max output CV**: CV inputs that override the Zero lvl and
  Max lvl knobs when connected.

## Outputs
- **Out**: trigger pulse or gate, at `Zero lvl` to `Max lvl` V.

## Behaviour
- The running count starts at 0. Each **Inc trig** event (or **Inc** button)
  adds 1.
- When the count reaches the **Count** target, the output fires.
  - **Trig mode** emits a short pulse.
  - **Gate mode** stays high until a **Zero** (in **Auto** mode the countdown
    restarts for the next cycle, but the gate is held until a Zero).
- **Max** (button or trig) sets the count to the **Count** target, firing the
  output immediately.
- **Zero** (button or trig) forces the output to the Zero level and restarts
  the countdown.