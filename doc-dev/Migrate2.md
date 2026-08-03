# Migrate2

> Source: https://vcvrack.com/manual/Migrate2

The API of VCV Rack 2 has been designed to be nearly backward-compatible with v1 plugins.
This means that 90% of plugins will only require a version update and a recompile (a 1-line edit, 15 seconds of work).

For the other 10% of plugins using advanced or unstable API features, updating to v2 is easy and involves following a few search-and-replace steps.

Additionally, Rack 2 includes several new optional enhancements that plugin developers can use to improve usability of their plugins in [section 3](#3-New-optional-v2-API-features).
### 1.1) Update `plugin.json` version string to v2 


The major version number (`MAJOR.MINOR.REVISION`) of your plugin must match the major version of Rack, so change the version number in your plugin’s `plugin.json` manifest to `2`.
For example, change
```
{
  "slug": "Fundamental",
  "version": "1.2.3",
  ...

```


to
```
{
  "slug": "Fundamental",
  "version": "2.0.0",
  ...

```


If you wish, you may keep the minor and revision numbers unchanged, e.g. `2.2.3`.

Download the latest [Rack 2 SDK](https://vcvrack.com/downloads/).
Clean and compile the plugin.
```
export RACK_DIR=/path/to/Rack-SDK
make clean
make dist

```


If your plugin compiles successfully, you can skip ahead to [Potential runtime bugs](#2-Potential-runtime-bugs).
If not, read on to solve potential errors or warnings.
### 1.2) `ParamWidget::paramQuantity` has been replaced by `getParamQuantity()`


If your `ParamWidget` methods use the `paramQuantity` member variable, replace it with the getter method, or add this line to the top of your method.
```
ParamQuantity* paramQuantity = getParamQuantity();

```

### 1.3) `ParamWidget::reset()` and `ParamWidget::randomize()` has been removed 


If you would like to disable resetting or randomization of a particular parameter when the user resets/randomizes the module, set these properties in your `Module` constructor.
```
getParamQuantity(MY_PARAM)->resetEnabled = false;
getParamQuantity(MY_PARAM)->randomizeEnabled = false;

```


Alternatively, you can override the following methods in your `Module` class to implement custom behavior for *all* parameters and internal state.
```
void onReset(const ResetEvent& e) override {
	params[MY_PARAM].setValue(0.f);
	// ...

	// Call super method if you wish to include default behavior
	// Module::onReset(e);
}

void onRandomize(const RandomizeEvent& e) override {
	params[MY_PARAM].setValue(random::uniform());
	// ...

	// Call super method if you wish to include default behavior
	// Module::onRandomize(e);
}

```


If your plugin still has build errors, open a thread in the [VCV development forum](https://community.vcvrack.com/c/development/8) or contact [VCV support](https://vcvrack.com/support) to describe your build error.
## 2) Potential runtime bugs 


You might encounter these issues while testing.
### 2.1) Don’t store `Font` and `Image` references across multiple frames 


The DAW plugin version of Rack uses a different OpenGL context each time the plugin editor window is closed and reopened.
This means that if you load and store a `Font` or `Image` in a widget’s constructor with `font = APP->window->loadFont(...)` or `image = APP->window->loadImage(...)`, its OpenGL reference will be invalid if the window is reopened later.

Instead, save only its path, and fetch the font/image each frame in `draw()`. Example:
```
void draw(const DrawArgs& args) override {
	std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
	if (font) {
		nvgFontFaceId(args.vg, font->handle);
		...
	}
}


```


`loadFont()` and `loadImage()` caches the font/image by its path, so this operation can be efficiently called in `draw()` every frame.
## 3) New optional v2 API features 


While not required, these new API features in Rack 2 can enhance the usability of your modules.
### 3.1) Add Port and Light labels 


Add names to your ports and lights which appear in tooltips.

For example, in your `Module` constructor:
```
configInput(PITCH_INPUT, "1V/oct pitch");
configOutput(SIN_OUTPUT, "Sine");
configLight(PHASE_LIGHT, "Phase");

```

### 3.2) Replace `configParam()` with `configButton()` or `configSwitch()` as applicable 


For momentary buttons and multi-state switches, displaying a real-valued parameter to the user doesn’t make much sense.

Instead, use `configButton()` to hide the text field in its context menu, or `configSwitch()` to offer a list of choices.
```
configButton(TAP_PARAM);
configSwitch(SYNC_PARAM, 0, 1, 0, "Sync mode", {"Soft", "Hard"});

```

### 3.3) Add bypass routes 


If your module is bypassed by the user (via the context menu or key command), you can route certain inputs directly to outputs, bypassing all processing.
This would make sense for a filter or reverb, or even a clock divider or quantizer, but it would not make sense for a VCO, since it generates a signal rather than processes one.

For example, in your `Module` constructor:
```
configBypass(LEFT_INPUT, LEFT_OUTPUT);
configBypass(RIGHT_INPUT, RIGHT_OUTPUT);

```


Alternatively, override `Module::processBypass()` to implement custom bypass behavior.
### 3.4) Store large data in the module’s patch storage directory 


Instead of serializing large (>100 KB) buffers in `toJson/fromJson()` methods which could lag the UI, read/write to the directory returned by `createPatchStorageDirectory()` and `getPatchStorageDirectory()`.

Example:
```
void onAdd(const AddEvent& e) override {
	std::string path = system::join(createPatchStorageDirectory(), "wavetable.wav");
	// Read file...
}

void onSave(const SaveEvent& e) override {
	std::string path = system::join(createPatchStorageDirectory(), "wavetable.wav");
	// Write file...
}

```


Note: You cannot call these methods in the `Module` constructor since it is not added to the Engine at that point.
### 3.5) Draw custom light-like widgets at full brightness 


Rack 2 allows users to decrease the brightness of the rack, leaving lights at full brightness.

If your custom widget should emit light, override `Widget::drawLayer()` and draw on layer 1 using
```
void drawLayer(const DrawArgs& args, int layer) override {
	if (layer == 1) {
		...
	}
	Widget::drawLayer(args, layer);
}

```
