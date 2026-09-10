# ra-logic — 8 Configurable Logic Gates

Eight independent boolean logic gates. Each gate combines its A and B inputs and drives its output to 10 V when the result is high, 0 V when low. Inputs above 1 V count as high.

## Controls
- **Screen**: shows the code of the selected gate mode.
- **Mode button**: advances the gate to the next mode. Modes cycle AND → OR → XOR → NAND → NOR → XNOR → NOT A → NOT B.

## Inputs
- **A**: first gate input.
- **B**: second gate input (unused in NOT A mode).

## Outputs
- **Out**: 10 V gate when the logic result is high.

## Modes
- **AND** — high when A and B are both high.
- **OR** — high when either A or B is high.
- **XOR** — high when A and B differ.
- **NAND** — inverse of AND.
- **NOR** — inverse of OR.
- **XNOR** — high when A and B match.
- **NOT A** — inverse of A.
- **NOT B** — inverse of B.

Gate modes are saved with the patch.
