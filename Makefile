
CC = g++
CFLAGS = -I. -std=gnu++11 -D_DEBUG -ggdb `pkg-config --cflags libftdi1`
LFLAGS = `pkg-config --libs libftdi1`
TARGET = ftdi_prog

HEADERS = Options.hpp ftdi_dev.hpp
SOURCES = Options.cpp ftdi_dev.cpp main.cpp

OBJS = $(patsubst %.cpp, %.o, $(SOURCES))


.PHONY: default all clean

default: $(TARGET)
all: default

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -Wall $(LFLAGS) -o $@

clean:
	-rm -f $(OBJS)
	-rm -f $(TARGET)
	-rm -f *.cpp~ *.hpp~ Makefile~
