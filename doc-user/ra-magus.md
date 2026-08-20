# ra-magus — Scheme Expression Evaluator

Evaluates a Scheme expression at audio rate using the four inputs A, B, C, D (or their knob values when unpatched). The result is written to the output.

## Controls
- **A / B / C / D**: manual values used when the corresponding input is unpatched.
- **Expression field**: enter a Scheme expression; `a`, `b`, `c`, `d` are the input variables.
- **write**: apply the edited expression.
- **clear**: clear the expression.
- **Status LED**: green = valid, red = evaluation error, blue = uncommitted edit (click to view errors).

## Inputs
- **A / B / C / D**: CV inputs available as expression variables.

## Outputs
- **Out**: the evaluated result (updated at audio rate).
