# BEGIN LIBRARY FUNCTIONS #

# Template to make compilation rules and flags for C and C++ files. Based on https://www.gnu.org/software/make/manual/html_node/Eval-Function.html
# Usage: `$(eval $(call C_AND_CXX_FLAGS_template,targetNameHere,cFlagsHere,cxxFlagsHere,cPrecompiledHeaders,cppPrecompiledHeaders))` where `targetNameHere` is the name of this specific configuration, `cFlagsHere` are CFLAGS to pass, if any, and cxxFlagsHere are CXXFLAGS if any.
# Example result of calling `$(eval $(call C_AND_CXX_FLAGS_template,release,-Ofast,,))` (no CXXFLAGS were provided):
#
# CFLAGS_release = $(CFLAGS) -Ofast
# %_r.o: %.c
# 	$(CC) $(CFLAGS_release) -c $< -o $@
# CXXFLAGS_release = $(CXXFLAGS) $(CFLAGS_release)
# %_r.o: %.cpp
# 	$(CXX) $(CXXFLAGS_release) -c $< -o $@
#
# Note for reference: `$$(VAR)` delays evaluation, where `VAR` is the thing to delay evaluation of.
define C_AND_CXX_FLAGS_template =
CFLAGS_$(1) = $(CFLAGS) $(2)
# C sources
%_$(1).o: %.c $(addsuffix _$(1).h.gch,$(patsubst %.h,%,$(4)))
	$(CC) $$(CFLAGS_$(1)) -c $$< -o $$@
CXXFLAGS_$(1) = $(CXXFLAGS) $$(CFLAGS_$(1)) $(3)
# C++ sources
%_$(1).o: %.cpp $(addsuffix _$(1).hpp.gch,$(patsubst %.hpp,%,$(5)))#<--Example: src/common_$(1).hpp.gch
	$(CXX) $$(CXXFLAGS_$(1)) -c $$< -o $$@
# C precompiled headers
%_$(1).h.gch: %.h
	$(CC) $$(CFLAGS_$(1)) -x c-header $$< -o $$@
	rm -f $$(addsuffix .h.gch,$$(patsubst %_$(1).h.gch,%,$$@)) # Remove old symlink if any
	cd $$(dir $$@) && ln -s $$(notdir $$@) $$(notdir $$(addsuffix .h.gch,$$(patsubst %_$(1).h.gch,%,$$@))) # Symlink into name the compiler expects
# ^^Note: `$(notdir $@) takes away the path from the file name leaving just the file name (so /x/y/foo.a becomes foo.a) $(basename ...) takes away the extension (so foo.a becomes foo)` ( https://stackoverflow.com/questions/16727021/understanding-a-makefile-with-basename-notdir ) + "$(dir names…)	Extracts the directory-part of each file name in names. The directory-part of the file name is everything up through (and including) the last slash in it. If the file name contains no slash, the directory part is the string ‘./’."[...] ( https://www.gnu.org/software/make/manual/html_node/File-Name-Functions.html )
# C++ precompiled headers
# https://stackoverflow.com/questions/58841/precompiled-headers-with-gcc
%_$(1).hpp.gch: %.hpp
	$(CXX) $$(CXXFLAGS_$(1)) -x c++-header $$< -o $$@
	rm -f $$(addsuffix .hpp.gch,$$(patsubst %_$(1).hpp.gch,%,$$@)) # Remove old symlink if any
	cd $$(dir $$@) && ln -s $$(notdir $$@) $$(notdir $$(addsuffix .hpp.gch,$$(patsubst %_$(1).hpp.gch,%,$$@))) # Symlink into name the compiler expects

ALL_PCH_FILES_FROM_TARGETS += $(addsuffix _$(1).h.gch,$(patsubst %.h,%,$(4))) $(addsuffix _$(1).hpp.gch,$(patsubst %.hpp,%,$(5)))
ALL_PCH_SYMLINK_FILES_FROM_TARGETS += $(addsuffix .h.gch,$(patsubst %.h,%,$(4))) $(addsuffix .hpp.gch,$(patsubst %.hpp,%,$(5)))
# Don't delete the precompiled headers (Make does this automatically it seems..)
.PRECIOUS: %_$(1).h.gch %_$(1).hpp.gch # https://stackoverflow.com/questions/15189704/makefile-removes-object-files-for-no-reason

# https://stackoverflow.com/questions/8025766/makefile-auto-dependency-generation
#-include $(SRC:%.cpp=%.d)
endef

# Usage: `$(eval $(call OBJECTS_LINKING_template,targetNameHere,release_orWhateverTargetYouWant,objectFilesHere,mainFile_folderPathHere,flagsHere,additionalDepsHere,extraCommandsToRunAfterCompilation))`
# Main file should be named `targetNameHere` followed by `Main` and `.cpp` or `.c`.
define OBJECTS_LINKING_template =
OBJECTS_$(1)_$(2) = $(3)
OBJECTS_$(1)_$(2) := $$(addsuffix _$(2).o, $$(patsubst %.o,%, $$(OBJECTS_$(1)_$(2)))) $(4)/$(1)Main_$(2).o
ALL_OBJECTS_FROM_TARGETS += $$(OBJECTS_$(1)_$(2))
ifeq ($(7),)
EXTRA_COMMANDS_$(1)_$(2) =
else
EXTRA_COMMANDS_$(1)_$(2) = && $(7)
endif
#$$(info $$(OBJECTS_$(1)_$(2)))
$(1)_exe_$(2): $$(OBJECTS_$(1)_$(2)) $(6)
	$(CXX) $$^ -o $$@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(5) $(6) $$(EXTRA_COMMANDS_$(1)_$(2))
endef

# END LIBRARY FUNCTIONS #

# BEGIN CONFIG #

# Leave empty to avoid running a specific test:
#CONFIG_TEST_DEFINES = -DSIFTGPU_TEST

# Choose a SIFT implementation here:
SIFT_IMPL=SIFTAnatomy
#SIFT_IMPL=SIFTOpenCV
#SIFT_IMPL=SIFTGPU

# If not using this, need to avoid linking jemalloc too:
USE_JEMALLOC=1
#1

USE_PTR_INC_MALLOC=0

# Can speed up compilation but at the expense of writing to more temporary .gch files.
USE_PRECOMPILED_HEADERS=1

# END CONFIG #



# https://stackoverflow.com/questions/12230230/what-gnu-makefile-rule-can-ensure-the-version-of-make-is-at-least-v3-82
# Check Make version (we need at least GNU Make 3.82). Fortunately,
# 'undefine' directive has been introduced exactly in GNU Make 3.82.
ifeq ($(filter undefine,$(value .FEATURES)),)
$(error Unsupported Make version. \
    The build system does not work properly with GNU Make $(MAKE_VERSION), \
    please use GNU Make 4.3 or above.) # Specifically, version 3.81 on macOS does all sorts of things wrongly
endif
SHELL := /usr/bin/env bash
OS := $(shell uname -s)
# -D_POSIX_C_SOURCE=200809L
additionalPkgconfigPackages=
ifeq ($(USE_JEMALLOC),1)
additionalPkgconfigPackages += jemalloc
CFLAGS += -DUSE_JEMALLOC
endif
ifeq ($(USE_PTR_INC_MALLOC),1)
CFLAGS += -DUSE_PTR_INC_MALLOC
endif
ifeq ($(SIFT_IMPL),SIFTGPU)
additionalPkgconfigPackages += gl glew glut IL
additionalCFLAGS = -isystem /nix/store/0khrl4yrg1qyz93nk6kq1s79c4a0w8i0-libdevil-1.7.8-dev/include -isystem /nix/store/i0rmp3app7yqd37ihgxlx9c3lwsj16kq-opencl-headers-2020.06.16/include -isystem /nix/store/92h4zdgadxpf77iajcx7s60aanqmz9xy-opencl-headers-2020.06.16/include -I/nix/store/gilc17g399q13hghm8zzkdcn0mv1n7y7-libunwind-1.4.0-dev/include
endif
CFLAGS += -Wall -pedantic `pkg-config --cflags opencv4 libpng python-3.7m ${additionalPkgconfigPackages}` -I/nix/store/zflx47lr00hipvkl5nncd2rnpzssnni6-backward-1.6/include -DBACKWARD_HAS_UNWIND=1 -DSIFT_IMPL=$(SIFT_IMPL) -D$(SIFT_IMPL)_ $(CONFIG_TEST_DEFINES) -IVectorNav/include    $(additionalCFLAGS) #$(NIX_CFLAGS_COMPILE)  #`echo "$NIX_CFLAGS_COMPILE"` # TODO: get backward-cpp working for segfault stack traces
#CFLAGS += -MD -MP # `-MD -MP` : https://stackoverflow.com/questions/8025766/makefile-auto-dependency-generation
$(info $(OS))
ifeq ($(OS),Darwin)
CFLAGS += -target x86_64-apple-macos10.15 #-isysroot $(shell xcrun --sdk macosx --show-sdk-path)
CFLAGS += -I/nix/store/kqqwh1xz3ri47dvg8ikj7w2yl344amw3-python3-3.7.11/include -I/nix/store/690a3qx1w73vd86aawgih3fv5bn393lf-python3.7-pybind11-2.7.0/include
endif
CXXFLAGS += -std=c++17 $(CFLAGS)
CLANGVERSION = $(shell clang --version | head -n 1 | sed -E 's/clang version (.*) .*/\1/' | awk '{$$1=$$1;print}') # https://stackoverflow.com/questions/5188267/checking-the-gcc-version-in-a-makefile
$(info $(CLANGVERSION)) # Example: "7.1.0 "
#ifeq ($(shell foo="$(CLANGVERSION)"; if [ "$${foo//./}" -le 710 ]; then echo 0; fi),0) #ifeq ($(shell foo="$(CLANGVERSION)"; test ${foo//./} -le 710; echo $$?),0) # TODO: check if version less than or equal to this
ifeq ($(OS),Darwin)
    LFLAGS += -lc++fs
endif
#endif
ifeq ($(OS),Darwin)
    LFLAGS += -framework CoreGraphics -framework Foundation
    ifeq ($(SIFT_IMPL),SIFTGPU)
        LFLAGS += -framework OpenCL
    endif
else ifeq ($(OS),Linux)
    LFLAGS += -lX11
    ifeq ($(SIFT_IMPL),SIFTGPU)
        #LFLAGS += -lOpenCL
    endif
endif
$(info $(LFLAGS))
LFLAGS += -lpng -lm -lpthread -ldl -lunwind #-ljpeg -lrt -lm
# ifeq ($(OS),Linux)
#     # Needed if gtk; nix hack:
#     LDFLAGS = -L/nix/store/vvird2i7lakg2awpwd360l77bbrwbwx0-opencv-4.5.2/lib `bash ./filter-hack.sh "${NIX_LDFLAGS}"`
# else
    LDFLAGS = ${NIX_LDFLAGS}
# endif
LDFLAGS += `pkg-config --libs opencv4 libpng python-3.7m ${additionalPkgconfigPackages}`

#CC := clang-12
#CXX := clang-12 -x c++
ifeq ($(OS),Darwin)
# https://stackoverflow.com/questions/16387484/clangllvm-compile-with-frameworks
CC := xcrun -sdk macosx clang
CXX := xcrun -sdk macosx clang++
else
CC := clang
CXX := clang++
endif
SRC := src
OBJ := obj

SIFT_ANATOMY := sift_anatomy_20141201
SIFT_ANATOMY_SRC := ./$(SIFT_ANATOMY)/src
ifeq ($(SIFT_IMPL),SIFTAnatomy)
#SIFT_SRC := /Volumes/MyTestVolume/SeniorSemester1_Vanderbilt_University/RocketTeam/MyWorkspaceAndTempFiles/MyDocuments/SIFT_articleAndImplementation/sift_anatomy_20141201/src
SIFT := $(SIFT_ANATOMY)
SIFT_SRC := $(SIFT_ANATOMY_SRC)
SIFT_SOURCES_C := $(filter-out $(SIFT_SRC)/example.c $(SIFT_SRC)/demo_extract_patch.c $(SIFT_SRC)/match_cli.c $(SIFT_SRC)/sift_cli.c $(SIFT_SRC)/sift_cli_default.c $(SIFT_SRC)/anatomy2lowe.c, $(wildcard $(SIFT_SRC)/*.c))
else ifeq ($(SIFT_IMPL),SIFTGPU)
SIFT := SiftGPU
SIFT_SRC := ./$(SIFT)/src/SiftGPU
CFLAGS += -DWINDOW_PREFER_GLUT
ifeq ($(OS),Darwin)
# OpenCL isn't supported on Raspberry Pi 4B yet apparently.. ( https://forums.raspberrypi.com/viewtopic.php?t=312646 )
CFLAGS += -DCL_SIFTGPU_ENABLED
endif
endif
ifneq ($(SIFT_IMPL),SIFTOpenCV)
SIFT_SOURCES_CPP := $(wildcard $(SIFT_SRC)/*.cpp)
SIFT_OBJECTS := $(SIFT_SOURCES_CPP:%.cpp=%.o) $(SIFT_SOURCES_C:%.c=%.o)
CFLAGS += -I$(SIFT_SRC)
endif
ifneq ($(SIFT_IMPL),SIFTAnatomy)
# From SIFTAnatomy: for loading in PNGs we need io_png.c, and for fatal_error we need lib_util.c
CFLAGS += -I$(SIFT_ANATOMY_SRC)
SOURCES_C += $(SIFT_ANATOMY_SRC)/io_png.c $(SIFT_ANATOMY_SRC)/lib_util.c
endif

ALL_SOURCES := $(wildcard $(SRC)/*.cpp)
# Pre-compiled headers (speedup for compilation) # # FIXME: hack here is that both SIFT and subscale driver depend on the same set of precompiled headers..
# SIFT
ifeq ($(USE_PRECOMPILED_HEADERS),1)
#PCH_C := $(wildcard src/tools/*.h) src/my_sift_additions.h
#PCH_CPP := ./vn_sensors_common.hpp src/lib/ctpl_stl.hpp ./common.hpp $(filter-out src/fdstream.hpp,$(wildcard src/*.hpp)) $(wildcard src/main/*.hpp) $(wildcard src/tools/*.hpp) $(wildcard src/tools/backtrace/*.hpp) #src/common.hpp
# Realized that actually only one precompiled header is allowed "per compilation unit" ( https://stackoverflow.com/questions/55709100/how-to-use-multiple-precompiled-headers-some-from-a-library ). ( Also for more reference: https://web.mit.edu/rhel-doc/3/rhel-gcc-en-3/precompiled-headers.html ) :
PCH_C :=
PCH_CPP := src/stdafx.hpp.pch
else
PCH_C :=
PCH_CPP :=
endif

# Subscale driver
ifeq ($(USE_PRECOMPILED_HEADERS),1)
#PCH_C += $(wildcard src/tools/*.h) $(wildcard subscale_driver/*.h) $(wildcard subscale_driver/lib/*.h)
#PCH_CPP += ./vn_sensors_common.hpp ./common.hpp $(wildcard subscale_driver/*.hpp) $(wildcard src/tools/*.hpp) $(wildcard src/tools/backtrace/*.hpp)
# Realized that actually only one precompiled header is allowed "per compilation unit" ( https://stackoverflow.com/questions/55709100/how-to-use-multiple-precompiled-headers-some-from-a-library ). ( Also for more reference: https://web.mit.edu/rhel-doc/3/rhel-gcc-en-3/precompiled-headers.html ) :
else
PCH_C :=
PCH_CPP :=
endif
# #
SOURCES := common.cpp $(filter-out src/siftMain.cpp src/quadcopter.cpp, $(ALL_SOURCES)) $(wildcard $(SRC)/tools/*.cpp) $(wildcard $(SRC)/tools/backtrace/*.cpp) $(wildcard $(SRC)/main/*.cpp) #$(wildcard $(SRC)/optick/src/*.cpp) # `filter-out`: Remove files with `int main`'s so we can add them later per subproject    # https://stackoverflow.com/questions/10276202/exclude-source-file-in-compilation-using-makefile/10280945
SOURCES_C := subscale_driver/py.c subscale_driver/lib/pf_string.c $(SOURCES_C) $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/tools/*.c)
ALL_OBJECTS := $(ALL_SOURCES:%.cpp=%.o) $(SOURCES_C:%.c=%.o)
OBJECTS := $(SOURCES:%.cpp=%.o) $(SOURCES_C:%.c=%.o) # https://stackoverflow.com/questions/60329676/search-for-all-c-and-cpp-files-and-compiling-them-in-one-makefile
$(info $(OBJECTS)) # https://stackoverflow.com/questions/19488990/how-to-add-or-in-pathsubst-in-makefile

all: common sift_exe quadcopter
# gcc -std=c99 -o example example2.c $(SIFT)/lib_sift.o $(SIFT)/lib_sift_anatomy.o \
# $(SIFT)/lib_keypoint.o  $(SIFT)/lib_scalespace.o $(SIFT)/lib_description.o \
# $(SIFT)/lib_discrete.o $(SIFT)/lib_util.o -lm -I$(SIFT)

setup:
	mkdir -p $(OBJ)

common:
	cd $(SIFT) && $(MAKE)



############################# SIFT targets #############################

# commandLine targets in general
ADDITIONAL_CFLAGS_ALL_COMMANDLINE = -DUSE_COMMAND_LINE_ARGS -no-pie -fno-PIE -fno-PIC # `-no-pie -fno-PIE -fno-PIC`: no position-indepdent code so that linux always uses constant addresses in the address space for functions in code section etc. -- this makes it more debuggable and reverse-engineerable for stack traces with addresses etc. at the cost of security

# release target
# NOTE: -DNDEBUG turns off assertions (only for the code being compiled from source, not for libraries, including those from Nix like OpenCV unless we set it explicitly..).
# NOTE: -g is needed for stack traces for https://github.com/bombela/backward-cpp
ADDITIONAL_CFLAGS_RELEASE = -march=native -mtune=native -Ofast -g -DNDEBUG -fno-stack-protector # -fno-stack-protector for speedup but possible stack corruption with no segfaults indicating it happened (it might be -fno-stack-protector by default though) at the cost of security.   #-O3 # TODO: check -Osize ( https://stackoverflow.com/questions/19470873/why-does-gcc-generate-15-20-faster-code-if-i-optimize-for-size-instead-of-speed )
# ^ `-mtune=native` optimizes for the machine being compiled on
$(eval $(call C_AND_CXX_FLAGS_template,release,$(ADDITIONAL_CFLAGS_RELEASE),,$(PCH_C),$(PCH_CPP)))

# release_commandLine target
ADDITIONAL_CFLAGS_RELEASE_COMMANDLINE = $(ADDITIONAL_CFLAGS_RELEASE) $(ADDITIONAL_CFLAGS_ALL_COMMANDLINE)
$(eval $(call C_AND_CXX_FLAGS_template,release_commandLine,$(ADDITIONAL_CFLAGS_RELEASE_COMMANDLINE),,$(PCH_C),$(PCH_CPP)))

# debug target
ADDITIONAL_CFLAGS_DEBUG = -O0 -g3 -DDEBUG # -Og # -O0
$(eval $(call C_AND_CXX_FLAGS_template,debug,$(ADDITIONAL_CFLAGS_DEBUG),,$(PCH_C),$(PCH_CPP)))

# debug_commandLine target
ADDITIONAL_CFLAGS_DEBUG_COMMANDLINE = $(ADDITIONAL_CFLAGS_DEBUG) $(ADDITIONAL_CFLAGS_ALL_COMMANDLINE)
$(eval $(call C_AND_CXX_FLAGS_template,debug_commandLine,$(ADDITIONAL_CFLAGS_DEBUG_COMMANDLINE),,$(PCH_C),$(PCH_CPP)))

# release linking
ADDITIONAL_CFLAGS_RELEASE += -ffast-math -flto=full # https://developers.redhat.com/blog/2019/08/06/customize-the-compilation-process-with-clang-making-compromises
$(eval $(call OBJECTS_LINKING_template,sift,release,$(OBJECTS) $(SIFT_OBJECTS),src,$(ADDITIONAL_CFLAGS_RELEASE),))
$(eval $(call OBJECTS_LINKING_template,sift,release_commandLine,$(OBJECTS) $(SIFT_OBJECTS),src,$(ADDITIONAL_CFLAGS_RELEASE),))

# debug linking
#ADDITIONAL_CFLAGS_DEBUG += 
$(eval $(call OBJECTS_LINKING_template,sift,debug,$(OBJECTS) $(SIFT_OBJECTS),src,$(ADDITIONAL_CFLAGS_DEBUG),))
$(eval $(call OBJECTS_LINKING_template,sift,debug_commandLine,$(OBJECTS) $(SIFT_OBJECTS),src,$(ADDITIONAL_CFLAGS_DEBUG),))

############################# Driver targets #############################

#$(eval $(call C_AND_CXX_FLAGS_template,release,$(ADDITIONAL_CFLAGS_RELEASE),))
SUBSCALE_SRC := ./subscale_driver/
SUBSCALE_SOURCES := common.cpp $(filter-out $(SUBSCALE_SRC)/subscaleMain.cpp,$(wildcard $(SUBSCALE_SRC)/*.cpp)) $(wildcard src/tools/backtrace/*.cpp) src/utils.cpp
SUBSCALE_SOURCES_C := $(wildcard $(SUBSCALE_SRC)/*.c) $(wildcard $(SUBSCALE_SRC)/lib/*.c) src/tools/printf.c src/tools/_putchar.c
SUBSCALE_OBJECTS := $(SUBSCALE_SRC)/subscaleMain.o $(SUBSCALE_SOURCES:%.cpp=%.o) $(SUBSCALE_SOURCES_C:%.c=%.o)
$(eval $(call OBJECTS_LINKING_template,subscale,release,$(SUBSCALE_OBJECTS),$(SUBSCALE_SRC),$(ADDITIONAL_CFLAGS_RELEASE) -lpigpio -lpython3.7m,./VectorNav/build/bin/libvncxx.a,sudo setcap cap_sys_rawio+ep $$@))
$(eval $(call OBJECTS_LINKING_template,subscale,debug,$(SUBSCALE_OBJECTS),$(SUBSCALE_SRC),$(ADDITIONAL_CFLAGS_DEBUG) -lpigpio -lpython3.7m,./VectorNav/build/bin/libvncxx.a,sudo setcap cap_sys_rawio+ep $$@)) # `sudo setcap cap_sys_rawio+ep head` is from https://unix.stackexchange.com/questions/475800/non-root-read-access-to-dev-mem-by-kmem-group-members-fails ; also made it run the other stuff from https://raspberrypi.stackexchange.com/questions/40105/access-gpio-pins-without-root-no-access-to-dev-mem-try-running-as-root

############################# VectorNav targets #############################

./VectorNav/build/bin/libvncxx.a:
	cd ./VectorNav && $(MAKE)

############################# End of targets #############################



quadcopter: $(OBJECTS) src/quadcopter.o
	$(CXX) $^ -o $@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(wildcard $(SIFT_SRC)/*.o)

# .PHONY: testObjFile
# testObjFile:
# 	$(CC) sift_anatomy_20141201/src/lib_sift_anatomy.c -c -o lib_sift_anatomy.o

.PHONY: clean
clean:
	rm -f $(ALL_OBJECTS_FROM_TARGETS) $(EXECUTABLE_RESULT) $(ALL_PCH_FILES_FROM_TARGETS) $(ALL_PCH_SYMLINK_FILES_FROM_TARGETS)
