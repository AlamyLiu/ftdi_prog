
CC = g++
CFLAGS = -I. -std=gnu++11 -D_DEBUG -ggdb
LIBS = -lftdi
TARGET = ftdi_prog

HEADERS = eeprom.hpp Options.hpp ftdi_dev.hpp
SOURCES = eeprom.cpp Options.cpp ftdi_dev.cpp main.cpp

OBJS = $(patsubst %.cpp, %.o, $(SOURCES))


.PHONY: default all clean

default: $(TARGET)
all: default

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -Wall $(LIBS) -o $@

clean:
	-rm -f $(OBJS)
	-rm -f $(TARGET)
	-rm -f *.cpp~ *.hpp~ Makefile~
