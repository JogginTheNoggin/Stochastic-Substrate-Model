#pragma once

#include <vector>
#include <stdexcept> // For exceptions if get() fails
#include <mutex> 	// For thread safety for static instance management

// Forward Declarations
class TimeController;
struct Payload; // From Payload.h

/**
 * @class Scheduler
 * @brief Helper class providing a restricted interface for scheduling TimeController events.
 * @details Uses a static management pattern (CreateInstance, get) to allow access
 * without passing instances. Decouples Operators from TimeController internals.
 */
class Scheduler {
private:
	// Static storage for instances
	static std::vector<Scheduler*> instances;
	static std::mutex instanceMutex; // Mutex to protect static instances vector

	// Associated TimeController instance (set in constructor)
	TimeController* timeControllerInstance;

	/**
 	* @brief Private constructor. Use CreateInstance factory method.
 	* @param controller Pointer to the TimeController this Scheduler will manage.
 	*/
	Scheduler(TimeController* controller);

public:
	// Prevent copying/assignment
	Scheduler(const Scheduler&) = delete;
	Scheduler& operator=(const Scheduler&) = delete;

	/**
 	* @brief Destructor. Removes this instance from the static list.
 	*/
	virtual ~Scheduler();

	/**
 	* @brief [Factory Method] Creates and registers a Scheduler instance.
 	* @param controller Pointer to the TimeController instance this Scheduler interacts with.
 	* @return Scheduler* Pointer to the newly created Scheduler instance.
 	* @note Should be called once during simulation setup.
 	*/
	static Scheduler* CreateInstance(TimeController* controller);

	/**
 	* @brief Gets the default (first) Scheduler instance.
 	* @return Scheduler* Pointer to the default Scheduler instance.
 	* @throws std::runtime_error if no Scheduler instance exists.
 	* @note Operators use this to access scheduling methods.
 	*/
	static Scheduler* get();

	// --- Instance Methods (called via Scheduler::get()->...) ---

	/**
 	* @brief Schedules a Payload to start its journey in the *next* time step.
 	* @param payload The Payload object to schedule (must have initial state set).
 	* @return Void.
 	* @note Called by Operators (typically `processData`).
 	*/
	void schedulePayloadForNextStep(const Payload& payload);

	/**
 	* @brief Schedules message delivery and operator flagging for the current step.
 	* @param targetOperatorId The ID of the operator receiving the message.
 	* @param messageData The integer data from the payload.
 	* @return Void.
 	* @note Called by Operators (typically `traverse`). Internally calls
 	* TimeController::deliverAndFlagOperator.
 	*/
	void scheduleMessage(int targetOperatorId, int messageData);


	// --- Static Cleanup (Optional) ---
	/**
 	* @brief Clears all Scheduler instances. Useful for simulation reset/shutdown.
 	*/
	static void ResetInstances();

	// --- Removed Methods ---
	// static void Initialize(TimeController* controller); // Replaced by CreateInstance
	// static void scheduleOpCheck(int operatorId); // Replaced by scheduleMessage
};
