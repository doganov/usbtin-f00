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
            string response = string(buff, bytesRead - 1);
            printf("  R: %s\n", response.c_str());
            return response;
        }

        if (buff[bytesRead - 1] == 7) {
            //printf("Device returned an error!\n");
            throw runtime_error("Device returned an error!");
        }
    }
}

void writeRaw(string data) {
    printf("  W: %s\n", data.c_str());
    int result = write(portDescriptor, data.c_str(), data.size());
    if (result < 0) {
        printf("FAILED TO WRITE!\n");
        throw runtime_error("Failed to write to CAN device");
    }
}

string transmit(string cmd) {
    writeRaw(cmd + "\r");
    return readRaw();
}

void echo(string cmd) {
    string response = transmit(cmd);
    if (response != "z") {
        throw runtime_error("Device rejected command: " + cmd);
        //printf("Device rejected command: %s\n", cmd.c_str());
    }

    string loopResponse = readRaw();
    if (loopResponse != cmd)
        throw runtime_error("Received mismatched loopback response: '" + cmd + "' vs '" + loopResponse + "'");
        //printf("Received mismatched loopback response: '%s' vs '%s'\n", cmd.c_str(), loopResponse.c_str());
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

    string version = transmit("v");
    printf("Connected to USBtin %s\n", version.c_str());

    transmit("W2D00");
    transmit("S6");
    transmit("l");
}

int main(int argc,char *argv[]) {
    if (argc != 2) {
        printf("You must pass the TTY device as an argument\n");
        return -1;
    }
    string portName(argv[1]);

    connect(portName);
    initDevice();

    // Test conversation from 2016/03/30
    echo("t750840013E0000000000");
    echo("t758340017E");
    echo("t75084002138100000000");
    echo("t75854003112233");

    // Test conversation from 2016/07/15
    echo("t3006A1018AFF4AFF");
    echo("t33E51000021089");
    echo("t3001B1");
    echo("t30051000025089");

    // Pinpoint
    echo("t0002000F");
    echo("t000100");

    return 0;
}
