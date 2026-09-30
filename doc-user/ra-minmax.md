# ra-minmax — Min/Max Math Module

A min/max math module with 3 independent channels. Each channel has two inputs (A and B), a mode switch, and one output. LEDs indicate which input is being sent to the output.

## Controls
- **Mode n** switch: selects min or max operation.
  - **Min** — output is the minimum of A and B.
  - **Max** — output is the maximum of A and B.

## Inputs
- **A n**: first input for channel n.
- **B n**: second input for channel n.

## Outputs
- **Out n**: the min or max of A and B for channel n.

## LEDs
- **A n**: purple LED that lights when input A is being sent to the output.
- **B n**: purple LED that lights when input B is being sent to the output.
- Only one LED is lit at a time per channel.

## Notes
- All three channels are independent.
- When A == B, the B LED is lit (B is considered the active input).
