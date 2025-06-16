#include <iostream>
#include <string>
#include <iostream>
#include <string>

// --- Include Core Controller Headers ---
#include "headers/Simulator.h"    //


// Example usage:
int main() {
    std::string input; 
    

    while(true){
        std::cout << "Enter 'q' to quit: ";
        std::cin >> input;
        if (input == "q") {
            break;
        }
    }
    return 0;
}

      