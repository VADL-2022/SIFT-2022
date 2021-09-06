CXXFLAGS += -g3 -Wall -pedantic -std=c++11 -D_POSIX_C_SOURCE=200809L `pkg-config --cflags opencv4`
LFLAGS += -lpng -lm -lpthread #-ljpeg -lrt -lm
LDFLAGS = `pkg-config --libs opencv4`

CC := clang++
SRC := src
OBJ := obj

#SIFT_SRC := /Volumes/MyTestVolume/SeniorSemester1_Vanderbilt_University/RocketTeam/MyWorkspaceAndTempFiles/MyDocuments/SIFT_articleAndImplementation/sift_anatomy_20141201/src
SIFT := sift_anatomy_20141201
SIFT_SRC := ./$(SIFT)/src

SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SOURCES))

EXECUTABLE_RESULT=example

all: $(EXECUTABLE_RESULT)
# gcc -std=c99 -o example example2.c $(SIFT)/lib_sift.o $(SIFT)/lib_sift_anatomy.o \
# $(SIFT)/lib_keypoint.o  $(SIFT)/lib_scalespace.o $(SIFT)/lib_description.o \
# $(SIFT)/lib_discrete.o $(SIFT)/lib_util.o -lm -I$(SIFT)

setup:
	mkdir -p $(OBJ)

$(EXECUTABLE_RESULT): $(OBJECTS)
	cd $(SIFT) && $(MAKE)
	$(CC) $^ -o $@ $(LIBS) $(LDFLAGS) $(LFLAGS) $(wildcard $(SIFT_SRC)/*.o)

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CC) $(CXXFLAGS) -I$(SIFT_SRC) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE_RESULT)
