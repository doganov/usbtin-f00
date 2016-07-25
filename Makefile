CC = c++
CXXFLAGS = -g -std=c++11 -fexceptions

.PHONY: clean

usbtin-f00: usbtin-f00.o

usbtin-f00.o: usbtin-f00.cpp

clean:
	rm -f usbtin-f00.o usbtin-f00
