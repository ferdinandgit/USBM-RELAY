#include <usbmrelay.hpp>
#include <string>
#include <iostream>
#include <cstdio>
#include <serialib.hpp>
#include <sstream>
#include <iomanip>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using std::string;
using std::cerr;
using std::cout;
using std::endl;

// Function to sleep for a specified number of milliseconds, platform-dependent
void os_sleep(unsigned long milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds); // Windows-specific sleep function
#else
    usleep(milliseconds * 1000); // Unix-specific sleep function
#endif
}

// Constructor for the Usbmrelay class, initializes the port and relay number
// Parameters: port - the communication port for the USB relay
//             relaynumber - the number of relays on the device
Usbmrelay::Usbmrelay(const std::string &port, int relaynumber) {
    this->device = port;
    this->baudrate = 9600; // Default baud rate
    this->delay = 20; // Default delay
    this->relaynumber = relaynumber;
}

// Opens the communication with the USB relay device
// Returns: 1 if the device is successfully opened, -1 otherwise
int Usbmrelay::openCom() {
    this->boardinterface = std::make_unique<serialib>(); // Create a new serial interface
    const char *device = this->device.c_str();
    this->boardinterface->openDevice(device, baudrate); // Open device with baud rate
    os_sleep(1); // Sleep for 1 millisecond
    if (!this->boardinterface->isDeviceOpen()) { // Check if the device opened successfully
        return -1; // Return -1 if the device is not open
    }
    return 1; // Return 1 if the device is open
}

// Closes the communication with the USB relay device
// Returns: 1 if the device is successfully closed, -1 otherwise
int Usbmrelay::closeCom() {
    this->boardinterface->closeDevice(); // Close the device
    if (this->boardinterface->isDeviceOpen()) { // Check if the device closed successfully
        return -1; // Return -1 if the device is still open
    }
    return 1; // Return 1 if the device is closed
}

// Adds a character to the receive buffer (FIFO style)
// Parameters: elt - the character to add to the receive buffer
void Usbmrelay::bufferrxAdd(char elt) {
    int length = sizeof(this->bufferrx);
    for (int k = length - 1; k >= 0; k--) {
        this->bufferrx[k + 1] = this->bufferrx[k]; // Shift elements to the right
    }
    this->bufferrx[0] = elt; // Add new element at the start
}

// Adds a character to the transmit buffer (FIFO style)
// Parameters: elt - the character to add to the transmit buffer
void Usbmrelay::buffertxAdd(char elt) {
    int length = sizeof(this->buffertx);
    for (int k = length - 1; k >= 0; k--) {
        this->buffertx[k + 1] = this->buffertx[k]; // Shift elements to the right
    }
    this->buffertx[0] = elt; // Add new element at the start
}

// Sends data to the USB relay and waits for the specified time
// Parameters: data - the data to send
//             milliseconds - the number of milliseconds to wait after sending
// Returns: the status of the write operation
int Usbmrelay::send(std::vector<int> data, unsigned long milliseconds) {
    char buffer[data.size()] = {0};
    for(int i = 0; i <= data.size() - 1; i++) {
        this->buffertxAdd(data[i]);
        buffer[i] = data[i];
    }
    int status = this->boardinterface->writeBytes(buffer, data.size()); // Write data to device
    os_sleep(milliseconds); // Sleep for the specified time
    return status; // Return the status of the write operation
}

// Receives a specified number of bytes from the USB relay
// Parameters: nbyte - the number of bytes to receive
// Returns: the status of the last read operation
int Usbmrelay::recieve(int nbyte) {
    int status;
    for (int k = 1; k <= nbyte; k++) {
        char tempbuffer[2];
        status = this->boardinterface->readChar(tempbuffer, 500); // Read character with 500ms timeout
        this->bufferrxAdd(tempbuffer[0]); // Add received character to buffer
        if (status != 1) {
            return status; // Return status if read operation failed
        }
    }
    return status; // Return status of the last read operation
}

// Returns the relay number of the USB relay
// Returns: relaynumber - the number of relays on the device
int Usbmrelay::getRelayNumber() {
    return relaynumber;
}

// Returns the communication speed (baud rate) of the USB relay
// Returns: baudrate - the communication speed in baud
int Usbmrelay::getSpeed() {
    return baudrate;
}

// Returns the communication port of the USB relay
// Returns: device - the communication port as a string
std::string Usbmrelay::getPort() {
    return device;
}

// Sets the communication port of the USB relay
// Parameters: port - the new communication port to be set
// Returns: 1 if successful
int Usbmrelay::setPort(const std::string &port) {
    this->device = port;
    return 1;
}

// Sets the delay between operations on the USB relay
// Parameters: delay - the delay in milliseconds
// Returns: 1 if successful
int Usbmrelay::setDelay(int delay) {
    this->delay = delay;
    return 1;
}

// Initializes the USB relay board
// Returns: 1 if the board is successfully initialized, -1 otherwise
int Usbmrelay::initBoard() {
    int status;
    for(int i = 1; i <= relaynumber; i++) {
        std::vector<int> buffer = {0xA0, i, 0, 0xA0 + i};
        status = send(buffer, delay);
        if(status != 1) {
            return -1;
        }
    }
    return 1;
}

// Sets the state of the relays using a command integer
// Parameters: command - the command to set the state of the relays
// Returns: 1 if the state is successfully set, -1 otherwise
int Usbmrelay::setState(int command) {
    int status;
    for(int i = 1; i <= relaynumber; i++) {
        int kstate = command & 1;
        std::vector<int> buffer = {0xA0, i, kstate, 0xA0 + i + kstate};
        status = send(buffer, delay);
        command = command >> 1;
        if(status != 1) {
            return -1;
        }
        boardstate[i - 1] = kstate;
    }
    return 1;
}

// Sets the state of the relays using a command array
// Parameters: commandarray - array of commands to set the state of each relay
// Returns: 1 if the state is successfully set, -1 otherwise
int Usbmrelay::setState(int commandarray[]) {
    int status;
    for(int i = 1; i <= relaynumber; i++) {
        std::vector<int> buffer = {0xA0, i, commandarray[i - 1], 0xA0 + i + commandarray[i - 1]};
        status = send(buffer, delay);
        if(status != 1)
            return -1;
        boardstate[i - 1] = commandarray[i - 1];
    }
    return 1;
}

// Returns the current state of the relay(s)
// Returns: a vector representing the state of the relay(s)
std::vector<int> Usbmrelay::getState() {
    return boardstate;
}

// Returns the transmit buffer
// Returns: a vector of characters representing the transmit buffer
std::vector<char> Usbmrelay::gettx() {
    return buffertx;
}

// Returns the receive buffer
// Returns: a vector of characters representing the receive buffer
std::vector<char> Usbmrelay::getrx() {
    return bufferrx;
}

// Scans for available USB relay devices and returns a list of available ports
// Returns: a vector of strings, each representing an available port
std::vector<std::string> scanBoard() {
    std::vector<std::string> portlist;
    std::string device_name;
    serialib* device = new serialib();
    for (int i = 1; i < 99; i++) {
        #if defined (_WIN32) || defined(_WIN64)
            device_name = "\\\\.\\COM" + std::to_string(i);
        #endif

        #ifdef __linux__
            device_name = "/dev/ttyACM" + std::to_string(i - 1);
        #endif

        if (device->openDevice(device_name.c_str(), 115200) == 1) {
            portlist.push_back(device_name);
            device->closeDevice();
        }
    }
    return portlist;
}

// Converts a character to a bitset of 8 bits


// Parameters: mychar - the character to convert
// Returns: a bitset of 8 bits representing the character
std::bitset<8> charToBitset(char mychar) {
    std::bitset<8> mybitset(mychar);
    return mybitset;
}
