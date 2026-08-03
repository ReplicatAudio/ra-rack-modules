# Panel

> Source: https://vcvrack.com/manual/Panel

## Setup 


Install [Inkscape](https://inkscape.org/), a cross-platform open-source vector graphics editor, to create panels for Rack modules in the [SVG](https://en.wikipedia.org/wiki/Scalable_Vector_Graphics) format.

  - Create a new document.
  - Open the “Document Properties” panel (Shift+Ctrl+D), and make sure “Units” and “Display units” are set to “mm”. “px” is not supported.
  - Set the height to 128.5 mm and the width to a desired multiple of 5.08 mm ([1 HP](http://www.doepfer.de/a100_man/a100m_e.htm) in Eurorack).
## Designing your panel 


Design the panel, or open an existing .pdf, .ai, .svg, etc file and copy-paste the contents into your new document.


Design recommendations from VCV:

  - Design panels as if you are designing hardware.


  - Make sure there is enough space between knobs and ports to put your thumbs between them.
  - Use an inverted background for output ports (see *VCV VCO* panel above).
  - Labels should succinctly state the purpose of knobs, switches, and ports.
  - Roughly follow the graphical density and text sizes of VCV modules.


  - Text should be readable at 100% on a non-high-DPI monitor.

Do not use other people’s intellectual property (IP) without their permission.
See the [VCV Plugin Ethics Guidelines](PluginLicensing#vcv-plugin-ethics-guidelines).

Don’t hesitate to ask the [VCV Community](https://community.vcvrack.com/c/development) for design help.
You may find several graphic designers seeking programmers for collaboration.
## SVG limitations 


The [SVG standard](https://www.w3.org/TR/SVG2/) is extremely broad, and very few (or no) renderers support the entire standard.
Rack’s SVG renderer does not support the following features.

  - **Text and fonts**: All text objects must be converted to paths. This can be achieved in Inkscape with *Path > Object to Path*. As a benefit, this avoids license issues with embedding proprietary fonts.
  - **Complex gradients**: Simple two-color linear gradients may work.
  - **CSS**: Although, most `fill-*` and `stroke-*` properties are supported in inline style attributes.
## Adding components 


A *component* is a graphical element that can be manipulated in C++.
A component can be a param (knob, switch, button, slider), input, output, light, or a custom widget (e.g. an LED display drawn procedurally in C++).

The actual component graphic should not be included on the panel.
Instead, add placeholders so that the `helper.py` script can generate C++.

  - Open the “Layers” panel (Ctrl+Shift+L), and create a layer named `components`.
  - For each desired component, create a particular shape on the components layer.


  - Use the circle tool (F5) to position a placeholder by its center.
The size of the circle does not matter, only the center point.
A `create*Centered()` function call is generated in C++.
  - Or use the rectangle tool (F4) to to position a placeholder by its top-left point.
This should only be used when the component’s size needs to be defined on the panel, such as a rectangular LED display.
A `create*()` function call is generated in C++.
  - Set the color of each shape depending on the component’s type.


  - **Param**: red `#ff0000`
  - **Input**: green `#00ff00`
  - **Output**: blue `#0000ff`
  - **Light**: magenta `#ff00ff`
  - **Custom widgets**: yellow `#ffff00`
  - To save time editing the .cpp file later, you may name each component to automatically generate names in C++.
Open the “Object Properties” panel (Shift+Ctrl+O), select a component placeholder, and type in the “Label” field.
Note that you must press “Set” or Enter to apply the new name.
  - Hide the components layer using the eye icon in the “Layers” panel and save the file to `res/<module slug>.svg`.

Run
```
<Rack SDK>/helper.py createmodule <module slug> res/<module slug>.svg src/<module slug>.cpp

```


to automatically convert your panel into a template .cpp source file.
