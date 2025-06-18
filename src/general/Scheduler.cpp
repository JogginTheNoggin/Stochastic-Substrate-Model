// Scheduler.cpp

#include "../headers/Scheduler.h"
#include "../headers/controllers/TimeController.h" // Required for interaction
#include "../headers/Payload.h"        // Required for method parameter type
#include <vector>
#include <mutex>
#include <stdexcept>
#include <algorithm> // For std::find in destructor

// Initialize static members
std::vector<Scheduler*> Scheduler::instances;
std::mutex Scheduler::instanceMutex;

/**
 * @brief Private constructor. Use CreateInstance factory method.
 * @param controller Pointer to the TimeController this Scheduler will manage.
 * @details Stores the pointer to the TimeController instance that this Scheduler
 * will interact with. Made private to enforce creation via the static
 * CreateInstance factory method.
 */
Scheduler::Scheduler(TimeController* controller) :
    timeControllerInstance(controller)
{
    if (!controller) {
        // Optional: Throw an exception or handle null controller pointer if necessary
        throw std::invalid_argument("TimeController pointer cannot be null for Scheduler construction.");
    }
}

/**
 * @brief Destructor. Removes this instance from the static list.
 * @details Acquires a lock on the static mutex, finds this instance within the
 * static `instances` vector, and removes it to prevent dangling pointers
 * when `get()` or `ResetInstances()` is called later.
 */
Scheduler::~Scheduler()
{
    std::lock_guard<std::mutex> lock(instanceMutex); // Lock the mutex for safe access
    // Find the iterator pointing to this instance in the vector
    auto it = std::find(instances.begin(), instances.end(), this);
    if (it != instances.end()) {
        // Remove the pointer from the vector
        instances.erase(it);
    }
    // Mutex is automatically unlocked when lock_guard goes out of scope
}

/**
 * @brief [Factory Method] Creates and registers a Scheduler instance.
 * @param controller Pointer to the TimeController instance this Scheduler interacts with.
 * @return Scheduler* Pointer to the newly created Scheduler instance.
 * @details Creates a new Scheduler instance linked to the provided TimeController.
 * It acquires a lock, adds the new instance pointer to the static `instances` vector,
 * and returns the pointer. This is the sole intended method for creating Scheduler objects.
 * @throws std::invalid_argument if the provided controller pointer is null.
 * @note Should be called once during simulation setup.
 */
Scheduler* Scheduler::CreateInstance(TimeController* controller)
{
    if (!controller) {
        throw std::invalid_argument("Cannot create Scheduler instance with a null TimeController.");
    }
    std::lock_guard<std::mutex> lock(instanceMutex); // Lock for safe modification
    Scheduler* newInstance = new Scheduler(controller);
    instances.push_back(newInstance);
    // Mutex is automatically unlocked here
    return newInstance;
}

/**
 * @brief Gets the default (first) Scheduler instance.
 * @return Scheduler* Pointer to the default Scheduler instance.
 * @throws std::runtime_error if no Scheduler instance exists (i.e., CreateInstance was never called or all instances were destroyed).
 * @details Acquires a lock, checks if the static `instances` vector is empty. If not,
 * returns the pointer to the first element (assumed to be the default). Otherwise,
 * throws an exception. This method is used by Operators or other components to
 * access the Scheduler functionality.
 * @note Operators use this to access scheduling methods.
 */
Scheduler* Scheduler::get()
{
    std::lock_guard<std::mutex> lock(instanceMutex); // Lock for safe access
    if (instances.empty()) {
        throw std::runtime_error("Scheduler::get() called but no Scheduler instance exists.");
    }
    // Return the first instance as the default
    return instances.front();
    // Mutex is automatically unlocked here, because exits scope
}

/**
 * @brief Schedules a Payload to start its journey in the *next* time step.
 * @param payload The Payload object to schedule (must have initial state set).
 * @return Void.
 * @details This is an instance method called via `Scheduler::get()->...`. It forwards
 * the request to the associated TimeController instance by calling its
 * `addToNextStepPayloads` method.
 * @note Called by Operators (typically `processData`). Requires a valid TimeController instance.
 */
void Scheduler::schedulePayloadForNextStep(const Payload& payload)
{
    if (timeControllerInstance) {
        timeControllerInstance->addToNextStepPayloads(payload);
    } else {
        // Handle error: Associated TimeController is somehow null
        // This shouldn't happen if CreateInstance enforces non-null controller
        // Maybe log an error or throw?
        // std::cerr << "Error: Scheduler cannot schedule payload, TimeController instance is null." << std::endl;
    }
}

/**
 * @brief Schedules message delivery and operator flagging for the current step.
 * @param targetOperatorId The ID of the operator receiving the message.
 * @param messageData The integer data from the payload.
 * @return Void.
 * @details This is an instance method called via `Scheduler::get()->...`. It forwards
 * the request to the associated TimeController instance by calling its
 * `deliverAndFlagOperator` method.
 * @note Called by Operators (typically `traverse`). Requires a valid TimeController instance.
 */
void Scheduler::scheduleMessage(int targetOperatorId, int messageData)
{
    if (timeControllerInstance) { // each scheduler will have own, based on one passed in constructor during creation
        timeControllerInstance->deliverAndFlagOperator(targetOperatorId, messageData);
    } else {
        // Handle error: Associated TimeController is somehow null
        // Maybe log an error or throw?
        // std::cerr << "Error: Scheduler cannot schedule message, TimeController instance is null." << std::endl;
    }
}

/**
 * @brief Clears all Scheduler instances. Useful for simulation reset/shutdown.
 * @details Acquires a lock, iterates through the static `instances` vector,
 * deletes each pointed-to Scheduler object (calling their destructors which
 * will attempt to remove them from the list, although we clear it anyway),
 * and finally clears the vector itself.
 * @warning This permanently deletes all Scheduler instances. Use with caution,
 * typically only during application shutdown or a full simulation reset.
 */
void Scheduler::ResetInstances() {
    // Purpose: Safely delete all managed Scheduler instances without causing a deadlock.
    // Key Logic:
    // 1. Create a temporary local vector to hold the pointers.
    // 2. Acquire a lock on the static mutex just long enough to move the pointers
    //    from the static vector to our local one. This leaves the static vector empty.
    // 3. The lock is released automatically.
    // 4. Iterate through the local vector and delete each pointer. The destructors will
    //    be called, but since the mutex is no longer held, they will not deadlock.

    std::vector<Scheduler*> instances_to_delete;
    {
        // Lock only to safely copy and clear the static list.
        std::lock_guard<std::mutex> lock(instanceMutex);
        instances.swap(instances_to_delete); // Efficiently move pointers to local vector
    } // Mutex is released here.

    // Now, delete the pointers from the local copy.
    // The destructors can now acquire the lock without deadlocking.
    for (Scheduler* instance : instances_to_delete) {
        delete instance;
    }
}