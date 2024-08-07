
#pragma once
#include <serialib.hpp>
#include <memory>
#include <string>
#include <vector>
#include <bitset>



using std::string;




class Usbmrelay
{

public:
    
    Usbmrelay(const string& port,int relaynumber = 8);
    int openCom();  
    int closeCom();
    int  initBoard();
    int setState(int*);
    int setState(int);
    std::vector<int> getState();
    std::vector<char> gettx();
    std::vector<char> getrx();
    int getSpeed();
    std::string getPort();
    int getRelayNumber();
    int setPort(const std::string &port);
    int setDelay(int delay);
    
private:

    int send(std::vector<int> data,unsigned long milliseconds);
    int recieve(int nbyte);
    void bufferrxAdd(char elt);
    void buffertxAdd(char elt);
    int baudrate;
    int relaynumber;
    int delay;
    std::string device;
    std::vector<int> boardstate = std::vector<int>(8); 
    std::vector<char> buffertx =  std::vector<char>(8);
    std::vector<char> bufferrx =  std::vector<char>(8);
    std::unique_ptr<serialib> boardinterface;
    
};

std::vector<std::string> scanBoard();
std::bitset<8> charToBitset(char);
void os_sleep(unsigned long);