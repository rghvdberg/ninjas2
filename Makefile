#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#
# Modified by Clearly Broken Software
#

include dpf/Makefile.base.mk

all: libs plugins gen

define MISSING_SUBMODULES_ERROR

Cannot find DGL! Please run "make submodules" to clone the missing submodules, then retry building the plugin.

endef

# --------------------------------------------------------------
submodules: 
	git submodule update --init --recursive

libs:
ifeq (,$(wildcard dpf/dgl))
	$(error $(MISSING_SUBMODULES_ERROR))
endif

ifneq ($(USE_SYSTEM_AUBIO),true)
	$(MAKE) -C aubio
endif

ifeq ($(HAVE_OPENGL),true)
	$(MAKE) -C dpf/dgl DGL_FLAGS="$(DGL_FLAGS) -DFONS_USE_FREETYPE $(shell $(PKG_CONFIG) --cflags freetype2)"
endif

plugins: libs
	$(MAKE) all -C plugins/Ninjas2

gen: plugins dpf/utils/lv2_ttl_generator
	@$(CURDIR)/dpf/utils/generate-ttl.sh
ifeq ($(MACOS),true)
	@$(CURDIR)/dpf/utils/generate-vst-bundles.sh
endif

dpf/utils/lv2_ttl_generator:
	$(MAKE) -C dpf/utils/lv2-ttl-generator

# --------------------------------------------------------------

clean:
ifneq ($(USE_SYSTEM_AUBIO),true)
	$(MAKE) clean -C aubio
endif
ifeq ($(HAVE_OPENGL),true)
	$(MAKE) clean -C dpf/dgl
endif
	$(MAKE) clean -C dpf/utils/lv2-ttl-generator
	$(MAKE) clean -C plugins/Ninjas2


# --------------------------------------------------------------

.PHONY: plugins

