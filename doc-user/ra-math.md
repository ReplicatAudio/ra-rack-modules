# ra-math — 4-Channel Math Module

A general-purpose math module with 4 independent channels. Each channel has two inputs (A and B), a mode button to select the operation, and one output.

## Controls
- **Mode n** button: cycles through the 16 math operations.
- **A n** knob + **In n A** input: first operand. Input overrides knob when connected.
- **B n** knob + **In n B** input: second operand. Input overrides knob when connected.

## Operations
- **ADD** — A + B
- **SUB** — A - B
- **MULT** - A × B
- **DIV** — A ÷ B (returns 0 if B = 0)
- **POW** — A^B
- **MOD** — A mod B (returns 0 if B = 0)
- **MAX** — maximum of A and B
- **MIN** — minimum of A and B
- **AVG** — (A + B) / 2
- **ABS** — |A + B|
- **SQRT** — √|A + B|
- **FLOOR** — floor(A + B)
- **CEIL** — ceil(A + B)
- **ROUND** — round(A + B)
- **SIN** — sin(A + B)
- **COS** — cos(A + B)

## Notes
- For single-input functions (ABS, SQRT, FLOOR, CEIL, ROUND, SIN, COS), both inputs are summed together.
- Inputs override knobs when connected — the knob value is ignored.
- All four channels are independent.
- The display above each channel shows the current operation name.
