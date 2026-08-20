# ra-tyche — Probabilistic Trigger Divider

Probabilistic trigger divider: a single trigger drives eight outputs, each with an independent probability determined by a divisor and the bias setting.

## Controls
- **Bias**: shifts the probability of all outputs (−5 to +5 V equivalent).

## Inputs
- **Bias CV**: modulates the bias.
- **Trigger**: input trigger that advances all divisions.

## Outputs
- **1/2 … 1/256**: each output fires a 10 V pulse with probability of its nominal division, altered by the bias.
