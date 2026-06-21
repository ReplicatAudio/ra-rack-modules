RACK_DIR ?= ../Rack-SDK

SOURCES = \
	dep/s7/s7.c \
	src/plugin.cpp \
	src/ra-add.cpp \
	src/ra-adsr.cpp \
	src/ra-blank.cpp \
	src/ra-buttons.cpp \
	src/ra-chance.cpp \
	src/ra-endless.cpp \
	src/ra-gnawbz-1x4.cpp \
	src/ra-gnawbz-4x.cpp \
	src/ra-klock.cpp \
	src/ra-scaler.cpp \
	src/ra-scheme.cpp \
	src/ra-seer.cpp \
	src/ra-shapes.cpp \
	src/ra-ulfo.cpp \
	src/ra-vca.cpp \
	src/ra-yscope.cpp

DISTRIBUTABLES += $(wildcard res/*)
FLAGS += -Idep/s7

include $(RACK_DIR)/plugin.mk
