#!/usr/bin/make -f
# DISTRHO Cardinal Plugin
# Copyright (C) 2021-2026 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-3.0-or-later

ROOT = .
include $(ROOT)/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Set version

# also set in:
# .github/ISSUE_TEMPLATE/bug.yaml
# src/CardinalCommon.cpp `CARDINAL_VERSION`
# src/CardinalPlugin.cpp `getVersion`
# utils/macOS/Info_{JACK,Native}.plist
# .github/ISSUE_TEMPLATE/bug.yaml src/CardinalCommon.cpp src/CardinalPlugin.cpp utils/macOS/Info_{JACK,Native}.plist
VERSION = 26.02

# ---------------------------------------------------------------------------------------------------------------------
# Build targets

all: cardinal deps dgl plugins gen resources

# ---------------------------------------------------------------------------------------------------------------------
# Build config

PREFIX  ?= /usr/local
DESTDIR ?=

# ---------------------------------------------------------------------------------------------------------------------
# Carla config

CARLA_EXTRA_ARGS = \
	CARLA_BACKEND_NAMESPACE=Cardinal \
	DGL_NAMESPACE=CardinalDGL \
	HAVE_FFMPEG=false \
	HAVE_FLUIDSYNTH=false \
	HAVE_PROJECTM=false \
	HAVE_ZYN_DEPS=false \
	HAVE_ZYN_UI_DEPS=false

ifneq ($(DEBUG),true)
CARLA_EXTRA_ARGS += EXTERNAL_PLUGINS=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# DGL config

ifeq ($(WASM),true)
UI_TYPE = gles2
else
UI_TYPE = opengl
endif

# ---------------------------------------------------------------------------------------------------------------------
# Check for required system-wide dependencies

ifeq ($(SYSDEPS),true)

ifneq ($(shell $(PKG_CONFIG) --exists jansson && echo true),true)
$(error jansson dependency not installed/available)
endif
ifneq ($(shell $(PKG_CONFIG) --exists libarchive && echo true),true)
$(error libarchive dependency not installed/available)
endif
ifneq ($(shell $(PKG_CONFIG) --exists samplerate && echo true),true)
$(error samplerate dependency not installed/available)
endif
ifneq ($(shell $(PKG_CONFIG) --exists speexdsp && echo true),true)
$(error speexdsp dependency not installed/available)
endif

endif

ifeq ($(HEADLESS),true)
ifneq ($(STATIC_BUILD),true)

ifneq ($(shell $(PKG_CONFIG) --exists liblo && echo true),true)
$(error liblo dependency not installed/available)
endif

endif
endif

# --------------------------------------------------------------
# Check for X11+OpenGL dependencies (unless headless build)

ifneq ($(HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS),true)
ifneq ($(HEADLESS),true)

ifneq ($(HAVE_OPENGL),true)
$(error OpenGL dependency not installed/available)
endif
ifneq ($(HAVE_X11),true)
$(error X11 dependency not installed/available)
endif
ifneq ($(HAVE_XCURSOR),true)
$(warning Xcursor dependency not installed/available)
endif
ifneq ($(HAVE_XEXT),true)
$(warning Xext dependency not installed/available)
endif
ifneq ($(HAVE_XRANDR),true)
$(warning Xrandr dependency not installed/available)
endif

else

CARLA_EXTRA_ARGS += HAVE_DBUS=false
CARLA_EXTRA_ARGS += HAVE_DGL=false
CARLA_EXTRA_ARGS += HAVE_OPENGL=false
CARLA_EXTRA_ARGS += HAVE_X11=false
CARLA_EXTRA_ARGS += HAVE_XCURSOR=false
CARLA_EXTRA_ARGS += HAVE_XEXT=false
CARLA_EXTRA_ARGS += HAVE_XRANDR=false

endif
endif

# --------------------------------------------------------------
# Check for optional system-wide dependencies

ifeq ($(shell $(PKG_CONFIG) --exists fftw3f && echo true),true)
HAVE_FFTW3F = true
else
$(warning fftw3f dependency not installed/available)
endif

# --------------------------------------------------------------
# MOD builds

EXTRA_MOD_FLAGS  = -I../include/single-precision -fsingle-precision-constant -Wno-attributes
ifeq ($(MODDUO),true)
EXTRA_MOD_FLAGS += -mno-unaligned-access
endif
ifeq ($(WITH_LTO),true)
EXTRA_MOD_FLAGS += -ffat-lto-objects
endif

MOD_ENVIRONMENT += HEADLESS=true
MOD_ENVIRONMENT += STATIC_BUILD=true

# --------------------------------------------------------------
# Individual targets

cardinal: deps dgl plugins resources
	$(MAKE) all -C src $(CARLA_EXTRA_ARGS)

deps:
ifeq ($(SYSDEPS),true)
	$(MAKE) quickjs surge -C deps
else
	$(MAKE) all -C deps NOPLUGINS=true
endif

dgl:
ifneq ($(HEADLESS),true)
	$(MAKE) $(UI_TYPE) -C dpf/dgl $(DGL_EXTRA_ARGS)
endif

plugins: deps
	$(MAKE) all -C plugins

resources:
	$(MAKE) resources -C plugins

loader-resources:
	$(MAKE) loader-resources -C plugins

mini-plugins: deps
	$(MAKE) mini -C plugins

mini-resources:
	$(MAKE) mini-resources -C plugins

ifneq ($(CROSS_COMPILING),true)
gen: cardinal resources dpf/utils/lv2_ttl_generator
	@$(CURDIR)/dpf/utils/generate-ttl.sh

loader-gen: loader loader-resources dpf/utils/lv2_ttl_generator
	@$(CURDIR)/dpf/utils/generate-ttl.sh

dpf/utils/lv2_ttl_generator:
	$(MAKE) -C dpf/utils/lv2-ttl-generator
else
gen:
loader-gen:
endif

# --------------------------------------------------------------
# --------------------------------------------------------------
# extra rules, for quick testing

au:
	$(MAKE) au -C src $(CARLA_EXTRA_ARGS)

clap: deps dgl plugins resources
	$(MAKE) clap -C src $(CARLA_EXTRA_ARGS)

lv2: deps dgl plugins resources
	$(MAKE) lv2 -C src $(CARLA_EXTRA_ARGS)

vst3: deps dgl plugins resources
	$(MAKE) vst3 -C src $(CARLA_EXTRA_ARGS)

# Clean step

clean:
	$(MAKE) clean -C deps
	$(MAKE) clean -C dpf/dgl
	$(MAKE) clean -C dpf/utils/lv2-ttl-generator
	$(MAKE) clean -C plugins
	$(MAKE) clean -C src
	rm -rf bin build build-headless dpf/utils/lv2_ttl_generator.d

# --------------------------------------------------------------
# Install step

install:
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/Cardinal.lv2
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/CardinalFX.lv2
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/CardinalSynth.lv2
	install -d $(DESTDIR)$(PREFIX)/lib/clap/Cardinal.clap
	install -d $(DESTDIR)$(PREFIX)/lib/clap/CardinalFX.clap
	install -d $(DESTDIR)$(PREFIX)/lib/clap/CardinalSynth.clap
ifneq ($(VST3_BINARY_DIR),)
	install -d $(DESTDIR)$(PREFIX)/lib/vst3/Cardinal.vst3/$(VST3_BINARY_DIR)
	install -d $(DESTDIR)$(PREFIX)/lib/vst3/CardinalFX.vst3/$(VST3_BINARY_DIR)
	install -d $(DESTDIR)$(PREFIX)/lib/vst3/CardinalSynth.vst3/$(VST3_BINARY_DIR)
endif
	install -d $(DESTDIR)$(PREFIX)/share/cardinal

	install -m 644 bin/Cardinal.lv2/*.*      $(DESTDIR)$(PREFIX)/lib/lv2/Cardinal.lv2/
	install -m 644 bin/CardinalFX.lv2/*.*    $(DESTDIR)$(PREFIX)/lib/lv2/CardinalFX.lv2/
	install -m 644 bin/CardinalSynth.lv2/*.* $(DESTDIR)$(PREFIX)/lib/lv2/CardinalSynth.lv2/

	install -m 644 bin/Cardinal.clap/*.*      $(DESTDIR)$(PREFIX)/lib/clap/Cardinal.clap/
	install -m 644 bin/CardinalFX.clap/*.*     $(DESTDIR)$(PREFIX)/lib/clap/CardinalFX.clap/
	install -m 644 bin/CardinalSynth.clap/*.*  $(DESTDIR)$(PREFIX)/lib/clap/CardinalSynth.clap/

ifneq ($(VST3_BINARY_DIR),)
	install -m 644 bin/Cardinal.vst3/$(VST3_BINARY_DIR)/*      $(DESTDIR)$(PREFIX)/lib/vst3/Cardinal.vst3/$(VST3_BINARY_DIR)/
	install -m 644 bin/CardinalFX.vst3/$(VST3_BINARY_DIR)/*    $(DESTDIR)$(PREFIX)/lib/vst3/CardinalFX.vst3/$(VST3_BINARY_DIR)/
	install -m 644 bin/CardinalSynth.vst3/$(VST3_BINARY_DIR)/* $(DESTDIR)$(PREFIX)/lib/vst3/CardinalSynth.vst3/$(VST3_BINARY_DIR)/
endif

	cp -rL bin/Cardinal.lv2/resources/* $(DESTDIR)$(PREFIX)/share/cardinal/

# --------------------------------------------------------------
# Tarball step, for releases

download:
	$(MAKE) download -C deps

tarball:
	$(MAKE) clean -C deps
	rm -f ../cardinal-$(VERSION).tar.xz
	tar -c --lzma $(TAR_ARGS) -f ../cardinal-$(VERSION).tar.xz .

tarball+deps: download
	rm -f ../cardinal+deps-$(VERSION).tar.xz
	tar -c --lzma $(TAR_ARGS) -f ../cardinal+deps-$(VERSION).tar.xz .

version:
	@echo $(VERSION)

# --------------------------------------------------------------

.PHONY: carla deps plugins
