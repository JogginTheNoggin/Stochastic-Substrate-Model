#include "../headers/Simulator.h"
#include "../headers/util/Randomizer.h"
#include "../headers/layers/InputLayer.h"
#include "../headers/layers/OutputLayer.h"
// #include "UpdateEvent.h" // Likely not needed here anymore
#include <iostream>      // For basic logging/output
#include <stdexcept>     // For exception handling during init

Simulator::Simulator(int numberOfOperators): 
    metaController(numberOfOperators), 
    updateController(metaController), 
    timeController(metaController){

    init(); 
}


/**
 * @brief Constructor for the Simulator class.
 * @details Instantiates the core controllers in the correct order, injecting
 * the MetaController instance. MetaController constructor handles initial
 * network setup based on configPath. Calls static factories for Schedulers.
 * @param configPath Optional path passed to MetaController for loading initial config.
 */
Simulator::Simulator(const std::string& configPath /*= ""*/) :
    metaController(configPath),           // 1. metaController constructed (loads/sets up network)
    updateController(metaController),   // 2. updateController constructed
    timeController(metaController)      // 3. timeController constructed
{
    init(); 
}

/**
 * @brief Destructor for the Simulator class.
 * @details Controller members handle their own cleanup via RAII.
 * Assumes Scheduler::ResetInstances() and UpdateScheduler::ResetInstances()
 * will be called globally after this destructor runs (e.g., in main).
 */
Simulator::~Simulator()
{
    requestStop(); // Ensure any background simulation thread is signaled to stop
    std::cout << "Simulator shutting down..." << std::endl;
    // Controllers are automatically destroyed here.
    // Assumes Schedulers are reset globally.
    std::cout << "Final Operator count from MetaController: " << metaController.getTotalOperatorCount() << std::endl;
    std::cout << "Simulator finished." << std::endl;
    std::cout << "Simulator shutting down..." << std::endl;
}


void Simulator::init() {
    // Purpose: Initialize the static Scheduler instances.
    // Parameters: None.
    // Return: Void.
    // Key Logic: Calls the static CreateInstance methods for both Scheduler and UpdateScheduler, linking them to the TimeController and UpdateController respectively. This is a critical setup step. 
    Scheduler::ResetInstances(); // Clear any previous instances from a prior simulation
    UpdateScheduler::ResetInstances();
    try {
        Scheduler::CreateInstance(&timeController);
        UpdateScheduler::CreateInstance(&updateController);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR during Scheduler setup: " << e.what() << std::endl;
        // Handle initialization failure (e.g., rethrow, exit)
        throw; // Rethrow to indicate simulation cannot start
    }

    // Initial network setup is now handled within metaController's constructor.
    // No call to setupInitialNetwork() needed here.
    // No initial ProcessUpdates() call needed here unless MetaController setup queues events.
    // If MetaController setup *does* queue events, the first ProcessUpdates() inside run() will handle them.

    std::cout << "Simulator initialized." << std::endl;
    std::cout << "Initial Operator count from MetaController: " << metaController.getTotalOperatorCount() << std::endl;
}


void Simulator::loadConfiguration(const std::string& filePath) {
    // Purpose: To load a network configuration from a file.
    // Parameters: @param filePath - The path to the configuration file.
    // Return: Void.
    // Key Logic: Acquires a lock to ensure thread safety, then delegates the call to MetaController.
    std::lock_guard<std::mutex> lock(simMutex);
    metaController.loadConfiguration(filePath);
}

void Simulator::saveConfiguration(const std::string& filePath) const {
    // Purpose: To save the current network configuration to a file.
    // Parameters: @param filePath - The path where the file will be saved.
    // Return: Void.
    // Key Logic: Acquires a lock for thread-safe access to the MetaController, then delegates the call.
    std::lock_guard<std::mutex> lock(simMutex);
    metaController.saveConfiguration(filePath);
}

void Simulator::createNewNetwork(int numOperators) {
    // Purpose: To create a new, randomly generated network.
    // Parameters: @param numOperators - The number of internal operators for the network.
    // Return: Void.
    // Key Logic: Acquires a lock, creates a local Randomizer, and delegates the network creation to the MetaController. 
    std::lock_guard<std::mutex> lock(simMutex);
    Randomizer r;
    metaController.randomizeNetwork(numOperators, &r);
}


// setupInitialNetwork() method removed.

/**
 * @brief Runs the simulation loop for a specified number of time steps.
 * @param numSteps The total number of discrete time steps to execute.
 * @details Executes the core simulation loop:
 * 1. Process the current time step's payload traversal and message delivery (`processCurrentStep`).
 * 2. Process any state/structural updates queued during the time step (`ProcessUpdates`).
 * 3. Advance the simulation state to the next time step (`advanceStep`).
 * Includes basic step logging.
 */
void Simulator::run(int numSteps)
{

    // Purpose: To run the simulation for a fixed number of steps.
    // Parameters: @param numSteps - The number of time steps to execute.
    // Return: Void.
    // Key Logic: Resets the stop flag. Loops for `numSteps`, acquiring a lock for each step to perform processing via TimeController and UpdateController. Checks the stop flag each iteration to allow for graceful interruption. 
    stopFlag = false;
    std::cout << "Starting simulation run for " << numSteps << " steps." << std::endl;
    for (int i = 0; i < numSteps; ++i) {
        if (stopFlag) {
            std::cout << "Simulation stopped by request." << std::endl;
            break;
        }
        
        {// Create a scope for the lock guard
            std::lock_guard<std::mutex> lock(simMutex);
            // Log current step
            if ((i % logFrequency == 0) || i == numSteps -1 ) { // Log every 10 steps and the last step
                getStatusNoLock().print();  // print status at specified frequency
            }
            // 1. Process signal propagation and firing decisions for the current step
            timeController.processCurrentStep();
            // 2. Process any state/structural updates requested during the step
            updateController.ProcessUpdates(); // Handles events queued by Operators or MetaController setup
            // 3. Advance time state (move next payloads to current, increment step counter)
            timeController.advanceStep();
            // Premature termination conditions
            if (timeController.getActivePayloadCount() == 0 && updateController.IsQueueEmpty()) {
                std::cout << "Simulation ended early at step " << timeController.getCurrentStep() << " due to inactivity." << std::endl;
                break;
            }
        }

    }
    std::cout << "Simulation run finished." << std::endl;
}

// TODO check comments
/**
 * @brief Runs the simulation until an inactive state is reached or default max steps exceeded.
 * @details An inactive state is defined as having no active payloads AND
 * no pending update events. The loop continues as long as steps are below
 * DEFAULT_MAX_STEPS AND the system still has activity (payloads or updates).
 */
void Simulator::run() // Overloaded run method
{
    std::cout << "Starting simulation run until inactive state or max " << DEFAULT_MAX_STEPS << " steps." << std::endl;

    // Loop condition: Continue WHILE the step count is less than the max
    // AND (there are active payloads OR there are pending updates).
    // Loop *terminates* when steps >= max OR (no active payloads AND no pending updates).
    while (!stopFlag) {
        {
            std::lock_guard<std::mutex> lock(simMutex);
            long long currentStep = timeController.getCurrentStep();
            // --- Logging ---
            if (currentStep % logFrequency == 0) { // TODO log every ten steps, add a parameter to make customizable 
                getStatusNoLock().print();
            }
            // --- Core Simulation Steps ---
            // 1. Process time step
            timeController.processCurrentStep();
            // 2. Process updates
            updateController.ProcessUpdates();
            // 3. Advance time state
            timeController.advanceStep();
             // --- Check for Inactivity---
            if (timeController.getActivePayloadCount() == 0 && updateController.IsQueueEmpty()) {
                std::cout << "Simulation ended due to inactivity." << std::endl;
                break;
            }
            if (timeController.getCurrentStep() >= DEFAULT_MAX_STEPS) {
                 std::cout << "Simulation stopped after reaching max steps." << std::endl;
                 break;
            }
        }
    } // end while

    
    std::lock_guard<std::mutex> lock(simMutex);
    // --- Post-Loop Logging ---
    long long finalStep = timeController.getCurrentStep();
    bool hitMaxSteps = (finalStep >= DEFAULT_MAX_STEPS);

    std::cout << "--- Simulation Run Finished ---" << std::endl;
    if (hitMaxSteps) {
         std::cout << "Reason: Reached maximum step limit (" << DEFAULT_MAX_STEPS << ")." << std::endl;
    } else {
         std::cout << "Reason: Reached inactive state (no payloads or pending updates)." << std::endl; // Changed terminology
    }
    
}

void Simulator::requestStop() {
    // Purpose: To signal the simulation to stop.
    // Parameters: None.
    // Return: Void.
    // Key Logic: Sets an atomic boolean flag to true. The running simulation loop will check this flag and terminate.
    stopFlag = true;
}

void Simulator::setLogFrequency(int newLogFreq){
    std::lock_guard<std::mutex> lock(simMutex);
    if(newLogFreq < 0){
        return ;
    }
    logFrequency = newLogFreq; 
}

void Simulator::submitText(const std::string& text) {
    // Purpose: To submit input text to the network.
    // Parameters: @param text - The text to submit.
    // Return: Void.
    // Key Logic: Acquires a lock, iterates through the layers managed by MetaController to find an InputLayer instance, and calls its `inputText` method.
    std::lock_guard<std::mutex> lock(simMutex);
    for (const auto& layerPtr : metaController.getAllLayers()) {
        if (auto* inputLayer = dynamic_cast<InputLayer*>(layerPtr.get())) {
            inputLayer->inputText(text);
            return;
        }
    }
    std::cerr << "Warning: No InputLayer found to submit text." << std::endl;
}

std::string Simulator::getOutput() {
    // Purpose: To retrieve processed output from the network.
    // Parameters: None.
    // Return: @return The output string.
    // Key Logic: Acquires a lock, iterates through layers to find an OutputLayer instance, and calls its `getTextOutput` method. 
    std::lock_guard<std::mutex> lock(simMutex);
    for (const auto& layerPtr : metaController.getAllLayers()) {
        if (auto* outputLayer = dynamic_cast<OutputLayer*>(layerPtr.get())) {
            return outputLayer->hasTextOutput()? outputLayer->getTextOutput() : "[ No New Output Text. ]";
        }
    }
    return "[No OutputLayer found]";
}

SimulationStatus Simulator::getStatus() const {
    // Purpose: To get a snapshot of the simulation's current status.
    // Parameters: None.
    // Return: @return A SimulationStatus struct with current metrics.
    // Key Logic: Acquires a lock for thread-safe reading of state from the controllers and returns the data in a struct.
    std::lock_guard<std::mutex> lock(simMutex);
    return {
        timeController.getCurrentStep(),
        timeController.getActivePayloadCount(),
        updateController.QueueSize(),
        metaController.getTotalOperatorCount(),
        metaController.getLayerCount()
    };
}

// used internally, and carefully, as not thread safe intentionally
SimulationStatus Simulator::getStatusNoLock() const {
    // Purpose: To get a snapshot of the simulation's current status.
    // Parameters: None.
    // Return: @return A SimulationStatus struct with current metrics.
    return {
        timeController.getCurrentStep(),
        timeController.getActivePayloadCount(),
        updateController.QueueSize(),
        metaController.getTotalOperatorCount(),
        metaController.getLayerCount()
    };
}

std::string Simulator::getNetworkJson(bool prettyPrint) const {
    // Purpose: To get a JSON representation of the network.
    // Parameters: @param prettyPrint - Whether to format the JSON for readability.
    // Return: @return The JSON string.
    // Key Logic: Acquires a lock and delegates the call to the MetaController. 
    std::lock_guard<std::mutex> lock(simMutex);
    return metaController.getOperatorsAsJson(prettyPrint);
}