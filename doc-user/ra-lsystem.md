# ra-lsystem — L-System Drum/Trigger Sequencer

An L-system drum/trigger module. Symbols are represented by 8 colors (off/black, red, green, blue, yellow, cyan, magenta, white). An **axiom** and a set of **rewrite rules** define a growing string; the module generates that string, steps through it as a sequencer, and fires a trigger/gate per colored symbol.

## Controls

### Axiom row (top)
- A row of **8 clickable color cells**. Click a cell to cycle it through the 8 colors. This row defines the starting string / axiom. Off/black cells are empty and contribute nothing.

### Rule rows (below the axiom)
- **6 rule rows**. Each row has **1 target cell** (left) and **6 result cells**. Click any cell to cycle its color.
- Each rule reads as `target → body`: whenever the symbol matching the target color appears in the generated string, it is replaced by the body (the 6 result cells, skipping off/black cells).
- If a symbol has **no matching rule**, it is left unchanged. Rules with an off/black target never match.

### Output matrix (right)
- An **8×8 non-editable matrix** showing the generated L-system output using the same color conventions, truncated to 64 cells as necessary. The **current sequencer position** is highlighted like in ra-vash.

### Outputs (right of the matrix)
- **7 outputs**, one per active colored symbol (red, green, blue, yellow, cyan, magenta, white). When the sequencer lands on a symbol, that color's output fires.

### Step
- **Step** button and **Step** trigger input step the sequencer forward by one position on each press / rising edge.
- **Output** switch selects `Gate` (default) — holds the output high for the whole step — or `Trig` — emits a ~10 ms pulse at the start of a set step.

## How it works

The generated string starts as the axiom. The rewrite rules are applied repeatedly (each generation replacing every symbol with its rule's body, skipping off/black cells) until the string fills the 8×8 matrix or a termination cap is reached, then the result is truncated to 64 cells. The sequencer loops over the generated string, and at each step fires the output matching the current symbol's color.

## Inputs
- **Step**: trigger input that advances the sequencer one step on each rising edge.

## Outputs
- **Red / Green / Blue / Yellow / Cyan / Magenta / White**: trigger/gate outputs corresponding to the 7 active symbol colors.