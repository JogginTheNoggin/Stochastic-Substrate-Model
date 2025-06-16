#pragma once

#include "controllers/MetaController.h"
#include "controllers/TimeController.h"
#include "controllers/UpdateController.h"
#include "Scheduler.h"
#include "UpdateScheduler.h"
#include <memory> // For smart pointers if desired, though using raw pointers for now

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
     * @brief Performs initial setup, e.g.
     */
    void init(); 


public:
    /**
     * @brief Constructor for the Simulator class.
     * @details Instantiates the core controllers, injecting dependencies,
     * and sets up the static Scheduler instances.
     * @param configPath Optional path to a configuration file for MetaController.
     */
    Simulator(const std::string& configPath = "");

    Simulator(int numberOfOperators);

    /**
     * @brief Destructor for the Simulator class.
     * @details Ensures cleanup of Scheduler instances if managed here.
     * Controllers are RAII-managed by composition.
     */
    ~Simulator();

    // Prevent copying/assignment
    Simulator(const Simulator&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    
    
    /**
     * @brief Runs the simulation until an inactive state is reached or default max steps exceeded.
     * @details An inactive state is defined as having no active payloads AND
     * no pending update events at the end of a time step. The loop continues as long as steps
     * are below DEFAULT_MAX_STEPS AND the system has activity (payloads or updates).
     */
    void run(); // Overloaded run method

    /**
     * @brief Runs the simulation for a specified number of time steps.
     * @param numSteps The total number of discrete time steps to execute.
     */
    void run(int numSteps);

};