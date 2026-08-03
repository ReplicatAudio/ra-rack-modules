# KeyCommands

> Source: https://vcvrack.com/manual/KeyCommands

*In this document, use Command (⌘) instead of Ctrl on Mac.*
## Global commands 


Key commands documented in Rack’s [menu bar](MenuBar) are not documented here.

  - **Enter** or **right-click on rack**: Launch the Module Browser to add modules to the rack.
  - **F1**: Open Rack manual in your browser.
  - **F3**: Show/hide CPU meter.
  - **F11**: Enter/exit fullscreen.
  - **Ctrl+0**: Set rack zoom level to 100%.
  - **Ctrl+-**: Zoom out.
  - **Ctrl+=**: Zoom in.
  - **Scroll**: Scroll rack vertically.
  - **Shift+scroll**: Scroll rack horizontally.
  - **Ctrl+scroll**: Zoom in/out.
  - **Middle mouse button drag**: Pan rack view.
  - **Arrow keys**: Pan rack view. You can use Ctrl, Shift, or Ctrl+shift to change the speed of scrolling.
  - **Ctrl+V**: Pastes a new module at the cursor position, from the module context menu “Preset > Copy” or Ctrl+C.
## Module commands 


You must hover your mouse over a module to issue key commands for it.
Key commands documented in the module context menu are not documented here.

  - **Right click** on panel: Open module context menu.
  - **Click and drag** on panel: Drag module in the rack.
  - **Ctrl+click and drag** on panel: Force-drag module. This moves other modules in order to place the dragged module.
  - **Backspace** or **Delete**: Delete module from the rack. You can hold this key while moving your mouse to delete multiple modules.
## Parameter commands 


A parameter is any knob, slider, button, switch, or other custom component that controls a module’s state.

  - **Click and drag vertically**: Adjust value. Hold **Ctrl** while dragging to move the knob slowly, **Ctrl+shift** to move very slowly, or **Shift** to move quickly.
  - **Double-click**: Reset to default value.
  - **Right-click**: Open parameter context menu. You can enter an exact value into the text field, or a mathematical expression.
Normal mathematical expressions like `2+2`, `2*2`, `1/2`, and `2^2` are evaluated to numerical values immediately upon pressing Enter.
You can use constants and functions below, which are case-insensitive.


  -

**C**, **A#**, **Gb**, etc: Frequencies in Hz of note names starting with middle C
  -

**C4**, **A#5**, **Gb0**, etc: Frequencies in Hz of note names with octave numbers (C4 = middle C)
  -

**C4v**, **A#5v**, **Gb0v**, etc: Voltage of notes assuming 1V/octave and C4v = 0V. *Added in Rack 2.6.1.*
  -

**log2(x)**: [Logarithm](https://en.wikipedia.org/wiki/Logarithm) of *x* with base 2
Example: `log2(8) = 3`
  -

**gaintodb(x)**: Converts a gain/amplitude factor to [decibel (dB)](https://en.wikipedia.org/wiki/Decibel) units.

Example: `gaintodb(1) = 0`, `gaintodb(0.5) = -6.0206`, `gaintodb(0) = -inf`
  -

**dbtogain(x)**: Converts a decibel (dB) value to gain units.

Example: `dbtogain(20) = 10`, `dbtogain(0) = 1`, `dbtogain(-inf) = 0`
  -

**vtof(x)**: Converts a 1V/octave value in volts to frequency in Hz, centered at middle-C (261.63 Hz).

Example: `vtof(0) = 261.63`, `vtof(-1) = 130.81`
  -

**ftov(x)**: Converts a frequency value in Hz to 1V/octave value in volts.

Example: `ftov(440) = 0.75`, `ftov(c#4) == 0.083333`
