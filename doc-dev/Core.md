# Core

> Source: https://vcvrack.com/manual/Core

The *Core* plugin (built into Rack itself) includes utilities and interfaces for interfacing between the virtual and hardware world.

  - [Audio](#Audio)
  - [MIDI input modules](#MIDI-input-modules)

  - [MIDI-CV](#MIDI-CV)
  - [MIDI-CC](#MIDI-CC)
  - [MIDI-Gate](#MIDI-Gate)
  - [MIDI-Map](#MIDI-Map)
  - [MIDI output modules](#MIDI-output-modules)

  - [CV-MIDI](#CV-MIDI)
  - [CV-CC](#CV-CC)
  - [CV-Gate](#CV-Gate)
  - [Blank](#Blank)
  - [Notes](#Notes)
## Audio 


The *Audio* module merges the virtual Rack world with the physical hardware world.
The **TO DEVICE** section sends Rack signals to a hardware audio device for playback, and the **FROM DEVICE** section receives hardware signals into Rack.

*Audio* currently supports the following **drivers**.

  - [Core Audio](https://developer.apple.com/library/content/documentation/MusicAudio/Conceptual/CoreAudioOverview/WhatisCoreAudio/WhatisCoreAudio.html) on MacOS
  - [WASAPI](https://msdn.microsoft.com/en-us/library/windows/desktop/dd371455%28v=vs.85%29.aspx) on Windows
  - [ASIO](https://en.wikipedia.org/wiki/Audio_Stream_Input/Output) on Windows
  - [ALSA](http://alsa-project.org/main/index.php/Main_Page) on Linux
  - [JACK](http://www.jackaudio.org/) on Linux

After a driver is selected, a particular **device** can be chosen for the driver.

The **sample rate** is the number of audio samples per second for the audio device to process.
Note that this rate is different than Rack’s [engine sample rate](MenuBar#sample-rate), which determines the number of samples per second for Rack modules to process.
If set to different rates, sample rate conversion will occur, resulting in slightly higher CPU usage, slightly less audio fidelity, and slightly more latency.

The **block size** sets the number of samples to store in the audio buffer before releasing to the audio device.
A higher size results in more latency (`blockSize / sampleRate` seconds), but a lower size requires your operating system to communicate with the audio device more frequently, resulting in potentially less audio stability.
A good balance can be found by increasing the block size until no audio “glitches” are heard.

Note: Using multiple Audio modules is experimental and may crash Rack or render unstable audio.
Most DAWs avoid this feature entirely by restricting audio to a single input and a single output device for stability reasons, but if using multiple audio devices in Rack works with your configuration, more power to you!
## MIDI input modules 


Each MIDI input module (described below) supports the following drivers.

  - [Core MIDI](https://developer.apple.com/documentation/coremidi?language=objc) on MacOS
  - [Windows MIDI](https://docs.microsoft.com/en-us/windows/desktop/Multimedia/midi-functions) on Windows
  - [ALSA](http://alsa-project.org/main/index.php/Main_Page) on Linux
  - [JACK](http://www.jackaudio.org/) on Linux
  - Gamepad
  - Computer keyboard

The *gamepad* MIDI driver allows USB video game controllers to be used for CV and gate sources, as an inexpensive alternative to MIDI controllers.
Gamepad buttons are mapped to MIDI note gates starting with `C-1`, `C#-1`, `D-1`, etc.
Joystick axes are mapped to MIDI CC messages starting with `CC0`, `CC1`, `CC2`, etc. with a nonstandard MIDI extension that allows negative CC values to be used.

The *computer keyboard* MIDI driver generates MIDI notes when keys are presses while the Rack window is focused.
Using four rows of computer keys, the two-row virtual MIDI keyboard spans approximately 2½ octaves and can be shifted down and up with the ``` and `1` keys.

The following is the layout for the QWERTY (US) keyboard.


### MIDI-CV 


  - **V/OCT** generates a 1V/oct pitch signal of the last held MIDI note.
  - **GATE** generates 10V when a key is held. It does not retrigger when notes are played legato.
  - **VEL** generates a CV signal from 0V to 10V of the velocity
  - **AFT** generates aftertouch CV (channel pressure, not polyphonic aftertouch)
  - **PW** generates pitch wheel CV from -5V to 5V
  - **MW** generates mod wheel CV.
  - **CLK** generates a clock for every [24-PPQN](https://en.wikipedia.org/wiki/Pulses_per_quarter_note) MIDI clock received.
  - **CLK/N** generates a clock specified by the division set by right-clicking on the panel and selecting the *CLK/N rate*.
  - **RTRG** generates a trigger when a new note is pressed, regardless of legato.
  - **STRT**, **STOP**, and **CONT** generate triggers when the MIDI device sends a start, stop, or continue event.

Right-click the panel to enable polyphony and select the number of polyphonic channels.

  - **Rotate**: Each pressed note selects the next available channel, or just the next channel if none are available, wrapping around to the first channel after the last channel is reached.
  - **Reuse**: If a channel with the same MIDI note has been previously used, reuse it. Otherwise, use the Reset mode behavior.
  - **Reset**: Each pressed note selects the lowest available channel. Releasing a note shifts all above channels down.

Multiple MIDI interfaces may be used simultaneously with the same driver, for example to combine the functionality of MIDI-CV and MIDI-CC.
### MIDI-CC 


Each output maps a MIDI CC (Continuous Controller) messages from 0 to 127 to a CV signal from 0V to 10V.
Some drivers like the gamepad driver generate nonstandard MIDI values from -128 to 127, which is mapped from -10V to 10V.
14-bit MIDI CC messages are not yet supported.

Each output can be assigned a particular CC number by clicking on the digital display corresponding to the position of the output port.
A **LRN** indicator is displayed to represent that the port is in “learn” mode.
Either type a number or move a controller to set the CC number.
### MIDI-Gate 


MIDI-Gate is similar to MIDI-CC, except that it generates 10V gate signals when a particular note is held.
For MIDI sequencers and drum machines that send immediate note ON/OFF messages in sequence, a 1 ms trigger is produced.

To generate a CV signal corresponding to the velocity of the note instead of 10V, right-click on the panel and enable *Velocity* mode.
This is useful for setting the amplitude of percussive sounds for MIDI controllers with velocity-capable pads, for example.
### MIDI-Map 


Controls parameters of other Rack modules via a hardware MIDI CC controller.

To map a hardware control to a Rack parameter, click an “Unmapped” slot in MIDI-Map’s display.
Its text will change to “Mapping…”.
Click a parameter (knob, slider, button, etc) of another module in Rack.
Then wiggle a knob/slider or press a CC button on your MIDI controller.
(These steps may be done in either order.)
The parameter is now mapped and will be moved when you use your MIDI controller.
## MIDI output modules 


The following modules send MIDI messages to external hardware such as desktop synthesizers, Eurorack modules, and drum machines.
### CV-MIDI 


Converts CV in Rack to MIDI for external hardware.

Useful for performing hardware synthesizers with Rack.
### CV-CC 


Converts CV in Rack to MIDI CC (Continuous Controller) commands for external hardware.

Useful for controlling parameters of hardware synthesizers with Rack.
### CV-Gate 


Converts gates in Rack to MIDI note on/off commands for external hardware.

Useful for performing hardware drum machines with Rack.
## Blank 


Useful for adding space between modules in your rack.

You can resize the panel by dragging the edges horizontally, with a minimum size of 3HP.
## Notes 


Useful for adding patch notes, section titles for organization, instructions, and author information to your patches.
You can copy and paste text with Ctrl+C and Ctrl+V.
