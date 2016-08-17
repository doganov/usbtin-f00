#include <algorithm>
#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace std;

#define STATIC_ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

string portName;
int portDescriptor = 0;
int testsRan = 0;
int testsPass = 0;
int testsFail = 0;

void connect() {
    portDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY);
    if (portDescriptor == -1) {
        printf("Unable to open port %s\n", portName.c_str());
        throw runtime_error("Unable to open port");
    } else {
        if (fcntl(portDescriptor, F_SETFL, 0) != 0) {
            throw runtime_error("fcntl() failed");
        }
    }

    struct termios options;
    // Get the current options for the port...
    if (tcgetattr(portDescriptor, &options) != 0) {
        throw runtime_error("tcgetattr() failed");
    }

    // Set the baud rates...
    if (cfsetispeed(&options, B115200) != 0) {
        throw runtime_error("cfsetispeed() failed");
    }
    if (cfsetospeed(&options, B115200) != 0) {
        throw runtime_error("cfsetospeed() failed");
    }

    // Enable the receiver and set local mode...
    cfmakeraw(&options);
    options.c_cflag |= (CLOCAL | CREAD);

    // Set the new options for the port...
    if (tcsetattr(portDescriptor, TCSANOW, &options) != 0) {
        throw runtime_error("tcsetattr() failed");
    }
}

string formatHex(char c) {
    char buff[3]; // 1 extra char for the \0
    snprintf(buff, STATIC_ARR_SIZE(buff), "%02X", c);
    return string(buff);
}

string formatHex(string data) {
    string buff;
    for (size_t i = 0; i < data.length(); i++) {
        if (i != 0) {
            buff.append(" ");
        }
        buff.append(formatHex(data[i]));
    }
    return buff;
}

void dump(string mode, string data) {
    string safeData = data;
    replace(safeData.begin(), safeData.end(), '\a', ' ');
    replace(safeData.begin(), safeData.end(), '\r', ' ');
    printf("  %s: %-24s  %s\n",
           mode.c_str(), safeData.c_str(), formatHex(data).c_str());
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
            string responseRaw = string(buff, bytesRead);
            string response = string(buff, bytesRead - 1);
            dump("R", responseRaw);
            return response;
        }

        if (buff[bytesRead - 1] == 7) {
            dump("R", string(buff, bytesRead));
            throw runtime_error("Device returned an error!");
        }
    }
}

void writeRaw(string data) {
    dump("W", data);
    int result = write(portDescriptor, data.c_str(), data.size());
    if (result < 0) {
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
    }

    string loopResponse = readRaw();
    if (loopResponse != cmd) {
        throw runtime_error("Mismatched loopback response, expected: "
                            + cmd + "', actual: '" + loopResponse + "'");
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
            dump("R", string(buff, 1));
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
    printf("Device initialized\n");
}

void test(string name, vector<string> sequence) {
    try {
        testsRan++;
        printf("\nStarting test '%s'...\n", name.c_str());
        connect();
        initDevice();
        for_each(sequence.begin(),
                 sequence.end(),
                 echo );
        testsPass++;
        printf("Test PASSED: '%s'\n", name.c_str());
    } catch (runtime_error& e) {
        testsFail++;
        printf("ERROR: %s\n", e.what());
        printf("Test FAILED: '%s'\n", name.c_str());
    }
}

int main(int argc,char *argv[]) {
    if (argc != 2) {
        printf("You must pass the TTY device as an argument\n");
        return -1;
    }
    portName = string(argv[1]);

    test("Standard doc example", vector<string>{
            "t001411223344"
        });

    test("Sequence from 2016/03/30", vector<string>{
            "t750840013E0000000000",
            "t758340017E",
            "t75084002138100000000",
            "t75854003112233"
        });

    test("Sequence from 2016/07/15", vector<string>{
            "t3006A1018AFF4AFF",
            "t33E51000021089",
            "t3001B1",
            "t30051000025089",
        });

    test("F00 after two bytes of data ending with F", vector<string>{
            "t0002000F",
            "t000100"
        });

    printf("Tests ran   : %d\n", testsRan);
    printf("Tests PASSED: %d\n", testsPass);
    printf("Tests FAILED: %d\n", testsFail);

    return testsFail;
}
