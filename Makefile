CFLAGS += -O3 -g3 -Wall -pedantic -D_POSIX_C_SOURCE=200809L `pkg-config --cflags opencv4` -I$(SIFT_SRC)
CXXFLAGS += -std=c++17 $(CFLAGS)
LFLAGS += -lpng -lm -lpthread -lc++fs #-ljpeg -lrt -lm
LDFLAGS = `pkg-config --libs opencv4`

CC := clang
SRC := src
OBJ := obj

#SIFT_SRC := /Volumes/MyTestVolume/SeniorSemester1_Vanderbilt_University/RocketTeam/MyWorkspaceAndTempFiles/MyDocuments/SIFT_articleAndImplementation/sift_anatomy_20141201/src
SIFT := sift_anatomy_20141201
SIFT_SRC := ./$(SIFT)/src

SOURCES := $(wildcard $(SRC)/*.cpp)
SOURCES_C := $(wildcard $(SRC)/*.c)
OBJECTS := $(SOURCES:%.cpp=%.o) $(SOURCES_C:%.c=%.o) # https://stackoverflow.com/questions/60329676/search-for-all-c-and-cpp-files-and-compiling-them-in-one-makefile
$(info $(OBJECTS)) # https://stackoverflow.com/questions/19488990/how-to-add-or-in-pathsubst-in-makefile

EXECUTABLE_RESULT=example

all: $(EXECUTABLE_RESULT)
# gcc -std=c99 -o example example2.c $(SIFT)/lib_sift.o $(SIFT)/lib_sift_anatomy.o \
# $(SIFT)/lib_keypoint.o  $(SIFT)/lib_scalespace.o $(SIFT)/lib_description.o \
# $(SIFT)/lib_discrete.o $(SIFT)/lib_util.o -lm -I$(SIFT)

setup:
	mkdir -p $(OBJ)

$(EXECUTABLE_RESULT): $(OBJECTS)
	cd $(SIFT) && $(MAKE)
	$(CC)++ $^ -o $@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(wildcard $(SIFT_SRC)/*.o)

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE_RESULT)
