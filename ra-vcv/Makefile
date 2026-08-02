RACK_DIR ?= Rack-SDK

SOURCES = \
	dep/s7/s7.c \
	src/plugin.cpp \
	src/ra-add.cpp \
	src/ra-adsr.cpp \
	src/ra-blank.cpp \
	src/ra-buttons.cpp \
	src/ra-tyche.cpp \
	src/ra-endless.cpp \
	src/ra-macrow.cpp \
	src/ra-gnawbz.cpp \
	src/ra-klock.cpp \
	src/ra-m2.cpp \
	src/ra-scaler.cpp \
	src/ra-magus.cpp \
	src/ra-seer.cpp \
	src/ra-shapes.cpp \
	src/ra-ulfo.cpp \
	src/ra-vca.cpp \
	src/ra-ntet.cpp \
	src/ra-think.cpp \
	src/ra-quant.cpp \
	src/ra-accumulator.cpp \
	src/ra-glitch.cpp \
	src/ra-dscope.cpp \
	src/ra-zeno.cpp \
	src/ra-chord.cpp \
	src/ra-tracker.cpp \
	src/ra-just.cpp


DISTRIBUTABLES += $(shell find res -type f)
FLAGS += -Idep/s7

include $(RACK_DIR)/plugin.mk
