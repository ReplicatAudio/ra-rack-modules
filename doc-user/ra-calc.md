# ra-calc — 4-Channel Math Module

A general-purpose math module with 4 independent channels. Each channel has two inputs (A and B), a mode button to select the operation, an attenuverter, a clamp toggle, and one output.

## Controls
- **Mode n** button: cycles through the 17 math operations.
- **A n** knob + **In n A** input: first operand. Input overrides knob when connected.
- **B n** knob + **In n B** input: second operand. Input overrides knob when connected.
- **Attn n** knob: attenuverter for the output (-1 to 1, default 1). Scales and optionally inverts the output.
- **Clamp n** LED button: toggles output clamping on/off. When on, output is limited to ±10 V. LED is purple when active.

## Operations
- **ADD** — A + B
- **SUB** — A - B
- **MULT** — A × B
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
- **LOG** — ln|A + B|

## Display
Each channel has two screens:
- **Top screen**: shows the current operation name.
- **Bottom screen**: shows input A (green), input B (green), and output (purple). Values exceeding 6 characters show `...`. Infinite values show `∞`.

## Notes
- For single-input functions (ABS, SQRT, FLOOR, CEIL, ROUND, SIN, COS, LOG), both inputs are summed together.
- Inputs override knobs when connected — the knob value is ignored.
- All four channels are independent.
- The attenuverter applies after the math operation and after clamping.
