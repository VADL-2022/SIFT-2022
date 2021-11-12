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

CC := clang
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

CFLAGS_RELEASE = $(CFLAGS) -Ofast #-O3 # TODO: check -Osize ( https://stackoverflow.com/questions/19470873/why-does-gcc-generate-15-20-faster-code-if-i-optimize-for-size-instead-of-speed )
%_r.o: %.c
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@
CXXFLAGS_RELEASE = $(CXXFLAGS) $(CFLAGS_RELEASE)
%_r.o: %.cpp
	$(CC)++ $(CXXFLAGS_RELEASE) -c $< -o $@

CFLAGS_DEBUG = $(CFLAGS) -O0 -g3
%_d.o: %.c
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@
CXXFLAGS_DEBUG = $(CXXFLAGS) $(CFLAGS_DEBUG)
%_d.o: %.cpp
	$(CC)++ $(CXXFLAGS_DEBUG) -c $< -o $@

OBJECTS_RELEASE = $(OBJECTS) $(SIFT_OBJECTS)
OBJECTS_RELEASE := $(addsuffix _r.o, $(patsubst %.o,%, $(OBJECTS_RELEASE))) src/siftMain_r.o
sift_exe: $(OBJECTS_RELEASE)
	$(CC)++ $^ -o $@ $(LIBS) $(LDFLAGS) $(LFLAGS)

OBJECTS_DEBUG = $(OBJECTS) $(SIFT_OBJECTS)
OBJECTS_DEBUG := $(addsuffix _d.o, $(patsubst %.o,%, $(OBJECTS_DEBUG))) src/siftMain_d.o
sift_exe_debug: $(OBJECTS_DEBUG)
	$(CC)++ $^ -o $@ $(LIBS) -ffast-math $(LDFLAGS) $(LFLAGS) -lm -lc -lgcc -flto=full # https://developers.redhat.com/blog/2019/08/06/customize-the-compilation-process-with-clang-making-compromises

quadcopter: $(OBJECTS) src/quadcopter.o
	$(CC)++ $^ -o $@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(wildcard $(SIFT_SRC)/*.o)

# .PHONY: testObjFile
# testObjFile:
# 	$(CC) sift_anatomy_20141201/src/lib_sift_anatomy.c -c -o lib_sift_anatomy.o

.PHONY: clean
clean:
	rm -f $(ALL_OBJECTS) $(EXECUTABLE_RESULT)
