# ra-bump — Trigger Sequencer

A trigger sequencer that steps through 8 toggleable on/off steps, outputting a trigger when an active step is reached.

## Controls
- **Step** knob: step time in milliseconds (0–1000 ms, default 250 ms).
- **Mode** switch: selects passthrough behavior.
  - **Passthrough** — input triggers pass through to the output.
  - **No pass** — input triggers are ignored; only the internal sequencer runs.
- **Push** button: manually advance to the next step.

## Inputs
- **In**: trigger input to advance the sequencer.
- **Clock**: clock input for syncing the sequencer to an external clock.

## Outputs
- **Out**: trigger output. Fires when an active step is reached.

## Notes
- The sequencer loops through 8 steps.
- Each step can be toggled on/off by clicking on it in the UI.
- The sequencer can be clocked externally or run at the internal step time.
