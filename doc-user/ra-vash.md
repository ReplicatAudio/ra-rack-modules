# ra-vash — 64-Step Drum Sequencer

Eight sequences of 64 steps, edited on an 8×8 pad grid and played through 8 trigger outputs.

## Controls
- **Length**: loop length in steps (0–64) for the **currently displayed sequence**. The knob applies pickup-style — it only updates that sequence's length while it is being turned, and switching sequences never rewrites stored lengths. Sequence 1 defaults to 16; sequences 2–8 default to 0 (off - silent, and skipped by trigger-driven sequence changes). Steps beyond the set length are dimmed on the grid.
- **Chance**: per-step probability (0–100%, default 100%) that a set step actually plays. The Chance CV input adds to it (±10 V = full range).
- **Randomize**: button that randomizes the current sequence — each step independently has a **Randomize chance** probability (0–100%, default 50%) of being flipped to a random on/off state.
- **Reset**: button that jumps back to step 1 of the current sequence and plays it immediately.
- **Reset sequence**: button that jumps to sequence 1 at step 1 and plays it immediately.
- **Run**: latching button that starts/stops the transport. While stopped, the external **Step next** / **Step prev** trigger inputs are ignored; the **Step next** / **Step prev** buttons still step the sequencer.
- **GOL step**: button that evolves the **currently displayed sequence's** grid by one generation of Conway's Game of Life. The whole 8×8 pad grid is the universe — all 64 cells, including steps beyond the loop length. Neighbours wrap around the edges (toroidal). Rules: a live cell survives with 2–3 live neighbours; a dead cell becomes live with exactly 3; everything else dies or stays dead. **Randomize** the sequence first to seed a pattern, then press repeatedly or clock the trigger input to watch it evolve.
- **Seq prev / Seq next**: buttons that step through all 8 sequences (displayed/edited, and played in song mode) — zero-length sequences are not skipped when using the buttons.
- **Step prev / Step next**: buttons that step the sequencer backward/forward manually, sounding the new step.
- **Shift left / Shift right**: buttons that rotate the current sequence's steps, wrapping around the full 64.
- **Clear**: clears all steps of the currently selected sequence.
- **Mode**: `Multi` (default) sends each sequence to its own output (sequence 1 → Out 1 … sequence 8 → Out 8), all in lockstep; `Song` sends only the selected sequence to **Out 1**, so you can chain patterns by changing the Sequence position. Trigger-driven chain changes skip zero-length sequences, so silent gaps are skipped automatically.
- **Output**: `Gate` (default) holds the output high for the whole duration of a set step; `Trig` emits a ~10 ms pulse at the start of each set step.

## Pads
- 8×8 grid of 64 LED buttons: **white = step set**, **purple = step unset**.
- The **current step** is shown as an extra-bright LED with a glow halo.
- A column of LED buttons to the **left** of the grid (sequence 1 at top → 8 at bottom) selects the current sequence directly; the lit one is active.
- Click a pad to toggle its step; drag across pads to paint.
- The pad grid doubles as the Game of Life universe: **GOL step** evolves it and **Randomize** can seed it.

## Inputs
- **Step next**: trigger input that advances the sequencer one step on each rising edge.
- **Step prev**: trigger input that steps the sequencer backward on each rising edge.
- **Seq prev / Seq next**: trigger inputs that step to the previous/next sequence (wrapping) on each rising edge, automatically skipping sequences whose length is 0.
- **Shift left / Shift right**: trigger inputs that rotate the current sequence's steps left/right, wrapping around the full 64.
- **Randomize**: trigger input that randomizes the current sequence, using the Randomize chance knob.
- **Reset**: trigger input that returns the sequencer to step 1 of the current sequence on each rising edge, playing it immediately.
- **Reset sequence**: trigger input that returns to sequence 1 at step 1 on each rising edge, playing it immediately.
- **Chance CV**: bipolar CV added to the Chance knob (±10 V = full range).
- **Run**: gate input — the sequencer runs while this is high or while the Run button is latched.
- **GOL step**: trigger input that runs the next Game of Life generation on the displayed sequence's grid on each rising edge — same as the button, useful for clocking the evolution.

## Outputs
- **Out 1–8**: trigger/gate outputs as described above.