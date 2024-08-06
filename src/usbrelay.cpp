#include <usbrelay.hpp>
#include <string>
#include <iostream>
#include <cstdio>
#include <serialib.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using std::string;
using std::cerr;
using std::cout;
using std::endl;

// Function to sleep for a given number of milliseconds, platform-dependent
// Parameters: milliseconds - the number of milliseconds to sleep
void os_sleep(unsigned long milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds); // Windows-specific sleep function
#else
    usleep(milliseconds * 1000); // Unix-specific sleep function
#endif
}

// Constructor for the Usbrelay class, initializes the port and relay number
// Parameters: port - the communication port for the USB relay
//             relaynumber - the number of relays on the device
Usbrelay::Usbrelay(const std::string &port, int relaynumber) {
    this->device = port;
    this->baudrate = 9600; // Default baud rate
    this->relaynumber = relaynumber;
}

// Opens the communication with the USB relay device
// Returns: 1 if the device is successfully opened, -1 otherwise
int Usbrelay::openCom() {
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
int Usbrelay::closeCom() {
    this->boardinterface->closeDevice(); // Close the device
    if (this->boardinterface->isDeviceOpen()) { // Check if the device closed successfully
        return -1; // Return -1 if the device is still open
    }
    return 1; // Return 1 if the device is closed
}

// Adds a character to the receive buffer (FIFO style)
// Parameters: elt - the character to add to the receive buffer
void Usbrelay::bufferrxAdd(char elt) {
    int length = sizeof(this->bufferrx);
    for (int k = length - 1; k >= 0; k--) {
        this->bufferrx[k + 1] = this->bufferrx[k]; // Shift elements to the right
    }
    this->bufferrx[0] = elt; // Add new element at the start
}

// Adds a character to the transmit buffer (FIFO style)
// Parameters: elt - the character to add to the transmit buffer
void Usbrelay::buffertxAdd(char elt) {
    int length = sizeof(this->buffertx);
    for (int k = length - 1; k >= 0; k--) {
        this->buffertx[k + 1] = this->buffertx[k]; // Shift elements to the right
    }
    this->buffertx[0] = elt; // Add new element at the start
}

// Sends a character to the USB relay and waits for the specified time
// Parameters: data - the character to send
//             milliseconds - the number of milliseconds to wait after sending
// Returns: 1 if the data is successfully written, -1 otherwise
int Usbrelay::send(char data, unsigned long milliseconds) {
    this->buffertxAdd(data); // Add data to transmit buffer
    int status = this->boardinterface->writeChar(buffertx[0]); // Write data to device
    os_sleep(milliseconds); // Sleep for the specified time
    return status; // Return the status of the write operation
}

// Receives a specified number of bytes from the USB relay
// Parameters: nbyte - the number of bytes to receive
// Returns: 1 if the data is successfully read, -1 otherwise
int Usbrelay::recieve(int nbyte) {
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
int Usbrelay::getRelayNumber() {
    return relaynumber;
}

// Returns the communication speed (baud rate) of the USB relay
// Returns: baudrate - the communication speed in baud
int Usbrelay::getSpeed() {
    return baudrate;
}

// Returns the communication port of the USB relay
// Returns: device - the communication port as a string
std::string Usbrelay::getPort() {
    return device;
}

// Sets the communication port of the USB relay
// Parameters: port - the new communication port to be set
//returns 1 if succesfull
int Usbrelay::setPort(const std::string &port) {
    this->device = port;
    return 1;
}


// Initializes the USB relay board 
// Returns: 1 if the board is successfully initialized, -1 otherwise
int Usbrelay::initBoard() {
    int status;
    for(int i=1;i<=relaynumber+1;i++){
        send(0xA0,1);
        send(i,1);
        send(0,1);
        status = send(0xA0+i+0,1);
        if(status!=1){
            return -1;
        }
    }
    return 1; // Return 1 if initialization is successful
}

// Sets the state of the relays using a command integer
// Parameters: command - the command to set the state of the relays
// Returns: 1 if the state is successfully set, -1 otherwise
int Usbrelay::setState(int command) {
    int status;
    for(int i=1;i<=relaynumber+1;i++){
        int kstate = command & 1;
        send(0xA0,1);
        send(i,0.1);
        send(kstate,1);
        status = send(0xA0+i+kstate,1);
        command = command >> 1;
        if(status!=1){
            return -1;
        }
        boardstate[i-1] = kstate;
    }
    return 1; // Return 1 if the state is successfully set
}

// Sets the state of the relays using a command array
// Parameters: commandarray - array of commands to set the state of each relay
// Returns: 1 if the state is successfully set, -1 otherwise
int Usbrelay::setState(int commandarray[]) {
    int status;
    for(int i=1;i<=relaynumber+1;i++){
        send(0xA0,1);
        send(i,1);
        send(commandarray[i-1],1);
        status = send(0xA0+i+commandarray[i-1],1);
        if(status!=1)
            return -1;
        boardstate[i-1] = commandarray[i-1];
    }
    return 1; // Return 1 if the state is successfully set
}

std::vector<int> Usbrelay::getState(){
    return boardstate; 
}

// Returns the current state of the relay(s)
// Returns: the state of the relay(s) as a character
std::vector<char> Usbrelay::gettx() {
    return buffertx; // Return state for more than 2 relays boards
}

// Returns the last received character from the receive buffer
// Returns: the last received character
std::vector<char> Usbrelay::getrx() {
    return bufferrx;
}

// Scans for available USB relay devices and returns a list of available ports
// Returns: a vector of strings, each representing an available port
std::vector<std::string> scanBoard() {
    std::vector<std::string> portlist;
    std::string device_name;
    serialib* device = new serialib();
    for (int i = 1; i < 99; i++) {
        // Check for Windows COM port
        #if defined (_WIN32) || defined(_WIN64)
            device_name = "\\\\.\\COM" + std::to_string(i);
        #endif

        // Check for Linux port
        #ifdef __linux__
            device_name = "/dev/ttyACM" + std::to_string(i - 1);
        #endif

        if (device->openDevice(device_name.c_str(), 115200) == 1) {
            portlist.push_back(device_name); // Add the device name to the port list
            device->closeDevice(); // Close the device
        }
    }
    return portlist; // Return the list of available ports
}

// Converts a character to a bitset of 8 bits
// Parameters: mychar - the character to convert
// Returns: a bitset of 8 bits representing the character
std::bitset<8> charToBitset(char mychar) {
    std::bitset<8> mybitset(mychar);
    return mybitset;
}
