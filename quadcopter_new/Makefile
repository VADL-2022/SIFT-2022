# https://www.manmade2.com/concurrent-c-opencv-video-streaming/

IDIR = .  
CC=g++
CFLAGS=-I$(IDIR) `pkg-config --cflags opencv4`
ODIR=.
#LDIR=../lib

LIBS=`pkg-config --cflags --libs opencv4`

DEPS = ./camLib.hpp
_OBJ = camLib.o camera.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

%.o: %.cpp $(DEPS)
	$(CC) -c -std=c++14 -pthread -o $@ $< $(LIBS) $(CFLAGS) 
camera: $(OBJ)
	$(CC) -std=c++14 -pthread -o $@ $^ $(LIBS) $(CFLAGS) 
	
.PHONY: clean
clean:
	rm -f *.o camera 
