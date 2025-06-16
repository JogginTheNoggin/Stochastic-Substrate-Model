#include "../headers/Simulator.h"
#include "../headers/util/Randomizer.h"
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
    std::cout << "Simulator shutting down..." << std::endl;
    // Controllers are automatically destroyed here.
    // Assumes Schedulers are reset globally.
    std::cout << "Final Operator count from MetaController: " << metaController.getTotalOperatorCount() << std::endl;
    std::cout << "Simulator finished." << std::endl;
}


void Simulator::init() {
    // Create and register the static Scheduler instances
    // We don't need to store the pointers if ResetInstances is called globally at shutdown.
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
    std::cout << "Starting simulation run for " << numSteps << " steps." << std::endl;

    for (int i = 0; i < numSteps; ++i) {
        // Log current step
        if ((i % 10 == 0) || i == numSteps -1 ) { // Log every 10 steps and the last step
             std::cout << "--- Step " << timeController.getCurrentStep() << " ---" << std::endl;
             std::cout << "Active Payloads: " << timeController.getActivePayloadCount() << std::endl;
             std::cout << "Pending Updates: " << updateController.QueueSize() << std::endl;
             std::cout << "Operator Count: " << metaController.getTotalOperatorCount() << std::endl; // Added operator count log
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
     std::cout << "Simulation run finished after " << timeController.getCurrentStep() << " steps." << std::endl;
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
    while ( (timeController.getCurrentStep() < DEFAULT_MAX_STEPS) &&
            (timeController.getActivePayloadCount() > 0 || !updateController.IsQueueEmpty()) )
    {
        long long currentStep = timeController.getCurrentStep();

        // --- Logging ---
        if (currentStep % 10 == 0) { // TODO log every ten steps, add a parameter to make customizable 
             std::cout << "--- Step " << currentStep << " ---" << std::endl;
             std::cout << "Active Payloads: " << timeController.getActivePayloadCount() << std::endl;
             std::cout << "Pending Updates: " << updateController.QueueSize() << std::endl;
             std::cout << "Operator Count: " << metaController.getTotalOperatorCount() << std::endl;
        }

        // --- Core Simulation Steps ---
        // 1. Process time step
        timeController.processCurrentStep();

        // 2. Process updates
        updateController.ProcessUpdates();

        // --- Check for Inactivity (Implicit in next loop condition check) ---

        // 3. Advance time state
        timeController.advanceStep();

    } // end while

    // --- Post-Loop Logging ---
    long long finalStep = timeController.getCurrentStep();
    bool hitMaxSteps = (finalStep >= DEFAULT_MAX_STEPS);

    std::cout << "--- Simulation Run Finished ---" << std::endl;
    if (hitMaxSteps) {
         std::cout << "Reason: Reached maximum step limit (" << DEFAULT_MAX_STEPS << ")." << std::endl;
    } else {
         std::cout << "Reason: Reached inactive state (no payloads or pending updates)." << std::endl; // Changed terminology
    }
    std::cout << "Final Step Count: " << finalStep << std::endl;
    std::cout << "Final Active Payloads: " << timeController.getActivePayloadCount() << std::endl;
    std::cout << "Final Pending Updates: " << updateController.QueueSize() << std::endl;
    std::cout << "Final Operator Count: " << metaController.getTotalOperatorCount() << std::endl;
}