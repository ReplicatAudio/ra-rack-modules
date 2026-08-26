/*
 * DISTRHO Cardinal Plugin
 * Copyright (C) 2021-2026 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Modified for the RaCardinal fork: only the built-in Cardinal Host modules
 * and the ReplicatAudio modules are compiled in. All third-party module
 * collections are removed.
 */

#include "rack.hpp"
#include "plugin.hpp"

#include "DistrhoUtils.hpp"

// Cardinal (built-in)
#include "Cardinal/src/plugin.hpp"

// ReplicatAudio (init is extern "C" per Rack's plugin/callbacks.hpp)
extern Plugin* pluginInstance__ReplicatAudio;
extern "C" void init__ReplicatAudio(Plugin* p);

// theme support (provided here since no third-party plugin headers define it)
enum ModuleTheme
{
    LIGHT_THEME,
    DARK_THEME
};

ModuleTheme defaultPanelTheme = DARK_THEME;
void addThemeMenuItems(Menu*, ModuleTheme*) {}

// known terminal modules
std::vector<Model*> hostTerminalModels;

// stuff that reads config files, we don't want that
int loadConsoleType() { return 0; }
bool loadDarkAsDefault() { return settings::preferDarkPanels; }
ModuleTheme loadDefaultTheme() { return settings::preferDarkPanels ? DARK_THEME : LIGHT_THEME; }
int loadDirectOutMode() { return 0; }
void readDefaultTheme() { defaultPanelTheme = loadDefaultTheme(); }
void saveConsoleType(int) {}
void saveDarkAsDefault(bool) {}
void saveDefaultTheme(ModuleTheme) {}
void saveDirectOutMode(bool) {}
void saveHighQualityAsDefault(bool) {}
void writeDefaultTheme() {}

// plugin instances
Plugin* pluginInstance__Cardinal;
// pluginInstance__ReplicatAudio is defined by ReplicatAudio/src/plugin.cpp

namespace rack {

namespace asset {
std::string pluginManifest(const std::string& dirname);
std::string pluginPath(const std::string& dirname);
}

namespace plugin {

static uint32_t numPluginModules = 0;

struct StaticPluginLoader {
    Plugin* const plugin;
    FILE* file;
    json_t* rootJ;

    StaticPluginLoader(Plugin* const p, const char* const name)
        : plugin(p),
          file(nullptr),
          rootJ(nullptr)
    {
#ifdef DEBUG
        DEBUG("Loading plugin module %s", name);
#endif

        p->path = asset::pluginPath(name);

        const std::string manifestFilename = asset::pluginManifest(name);

        if ((file = std::fopen(manifestFilename.c_str(), "r")) == nullptr)
        {
            d_stderr2("Manifest file %s does not exist", manifestFilename.c_str());
            return;
        }

        json_error_t error;
        if ((rootJ = json_loadf(file, 0, &error)) == nullptr)
        {
            d_stderr2("JSON parsing error at %s %d:%d %s", manifestFilename.c_str(), error.line, error.column, error.text);
            return;
        }

        std::string version;
        if (json_t* const versionJ = json_object_get(rootJ, "version"))
            version = json_string_value(versionJ);

        if (!string::startsWith(version, APP_VERSION_MAJOR + "."))
        {
            // force ABI, we use static plugins so this doesnt matter as long as it builds
            json_t* const versionJ = json_string((APP_VERSION_MAJOR + ".0").c_str());
            json_object_set(rootJ, "version", versionJ);
            json_decref(versionJ);
        }

        // Load manifest
        p->fromJson(rootJ);

        // Reject plugin if slug already exists
        if (Plugin* const existingPlugin = getPlugin(p->slug))
            throw Exception("Plugin %s is already loaded, not attempting to load it again", p->slug.c_str());
    }

    ~StaticPluginLoader()
    {
        if (rootJ != nullptr)
        {
            // Load modules manifest
            json_t* const modulesJ = json_object_get(rootJ, "modules");
            plugin->modulesFromJson(modulesJ);

            json_decref(rootJ);
            plugins.push_back(plugin);

            numPluginModules += plugin->models.size();
        }

        if (file != nullptr)
            std::fclose(file);
    }

    bool ok() const noexcept
    {
        return rootJ != nullptr;
    }
};

static void initStatic__Cardinal()
{
    Plugin* const p = new Plugin;
    pluginInstance__Cardinal = p;

    const StaticPluginLoader spl(p, "Cardinal");
    if (spl.ok())
    {
        p->addModel(modelCardinalBlank);
        p->addModel(modelExpanderInputMIDI);
        p->addModel(modelExpanderOutputMIDI);
        p->addModel(modelHostAudio2);
        p->addModel(modelHostAudio8);
        p->addModel(modelHostCV);
        p->addModel(modelHostMIDI);
        p->addModel(modelHostMIDICC);
        p->addModel(modelHostMIDIGate);
        p->addModel(modelHostMIDIMap);
        p->addModel(modelHostParameters);
        p->addModel(modelHostParametersMap);
        p->addModel(modelHostTime);

        hostTerminalModels = {
            modelHostAudio2,
            modelHostAudio8,
            modelHostCV,
            modelHostMIDI,
            modelHostMIDICC,
            modelHostMIDIGate,
            modelHostMIDIMap,
            modelHostParameters,
            modelHostParametersMap,
            modelHostTime,
        };
    }
}

static void initStatic__ReplicatAudio()
{
    Plugin* const p = new Plugin;
    pluginInstance__ReplicatAudio = p;

    const StaticPluginLoader spl(p, "ReplicatAudio");
    if (spl.ok())
        init__ReplicatAudio(p);
}

void initStaticPlugins()
{
    initStatic__Cardinal();
    initStatic__ReplicatAudio();

    INFO("Have %u modules from %u plugin collections",
         numPluginModules, static_cast<uint32_t>(plugins.size()));
}

void destroyStaticPlugins()
{
    for (Plugin* p : plugins)
        delete p;
    plugins.clear();
}

void updateStaticPluginsDarkMode()
{
    const bool darkMode = settings::preferDarkPanels;
    // no bundled third-party plugins support dark panels in this fork
    (void) darkMode;
}

}
}