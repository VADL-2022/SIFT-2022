# BEGIN LIBRARY FUNCTIONS #

# Template to make compilation rules and flags for C and C++ files. Based on https://www.gnu.org/software/make/manual/html_node/Eval-Function.html
# Usage: `$(eval $(call C_AND_CXX_FLAGS_template,targetNameHere,cFlagsHere,cxxFlagsHere))` where `targetNameHere` is the name of this specific configuration, `cFlagsHere` are CFLAGS to pass, if any, and cxxFlagsHere are CXXFLAGS if any.
# Example result of calling `$(eval $(call C_AND_CXX_FLAGS_template,release,-Ofast,))` (no CXXFLAGS were provided):
#
# CFLAGS_release = $(CFLAGS) -Ofast
# %_r.o: %.c
# 	$(CC) $(CFLAGS_release) -c $< -o $@
# CXXFLAGS_release = $(CXXFLAGS) $(CFLAGS_release)
# %_r.o: %.cpp
# 	$(CXX) $(CXXFLAGS_release) -c $< -o $@
#
define C_AND_CXX_FLAGS_template =
CFLAGS_$(1) = $(CFLAGS) $(2)
%_$(1).o: %.c
	$(CC) $$(CFLAGS_$(1)) -c $$< -o $$@
CXXFLAGS_$(1) = $(CXXFLAGS) $$(CFLAGS_$(1)) $(3)
%_$(1).o: %.cpp
	$(CXX) $$(CXXFLAGS_$(1)) -c $$< -o $$@
endef

# Usage: `$(eval $(call OBJECTS_LINKING_template,release_orWhateverTargetYouWant,flagsHere))`
define OBJECTS_LINKING_template =
OBJECTS_$(1) = $(OBJECTS) $(SIFT_OBJECTS)
OBJECTS_$(1) := $$(addsuffix _$(1).o, $$(patsubst %.o,%, $$(OBJECTS_$(1)))) src/siftMain_$(1).o
ALL_OBJECTS_FROM_TARGETS += $$(OBJECTS_$(1))
sift_exe_$(1): $$(OBJECTS_$(1))
	$(CXX) $$^ -o $$@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(2)
endef

# END LIBRARY FUNCTIONS #



SHELL := /usr/bin/env bash
CFLAGS += -Wall -pedantic -D_POSIX_C_SOURCE=200809L `pkg-config --cflags opencv4` -I$(SIFT_SRC)
CXXFLAGS += -std=c++17 $(CFLAGS)
CLANGVERSION = $(shell clang --version | head -n 1 | sed -E 's/clang version (.*) .*/\1/' | awk '{$$1=$$1;print}') # https://stackoverflow.com/questions/5188267/checking-the-gcc-version-in-a-makefile
$(info $(CLANGVERSION)) # Example: "7.1.0 "
OS := $(shell uname -s)
ifeq ($(shell foo="$(CLANGVERSION)"; if [ "$${foo//./}" -le 710 ]; then echo 0; fi),0) #ifeq ($(shell foo="$(CLANGVERSION)"; test ${foo//./} -le 710; echo $$?),0) # TODO: check if version less than or equal to this
ifeq ($(OS),Darwin)
    LFLAGS += -lc++fs
endif
endif
$(info $(LFLAGS))
LFLAGS += -lpng -lm -lpthread #-ljpeg -lrt -lm
LDFLAGS = `pkg-config --libs opencv4`

#CC := clang-12
#CXX := clang-12 -x c++
CC := clang
CXX := clang++
SRC := src
OBJ := obj

#SIFT_SRC := /Volumes/MyTestVolume/SeniorSemester1_Vanderbilt_University/RocketTeam/MyWorkspaceAndTempFiles/MyDocuments/SIFT_articleAndImplementation/sift_anatomy_20141201/src
SIFT := sift_anatomy_20141201
SIFT_SRC := ./$(SIFT)/src
SIFT_SOURCES := $(filter-out $(SIFT_SRC)/example.c $(SIFT_SRC)/demo_extract_patch.c $(SIFT_SRC)/match_cli.c $(SIFT_SRC)/sift_cli.c $(SIFT_SRC)/sift_cli_default.c $(SIFT_SRC)/anatomy2lowe.c, $(wildcard $(SIFT_SRC)/*.c))
SIFT_OBJECTS := $(SIFT_SOURCES:%.c=%.o)

ALL_SOURCES := $(wildcard $(SRC)/*.cpp)
SOURCES := $(filter-out src/siftMain.cpp src/quadcopter.cpp, $(ALL_SOURCES)) # `filter-out`: Remove files with `int main`'s so we can add them later per subproject    # https://stackoverflow.com/questions/10276202/exclude-source-file-in-compilation-using-makefile/10280945
SOURCES_C := $(wildcard $(SRC)/*.c)
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



# release target
ADDITIONAL_CFLAGS_RELEASE = -Ofast #-O3 # TODO: check -Osize ( https://stackoverflow.com/questions/19470873/why-does-gcc-generate-15-20-faster-code-if-i-optimize-for-size-instead-of-speed )
$(eval $(call C_AND_CXX_FLAGS_template,release,$(ADDITIONAL_CFLAGS_RELEASE),))

# release_commandLine target
ADDITIONAL_CFLAGS_RELEASE_COMMANDLINE = $(ADDITIONAL_CFLAGS_RELEASE) -DUSE_COMMAND_LINE_ARGS
$(eval $(call C_AND_CXX_FLAGS_template,release_commandLine,$(ADDITIONAL_CFLAGS_RELEASE_COMMANDLINE),))

# debug target
ADDITIONAL_CFLAGS_DEBUG = -Og -g3 # -O0
$(eval $(call C_AND_CXX_FLAGS_template,debug,$(ADDITIONAL_CFLAGS_DEBUG),))

# debug_commandLine target
ADDITIONAL_CFLAGS_DEBUG_COMMANDLINE = $(ADDITIONAL_CFLAGS_DEBUG) -DUSE_COMMAND_LINE_ARGS
$(eval $(call C_AND_CXX_FLAGS_template,debug_commandLine,$(ADDITIONAL_CFLAGS_DEBUG_COMMANDLINE),))

# release linking
$(eval $(call OBJECTS_LINKING_template,release,$(ADDITIONAL_CFLAGS_RELEASE)))
$(eval $(call OBJECTS_LINKING_template,release_commandLine,$(ADDITIONAL_CFLAGS_RELEASE)))

# debug linking
ADDITIONAL_CFLAGS_DEBUG += -ffast-math -flto=full # https://developers.redhat.com/blog/2019/08/06/customize-the-compilation-process-with-clang-making-compromises
$(eval $(call OBJECTS_LINKING_template,debug,$(ADDITIONAL_CFLAGS_DEBUG)))
$(eval $(call OBJECTS_LINKING_template,debug_commandLine,$(ADDITIONAL_CFLAGS_DEBUG)))



quadcopter: $(OBJECTS) src/quadcopter.o
	$(CXX) $^ -o $@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(wildcard $(SIFT_SRC)/*.o)

# .PHONY: testObjFile
# testObjFile:
# 	$(CC) sift_anatomy_20141201/src/lib_sift_anatomy.c -c -o lib_sift_anatomy.o

.PHONY: clean
clean:
	rm -f $(ALL_OBJECTS_FROM_TARGETS) $(EXECUTABLE_RESULT)
