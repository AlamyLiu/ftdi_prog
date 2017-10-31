
CC = g++
CFLAGS = -I. -std=gnu++11 -D_DEBUG -ggdb -I/usr/local/include/libftdi1
LFLAGS = -L/usr/local/lib
LIBS = -lftdi1
#LIBS = /usr/local/lib/libftdi1.so.2.4.0
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
	$(CC) $(OBJS) -Wall $(LFLAGS) $(LIBS) -o $@

clean:
	-rm -f $(OBJS)
	-rm -f $(TARGET)
	-rm -f *.cpp~ *.hpp~ Makefile~
