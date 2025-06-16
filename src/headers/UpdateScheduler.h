#pragma once

#include <vector>
#include <stdexcept> // For exceptions if get() fails
#include <mutex> 	// For thread safety for static instance management

// Forward Declarations
class UpdateController;
struct UpdateEvent; // From UpdateEvent.h

/**
 * @class UpdateScheduler
 * @brief Helper class providing the sole public interface for submitting UpdateEvents.
 * @details Uses a static management pattern (CreateInstance, get) to allow access
 * without passing instances. Decouples event requestors from UpdateController internals.
 */
class UpdateScheduler {
private:
	// Static storage for instances
	static std::vector<UpdateScheduler*> instances;
	static std::mutex instanceMutex; // Mutex to protect static instances vector

	// Associated UpdateController instance (set in constructor)
	UpdateController* updateControllerInstance;

	/**
 	* @brief Private constructor. Use CreateInstance factory method.
 	* @param controller Pointer to the UpdateController this UpdateScheduler interacts with.
 	*/
	UpdateScheduler(UpdateController* controller);

public:
	// Prevent copying/assignment
	UpdateScheduler(const UpdateScheduler&) = delete;
	UpdateScheduler& operator=(const UpdateScheduler&) = delete;

	/**
 	* @brief Destructor. Removes this instance from the static list.
 	*/
	virtual ~UpdateScheduler();

	/**
 	* @brief [Factory Method] Creates and registers an UpdateScheduler instance.
 	* @param controller Pointer to the UpdateController instance this UpdateScheduler interacts with.
 	* @return UpdateScheduler* Pointer to the newly created UpdateScheduler instance.
 	* @note Should be called once during simulation setup.
 	*/
	static UpdateScheduler* CreateInstance(UpdateController* controller);

	/**
 	* @brief Gets the default (first) UpdateScheduler instance.
 	* @return UpdateScheduler* Pointer to the default UpdateScheduler instance.
 	* @throws std::runtime_error if no UpdateScheduler instance exists.
 	* @note Operators/components use this to submit update events.
 	*/
	static UpdateScheduler* get();

	// --- Instance Methods (called via UpdateScheduler::get()->...) ---

	/**
 	* @brief Submits an update event to the central UpdateController's queue.
 	* @param event The UpdateEvent object to be queued.
 	* @return Void.
 	* @note Called by any component needing to request a state/structural change.
 	* Internally calls UpdateController::AddToQueue().
 	*/
	void Submit(const UpdateEvent& event);


	// --- Static Cleanup (Optional) ---
	/**
 	* @brief Clears all UpdateScheduler instances. Useful for simulation reset/shutdown.
 	*/
	static void ResetInstances();


	// --- Removed Methods ---
	// static void Initialize(UpdateController* controller); // Replaced by CreateInstance
};
