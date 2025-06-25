#pragma once

#include <string>
#include <memory>
#include <atomic>

// Forward-declare the Simulator class to avoid including the full header
class Simulator; 

/**
 * @class CLI
 * @brief Manages the Command Line Interface for interacting with the Simulator.
 * @details This class is responsible for running the input loop, parsing user commands,
 * and dispatching those commands to the high-level interface of a Simulator instance.
 * It is designed to run in the main application thread.
 *
 * @property sim - A shared pointer to the Simulator instance it controls.
 * @property isRunning - An atomic flag to control the execution of the main loop, ensuring thread-safe termination.
 */
class CLI {
public:
    /**
     * @brief Constructor for the CLI class.
     * @param simulator A shared pointer to the Simulator object that this CLI will control.
     */
    explicit CLI(std::shared_ptr<Simulator> simulator);

    /**
     * @brief Starts the main command-line loop, reading from the provided input stream.
     * @param inputStream The stream to read commands from (e.g., std::cin or a std::stringstream for tests).
     * @details This method will block until the user quits or the stream ends. It continuously reads commands
     * and processes them.
     */
    void run(std::istream& inputStream);

    /**
     * @brief Starts and runs the main command-line input loop.
     * @details This method will block until the user quits. It continuously prompts for input,
     * reads commands, and processes them.
     */
    void run();

    /**
     * @brief Signals the CLI loop to stop.
     * @details This method is thread-safe.
     */
    void stop();

private:
    /**
     * @brief Parses a single line of input from the user and executes the corresponding command.
     * @param line The raw string of input from the user.
     */
    void processCommand(const std::string& line);

    std::shared_ptr<Simulator> sim;
    std::atomic<bool> isRunning{false};
};