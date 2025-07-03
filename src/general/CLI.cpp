#include "../headers/cli/CLI.h"
#include "../headers/Simulator.h"
#include <iostream>
#include <sstream>
#include <thread>

/**
 * @brief Constructor for the CLI class.
 * @param simulator A shared pointer to the Simulator object that this CLI will control.
 */
CLI::CLI(std::shared_ptr<Simulator> simulator) : sim(std::move(simulator)) {
}

/**
 * @brief Convenience overload for run() that defaults to using standard input (std::cin).
 */
void CLI::run() {
    run(std::cin);
}

/**
 * @brief Starts and runs the main command-line input loop, reading from the provided input stream.
 * @param inputStream The stream to read commands from (e.g., std::cin or a std::stringstream for tests).
 */
void CLI::run(std::istream& inputStream) {
    isRunning = true;
    std::string line;

    if (&inputStream == &std::cin) {
        std::cout << "Neuron Simulator CLI started. Type 'help' for a list of commands." << std::endl;
    }

    while (isRunning) {
        if (&inputStream == &std::cin) {
            std::cout << "> ";
        }
        if (!std::getline(inputStream, line)) {
            stop(); // Handle End-of-File
            break;
        }
        processCommand(line);
    }
    
    if (&inputStream == &std::cin) {
        std::cout << "Exiting CLI." << std::endl;
    }
}


/**
 * @brief Signals the CLI loop to stop.
 * @details This method sets the atomic isRunning flag to false, causing the run() loop to terminate.
 */
void CLI::stop() {
    isRunning = false;
}


/**
 * @brief Parses a single line of input from the user and executes the corresponding command.
 * @param line The raw string of input from the user.
 * @details This function contains the primary command-dispatching logic. It uses a stringstream to separate
 * the command from its arguments and then calls the appropriate method on the Simulator instance.
 */
void CLI::processCommand(const std::string& line) {
    std::stringstream ss(line);
    std::string command;
    ss >> command;

    if (command == "quit" || command == "exit") {
        stop();
        sim->requestStop(); // Also tell any running simulation thread to stop
    } else if (command == "load-config") {
        std::string path;
        ss >> path;
        if (path.empty()) {
            std::cout << "Error: Please provide a file path." << std::endl;
        } else {
            try {
                sim->loadConfiguration(path);
                std::cout << "Configuration loaded from " << path << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Error loading configuration: " << e.what() << std::endl;
            }
        }
    } else if (command == "save-config") {
        std::string path;
        ss >> path;
        if (path.empty()) {
            std::cout << "Error: Please provide a file path." << std::endl;
        } else {
            bool result = sim->saveConfiguration(path);
            if(result){
                std::cout << "Configuration saved to " << path << std::endl;
            }
            else{
                std::cout << "Failed to save file to " << path << std::endl;
            }
        }
    } else if (command == "load-state") {
        std::string path;
        ss >> path;
        if (path.empty()) {
            std::cout << "Error: Please provide a file path." << std::endl;
        } else {
            try {
                sim->loadState(path);
                std::cout << "Network state loaded from " << path << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Error loading state: " << e.what() << std::endl;
            }
        }
    } else if (command == "save-state") {
        std::string path;
        ss >> path;
        if (path.empty()) {
            std::cout << "Error: Please provide a file path." << std::endl;
        } else {
            sim->saveState(path);
            std::cout << "Network State saved to " << path << std::endl;
        }
    } else if (command == "new-network") {
        int num_ops;
        if (!(ss >> num_ops)) {
            std::cout << "Error: Please provide a valid number of operators." << std::endl;
        } else {
            sim->createNewNetwork(num_ops);
            std::cout << "New network created with " << num_ops << " internal operators." << std::endl;
        }
    } else if (command == "run") {
        std::string steps_str;
        ss >> steps_str;
        if (steps_str.empty()) {
            std::thread simThread([this] { sim->run(); });
            simThread.detach();
            std::cout << "Simulation running in background until inactive..." << std::endl;
        } else {
            try {
                int steps = std::stoi(steps_str);
                std::thread simThread([this, steps] { sim->run(steps); });
                simThread.detach();
                std::cout << "Simulation running in background for " << steps << " steps..." << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Error: Invalid number of steps." << std::endl;
            }
        }
    } else if (command == "pause" || command == "stop") {
        sim->requestStop();
        std::cout << "Stop request sent to simulation." << std::endl;
    } else if (command == "submit-text") {
        std::string text;
        std::getline(ss, text);
        if (!text.empty() && text.front() == ' ') {
            text = text.substr(1);
        }
        if (text.empty()) {
            std::cout << "Error: Please provide text to submit." << std::endl;
        } else {
             sim->submitText(text);
             std::cout << "Text submitted." << std::endl;
        }
    } else if (command == "get-output") {
        std::string output = sim->getOutput();
        std::cout << "Output: " << output << std::endl;
    } else if (command == "get-text-count"){
        int count = sim->getTextCount();
        std::cout << "Text Count: " << count << std::endl;
    } else if (command == "status") {
        SimulationStatus status = sim->getStatus();
        status.print(); // Use the print method from the struct
    } else if (command == "print-network") {
        std::cout << sim->getNetworkJson(true) << std::endl;
    } else if (command == "print-current-payloads") {
        std::cout << sim->getCurrentPayloadsJson(true) << std::endl;
    } else if (command == "print-next-payloads") {
        std::cout << sim->getNextPayloadsJson(true) << std::endl;
    } else if (command == "set-batch-size"){
        int size;
        if (!(ss >> size)) {
            std::cout << "Error: Please provide a valid batch size." << std::endl;
        } else {
            sim->setTextBatchSize(size);
            std::cout << "Batch size set to " << size << " ." << std::endl;
        }
    } else if (command == "log-frequency") {
        int frequency;
        if (!(ss >> frequency) || frequency <= 0) {
            std::cout << "Error: Please provide a positive integer for the frequency." << std::endl;
        } else {
            sim->setLogFrequency(frequency);
            std::cout << "Log frequency set to every " << frequency << " steps." << std::endl;
        }
    } else if (command == "clear-text-output"){
        sim->clearTextOutput();
        std::cout << "Output has been cleared" << std::endl; 
    } else if (command == "help") {
         std::cout << "Available Commands:\n"
              << "  load-config <path>      - Load network structure from a file.\n"
              << "  save-config <path>      - Save network structure to a file.\n"
              << "  load-state <path>       - Load network state from a file.\n"
              << "  save-state <path>       - Save network state to a file.\n"
              << "  new-network <count>     - Create a new random network.\n"
              << "  run [steps]             - Run simulation for N steps or until inactive.\n"
              << "  pause / stop            - Request the running simulation to stop.\n"
              << "  submit-text <text>      - Submit text to the input layer.\n"
              << "  get-output              - Retrieve and print text from the output layer.\n"
              << "  get-text-count          - Display the current amount of text output.\n"
              << "  status                  - Display the current status of the simulation.\n"
              << "  print-network           - Display the entire network structure as JSON.\n"
              << "  print-current-payloads  - Display payloads for current time step.\n"
              << "  print-next-payloads     - Display payloads for next time step.\n"
              << "  set-batch-size          - Set how many characters to return each call to get-ouput\n"
              << "  log-frequency <steps>   - Set how often status is logged during a run.\n"
              << "  clear-text-output            - Removes all output data currently stored\n"
              << "  quit / exit             - Exit the application.\n"
              << std::endl;
    } else {
        if(!command.empty()) {
            std::cout << "Unknown command: '" << command << "'. Type 'help' for a list of commands." << std::endl;
        }
    }
}