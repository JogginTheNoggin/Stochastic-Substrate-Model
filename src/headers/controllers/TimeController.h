#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <iosfwd> // For std::ostream forward declaration
#include <cstddef> // For std::byte
#include <cstdint> // For uint64_t etc.

// Forward Declarations
class MetaController; // Required for dependency injection
struct Payload;   	// Required for payload lists
class Scheduler;  	// Can forward declare if only used for pointer type

/**
 * @class TimeController
 * @brief Manages the progression of the simulation through discrete time steps.
 * @details Orchestrates the processing phases for Operators and Payloads,
 * using the MetaController for ID-to-pointer lookups and coordinating
 * with the Scheduler for event timing. Instantiated externally.
 */
class TimeController {
protected:
	// Injected dependency (set via constructor)
	MetaController& metaControllerInstance;

    // TODO "nextPayloads" being moved to "currentPayloads" is not valid as the number of payloads increases.  
	// Payloads currently being processed in this step
	std::vector<Payload> currentStepPayloads;
	// Payloads scheduled to start processing in the next step
	std::vector<Payload> nextStepPayloads;
	// IDs of operators that received messages in the previous step and need processData called
	std::unordered_set<uint32_t> operatorsToProcess; // TODO, In order for this to work,  would need to store accumulated data when serializing operator objects. 

	// Internal step counter (optional)
	long long currentStep = 0; // TODO we currently do not store current Step, not really need, but may be nice to have. 

	/**
     * @brief Loads a specific number of payloads from the input stream.
     * @param in The input stream to read from.
     * @param count The exact number of payloads to load.
     * @return std::vector<Payload> The vector of loaded payloads.
     * @throws std::runtime_error On read errors, EOF, or parsing errors.
     */
    std::vector<Payload> loadPayloads(std::istream& in, uint64_t count);

    /**
     * @brief Loads a specific number of operator IDs from the input stream.
     * @param in The input stream to read from.
     * @param count The exact number of operator IDs to load.
     * @return std::unordered_set<int> The set of loaded operator IDs.
     * @throws std::runtime_error On read errors, EOF, or parsing errors.
     */
    std::unordered_set<uint32_t> loadOperatorsToProcess(std::istream& in, uint64_t count);

	/**
     * @brief Saves active payloads from a given vector to the output stream.
     * @param out The output stream to write to.
     * @param payloads The vector of Payloads to iterate through.
     * @details Checks if payload is active, calls serializeToBytes, writes result.
     * Does not write count. Throws exceptions on stream errors or serialization errors.
     */
    void savePayloads(std::ostream& out, const std::vector<Payload>& payloads) const;

    /**
     * @brief Saves operator IDs from the operatorsToProcess set to the output stream.
     * @param out The output stream to write to.
     * @details Iterates the set, serializes each ID using Serializer::appendSizedIntBE.
     * Does not write count. Throws exceptions on stream errors.
     */
    void saveOperatorsToProcess(std::ostream& out) const;


	// Internal methods for phase processing
	void processOperatorChecks();
	void processPayloadTraversal();

public:
	/**
 	 * @brief Constructor for TimeController.
 	 * @param metaController A reference to the simulation's MetaController instance.
 	 */
	TimeController(MetaController& metaController);


	/**
     * @brief Constructor for TimeController.
     * @param metaController A reference to the simulation's MetaController instance.
     * @param stateFilePath Optional path to a state data file to load immediately upon construction.
     * @details Initializes the TimeController, associating it with a MetaController.
     * If a stateFilePath is provided and not empty, attempts to load the dynamic state
     * (payloads, operator flags) by calling loadState(). Logs a warning on load failure.
     * State File Format (Condensed - Big Endian):
     * [uint64_t currentPayloadCount]
     * [uint64_t nextPayloadCount]
     * [uint64_t operatorsToProcessCount]
     * [currentPayloadCount x Payload Blocks (1-byte size + data)]
     * [nextPayloadCount x Payload Blocks (1-byte size + data)]
     * [operatorsToProcessCount x OperatorID Blocks (1-byte size + int data)]
     * - OperatorID Block: [uint8_t size = sizeof(int)][sizeof(int) bytes value BE]
     */
    explicit TimeController(MetaController& metaController, const std::string& stateFilePath);

	/**
 	 * @brief Destructor for TimeController.
	 * 
 	 */
	virtual ~TimeController();


	/**
 	 * @brief Executes one full time step of the simulation.
 	 * @param None
 	 * @return Void.
 	 * @details Calls internal methods for each processing phase in order.
 	 */
	virtual void processCurrentStep();

	/**
 	 * @brief Prepares the controller for the next time step.
 	 * @param None
 	 * @return Void.
 	 * @details Moves payloads from nextStepPayloads to currentStepPayloads, clears nextStepPayloads.
 	 */
	virtual void advanceStep();

	/**
 	 * @brief Adds a newly created payload to be processed starting next step.
 	 * @param payload The Payload object to add.
 	 * @return Void.
 	 * @note Called by Scheduler::schedulePayloadForNextStep.
 	 */
	virtual void addToNextStepPayloads(const Payload& payload);

	/**
 	 * @brief Delivers message data immediately and flags operator for next step processing.
 	 * @param targetOperatorId The ID of the operator receiving the message.
 	 * @param messageData The integer data from the payload.
 	 * @return Void.
 	 * @note Called by Scheduler::scheduleMessage. Uses MetaController to get pointer,
 	 * calls Operator::message(), and adds ID to operatorsToProcess.
 	 */
	virtual void deliverAndFlagOperator(uint32_t targetOperatorId, int messageData);


	// --- Getters (Optional) ---
	virtual long long getCurrentStep() const;
	virtual size_t getActivePayloadCount() const;

	// --- Public State Persistence Methods ---

    /**
     * @brief Saves the dynamic state (payloads, operator flags) to a file.
     * @param filePath The path to the file where the state should be saved.
     * @return bool True if saving was successful, false otherwise.
     * @details Writes the count of active current payloads, active next payloads,
     * and operators to process, followed by the serialized data for each category.
     * State File Format (Condensed - Big Endian):
     * [uint64_t currentPayloadCount]
     * [uint64_t nextPayloadCount]
     * [uint64_t operatorsToProcessCount]
     * [currentPayloadCount x Payload Blocks (1-byte size + data)]
     * [nextPayloadCount x Payload Blocks (1-byte size + data)]
     * [operatorsToProcessCount x OperatorID Blocks (1-byte size + int data)]
     * - OperatorID Block: [uint8_t size = sizeof(int)][sizeof(int) bytes value BE]
     */
    virtual bool saveState(const std::string& filePath) const;

    /**
     * @brief Loads the dynamic state (payloads, operator flags) from a file.
     * @param filePath The path to the file containing the state.
     * @return bool True if loading was successful, false otherwise.
     * @details Clears existing dynamic state (payloads, flags). Reads the counts header,
     * then loads the specified number of payloads and operator IDs. Resets currentStep to 0.
     * State File Format (Condensed - Big Endian):
     * [uint64_t currentPayloadCount]
     * [uint64_t nextPayloadCount]
     * [uint64_t operatorsToProcessCount]
     * [currentPayloadCount x Payload Blocks (1-byte size + data)]
     * [nextPayloadCount x Payload Blocks (1-byte size + data)]
     * [operatorsToProcessCount x OperatorID Blocks (1-byte size + int data)]
     * - OperatorID Block: [uint8_t size = sizeof(int)][sizeof(int) bytes value BE]
     */
    virtual bool loadState(const std::string& filePath);


	/**
     * @brief Prints the current list of payloads (currentStepPayloads) in JSON array format.
     * @param out The output stream (defaults to std::cout if implemented in cpp).
     * @param pretty If true, format with indentation and newlines. If false, compact output.
     * @return void
     */
    virtual void printCurrentPayloads(std::ostream& out /* = std::cout */, bool pretty = false) const;

	// Prevent copying/assignment
	TimeController(const TimeController&) = delete;
	TimeController& operator=(const TimeController&) = delete;



	// --- Removed Methods ---
	// void flagOperator(int operatorId); // Replaced by deliverAndFlagOperator
};
