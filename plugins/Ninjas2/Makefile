#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#
# Modified by Clearly Broken Software
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = ninjas2

# --------------------------------------------------------------
# Plugin types to build

BUILD_LV2 ?= true
BUILD_VST2 ?= true
BUILD_JACK ?= true
BUILD_DSSI ?= false
BUILD_LADSPA = false

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	Ninjas2Plugin.cpp

FILES_UI  = \
	Utils/src/Mathf.cpp \
	Structures/src/Animation.cpp \
	Structures/src/Layout.cpp \
	Widgets/src/WolfWidget.cpp \
	Widgets/src/NanoSwitch.cpp \
	Widgets/src/RemoveDCSwitch.cpp \
	Widgets/src/NanoButton.cpp \
	Widgets/src/NanoWheel.cpp \
	Widgets/src/NanoKnob.cpp \
	Widgets/src/VolumeKnob.cpp \
	Widgets/src/ResizeHandle.cpp \
	Widgets/src/LabelBox.cpp \
	Widgets/src/LabelContainer.cpp \
	Widgets/src/BipolarModeSwitch.cpp \
	Widgets/src/GlowingLabelsBox.cpp \
	Widgets/src/NanoLabel.cpp \
	Widgets/src/SpinBox.cpp \
	Widgets/src/NanoSpinBox.cpp \
	Widgets/src/TextButton.cpp \
	Widgets/src/PlayModeSwitch.cpp \
	Resources/fonts.cpp \
	Widgets/src/StateSwitch.cpp \
	Widgets/src/PianoKeyboard.cpp \
	Widgets/src/LabelBox.cpp \
	Ninjas2UI.cpp \
	Ninjas2Resources.cpp
	

# --------------------------------------------------------------
# Do some magic

include ../../dpf/Makefile.plugins.mk

# --------------------------------------------------------------
# Extra flags

BASE_FLAGS += -I. -I./Widgets -I./Utils -I./Structures

# TODO: change pkg-config to $(PKG_CONFIG) with updated DPF
ifeq ($(USE_SYSTEM_AUBIO),true)
BASE_FLAGS += $(shell $(PKG_CONFIG) --cflags-only-other aubio) \
              -I$(shell $(PKG_CONFIG) --variable includedir aubio)/aubio
LINK_FLAGS += $(shell $(PKG_CONFIG) --libs aubio)
else
BASE_FLAGS += -I../../aubio/src
LINK_FLAGS += ../../aubio/libaubio.a
endif

BASE_FLAGS += $(shell $(PKG_CONFIG) --cflags sndfile samplerate)
LINK_FLAGS += $(shell $(PKG_CONFIG) --libs sndfile samplerate freetype2)

# Set PARANOID_MODE=true to make the build error out on warnings
ifeq ($(PARANOID_MODE),true)
BASE_FLAGS += -Wall -Wextra -Werror
endif

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(BUILD_JACK),true)
ifeq ($(HAVE_JACK),true)
TARGETS += jack
endif
endif

ifeq ($(BUILD_LADSPA),true)
TARGETS += ladspa
endif

ifeq ($(BUILD_DSSI),true)
ifeq ($(HAVE_OPENGL),true)
ifeq ($(HAVE_LIBLO),true)
TARGETS += dssi
endif
endif
endif

ifeq ($(BUILD_LV2),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += lv2
else
TARGETS += lv2_dsp
endif
endif


ifeq ($(BUILD_VST2),true)
TARGETS += vst
endif

all: $(TARGETS)

# --------------------------------------------------------------
