# fluidlite

FLUID_GITURL := https://github.com/divideconcept/FluidLite.git
FLUID_HASH := 238997654efb20e736512847f3f5f6d618de9423

ifdef HAVE_WIN32
PKGS += fluidlite
endif

ifeq ($(call need_pkg,"fluidlite"),)
PKGS_FOUND += fluidlite
endif

$(TARBALLS)/fluidlite-$(FLUID_HASH).tar.xz:
	$(call download_git,$(FLUID_GITURL),,$(FLUID_HASH))

.sum-fluidlite: fluidlite-$(FLUID_HASH).tar.xz
	$(call check_githash,$(FLUID_HASH))
	touch $@

DEPS_fluidlite = ogg $(DEPS_ogg)

fluidlite: fluidlite-$(FLUID_HASH).tar.xz .sum-fluidlite
	$(UNPACK)
	$(APPLY) $(SRC)/fluidlite/add-pic.diff
	$(MOVE)

.fluidlite: fluidlite toolchain.cmake
	rm -f $</build/CMakeCache.txt
	$(HOSTVARS) $(CMAKE) -S $<
	+$(CMAKEBUILD) $</build --target install
	touch $@
