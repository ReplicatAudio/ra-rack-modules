# FAQ

> Source: https://vcvrack.com/manual/FAQ

## What does “VCV” stand for? 


There is no official meaning of the name “VCV”, but some users have suggested “Virtual Control Voltage” or “Voltage Controlled Virtualization”.
These are good guesses, but “VCV” was chosen simply because it is easy to remember and type.

[VCV Rack](https://vcvrack.com/Rack) is the full name of our flagship software product.


## Where is the “Rack user folder”? 


The Rack user folder stores data readable/writable by Rack.
You can open it by choosing `Help > Open user folder` in the Rack [menu bar](MenuBar), or by navigating to

  - MacOS: `~/Library/Application Support/Rack2/`
  - Windows: `C:\Users\<username>\AppData\Local\Rack2\`
  - Linux: `~/.local/share/Rack2/`

When running Rack in development mode, it is your current working directory instead.


## Does VCV Rack work with touch screens? 


Yes, disable “View > Lock cursor while dragging params” in Rack’s menu and optionally set the “Knob mode” to rotary if you prefer.

VCV Rack does not currently support multi-touch gestures.


## What is a VCV Rack plugin? 


A plugin is a single software unit typically developed by one company or individual that can contain multiple VCV Rack modules.
Plugins are loaded from `<Rack user folder>/plugins-<OS>-<CPU>/`.
