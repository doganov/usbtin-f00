# usbtin-f00

Simple test suite demonstrating some problems with USBtin on Mac OS X.  All
tests pass on GNU/Linux, but most fail on Mac.

USBtin is tested in loopback mode, so no other attached device is required.

## Instructions

```
$ make
$ ./usbtin-f00 /dev/cu.usbmodemA0216D01
```

Use your actual TTY device instead of `/dev/cu.usbmodemA0216D01`.  Also, make
sure you have read/write permissions to it.
