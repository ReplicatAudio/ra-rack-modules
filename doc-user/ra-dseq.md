# ra-dseq — 64-Step Drum Sequencer

Eight sequences of 64 steps, edited on an 8×8 pad grid and played through 8 trigger outputs.

## Controls
- **Length**: loop length in steps (1–64, default 16). Steps beyond the set length are dimmed on the grid.
- **Chance**: per-step probability (0–100%, default 100%) that a set step actually plays. The Chance CV input adds to it (±10 V = full range).
- **Seq prev / Seq next**: buttons that step through the 8 sequences (displayed/edited, and played in song mode).
- **Step prev / Step next**: buttons that step the sequencer backward/forward manually, sounding the new step.
- **Shift left / Shift right**: buttons that rotate the current sequence's steps, wrapping around the full 64.
- **Clear**: clears all steps of the currently selected sequence.
- **Mode**: `Multi` (default) sends each sequence to its own output (sequence 1 → Out 1 … sequence 8 → Out 8), all in lockstep; `Song` sends only the selected sequence to **Out 1**, so you can chain patterns by changing the Sequence position.
- **Output**: `Gate` (default) holds the output high for the whole duration of a set step; `Trig` emits a ~10 ms pulse at the start of each set step.

## Pads
- 8×8 grid of 64 LED buttons: **white = step set**, **purple = step unset**.
- The **current step** is shown as an extra-bright LED with a glow halo.
- A row of LED buttons below the grid selects the current sequence directly (the lit one is active).
- Click a pad to toggle its step; drag across pads to paint.

## Inputs
- **Step next**: trigger input that advances the sequencer one step on each rising edge.
- **Step prev**: trigger input that steps the sequencer backward on each rising edge.
- **Seq prev / Seq next**: trigger inputs that step to the previous/next sequence (wrapping) on each rising edge.
- **Shift left / Shift right**: trigger inputs that rotate the current sequence's steps left/right, wrapping around the full 64.
- **Chance CV**: bipolar CV added to the Chance knob (±10 V = full range).

## Outputs
- **Out 1–8**: trigger/gate outputs as described above.