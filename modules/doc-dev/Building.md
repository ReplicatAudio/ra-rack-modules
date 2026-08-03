# Building

> Source: https://vcvrack.com/manual/Building

## Setting up your development environment 


Before building Rack or Rack plugins, you must install build dependencies provided by your system’s package manager.
Rack’s own dependencies (GLEW, glfw, etc) do not need to be installed on your system, since specific versions are compiled locally during the build process.
However, you need proper tools to build Rack and these dependencies.
### Windows 


If you have an anti-virus program running, disable it or it may interfere with the build process or make builds very slow.

Install [MSYS2](http://www.msys2.org/) and launch the MinGW 64-bit shell from the Start menu, *not the default MSYS shell*.
Update the package manager itself:
```
pacman -Syu

```


Then restart the shell and install packages.
```
pacman -Syu git wget make tar unzip zip mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-cmake autoconf automake libtool mingw-w64-x86_64-jq python zstd mingw-w64-x86_64-pkgconf

```

### Mac 


Install [Homebrew](https://brew.sh/), and install build dependencies.
```
brew install git wget cmake autoconf automake libtool jq python zstd pkg-config

```

### Linux 


On Ubuntu 16.04+:
```
sudo apt install unzip git gdb curl cmake libx11-dev libglu1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev zlib1g-dev libasound2-dev libgtk2.0-dev libgtk-3-dev libjack-jackd2-dev jq zstd libpulse-dev pkg-config autoconf libtool

```


On Arch Linux:
```
pacman -S git wget gcc gdb make cmake tar unzip zip curl jq python zstd libpulse pkgconf

```

## Building Rack 


*You do not need to build Rack to build plugins if you use the Rack SDK.*

Clone this repository with `git clone https://github.com/VCVRack/Rack.git` and `cd Rack`.
Make sure there are no spaces in your absolute path, since this breaks the Makefile-based build system.

Clone submodules.
```
git submodule update --init --recursive

```


Build dependencies locally.
You may add `-j4` (or your number of logical cores) to your `make` commands to parallelize builds.
This may take 15-60 minutes.
```
make dep

```


Build Rack.
This may take 1-5 minutes.
```
make

```


Run Rack.
```
make run

```

## Building Rack plugins 


Complete the [Setting up your development environment](#Setting-up-your-development-environment) section.

Plugins can be built in three ways:

  - Download [VCV Rack](https://vcvrack.com/Rack) and the Rack SDK ([Windows x64](https://vcvrack.com/downloads/Rack-SDK-latest-win-x64.zip) / [Mac x64](https://vcvrack.com/downloads/Rack-SDK-latest-mac-x64.zip) / [Mac ARM64](https://vcvrack.com/downloads/Rack-SDK-latest-mac-arm64.zip) / [Linux x64](https://vcvrack.com/downloads/Rack-SDK-latest-lin-x64.zip)), and build plugins from any folder. (Easiest/fastest.)
  - [Build Rack from source](#Building-Rack) and build plugins in the `plugins/` folder. (Recommended for advanced developers.)
  - Build for all architectures with one command using the [VCV Rack Plugin Toolchain](https://github.com/VCVRack/rack-plugin-toolchain). Native (Linux) or Docker (Linux, Mac, Windows). Recommended 15 GB disk space, 8 GB RAM.

Download or clone the plugin source code, e.g.
```
git clone https://github.com/VCVRack/Fundamental.git

```


Clone the git repo’s submodules.
```
cd Fundamental
git submodule update --init --recursive

```


If using the Rack SDK, unzip it and set the `RACK_DIR` environment variable by running `export RACK_DIR=<Rack SDK dir>`.

Build plugin dependencies. (Most plugins don’t require this step.)
```
make dep

```


Build the plugin.
```
make

```


Create the distributable plugin package.
```
make dist

```


Your package is created at `dist/<slug>-<version>-<os>-<cpu>.vcvplugin`.

Or you may build, package, and install plugins to your [Rack user folder](FAQ#Where-is-the-Rack-user-folder) in one step.
```
make install

```
