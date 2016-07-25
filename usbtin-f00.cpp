#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <termios.h>
#include <unistd.h>

using namespace std;

int portDescriptor = 0;

void connect(string portName) {
    portDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY);
    if (portDescriptor == -1) {
        printf("Unable to open port %s\n", portName.c_str());
        throw runtime_error("Unable to open port");
    } else {
        fcntl(portDescriptor, F_SETFL, 0);
    }

    struct termios options;
    // Get the current options for the port...
    tcgetattr(portDescriptor, &options);

    // Set the baud rates...
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // Enable the receiver and set local mode...
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // Set the new options for the port...
    tcsetattr(portDescriptor, TCSANOW, &options);
}

string readRaw() {
    int buffSize = 512;
    char buff[512];
    int bytesRead = 0;
    while (true) {
        int newlyRead = read(portDescriptor, buff + bytesRead, 1);
        if (newlyRead < 0) {
            printf("Can't read from port\n");
            throw runtime_error("Can't read from port");
        }
        bytesRead += newlyRead;

        if (buff[bytesRead - 1] == '\r') {
            return string(buff, bytesRead - 1);
        }

        if (buff[bytesRead - 1] == 7) {
            printf("Device returned an error!\n");
            throw runtime_error("Device returned an error!");
        }
    }
}

void writeRaw(string data) {
    int result = write(portDescriptor, data.c_str(), data.size());
    if (result < 0) {
        printf("FAILED TO WRITE!\n");
        throw runtime_error("Failed to write to CAN device");
    }
}

void drainBuffer() {
    char buff[1];
    while (true) {
        int bytesRead = read(portDescriptor, buff, 1);
        if (bytesRead < 0) {
            throw runtime_error("Can't read from port");
        }
        if (bytesRead == 1) {
            if (buff[0] == '\r' || buff[0] == 7) {
                break;
            }
        }
        usleep(100000);
    }
}

void initDevice() {
    writeRaw("\rC\r");

    usleep(100000);

    tcflush(portDescriptor, TCIOFLUSH);

    writeRaw("C\r");
    drainBuffer();

    writeRaw("v\r");
    string version = readRaw();
    printf("Connected to USBtin %s\n", version.c_str());

    writeRaw("W2D00\r");
    readRaw();

    writeRaw("S6\r");
    readRaw();

    writeRaw("O\r");
    readRaw();
}

int main(int argc,char *argv[]) {
    if (argc != 2) {
        printf("You must pass the TTY device as an argument\n");
        return -1;
    }
    string portName(argv[1]);

    connect(portName);

    //EcuSimulatorApp simApp;
    //simApp.run(portName); // runs endlessly

    return 0;
}
