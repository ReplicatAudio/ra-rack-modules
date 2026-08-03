# Manifest

> Source: https://vcvrack.com/manual/Manifest

This document defines the `plugin.json` file found at the root of your plugin folder.
For example, see Fundamental’s [plugin.json](https://github.com/VCVRack/Fundamental/blob/v1/plugin.json) file.

JSON paths are denoted in “flat format”, as used by [jq](https://stedolan.github.io/jq/manual/).
## `.slug`


*String. Required.*

The unique identifier for your plugin.
Case-sensitive.
Slugs may only contain letters `a-z` and `A-Z`, numbers `0-9`, hyphens `-`, and underscores `_`.

After your plugin is released, the slug must *never* change, otherwise patch compatibility would be broken.
To guarantee uniqueness, it is a good idea to prefix the slug by your “brand name” if available, e.g. “MyCompany-MyPlugin”.

The word “slug” [comes from web publishing](https://en.wikipedia.org/wiki/Clean_URL#Slug) to represent URL paths that never change, which in turn [comes from typesetting](https://en.wikipedia.org/wiki/Slug_(typesetting)).
## `.name`


*String. Required.*

The human-readable name for your plugin.
Used for labeling your plugin the VCV Library.

Unlike slugs, the name can be changed at any time without breaking patch compatibility.
## `.version`


*String. Required.*

The version of your plugin should follow the form `MAJOR.MINOR.REVISION`.
Do not include the “v” prefix—this is added automatically where appropriate.

The `MAJOR` version should match the version of Rack your plugin is built for, e.g. 2.
You are free to choose the `MINOR.REVISION` part of your plugin version.
For example, *MyPlugin 2.4.2* would specify that your plugin is compatible with *Rack 2.X*.

If you publish the source code in a git repository, it is recommended to add a git tag with `git tag vX.Y.Z` and `git push --tags`.
## `.license`


*String. Required.*

The license of your plugin.

For open-source plugins, use the license identifier string from the [SPDX License List](https://spdx.org/licenses/), such as `"GPL-3.0-or-later"`, `"GPL-3.0-only"`, `"MIT"`, `"BSD-3-Clause"`, `"CC0-1.0"`, etc.

For freeware plugins, use `"proprietary"` (TODO: use `"https://vcvrack.com/freeware-eula"` when available), or your own freeware license URL.

For commercial plugins, use `"https://vcvrack.com/eula"` if sold on the VCV Library, or your own commercial license URL.
## `.brand`


*String. Optional.*

Prefix string for all modules in your plugin.
For example, the brand “VCV” is used by the Fundamental plugin to create “VCV VCF”, VCV Unity", etc.
If blank or undefined, the plugin name is used.
## `.description`


*String. Optional.*

A one-line summary of the plugin’s purpose.
If your plugin doesn’t follow a theme, it’s probably best to omit this.
## `.author`


*String. Required.*

Your name, company, alias, GitHub username, etc.
## `.authorEmail`


*String. Optional.*

Your email address for support inquiries.
## `.authorUrl`


*String. Optional.*

Homepage for yourself or your company.
## `.pluginUrl`


*String. Optional.*

Homepage for the plugin itself.
## `.manualUrl`


*String. Optional.*

The manual website of your plugin.
E.g. HTML, PDF, GitHub readme, GitHub wiki.
## `.sourceUrl`


*String. Optional.*

Homepage for the source code.
For GitHub URLs, use the main project page, not the `.git` URL.
## `.donateUrl`


*String. Optional.*

Link to donation page for users who wish to donate.
E.g. [PayPal.Me](https://www.paypal.me/), [Cash App](https://cash.app/) links.
## `.changelogUrl`


*String. Optional.*

Link to the changelog of the plugin.
## `.minRackVersion`


*String. Optional.*

Minimum version of Rack required to download the plugin from the VCV Library.

VCV Rack 2.4.0+ is required to prevent downloading plugin packages with incompatible `minRackVersion`.
Older Rack versions do not support this property and will download all plugin versions regardless of its value.
## `.modules[].slug`


*String. Required.*

The unique identifier for the module.
Similar guidelines from [.slug](#slug) apply.
## `.modules[].name`


*String. Required.*

The human-readable name for the module.
## `.modules[].tags`


*Array of strings. Optional.*

List of tags representing the functions and/or properties of the module.
All tags must match the following strings, case-insensitive.“Arpeggiator” 

Breaks a chord into a sequence of single notes.“Attenuator” 

With a level knob and not much else.“Blank” 

No parameters or ports. Serves no purpose except visual.“Chorus” “Clock generator” 

*Deprecated aliases: “Clock”*“Clock modulator” 

Clock dividers, multipliers, etc.“Compressor” 

With threshold, ratio, knee, etc parameters.“Controller” 

Use only if the artist “performs” with this module. Simply having knobs is not enough. Examples: on-screen keyboard, XY pad.“Delay” “Digital” “Distortion” “Drum” 

*Deprecated aliases: “Drums”, “Percussion”*“Dual” 

The core functionality times two. If multiple channels are a requirement for the module to exist (ring modulator, mixer, etc), it is not a Dual module.“Dynamics” “Effect” “Envelope follower” “Envelope generator” “Equalizer” 

*Deprecated aliases: “EQ”*“Expander” 

Expands the functionality of a “mother” module when placed next to it. Expanders should inherit the tags of its mother module.“External” 

Interfaces with hardware or software outside VCV Rack such as a MIDI/audio interface, webcam, DAW, robotic dog, etc.“Filter” 

*Deprecated aliases: “VCF”, “Voltage controlled filter”*“Flanger” “Function generator” 

Generates complex envelopes or cycles via a large selection of general parameters.“Granular” “Hardware clone” 

*Deprecated aliases: “Hardware”*

Clones the functionality, panel design, and component layout of a hardware module on the market.

Remember to follow the [VCV Plugin Ethics Guidelines](PluginLicensing#vcv-plugin-ethics-guidelines) and only clone hardware with permission from its owner.

If there is a [ModularGrid](https://www.modulargrid.net/) page for the hardware module, add the [modularGridUrl](Manifest#modules-modulargridurl) property.

If the module name does not include the hardware’s name or brand, add that information to the [module description](Manifest#modules-description) so users can search for the module by its hardware name, e.g. “Based on Mutable Instruments Clouds”.“Limiter” 

Limits a signal from exceeding a threshold, e.g. with soft clipping or dynamic range compression.“Logic” 

Implements binary logic with gate signals.“Low-frequency oscillator” 

*Deprecated aliases: “LFO”, “Low frequency oscillator”*“Low-pass gate” 

*Deprecated aliases: “Low pass gate”, “Lowpass gate”*“MIDI” 

Generates, processes, or interprets MIDI messages, e.g. for a MIDI interface.“Mixer” 

Mixes/sums audio with optional level adjustment, panning, and/or mix buses.“Multiple” 

Copies an input directly to multiple outputs.
While VCV Rack supports stacking cables on output ports, users sometimes prefer organizing their cables with multiples, allowing easy access to each cable.“Noise” 

Generates audio-rate or CV-rate random noise.“Oscillator” 

*Deprecated aliases: “VCO”, “Voltage controlled oscillator”*“Panning” 

*Deprecated aliases: “Pan”*“Phaser” “Physical modeling” 

Simulates a physical system such as a string, metal bar, vocal chords, chaotic system, celestial bodies, etc.“Polyphonic” 

*Deprecated aliases: “Poly”*“Quad” 

The core functionality times four. If multiple channels are a requirement for the module to exist (ring modulator, mixer, etc), it is not a Quad module.“Quantizer” “Random” “Recording” “Reverb” “Ring modulator” “Sample and hold” 

*Deprecated aliases: “S&H”, “Sample & hold”*“Sampler” “Sequencer” “Slew limiter” “Speech” “Switch” “Synth voice” 

A synth voice must have, at the minimum, a built-in oscillator and envelope.“Tuner” “Utility” 

Serves only extremely basic functions, like inverting, max, min, multiplying by 2, etc.“Visual” “Vocoder” “Voltage-controlled amplifier” 

*Deprecated aliases: “Amplifier”, “VCA”, “Voltage controlled amplifier”*“Waveshaper” 
## `.modules[].description`


*String. Optional.*

A one-line summary of the module’s purpose.
Displayed in the Module Browser tooltip.
## `.modules[].keywords`


*String. Optional.*

String of words, aliases, and abbreviations describing the module to improve searchability in the VCV Library and Rack’s module browser.
It is *not* necessary to copy tags from `modules[].tags`.
Not displayed to the user.

Example: `"24dB 12dB V2164 LM13700 OTA multi-mode analog"`
## `modules[].manualUrl`


*String. Optional.*

The manual website of the module.
If omitted, the plugin’s manual is used.
## `modules[].modularGridUrl`


*String. Optional.*

If this module has the [Hardware clone](#Hardware-clone) tag, this is the URL to the [ModularGrid](https://www.modulargrid.net/) page for that module.
Example: `"https://www.modulargrid.net/e/mutable-instruments-clouds"`

In a partnership with ModularGrid, modules released to the [VCV Library](https://library.vcvrack.com/) will be linked to and from their ModularGrid page.
For example, [Mutable Instruments Clouds on ModularGrid](https://www.modulargrid.net/e/mutable-instruments-clouds) has an “Available for VCV Rack” link to [Audible Instruments Texture Synthesizer on the VCV Library](https://library.vcvrack.com/AudibleInstruments/Clouds).
## `.modules[].hidden`


*Boolean. Optional.*

Specifies that the module should be hidden from Rack’s module browser and VCV Library search results.
If this module is deprecated and a successor module exists, add its name to the [description](Manifest#modules-description), e.g. “Replaced by MyModule 2”.
