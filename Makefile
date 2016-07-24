CC = c++

.PHONY: clean

usbtin-f00: usbtin-f00.o

usbtin-f00.o: usbtin-f00.cpp

clean:
	rm -f usbtin-f00.o usbtin-f00
