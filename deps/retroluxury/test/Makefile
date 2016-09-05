SOEXT=.dll
TARGET=test$(SOEXT)
OBJEXT=.o
CC=gcc
CXX=g++
#CCFLAGS=-Wall -O2 --std=c99 -I. -I../src -fPIC
CCFLAGS=-Wall -O0 -g --std=c99 -I. -I../src -fPIC
CXXFLAGS=$(CCFLAGS)
LDFLAGS=-L../src
LIBS=-lretroluxury

OBJS=libretro.o
RES=res/smile.h res/sketch008.h res/bounce.h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

%.h: %.png
	xxd -i $< | sed "s/unsigned/const unsigned/g" > $@

%.h: %.ogg
	xxd -i $< | sed "s/unsigned/const unsigned/g" > $@

%.h: %.wav
	xxd -i $< | sed "s/unsigned/const unsigned/g" > $@

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared $(LDFLAGS) -o $@ $+ $(LIBS)

libretro.o: $(RES)

clean:
	rm -f $(TARGET) $(OBJS) $(RES)

.PHONY: clean
