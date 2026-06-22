# MenuBar

> Source: https://vcvrack.com/manual/MenuBar

## File 
New 

Clears the patch and loads the template patch (`<Rack user folder>/template.vcv`).Open/Save/Save as 

Opens/saves the patch from/to a `.vcv` file.Save template 

Saves the patch as `<Rack user folder>/template.vcv` so it is loaded when creating new patches.Revert 

Restores the patch from the last saved state.Quit 

Autosaves the patch to `<Rack user folder>/autosave.vcv` and closes Rack.
Rack also autosaves every 15 seconds to prevent losing work due to a crash, power failure, etc.
## Edit 
Undo/Redo 

Rewinds and plays back all actions that edit the patch.Clear cables 

Removes all cables from the patch.
## View 
Show tooltips 

Enables/disables tooltips when hovering over parameters (knobs, sliders, buttons, etc), ports, and lights which display useful information like the name, value, and description.Zoom 

Adjusts the zoom level for the rack from 25-400%.
Double-click to reset to 100%.

You can also use Ctrl-`-` and Ctrl-`=` (or Cmd on Mac) to zoom, or Ctrl-scroll, or mouse buttons #4 and #5 for multi-button mice.Cable opacity 

Sets the transparency level for patch cables.
Double-click to reset to 50%.Cable tension 

Sets the relative length of patch cables.
Double-click to reset to 50%.Frame rate 

Sets Rack’s screen refresh frequency.
Rack re-draws its screen every [monitor v-sync](https://en.wikipedia.org/wiki/Refresh_rate), or once every  v-syncs determined by this setting.
Decreasing the frame rate results in almost exactly proportionally decreased GPU resources consumed by Rack, at the cost of more choppy motion.Fullscreen 

Expands Rack to fill the screen with no window borders.Lock cursor when dragging params 

*Documentation coming soon*Knob mode 

*Documentation coming soon*Scroll wheel knob control 

*Documentation coming soon*Lock module positions 

Locks/unlocks modules from being moved by the mouse.
Prevents accidentally dragging modules.
## Engine 
Performance meters 

Enables/disables the measurement of time for each module to generate each sample (see [Sample rate](#sample-rate)).
This displays a meter on each module with the percentage of time spent in the module’s assigned thread (see [Threads](#threads)).

The CPU meter consumes engine CPU time itself, so it is recommended to disable it when it is not needed for best performance.Sample rate 

Sets Rack’s engine sample rate.

Rack’s engine works by repeatedly stepping the state of each module by `1 / sampleRate` seconds, producing 1 new sample for each output.
A higher sample rate decreases the timestep, resulting in more accurate analog circuit modeling and digital algorithms, while a lower sample rate decreases engine CPU usage.
Engine CPU usage is almost exactly proportional to its sample rate.Threads 

Sets the number of cores to be used in Rack’s multithreaded engine.

To minimize power usage and CPU resources for other programs to use, disable multithreading by selecting 1 thread.
Using more threads increases CPU usage but increases the maximum number of modules you can use in the rack.

It is recommended to begin with 1 thread, and if you experience audio hiccups, try increasing the thread count by 1.
Continue adding threads until no hiccups occur.
The maximum recommended number of threads is the number of physical cores in your machine.
For example, a quad-core machine would likely maximize the number of modules by using 4 threads.

Due to [simultaneous multithreading](https://en.wikipedia.org/wiki/Simultaneous_multithreading) such as [Intel Hyper-Threading](https://en.wikipedia.org/wiki/Hyper-threading), you may attempt to use up to twice as many threads as physical cores in your machine, but typically this results in worse performance with higher power usage.
## Library 
Login 

Logs into your VCV account registered at [vcvrack.com](https://vcvrack.com/).
Email [support@vcvrack.com](mailto:support@vcvrack.com) for account assistance.Update all 

Updates and downloads all new plugins and plugin versions added to your VCV account.
Rack must be restarted to load new plugin updates.
## Help 
Tips 

Displays the tip window with several useful tips for learning VCV Rack.

By default, the tip window displays at launch but can be disabled by unchecking “Show tips at startup”.User manual 

Opens [this manual](.).Open user folder 

Opens the [Rack user folder](FAQ#where-is-the-rack-user-folder) in Windows Explorer, Apple Finder, etc.
