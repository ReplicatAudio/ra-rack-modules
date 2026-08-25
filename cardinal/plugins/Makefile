#!/usr/bin/make -f
# Makefile for Cardinal #
# --------------------- #
# Created by falkTX
#

BUILDING_RACK = true
ROOT = ..
include $(ROOT)/Makefile.base.mk

# --------------------------------------------------------------
# List of drwav functions, used in several modules

DRWAV  = drwav
DRWAV += drwav__on_read
DRWAV += drwav__on_seek
DRWAV += drwav__read_and_close_f32
DRWAV += drwav__read_and_close_s16
DRWAV += drwav__read_and_close_s32
DRWAV += drwav_alaw_to_f32
DRWAV += drwav_alaw_to_s16
DRWAV += drwav_alaw_to_s16
DRWAV += drwav_alaw_to_s32
DRWAV += drwav_bytes_to_f32
DRWAV += drwav_bytes_to_s16
DRWAV += drwav_bytes_to_s32
DRWAV += drwav_bytes_to_s64
DRWAV += drwav_bytes_to_u16
DRWAV += drwav_bytes_to_u32
DRWAV += drwav_bytes_to_u64
DRWAV += drwav_close
DRWAV += drwav_close
DRWAV += drwav_container
DRWAV += drwav_data_chunk_size_riff
DRWAV += drwav_data_chunk_size_w64
DRWAV += drwav_data_format
DRWAV += drwav_f32_to_s16
DRWAV += drwav_f32_to_s32
DRWAV += drwav_f64_to_f32
DRWAV += drwav_f64_to_s16
DRWAV += drwav_f64_to_s16
DRWAV += drwav_f64_to_s32
DRWAV += drwav_fmt_get_format
DRWAV += drwav_fopen
DRWAV += drwav_fourcc_equal
DRWAV += drwav_free
DRWAV += drwav_get_cursor_in_pcm_frames
DRWAV += drwav_get_length_in_pcm_frames
DRWAV += drwav_guid_equal
DRWAV += drwav_init
DRWAV += drwav_init_ex
DRWAV += drwav_init_file
DRWAV += drwav_init_file_ex
DRWAV += drwav_init_file_ex_w
DRWAV += drwav_init_file_w
DRWAV += drwav_init_file_with_metadata
DRWAV += drwav_init_file_with_metadata_w
DRWAV += drwav_init_file_write
DRWAV += drwav_init_file_write
DRWAV += drwav_init_file_write__internal
DRWAV += drwav_init_file_write__internal
DRWAV += drwav_init_file_write_sequential
DRWAV += drwav_init_file_write_sequential
DRWAV += drwav_init_file_write_sequential_pcm_frames
DRWAV += drwav_init_file_write_sequential_pcm_frames_w
DRWAV += drwav_init_file_write_sequential_w
DRWAV += drwav_init_file_write_w
DRWAV += drwav_init_memory
DRWAV += drwav_init_memory_ex
DRWAV += drwav_init_memory_with_metadata
DRWAV += drwav_init_memory_write
DRWAV += drwav_init_memory_write
DRWAV += drwav_init_memory_write__internal
DRWAV += drwav_init_memory_write__internal
DRWAV += drwav_init_memory_write_sequential
DRWAV += drwav_init_memory_write_sequential_pcm_frames
DRWAV += drwav_init_with_metadata
DRWAV += drwav_init_write
DRWAV += drwav_init_write
DRWAV += drwav_init_write__internal
DRWAV += drwav_init_write_sequential
DRWAV += drwav_init_write_sequential_pcm_frames
DRWAV += drwav_init_write_with_metadata
DRWAV += drwav_metadata
DRWAV += drwav__metadata_parser
DRWAV += drwav_mulaw_to_f32
DRWAV += drwav_mulaw_to_s16
DRWAV += drwav_mulaw_to_s16
DRWAV += drwav_mulaw_to_s32
DRWAV += drwav_open
DRWAV += drwav_open_and_read_f32
DRWAV += drwav_open_and_read_file_f32
DRWAV += drwav_open_and_read_file_s16
DRWAV += drwav_open_and_read_file_s32
DRWAV += drwav_open_and_read_memory_f32
DRWAV += drwav_open_and_read_memory_s16
DRWAV += drwav_open_and_read_memory_s32
DRWAV += drwav_open_and_read_pcm_frames_f32
DRWAV += drwav_open_and_read_pcm_frames_s16
DRWAV += drwav_open_and_read_pcm_frames_s32
DRWAV += drwav_open_and_read_s16
DRWAV += drwav_open_and_read_s32
DRWAV += drwav_open_ex
DRWAV += drwav_open_file
DRWAV += drwav_open_file_and_read_f32
DRWAV += drwav_open_file_and_read_pcm_frames_f32
DRWAV += drwav_open_file_and_read_pcm_frames_f32_w
DRWAV += drwav_open_file_and_read_pcm_frames_s16
DRWAV += drwav_open_file_and_read_pcm_frames_s16_w
DRWAV += drwav_open_file_and_read_pcm_frames_s32
DRWAV += drwav_open_file_and_read_pcm_frames_s32_w
DRWAV += drwav_open_file_and_read_s16
DRWAV += drwav_open_file_and_read_s32
DRWAV += drwav_open_file_ex
DRWAV += drwav_open_file_write
DRWAV += drwav_open_file_write
DRWAV += drwav_open_file_write__internal
DRWAV += drwav_open_file_write__internal
DRWAV += drwav_open_file_write_sequential
DRWAV += drwav_open_file_write_sequential
DRWAV += drwav_open_memory
DRWAV += drwav_open_memory_and_read_f32
DRWAV += drwav_open_memory_and_read_pcm_frames_f32
DRWAV += drwav_open_memory_and_read_pcm_frames_s16
DRWAV += drwav_open_memory_and_read_pcm_frames_s32
DRWAV += drwav_open_memory_and_read_s16
DRWAV += drwav_open_memory_and_read_s32
DRWAV += drwav_open_memory_ex
DRWAV += drwav_open_memory_write
DRWAV += drwav_open_memory_write
DRWAV += drwav_open_memory_write__internal
DRWAV += drwav_open_memory_write__internal
DRWAV += drwav_open_memory_write_sequential
DRWAV += drwav_open_write
DRWAV += drwav_open_write
DRWAV += drwav_open_write__internal
DRWAV += drwav_open_write_sequential
DRWAV += drwav_read
DRWAV += drwav_read_f32
DRWAV += drwav_read_f32__alaw
DRWAV += drwav_read_f32__alaw
DRWAV += drwav_read_f32__ieee
DRWAV += drwav_read_f32__ieee
DRWAV += drwav_read_f32__ima
DRWAV += drwav_read_f32__ima
DRWAV += drwav_read_f32__msadpcm
DRWAV += drwav_read_f32__msadpcm
DRWAV += drwav_read_f32__mulaw
DRWAV += drwav_read_f32__mulaw
DRWAV += drwav_read_f32__pcm
DRWAV += drwav_read_f32__pcm
DRWAV += drwav_read_pcm_frames
DRWAV += drwav_read_pcm_frames_be
DRWAV += drwav_read_pcm_frames_f32
DRWAV += drwav_read_pcm_frames_f32be
DRWAV += drwav_read_pcm_frames_f32le
DRWAV += drwav_read_pcm_frames_le
DRWAV += drwav_read_pcm_frames_s16
DRWAV += drwav_read_pcm_frames_s16be
DRWAV += drwav_read_pcm_frames_s16le
DRWAV += drwav_read_pcm_frames_s32
DRWAV += drwav_read_pcm_frames_s32be
DRWAV += drwav_read_pcm_frames_s32le
DRWAV += drwav_read_raw
DRWAV += drwav_read_s16
DRWAV += drwav_read_s16__alaw
DRWAV += drwav_read_s16__ieee
DRWAV += drwav_read_s16__ima
DRWAV += drwav_read_s16__msadpcm
DRWAV += drwav_read_s16__mulaw
DRWAV += drwav_read_s16__pcm
DRWAV += drwav_read_s32
DRWAV += drwav_read_s32__alaw
DRWAV += drwav_read_s32__alaw
DRWAV += drwav_read_s32__ieee
DRWAV += drwav_read_s32__ieee
DRWAV += drwav_read_s32__ima
DRWAV += drwav_read_s32__ima
DRWAV += drwav_read_s32__msadpcm
DRWAV += drwav_read_s32__msadpcm
DRWAV += drwav_read_s32__mulaw
DRWAV += drwav_read_s32__mulaw
DRWAV += drwav_read_s32__pcm
DRWAV += drwav_read_s32__pcm
DRWAV += drwav_riff_chunk_size_riff
DRWAV += drwav_riff_chunk_size_w64
DRWAV += drwav_s16_to_f32
DRWAV += drwav_s16_to_s32
DRWAV += drwav_s24_to_f32
DRWAV += drwav_s24_to_s16
DRWAV += drwav_s24_to_s16
DRWAV += drwav_s24_to_s32
DRWAV += drwav_s32_to_f32
DRWAV += drwav_s32_to_s16
DRWAV += drwav_s32_to_s16
DRWAV += drwav_seek_to_pcm_frame
DRWAV += drwav_seek_to_sample
DRWAV += drwav_seek_to_sample
DRWAV += drwav_smpl
DRWAV += drwav_smpl_loop
DRWAV += drwav_take_ownership_of_metadata
DRWAV += drwav_target_write_size_bytes
DRWAV += drwav_u8_to_f32
DRWAV += drwav_u8_to_s16
DRWAV += drwav_u8_to_s16
DRWAV += drwav_u8_to_s32
DRWAV += drwav_uninit
DRWAV += drwav_version
DRWAV += drwav_version_string
DRWAV += drwav_write
DRWAV += drwav_write
DRWAV += drwav_write_pcm_frames
DRWAV += drwav_write_pcm_frames_be
DRWAV += drwav_write_pcm_frames_le
DRWAV += drwav_write_raw

# --------------------------------------------------------------
# --------------------------------------------------------------
# Files to build

PLUGIN_FILES = plugins.cpp

# --------------------------------------------------------------
# Cardinal (built-in, Host modules only)

PLUGIN_FILES += Cardinal/src/Blank.cpp
PLUGIN_FILES += Cardinal/src/ExpanderInputMIDI.cpp
PLUGIN_FILES += Cardinal/src/ExpanderOutputMIDI.cpp
PLUGIN_FILES += Cardinal/src/HostAudio.cpp
PLUGIN_FILES += Cardinal/src/HostCV.cpp
PLUGIN_FILES += Cardinal/src/HostMIDI.cpp
PLUGIN_FILES += Cardinal/src/HostMIDI-CC.cpp
PLUGIN_FILES += Cardinal/src/HostMIDI-Gate.cpp
PLUGIN_FILES += Cardinal/src/HostMIDI-Map.cpp
PLUGIN_FILES += Cardinal/src/HostParameters.cpp
PLUGIN_FILES += Cardinal/src/HostParameters-Map.cpp
PLUGIN_FILES += Cardinal/src/HostTime.cpp

# --------------------------------------------------------------
# ReplicatAudio (user modules)

# Mirrors modules/Makefile SOURCES (ReplicatAudio -> ../../modules symlink)
PLUGIN_FILES += ReplicatAudio/src/plugin.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-vipberus.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-adsr.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-blank.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-buttons.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-tyche.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-reflectingpool.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-macrow.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-gnawbz.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-klock.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-m2.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-freeberd.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-delay-cv.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-ranger.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-magus.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-mothership.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-seer-mini.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-shapes.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-ulfo.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-vca.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-ntet.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-think.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-quant.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-accumulator.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-glitch.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-seer.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-zeno.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-chord.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-tracker.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-just.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-kick.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-snare.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-tom.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-clap.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-hat.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-cymbal.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-ride.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-meteor.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-tuner.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-vash.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-vnote.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-karplus-strong.cpp
PLUGIN_FILES += ReplicatAudio/src/ra-krush.cpp
PLUGIN_FILES += ReplicatAudio/dep/s7/s7.c

# Build setup

ifeq ($(HEADLESS),true)
BUILD_DIR = ../build-headless/plugins
else
BUILD_DIR = ../build/plugins
endif

BASE_FLAGS += -DBUILDING_PLUGIN_MODULES
BASE_FLAGS += -DDONT_SET_USING_DISTRHO_NAMESPACE
BASE_FLAGS += -I$(abspath $(ROOT)/include/osdialog-stub)

ifeq ($(BSD)$(WASM),true)
BASE_FLAGS += -D'aligned_alloc_16(ptr)'='aligned_alloc(16,ptr)'
BASE_FLAGS += -D'aligned_free_16(ptr)'='free(ptr)'
endif

ifeq ($(WASM),true)
BASE_FLAGS += -DEIGEN_DONT_VECTORIZE=1
endif

ifeq ($(shell $(PKG_CONFIG) --exists fftw3f && echo true),true)
BASE_FLAGS += -DHAVE_FFTW3F
endif

ifeq ($(shell $(PKG_CONFIG) --exists sndfile && echo true),true)
BASE_FLAGS += -DHAVE_SNDFILE
endif

ifeq ($(HAVE_X11),true)
BASE_FLAGS += -DHAVE_X11
endif

# --------------------------------------------------------------
# lots of warnings from plugins

BASE_FLAGS += -Wno-deprecated-declarations

ifeq ($(MACOS),true)
BASE_FLAGS += -Wno-unknown-warning-option
endif

# --------------------------------------------------------------
# Carla and friends build flags

CARLA_FLAGS = \
	-DCARLA_BACKEND_NAMESPACE=Cardinal \
	-DREAL_BUILD \
	-DSTATIC_PLUGIN_TARGET \
	-I../deps/aubio/src \
	-I../carla/source/backend \
	-I../carla/source/includes \
	-I../carla/source/modules \
	-I../carla/source/utils

ifeq ($(WASM),true)
CARLA_FLAGS += -DDISTRHO_RUNNER_INDIRECT_WASM_CALLS
endif

# --------------------------------------------------------------
# RTNeural flags, used in AIDA-X

RTNEURAL_FLAGS  = -std=gnu++17
RTNEURAL_FLAGS += -DBiquad=AidaBiquad
RTNEURAL_FLAGS += -DEigen=Aida_Eigen
RTNEURAL_FLAGS += -Dconst_blas_data_mapper=Aida_const_blas_data_mapper
RTNEURAL_FLAGS += -Devaluator=Aida_evaluator
RTNEURAL_FLAGS += -DSKIP_MINGW_FORMAT
RTNEURAL_FLAGS += -DRTNEURAL_DEFAULT_ALIGNMENT=16
RTNEURAL_FLAGS += -DRTNEURAL_USE_EIGEN=1

RTNEURAL_FLAGS += -ICardinal/src/AIDA-X/RTNeural
RTNEURAL_FLAGS += -ICardinal/src/AIDA-X/RTNeural/modules/Eigen

# --------------------------------------------------------------
# Build targets

ifeq ($(HEADLESS),true)
TARGET_SUFFIX = -headless
endif

TARGETS = plugins$(TARGET_SUFFIX).a

all: $(TARGETS)

clean:
	rm -f *.a
	rm -rf $(BUILD_DIR)

# --------------------------------------------------------------

PLUGIN_LIST = $(subst /plugin.json,,$(wildcard */plugin.json))

RESOURCE_FILES = \
	$(wildcard */res/*.svg) \
	$(wildcard */res/*/*.svg) \
	$(wildcard */res/*/*/*.svg) \
	$(wildcard */res/*/*/*/*.svg) \
	$(wildcard */res/*/*/*/*/*.svg) \
	$(wildcard */res/*.otf) \
	$(wildcard */res/*/*.otf) \
	$(wildcard */res/*/*/*.otf) \
	$(wildcard */res/*.ttf) \
	$(wildcard */res/*/*.ttf) \
	$(wildcard */res/*/*/*.ttf)

RESOURCE_FILES += $(wildcard */presets)
RESOURCE_FILES += $(wildcard Cardinal/res/*.png)

ifeq ($(MOD_BUILD),true)

MINI_RESOURCES  = $(MINIPLUGIN_LIST:%=../bin/CardinalMini.lv2/resources/PluginManifests/%.json)
MINI_RESOURCES += $(MINIRESOURCE_FILES:%=../bin/CardinalMini.lv2/resources/%)

LV2_RESOURCES  = $(MINI_RESOURCES)
LV2_RESOURCES += $(PLUGIN_LIST:%=../bin/Cardinal.lv2/resources/PluginManifests/%.json)
LV2_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalFX.lv2/resources/PluginManifests/%.json)
LV2_RESOURCES += $(RESOURCE_FILES:%=../bin/Cardinal.lv2/resources/%)
LV2_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalFX.lv2/resources/%)

# wasm builds only have standalone and mini
else ifeq ($(WASM),true)

MINI_RESOURCES  = $(MINIPLUGIN_LIST:%=../bin/CardinalMini.lv2/resources/PluginManifests/%.json)
MINI_RESOURCES += $(MINIRESOURCE_FILES:%=../bin/CardinalMini.lv2/resources/%)

LV2_RESOURCES  = $(MINI_RESOURCES)
LV2_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalNative.lv2/resources/PluginManifests/%.json)
LV2_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalNative.lv2/resources/%)

else

LV2_RESOURCES += $(MINIPLUGIN_LIST:%=../bin/CardinalMini.lv2/resources/PluginManifests/%.json)
LV2_RESOURCES += $(MINIRESOURCE_FILES:%=../bin/CardinalMini.lv2/resources/%)

LV2_RESOURCES  = $(MINI_RESOURCES)
LV2_RESOURCES += $(PLUGIN_LIST:%=../bin/Cardinal.lv2/resources/PluginManifests/%.json)
LV2_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalFX.lv2/resources/PluginManifests/%.json)
LV2_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalSynth.lv2/resources/PluginManifests/%.json)
LV2_RESOURCES += $(RESOURCE_FILES:%=../bin/Cardinal.lv2/resources/%)
LV2_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalFX.lv2/resources/%)
LV2_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalSynth.lv2/resources/%)

ifeq ($(MACOS),true)
VST2_RESOURCES  = $(PLUGIN_LIST:%=../bin/CardinalFX.vst/Contents/Resources/PluginManifests/%.json)
VST2_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalSynth.vst/Contents/Resources/PluginManifests/%.json)
VST2_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalFX.vst/Contents/Resources/%)
VST2_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalSynth.vst/Contents/Resources/%)
CLAP_RESOURCES  = $(PLUGIN_LIST:%=../bin/Cardinal.clap/Contents/Resources/PluginManifests/%.json)
CLAP_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalFX.clap/Contents/Resources/PluginManifests/%.json)
CLAP_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalSynth.clap/Contents/Resources/PluginManifests/%.json)
CLAP_RESOURCES += $(RESOURCE_FILES:%=../bin/Cardinal.clap/Contents/Resources/%)
CLAP_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalFX.clap/Contents/Resources/%)
CLAP_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalSynth.clap/Contents/Resources/%)
else
CLAP_RESOURCES  = $(PLUGIN_LIST:%=../bin/Cardinal.clap/resources/PluginManifests/%.json)
CLAP_RESOURCES += $(RESOURCE_FILES:%=../bin/Cardinal.clap/resources/%)
endif

VST3_RESOURCES  = $(PLUGIN_LIST:%=../bin/Cardinal.vst3/Contents/Resources/PluginManifests/%.json)
VST3_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalFX.vst3/Contents/Resources/PluginManifests/%.json)
VST3_RESOURCES += $(PLUGIN_LIST:%=../bin/CardinalSynth.vst3/Contents/Resources/PluginManifests/%.json)
VST3_RESOURCES += $(RESOURCE_FILES:%=../bin/Cardinal.vst3/Contents/Resources/%)
VST3_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalFX.vst3/Contents/Resources/%)
VST3_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalSynth.vst3/Contents/Resources/%)

endif

LOADER_RESOURCES  = $(PLUGIN_LIST:%=../bin/CardinalLoader.lv2/resources/PluginManifests/%.json)
LOADER_RESOURCES += $(RESOURCE_FILES:%=../bin/CardinalLoader.lv2/resources/%)

resources: $(JACK_RESOURCES) $(LV2_RESOURCES) $(VST2_RESOURCES) $(VST3_RESOURCES) $(CLAP_RESOURCES)

loader-resources: $(LOADER_RESOURCES)

mini-resources: $(MINI_RESOURCES)

../bin/Cardinal.lv2/resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalFX.lv2/resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalLoader.lv2/resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalMini.lv2/resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalNative.lv2/resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.lv2/resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

ifeq ($(MOD_BUILD),true)
../bin/Cardinal.lv2/resources/%.svg: %.svg ../deps/svg2stub.py
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)python3 ../deps/svg2stub.py $< $@

../bin/CardinalFX.lv2/resources/%.svg: %.svg ../deps/svg2stub.py
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)python3 ../deps/svg2stub.py $< $@

../bin/CardinalMini.lv2/resources/%.svg: %.svg ../deps/svg2stub.py
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)python3 ../deps/svg2stub.py $< $@
endif

../bin/CardinalLoader.lv2/resources/%.svg: %.svg ../deps/svg2stub.py
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)python3 ../deps/svg2stub.py $< $@

../bin/Cardinal.lv2/resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalFX.lv2/resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalLoader.lv2/resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalMini.lv2/resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalNative.lv2/resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.lv2/resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/Cardinal.vst3/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalFX.vst3/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.vst3/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/Cardinal.vst3/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalFX.vst3/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.vst3/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

ifeq ($(MACOS),true)
../bin/CardinalFX.vst/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.vst/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalFX.vst/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.vst/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/Cardinal.clap/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalFX.clap/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.clap/Contents/Resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/Cardinal.clap/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalFX.clap/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/CardinalSynth.clap/Contents/Resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@
else
../bin/Cardinal.clap/resources/%: %
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@

../bin/Cardinal.clap/resources/PluginManifests/%.json: %/plugin.json
	-@mkdir -p "$(shell dirname $@)"
	$(SILENT)ln -sf $(abspath $<) $@
endif

# --------------------------------------------------------------
# Build commands

PLUGIN_OBJS  = $(PLUGIN_FILES:%=$(BUILD_DIR)/%.o)
PLUGIN_OBJS += $(PLUGIN_BINARIES:%=$(BUILD_DIR)/%.bin.o)

.PRECIOUS: $(PLUGIN_BINARIES:%=$(BUILD_DIR)/%.bin.c)

plugins$(TARGET_SUFFIX).a: $(PLUGIN_OBJS)
	@echo "Creating $@"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(BUILD_DIR)/%.bin.c: % ../deps/res2c.py
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Generating $*.bin.c"
	$(SILENT)python3 ../deps/res2c.py $< > $@

$(BUILD_DIR)/%.bin.o: $(BUILD_DIR)/%.bin.c
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $*.bin"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/plugins.cpp.o: plugins.cpp
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(BUILD_DIR)/Cardinal/%.cpp.o: Cardinal/%.cpp
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@ \
		-DpluginInstance=pluginInstance__Cardinal \
		-Dstbrp_context=stbrp_context_cardinal \
		-Dstbrp_coord=stbrp_coord_cardinal \
		-Dstbtt_fontinfo=stbtt_fontinfo_cardinal \
		-Dstbrp_node=stbrp_node_cardinal \
		-Dstbrp_rect=stbrp_rect_cardinal \
		$(CARLA_FLAGS)

$(BUILD_DIR)/ReplicatAudio/%.cpp.o: ReplicatAudio/%.cpp
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@ \
		-DpluginInstance=pluginInstance__ReplicatAudio \
		-Dinit=init__ReplicatAudio \
		-IReplicatAudio/dep/s7

$(BUILD_DIR)/ReplicatAudio/%.c.o: ReplicatAudio/%.c
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

# --------------------------------------------------------------

-include $(PLUGIN_OBJS:%.o=%.d)
