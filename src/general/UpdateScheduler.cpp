// UpdateScheduler.cpp

#include "../headers/UpdateScheduler.h"
#include "../headers/controllers/UpdateController.h" // Required for interaction
#include "../headers/UpdateEvent.h"      // Required for method parameter type
#include <vector>
#include <mutex>
#include <stdexcept>
#include <algorithm> // For std::find in destructor

// Initialize static members
std::vector<UpdateScheduler*> UpdateScheduler::instances;
std::mutex UpdateScheduler::instanceMutex;

/**
 * @brief Private constructor. Use CreateInstance factory method.
 * @param controller Pointer to the UpdateController this UpdateScheduler will interact with.
 * @details Stores the pointer to the UpdateController instance that this UpdateScheduler
 * will forward Submit requests to. Made private to enforce creation via the static
 * CreateInstance factory method.
 * @throws std::invalid_argument if the provided controller pointer is null.
 */
UpdateScheduler::UpdateScheduler(UpdateController* controller) :
    updateControllerInstance(controller)
{
    if (!controller) {
        // Optional: Throw an exception or handle null controller pointer
        throw std::invalid_argument("UpdateController pointer cannot be null for UpdateScheduler construction.");
    }
}

/**
 * @brief Destructor. Removes this instance from the static list.
 * @details Acquires a lock on the static mutex, finds this instance within the
 * static `instances` vector, and removes it to prevent dangling pointers
 * when `get()` or `ResetInstances()` is called later.
 */
UpdateScheduler::~UpdateScheduler()
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
 * @brief [Factory Method] Creates and registers an UpdateScheduler instance.
 * @param controller Pointer to the UpdateController instance this UpdateScheduler interacts with.
 * @return UpdateScheduler* Pointer to the newly created UpdateScheduler instance.
 * @details Creates a new UpdateScheduler instance linked to the provided UpdateController.
 * It acquires a lock, adds the new instance pointer to the static `instances` vector,
 * and returns the pointer. This is the sole intended method for creating UpdateScheduler objects.
 * @throws std::invalid_argument if the provided controller pointer is null.
 * @note Should be called once during simulation setup.
 */
UpdateScheduler* UpdateScheduler::CreateInstance(UpdateController* controller)
{
    if (!controller) {
        throw std::invalid_argument("Cannot create UpdateScheduler instance with a null UpdateController.");
    }
    std::lock_guard<std::mutex> lock(instanceMutex); // Lock for safe modification
    UpdateScheduler* newInstance = new UpdateScheduler(controller);
    instances.push_back(newInstance);
    // Mutex is automatically unlocked here
    return newInstance;
}

/**
 * @brief Gets the default (first) UpdateScheduler instance.
 * @return UpdateScheduler* Pointer to the default UpdateScheduler instance.
 * @throws std::runtime_error if no UpdateScheduler instance exists (i.e., CreateInstance was never called or all instances were destroyed).
 * @details Acquires a lock, checks if the static `instances` vector is empty. If not,
 * returns the pointer to the first element (assumed to be the default). Otherwise,
 * throws an exception. This method is used by Operators or other components to
 * submit UpdateEvents.
 * @note Operators/components use this to submit update events.
 */
UpdateScheduler* UpdateScheduler::get()
{
    std::lock_guard<std::mutex> lock(instanceMutex); // Lock for safe access
    if (instances.empty()) {
        throw std::runtime_error("UpdateScheduler::get() called but no UpdateScheduler instance exists.");
    }
    // Return the first instance as the default
    return instances.front();
    // Mutex is automatically unlocked here
}

/**
 * @brief Submits an update event to the central UpdateController's queue.
 * @param event The UpdateEvent object to be queued.
 * @return Void.
 * @details This is an instance method called via `UpdateScheduler::get()->...`. It forwards
 * the UpdateEvent to the associated UpdateController instance by calling its
 * `AddToQueue` method (or similar public queuing method).
 * @note Called by any component needing to request a state/structural change. Requires a valid UpdateController instance.
 */
void UpdateScheduler::Submit(const UpdateEvent& event)
{
    if (updateControllerInstance) {
        updateControllerInstance->AddToQueue(event); // Assuming AddToQueue is the public method on UpdateController
    } else {
        // Handle error: Associated UpdateController is somehow null
        // This shouldn't happen if CreateInstance enforces non-null controller
        // Maybe log an error or throw?
        // std::cerr << "Error: UpdateScheduler cannot submit event, UpdateController instance is null." << std::endl;
    }
}

/**
 * @brief Clears all UpdateScheduler instances. Useful for simulation reset/shutdown.
 * @details Acquires a lock, iterates through the static `instances` vector,
 * deletes each pointed-to UpdateScheduler object (calling their destructors which
 * will attempt to remove them from the list, although we clear it anyway),
 * and finally clears the vector itself.
 * @warning This permanently deletes all UpdateScheduler instances. Use with caution,
 * typically only during application shutdown or a full simulation reset.
 */
void UpdateScheduler::ResetInstances()
{
    std::lock_guard<std::mutex> lock(instanceMutex); // Lock for safe modification
    // Delete all pointed-to instances
    for (UpdateScheduler* instance : instances) {
        delete instance; // This will call the destructor, attempting removal again, which is safe
    }
    // Clear the vector of pointers
    instances.clear();
    // Mutex is automatically unlocked here
}