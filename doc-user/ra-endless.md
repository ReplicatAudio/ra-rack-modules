# ra-endless — Endless Sequencer

Records and plays back sequences of 1 V/oct pitch CV, in the style of a tape-style (endless) sequencer. Two independent tracks.

## Controls
- **Track select**: choose which track to record/edit.
- **Write / Rest / Clear / Reset**: recording and editing actions.
- **Step next / Step prev**: move the edit cursor.
- **Seq next / Seq prev / Seq reset**: sequence navigation.
- **Sequences**: number of sequences.
- **Repeats**: number of repeats.
- **Run**: start/stop playback.
- **Song mode**: chain sequences (Off, A, B).
- **Track A/B slew**: portamento between steps per track.
- **Passthrough**: pass external CV through when not running.

## Inputs
- **Pitch CV (1V/oct)**: CV recorded into steps.
- **Position CV**: external step position (0–10 V).
- **Write / Rest / Clear / Reset / Step next / Step prev / Seq next / Seq prev / Seq reset / Run**: trigger/CV inputs for each action.

## Outputs
- **Track A/B pitch CV**: sequence pitch output per track.
- **Track A/B step trigger**: pulse on each step advance.
- **Track A/B sequence end**: pulse at end of sequence.
