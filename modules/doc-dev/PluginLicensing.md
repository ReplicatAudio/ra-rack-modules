# PluginLicensing

> Source: https://vcvrack.com/manual/PluginLicensing

VCV Rack is free/open-source software, but you should still familiarize yourself with the [VCV Rack licenses](https://github.com/VCVRack/Rack/blob/v2/LICENSE.md) before releasing your plugin, to avoid misuse of intellectual property. If in doubt, send your licensing questions to [support@vcvrack.com](mailto:support@vcvrack.com).
## I want to release my plugin under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.en.html) (or later) free/open-source license (GPLv3+). 


Since Rack is licensed under GPLv3+, you may license your plugin under GPLv3+ as well.

VCV recommends that supporters of free/open-source software license their Rack plugins under GPLv3+ ([SPDX license identifier](https://spdx.org/licenses/)`GPL-3.0-or-later`).
This license gives users the [Four Essential Freedoms](https://www.gnu.org/philosophy/free-sw.html) of free software and follows [OSI’s Open Source Definition](https://opensource.org/osd-annotated).

Licensing your Rack plugin under GPLv3+ requires that derivative works are also licensed as free software under the GPLv3.
This prevents a company from turning your source code into proprietary (non-free) software without your explicit permission.
## I want to release my plugin under a different open-source license or freeware. 


Rack offers a [VCV Rack Non-Commercial Plugin License Exception](https://github.com/VCVRack/Rack/blob/v1/LICENSE.md) which allows you to license your plugin under any terms of your choice, as long as it is offered free of charge.
You may choose:

  - **Open-source**. Examples of non-GPL licenses: [BSD 3-clause](https://opensource.org/licenses/BSD-3-Clause), [MIT](https://opensource.org/licenses/MIT), and [CC0](https://creativecommons.org/publicdomain/zero/1.0/).
  - **Closed-source freeware**.
  - **Donationware**, as long as a donation is not required for use (otherwise you need a [commercial plugin license](#i-want-to-sell-my-plugin-commercially-under-non-gplv3-terms)).

Note that if you copy significant portions of Rack’s code into your own plugin, you must license it under GPLv3.

If someone makes a fork of your non-GPLv3 open-source plugin that is not a Rack plugin (e.g. a port to VST or a digital hardware module), their source code becomes no longer linked to VCV Rack and is thus no longer a “derived work” of Rack, so Rack’s license does not apply to their source code.
## I want to sell my plugin commercially under non-GPLv3 terms. 


VCV offers commercial royalty licensing for Rack plugins by emailing [support@vcvrack.com](mailto:support@vcvrack.com).
This license also includes permission to use the [Component Library](https://github.com/VCVRack/Rack/blob/v1/include/componentlibrary.hpp) graphics by [Grayscale](https://grayscale.info/), which are normally licensed for non-commercial use only.

It is recommended to contact VCV as early as possible in your development process to make sure the license agreement is ready well before you release your plugin.
You can expedite the licensing processing by sending concepts or design mockups along with your license request.

You may also wish to sell your plugin on the [VCV Library](https://library.vcvrack.com/).
Some benefits of distributing your plugin on the VCV Library:

  - Most Rack users are already familiar with the VCV Library checkout system.
  - Plugin updates are automatically synchronized to users’ computers.
  - VCV offers advanced technical support with the Rack SDK and DSP library.
  - You may supply VCV with either binary packages for Mac/Windows/Linux, or a source package which we will build for you.
  - Access to dashboard for managing customers’ purchases and viewing real-time statistics.
## VCV Plugin Ethics Guidelines 


  - You may not clone the brand name, model name, logo, panel design, or layout of components (knobs, ports, switches, etc) of an existing hardware or software product without permission from its owner, regardless of whether these are covered under trademark/copyright law.

It is recommended to follow these guidelines for all plugins, but you are not legally obligated to do so.
However, it is a requirement for:

  - distributing your plugin on the [VCV Library](https://library.vcvrack.com/).
  - obtaining a [commercial plugin license](#i-want-to-sell-my-plugin-commercially-under-non-gplv3-terms).
