# Rackmods (RA-VCV)

A collection of free ReplicatAudio VCV-Rack modules. 

**Major work in progress. Many things are incomplete and will change. Do not use these in serious projects that you want to come back to later!** 

More docs and graphics coming soon. 

## Docs

[Module docs](./doc-user/)!

See [Dev docs](./doc-dev/) for development and building info. Specifially the [tips doc](./doc-dev/tips.md). 

## Usage

```bash
# Clone
git clone https://github.com/ReplicatAudio/ra-rack-modules

# Move to project root
cd ra-rack-modules

# Pull the vcv rack sdk
./util/pull-sdk.sh

# Build and install the plugin (all modules)
./util/make.sh
```

This will automatically install the modules as well (at least for linux).

## Building the Cardinal fork

The repo also contains a fork of [Cardinal](https://github.com/DISTRHO/Cardinal) in
[`cardinal/`](./cardinal/) (RaCardinal). It statically compiles the same ReplicatAudio
modules — `cardinal/plugins/ReplicatAudio` is a symlink to [`modules/`](./modules/) —
together with Cardinal's built-in host modules. Instead of a VCV Rack plugin, the build
produces LV2, VST3, and CLAP plugin bundles you can load in a regular DAW.

### Requirements

- Cardinal ships its dependencies as git submodules (Rack, DPF, Carla, QuickJS, plus their
  nested submodules). After cloning, initialize them:

  ```bash
  git submodule update --init --recursive
  ```

- Linux build dependencies. See [`cardinal/docs/BUILDING.md`](./cardinal/docs/BUILDING.md)
  for your distribution. For Debian/Ubuntu (vendored libraries, the default):

  ```bash
  sudo apt install cmake libdbus-1-dev libgl1-mesa-dev liblo-dev libfftw3-dev libmagic-dev libsndfile1-dev libx11-dev libxcursor-dev libxext-dev libxrandr-dev python3 wget
  ```

### Build & install

[`./util/build-cardinal.sh`](./util/build-cardinal.sh) builds the fork with all modules
and installs the bundles into your user plugin directories (`~/.lv2`, `~/.vst3`, `~/.clap`):

```bash
./util/build-cardinal.sh
```

Set `JOBS` to override the parallel build jobs: `JOBS=8 ./util/build-cardinal.sh`.

To only build without installing, run make directly:

```bash
make -C cardinal all
```

The bundles end up in `cardinal/bin/`. To install them manually, symlink them into your
plugin directories, e.g.:

```bash
ln -s "$(pwd)"/cardinal/bin/*.lv2 ~/.lv2/
```

Note: the build-cardinal.sh install script currently only supports Linux. Restart your
DAW after installing to pick up the new bundles.

## Contibutions

Contibutions are limited to people I know personally at this time. If you know me and want to make a PR feel free. PRs may be open to the public in the future. 

## License

All code is GPL3 or later unless otherwise specified. 

All art assets are copyright Mathieu Dombrock. No 3rd party artwork has been used. 

### Font

Internal svg font is `Bitstream Vera Sans Mono` (OFL)

URW GOTHIC

