#pragma once

#include "controllers/MetaController.h"
#include "controllers/TimeController.h"
#include "controllers/UpdateController.h"
#include "Scheduler.h"
#include "UpdateScheduler.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <memory> // For smart pointers if desired, though using raw pointers for now

/**
 * @struct SimulationStatus
 * @brief A simple data structure to hold and transfer a snapshot of the simulation's status.
 * @details This is used to pass key metrics from the Simulator to the CLI without giving the CLI direct access to the controllers.
 */
struct SimulationStatus {
    long long currentStep;
    size_t activePayloads;
    size_t pendingUpdates;
    size_t totalOperators;
    size_t layerCount;

    void print(){
        std::cout << "--- Step " << currentStep << " ---" << std::endl;
        std::cout << "Active Payloads: " << activePayloads<< std::endl;
        std::cout << "Pending Updates: " << pendingUpdates << std::endl;
        std::cout << "Operator Count: " << totalOperators << std::endl; // Added operator count log
        std::cout << "Layer Count: " << layerCount << std::endl; 
    }
};

// TODO currently no comprehensive testing
class Simulator {
private:
    MetaController metaController;
    UpdateController updateController;
    TimeController timeController;

    // Store pointers to manage scheduler lifetime if needed,
    // especially if ResetInstances isn't called globally at shutdown.
    Scheduler* schedulerInstance = nullptr;
    UpdateScheduler* updateSchedulerInstance = nullptr;

    /**
     * @brief Default maximum steps for the run-until-stable method to prevent potential infinite loops.
     */
    static constexpr long long DEFAULT_MAX_STEPS = 1000000; // TODO maybe change MAX_STEPS, or make parameter

    /**
     * @brief Performs initial setup, e.g., creating initial Operators.
     * @details This is where you might load a configuration or programmatically
     * create the starting state of the network using UpdateScheduler.
     */
    // void setupInitialNetwork();

    /**
     * @brief Private helper to perform initial setup of schedulers.
     * @details This method is called by the constructors to ensure that the static Scheduler instances are created and linked to the correct controllers.
     */
    void init();

    /**
     * @brief Gets a snapshot of the current status of the simulation.
     * @return A SimulationStatus struct containing key metrics.
     * @details This method is not thread-safe, use appropriately.
     */
    SimulationStatus getStatusNoLock() const;

    // --- Threading & Synchronization ---
    mutable std::mutex simMutex;
    std::atomic<bool> stopFlag{false};
    std::atomic<bool> isRunning{false}; // Flag to prevent concurrent runs
    std::atomic<bool> hasNetwork{false}; 
    std::atomic<int> logFrequency{10}; 
public:



    /**
     * @brief Constructor for the Simulator class.
     * @details Instantiates the core controllers, injecting dependencies,
     * and sets up the static Scheduler instances.
     * @param configPath Optional path to a configuration file for MetaController.
     */
    Simulator(const std::string& configPath = "");

    /**
     * @brief Constructor for creating a Simulator with a programmatically generated network.
     * @param numberOfOperators The number of internal operators to create in the random network.
     */
    Simulator(int numberOfOperators);

    /**
     * @brief Destructor for the Simulator class.
     * @details Ensures cleanup of Scheduler instances if managed here.
     * Controllers are RAII-managed by composition.
     */
    virtual ~Simulator();

    // Prevent copying/assignment
    Simulator(const Simulator&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    // --- High-Level Command Interface for CLI ---

    /**
     * @brief Loads a network structure from a file, replacing the current one.
     * @param filePath The path to the configuration file. 
     * @details This method is thread-safe.
     */
    virtual void loadConfiguration(const std::string& filePath);

    /**
     * @brief Saves the current network structure to a file.
     * @param filePath The path where the configuration file will be saved. 
     * @details This method is thread-safe and can be called while the simulation is paused.
     */
    virtual void saveConfiguration(const std::string& filePath) const;


    /**
     * @brief Saves the current network state (payloads) to a file.
     * @param filePath The path where the state file will be saved. 
     * @details This method is thread-safe and can be called while the simulation is paused.
     */
    virtual void saveState(const std::string& filePath) const;


    /**
     * @brief Loads a network state from a file, replacing the current one.
     * @param filePath The path to the state file. 
     * @details This method is thread-safe.
     */
    virtual void loadState(const std::string& filePath);

    /**
     * @brief Creates a new, randomly initialized network, replacing the current one.
     * @param numOperators The number of internal operators for the new network. 
     * @details This method is thread-safe.
     */
    virtual void createNewNetwork(int numOperators);
    
    /**
     * @brief Runs the simulation until an inactive state is reached or default max steps exceeded.
     * @details An inactive state is defined as having no active payloads AND
     * no pending update events at the end of a time step. The loop continues as long as steps
     * are below DEFAULT_MAX_STEPS AND the system has activity (payloads or updates).
     */
    virtual void run(); // Overloaded run method

    /**
     * @brief Runs the simulation for a specified number of time steps.
     * @param numSteps The total number of discrete time steps to execute.
     */
    virtual void run(int numSteps);

    /**
     * @brief Signals the simulation loop to stop gracefully after the current step.
     * @details This method is thread-safe and can be called from any thread.
     */
    virtual void requestStop();


    /**
     * @brief Adjusts how frequent logs are displayed during simulator run
     * @param newLogFreq the new desired log frequency
     * @details Thread-safe, new log frequency cannot be negative
     */
    virtual void setLogFrequency(int newLogFreq);

    /**
     * @brief Submits a line of text to the InputLayer of the network.
     * @param text The string of text to submit as input.
     * @details This method finds the InputLayer and calls its `inputText` method. This action is thread-safe. 
     */
    virtual void submitText(const std::string& text);

    /**
     * @brief Checks if the simulator has finished running, and prints why
     * @details Used to verify the end of the simulation using timeController and UpdateController details
     * @note ! NOT THREAD SAFE ! Use when a lock is already present
     */
    virtual bool isFinished();

    /**
     * @brief Retrieves the collected output from the OutputLayer.
     * @return A string containing the processed output.
     * @details This method finds the OutputLayer and calls its `getTextOutput` method. This action is thread-safe. 
     */
    virtual std::string getOutput();

    /**
     * @brief Gets a snapshot of the current status of the simulation.
     * @return A SimulationStatus struct containing key metrics.
     * @details This method is thread-safe.
     */
    virtual SimulationStatus getStatus() const;

    /**
     * @brief Gets a JSON representation of the entire network structure.
     * @param prettyPrint If true, format the JSON with indentation for readability. 
     * @return A string containing the JSON data.
     * @details This method is thread-safe.
     */
    virtual std::string getNetworkJson(bool prettyPrint = true) const;


    

};