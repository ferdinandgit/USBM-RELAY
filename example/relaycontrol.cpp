#include <usbmrelay.hpp>
#include <iostream>
#include <string>
#include <vector>



void printStatus(Usbmrelay *usbmrelay){ //Format for terminal usb board relays status
    std::cout<<"=====Board Status====="<<std::endl;
    std::vector<int> status= usbmrelay->getState();
    int relaynumber = usbmrelay->getRelayNumber();
    for(int i=1;i<=relaynumber;i++){
        int kstate = status[i-1]; 
        if(kstate){
            std::cout<<"K"+std::to_string(i)+": "+"ON"<<std::endl;
        }
        else{
            std::cout<<"K"+std::to_string(i)+": "+"OFF"<<std::endl;
        }
    }
}


int main(){

    auto devicescan = scanBoard(); //Scan online COM or /dev/tty... device 
    std::cout << "=====Scanned Device=====" << std::endl; 
    for( auto device : devicescan){
        std::cout << device << std::endl;
    }
    
    //Connection and init to the board
    std::cout << "=====Connection======" << std::endl;
    
    Usbmrelay* usbmrelay = new Usbmrelay("COM7",8); //Create a new board, please specify: port, default relaynumber
    
    if(usbmrelay->openCom()!=1){//Open commmunication with the board
        std::cout << "Connection Failed" << std::endl;
        return -1;   
    }
    string choice;
    std::cout << "Board Already initialized?(y/n):";
    std::cin >> choice;
    if(choice == "N" || choice == "n"){
        if (usbmrelay->initBoard()!=1){ //Init communication protocol with the board, can be initialized only once after power reset
            std::cout << "Init Failed" << std::endl;
            return -1;
        }
    }
    
 
    //Info function
    std::cout << "=====Device Data=====" << std::endl;
    std::cout << "Port Name:" << std::endl;
    std::cout << usbmrelay->getPort() << std::endl;
    std::cout << "Relay Numbner:" << std::endl;
    std::cout << usbmrelay->getRelayNumber() << std::endl;
    std::cout << "Baudrate" << std::endl;
    std::cout << usbmrelay->getSpeed() << std::endl;
    

    //Create Command array for  relays board 
    int command1 [] = {1,1,1,1,0,0,0,0}; //Create command activate K1 and deactivate all other relays 
    int command2 = 0xf0;     //Create command activate K2 and deactivate all ather relays
    
    
    for(int k=0;k<=10;k++){
        if(usbmrelay->setState(command1)!=1)//Change the state of the board with setState(int*)
            return -1; 
        printStatus(usbmrelay); //print the status of each relays 
        os_sleep(1000); //os specific sleep
        if(usbmrelay->setState(command2)!=1)//Change the state of the board with setState(int)
            return -1; 
        printStatus(usbmrelay); //print the status of each relays
        os_sleep(1000); //os specific sleep
    }
    usbmrelay->closeCom();
}


