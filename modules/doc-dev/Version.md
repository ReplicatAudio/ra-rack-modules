# Version

> Source: https://vcvrack.com/manual/Version

An **ABI** ([Application Binary Interface](https://en.wikipedia.org/wiki/Application_binary_interface)) is a machine-readable list of symbols (functions, globals, and classes) that can only be called, accessed, and used by a computer in specific ways.
An **API** ([Application Programming Interface](https://en.wikipedia.org/wiki/Application_programming_interface)) consists of C and C++ headers (`.h`/`.hpp`) that define how the ABI can be used by the compiler.

A version string is structured as `vMAJOR.MINOR.REVISION` and roughly follows the [Semantic Versioning](https://semver.org/) standard.

Rack can be considered as a library for plugins. Applying Semantic Versioning, this means:

  - A major Rack update makes incompatible plugin API and ABI changes.
  - A minor Rack update adds (but not removes or changes) symbols to the plugin API and ABI, making Rack backward compatible with plugins compiled against an older minor Rack version but not forward compatible with plugins compiled against a newer minor Rack version. Users are encouraged to update Rack before updating plugins to ensure that all plugins can be loaded.
  - A revision Rack update does not modify symbols but only changes functionality and fixes bugs in a compatible way.

Rack *plugins* can be considered as applications that link to the Rack library. This means:

  - A plugin’s major version must match Rack’s major version in order to be loaded by Rack. To make a plugin available for a new major Rack version, the plugin’s source must be updated and recompiled against the new Rack SDK.
  - A minor plugin update means that new features or modules were added.
  - A revision plugin update means that bugs were fixed or existing behavior was tweaked.

The most important point for users is that **Rack will only load plugins that match its major (`vX.*`) version.**
## Git branches and tags 


In [Rack’s git repository](https://github.com/VCVRack/Rack), each major version has its own branch, labeled `v1`, `v2`, etc.

There is no `master` branch.
The default branch on GitHub is the major version for the latest release of Rack listed at [vcvrack.com](https://vcvrack.com/).
To update the major version of your source tree, simply check out its branch.
It is not recommended to checkout a branch to upgrade Rack’s major version unless you know how to fully clean a git tree with submodules.

When any version of Rack is released, the build’s commit is tagged with its full version name (e.g. `v1.0.0`).
However, this is only for informative purposes.
Building from “detached heads” of branches is not supported, since dependency URLs and other issues may need to be fixed after time has passed from the release date.
