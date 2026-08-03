# Installing

> Source: https://vcvrack.com/manual/Installing

To make sure Rack can run on your computer, see [Rack 2 System Requirements](https://vcvrack.com/Rack#info).


## Installing Rack 


Download Rack on the [VCV Rack product page](https://vcvrack.com/Rack).
### Installing on Mac 


Double-click on the installer and follow the on-screen steps.

Launch VCV Rack from the Applications folder.
### Installing on Windows 


Run the installer and follow the on-screen steps.

In the Rack Pro installer, you may set a custom VST path during the installation process.

Launch VCV Rack from the desktop icon or start menu item.
### Installing on Linux 


Unzip the ZIP file.
Make sure `zenity` is installed on your system.

Open Rack’s folder and double click the `Rack` binary.
Or with the command-line, `cd` into Rack’s folder and run `./Rack`.
## Installing Rack plugins 


Plugins extend VCV Rack’s functionality by adding one or more modules to use in your patch.
Plugins are typically installed via the [VCV Library](https://library.vcvrack.com/).
See the *VCV Library Instructions* section at the bottom of the VCV Library page.

If your computer is offline, you may download plugins using another computer and transfer `<Rack user folder>/plugins-<OS>-<CPU>/` (See [Where is the “Rack user folder”?](FAQ#where-is-the-rack-user-folder)) to the offline computer.
Downloading plugins directly from the VCV Library is not supported at this time.
## Installing plugins not available on the VCV Library 


*Install third-party plugins at your own risk. Like VST plugins, installing plugins from unknown sources may compromise your computer and personal information.*

Plugins for Rack are distributed as `.vcvplugin` files.
Download the plugin package from the vendor’s website to `<Rack user folder>/plugins-<OS>-<CPU>/` (See [Where is the “Rack user folder”?](FAQ#where-is-the-rack-user-folder)).
Rack will extract and load the plugin upon launch.

Note: The “major” version number (e.g. the `2` in `v2.3.4`) must match the major version number of Rack. See [ABI/API Version](Version).
## Command line usage 


To launch Rack from the command line, `cd` into Rack’s folder, and run `./Rack`, optionally with the following options.

  - `<patch filename>`: Loads a patch file.
  - `-s / --system <Rack system folder>`: Sets Rack’s system folder, containing read-only program resources. Defaults to


  - MacOS: `<app bundle path>/Contents/Resources`
  - Windows: install location, such as `C:\Program Files\VCV\Rack`
  - Linux: current working directory
  - `-u / --user <Rack user folder>`: Sets Rack’s user folder, containing settings, plugins, and patches. See [Where is the “Rack user folder”?](FAQ##Where-is-the-Rack-user-folder) for the default location.
  - `-d / --dev`: Enables development mode.
This sets the system and user folders to the current working directory, uses the terminal (stderr) for logging, and disables Rack’s Library menu to prevent overwriting plugins.
  - `-h / --headless`: Launches the autosaved patch with no window.
Great for generative patches in museum exhibits.
Patch can be controlled with MIDI.
  - `-a / --safe`: Launches Rack with no plugins or autosave patch.
Useful for testing.
  - `-t / --screenshot <zoom factor>`: Captures screenshots of all installed modules and saves each to `<Rack user folder>/screenshots/<plugin slug>/<module slug>.png`. A zoom factor of 1 generates a screenshot with 380px height.
  - `-v / --version`: Prints Rack version and exits.

You may also set the `RACK_SYSTEM_DIR` or `RACK_USER_DIR` environment variables to override these locations.
