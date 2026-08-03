# Rackmods (RA-VCV)

A collection of free ReplicatAudio VCV-Rack modules. 

**Major work in progress. Many things are incomplete and will change. Do not use these in serious projects that you want to come back to later!** 

More docs and graphics coming soon. 

## Docs

For now, see [dev docs](./doc-dev/) for development and building info. Specifially the [tips doc](./doc-dev/tips.md). 

I'm also working on [user docs](./doc-user/)!

## Usage

```bash
git clone https://github.com/ReplicatAudio/ra-rack-modules

# project root
cd ra-rack-modules/modules

# pull the vcv rack sdk
./util/pull-sdk.sh

# build and install the plugin (all modules)
./util/make.sh
```

This will automatically install the modules as well (at least for linux).

## Contibutions

Contibutions are limited to people I know personally at this time. If you know me and want to make a PR feel free. PRs may be open to the public in the future. 

## License

All code is GPL3 or later unless otherwise specified. 
