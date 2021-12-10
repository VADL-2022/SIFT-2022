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

# https://stackoverflow.com/questions/8025766/makefile-auto-dependency-generation
#-include $(SRC:%.cpp=%.d)
endef

# Usage: `$(eval $(call OBJECTS_LINKING_template,release_orWhateverTargetYouWant,flagsHere))`
define OBJECTS_LINKING_template =
OBJECTS_$(1) = $(OBJECTS) $(SIFT_OBJECTS)
OBJECTS_$(1) := $$(addsuffix _$(1).o, $$(patsubst %.o,%, $$(OBJECTS_$(1)))) src/siftMain_$(1).o
ALL_OBJECTS_FROM_TARGETS += $$(OBJECTS_$(1))
sift_exe_$(1): $$(OBJECTS_$(1))
	$(CXX_LINK) $$^ -o $$@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(2)
endef

# END LIBRARY FUNCTIONS #

# BEGIN CONFIG #

# If not using this, need to avoid linking jemalloc too:
USE_JEMALLOC=1
#1

USE_PTR_INC_MALLOC=0

# END CONFIG #



SHELL := /usr/bin/env bash
OS := $(shell uname -s)
# -D_POSIX_C_SOURCE=200809L
ifeq ($(USE_JEMALLOC),1)
additionalPkgconfigPackages = jemalloc
CFLAGS += -DUSE_JEMALLOC
endif
ifeq ($(USE_PTR_INC_MALLOC),1)
CFLAGS += -DUSE_PTR_INC_MALLOC
endif
CFLAGS += -Wall -pedantic `pkg-config --cflags opencv4 libpng ${additionalPkgconfigPackages}` -I$(SIFT_SRC) -I/nix/store/zflx47lr00hipvkl5nncd2rnpzssnni6-backward-1.6/include -DBACKWARD_HAS_UNWIND=1  #`echo "$NIX_CFLAGS_COMPILE"` # TODO: get backward-cpp working for segfault stack traces
#CFLAGS += -MD -MP # `-MD -MP` : https://stackoverflow.com/questions/8025766/makefile-auto-dependency-generation
$(info $(OS))
ifeq ($(OS),Darwin)
CFLAGS += -target x86_64-apple-macos10.15 #-isysroot $(shell xcrun --sdk macosx --show-sdk-path)
endif
CXXFLAGS += -std=c++17 $(CFLAGS)
CLANGVERSION = $(shell clang --version | head -n 1 | sed -E 's/clang version (.*) .*/\1/' | awk '{$$1=$$1;print}') # https://stackoverflow.com/questions/5188267/checking-the-gcc-version-in-a-makefile
$(info $(CLANGVERSION)) # Example: "7.1.0 "
ifeq ($(shell foo="$(CLANGVERSION)"; if [ "$${foo//./}" -le 710 ]; then echo 0; fi),0) #ifeq ($(shell foo="$(CLANGVERSION)"; test ${foo//./} -le 710; echo $$?),0) # TODO: check if version less than or equal to this
ifeq ($(OS),Darwin)
    LFLAGS += -lc++fs
endif
endif
ifeq ($(OS),Darwin)
    LFLAGS += -framework CoreGraphics
else ifeq ($(OS),Linux)
    LFLAGS += -lX11
endif
$(info $(LFLAGS))
LFLAGS += -lpng -lm -lpthread -ldl #-ljpeg -lrt -lm
# ifeq ($(OS),Linux)
#     # Needed if gtk; nix hack:
#     LDFLAGS = -L/nix/store/vvird2i7lakg2awpwd360l77bbrwbwx0-opencv-4.5.2/lib `bash ./filter-hack.sh "${NIX_LDFLAGS}"`
# else
    LDFLAGS = ${NIX_LDFLAGS}
# endif
LDFLAGS += `pkg-config --libs opencv4 libpng ${additionalPkgconfigPackages}`

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

#SIFT_SRC := /Volumes/MyTestVolume/SeniorSemester1_Vanderbilt_University/RocketTeam/MyWorkspaceAndTempFiles/MyDocuments/SIFT_articleAndImplementation/sift_anatomy_20141201/src
SIFT := sift_anatomy_20141201
SIFT_SRC := ./$(SIFT)/src
SIFT_SOURCES_C := $(filter-out $(SIFT_SRC)/example.c $(SIFT_SRC)/demo_extract_patch.c $(SIFT_SRC)/match_cli.c $(SIFT_SRC)/sift_cli.c $(SIFT_SRC)/sift_cli_default.c $(SIFT_SRC)/anatomy2lowe.c, $(wildcard $(SIFT_SRC)/*.c))
SIFT_SOURCES_CPP := $(wildcard $(SIFT_SRC)/*.cpp)
SIFT_OBJECTS := $(SIFT_SOURCES_CPP:%.cpp=%.o) $(SIFT_SOURCES_C:%.c=%.o)

ALL_SOURCES := $(wildcard $(SRC)/*.cpp)
SOURCES := $(filter-out src/siftMain.cpp src/quadcopter.cpp, $(ALL_SOURCES)) $(wildcard $(SRC)/tools/*.cpp) #$(wildcard $(SRC)/optick/src/*.cpp) # `filter-out`: Remove files with `int main`'s so we can add them later per subproject    # https://stackoverflow.com/questions/10276202/exclude-source-file-in-compilation-using-makefile/10280945
SOURCES_C := $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/tools/*.c)
ALL_OBJECTS := $(ALL_SOURCES:%.cpp=%.o) $(SOURCES_C:%.c=%.o)
OBJECTS := $(SOURCES:%.cpp=%.o) $(SOURCES_C:%.c=%.o) # https://stackoverflow.com/questions/60329676/search-for-all-c-and-cpp-files-and-compiling-them-in-one-makefile
$(info $(OBJECTS)) # https://stackoverflow.com/questions/19488990/how-to-add-or-in-pathsubst-in-makefile

# https://stackoverflow.com/questions/4219255/how-do-you-get-the-list-of-targets-in-a-makefile/58316463#58316463
list:
	@echo
	@echo "The default target is not supported. Run one of the specific targets such as one of the below targets:"
	@sh -c "$(MAKE) -p no_targets__ | awk -F':' '/^[a-zA-Z0-9][^\$$#\/\\t=]*:([^=]|$$)/ {split(\$$1,A,/ /);for(i in A) { if (A[i] !~ /make\[1\]/ && A[i] !~ /Makefile/) print \"   \",A[i]} }' | grep -v '__\$$' | sort"
.PHONY: no_targets__ list
no_targets__:
# ^Empty

#all: common sift_exe quadcopter
# gcc -std=c99 -o example example2.c $(SIFT)/lib_sift.o $(SIFT)/lib_sift_anatomy.o \
# $(SIFT)/lib_keypoint.o  $(SIFT)/lib_scalespace.o $(SIFT)/lib_description.o \
# $(SIFT)/lib_discrete.o $(SIFT)/lib_util.o -lm -I$(SIFT)

# setup:
# 	mkdir -p $(OBJ)

# common:
# 	cd $(SIFT) && $(MAKE)



# commandLine targets in general
ADDITIONAL_CFLAGS_ALL_COMMANDLINE = -DUSE_COMMAND_LINE_ARGS

# release target
# NOTE: -DNDEBUG turns off assertions (only for the code being compiled from source, not for libraries, including those from Nix like OpenCV unless we set it explicitly..).
# NOTE: -g is needed for stack traces for https://github.com/bombela/backward-cpp
ADDITIONAL_CFLAGS_RELEASE = -Ofast -g -DNDEBUG #-O3 # TODO: check -Osize ( https://stackoverflow.com/questions/19470873/why-does-gcc-generate-15-20-faster-code-if-i-optimize-for-size-instead-of-speed )
$(eval $(call C_AND_CXX_FLAGS_template,release,$(ADDITIONAL_CFLAGS_RELEASE),))

# release_commandLine target
ADDITIONAL_CFLAGS_RELEASE_COMMANDLINE = $(ADDITIONAL_CFLAGS_RELEASE) $(ADDITIONAL_CFLAGS_ALL_COMMANDLINE)
$(eval $(call C_AND_CXX_FLAGS_template,release_commandLine,$(ADDITIONAL_CFLAGS_RELEASE_COMMANDLINE),))

# debug target
ADDITIONAL_CFLAGS_DEBUG = -Og -g3 -DDEBUG # -O0
$(eval $(call C_AND_CXX_FLAGS_template,debug,$(ADDITIONAL_CFLAGS_DEBUG),))

# debug_commandLine target
ADDITIONAL_CFLAGS_DEBUG_COMMANDLINE = $(ADDITIONAL_CFLAGS_DEBUG) $(ADDITIONAL_CFLAGS_ALL_COMMANDLINE)
$(eval $(call C_AND_CXX_FLAGS_template,debug_commandLine,$(ADDITIONAL_CFLAGS_DEBUG_COMMANDLINE),))

# release linking
ADDITIONAL_CFLAGS_RELEASE += -ffast-math -flto=full # https://developers.redhat.com/blog/2019/08/06/customize-the-compilation-process-with-clang-making-compromises
$(eval $(call OBJECTS_LINKING_template,release,$(ADDITIONAL_CFLAGS_RELEASE)))
$(eval $(call OBJECTS_LINKING_template,release_commandLine,$(ADDITIONAL_CFLAGS_RELEASE)))

# debug linking
#ADDITIONAL_CFLAGS_DEBUG += 
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
