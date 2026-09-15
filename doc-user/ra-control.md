# ra-control — Animated Cables & Scope

A global display module (ported from VCV-Biset's Blank) that re-draws every cable
in the patch with the signal travelling along it, and shows an oscilloscope of any
hovered cable's waveform. Unlike Blank, ra-control has a fixed-width panel with
every option exposed as an actual on-panel control instead of a context menu.

Only one instance of ra-control drives the overlays at a time; adding a second has
no effect until the first is removed.

## Cable Controls

- **Enable** (on/off): master toggle for the animated-cable rendering. When on, the
  rack's default cables are hidden and replaced by the animated ones.
- **LED** (on/off): lights the plug ends with the signal polarity (green positive,
  red negative) and draws a soft halo around the ports.
- **Bright** (on/off): make the cable colour follow the rack brightness setting.
- **PolyT** (on/off): draw polyphonic cables thicker than monophonic ones.
- **Poly** (1st / Sum / Sum/count): how a polyphonic signal is sampled — first
  channel, the sum of all channels, or the sum divided by the channel count.
- **Fast** (on/off): use the cheaper, less precise cable animation mode.
- **Slew**: smooths the animated displacement of the cable.
- **Scale**: amplitude of the signal-driven wave displacement along the cable.

## Scope Controls

- **Enable** (on/off): master toggle for the hovered-cable oscilloscope.
- **Shift** (on/off): when on, the scope only appears while Shift is held.
- **Circle** (Circular / Linear): scope timebase — circular (scrolls as a ring) or
  linear (redraws from the start of the buffer).
- **Pos** (Top left / Top right / Bottom left / Bottom right / Center): where the
  scope is drawn on the screen.
- **Scale**: size of the scope rectangle.
- **Width**: scope line thickness.
- **Back**: background darkness inside the scope.
- **Volt**: alpha of the 0/5/10 V guide lines.
- **Label**: alpha of the "X output to Y input" label.
- **Alpha**: overall opacity of the scope.

## Ports

This module has no inputs or outputs — it only observes and redraws the cables
already present in the patch.