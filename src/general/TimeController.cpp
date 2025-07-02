// TimeController.cpp

#include "../headers/controllers/TimeController.h"
#include "../headers/controllers/MetaController.h" // For getting Operator pointers
#include "../headers/operators/Operator.h"       // For calling Operator methods
#include "../headers/Payload.h"        // For managing payload vectors
#include <vector>
#include <unordered_set>
#include <stdexcept>        // Potentially for error handling
#include <algorithm>        // For removing inactive payloads
#include <cstddef>
#include <iostream>         // For error logging
#include <fstream>          // For file I/O
/**
 * @brief Constructor for TimeController.
 * @param metaController A reference to the simulation's MetaController instance.
 * @details Stores a reference to the essential MetaController for later use in
 * retrieving Operator pointers based on their IDs. Initializes the step counter.
 * @throws std::invalid_argument if metaController reference is somehow invalid (though references usually can't be null).
 */
TimeController::TimeController(MetaController& metaController) :
    metaControllerInstance(metaController), // Initialize the reference member
    currentStep(0)
{
    // Member vectors (currentStepPayloads, nextStepPayloads) and set (operatorsToProcess)
    // are default-initialized.
}


/**
 * @brief Constructor for TimeController.
 * @param metaController A reference to the simulation's MetaController instance.
 * @param stateFilePath Optional path to a state file to load immediately upon construction.
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
TimeController::TimeController(MetaController& metaController, const std::string& stateFilePath /*= ""*/) :
    metaControllerInstance(metaController),
    currentStep(0) // Initialize step counter
{
    // Purpose: Initialize TimeController, optionally load initial state.
    // Parameters: metaController reference, optional stateFilePath.
    // Return: N/A.
    // Key Logic: Store metaController, init members, call loadState if path given.
    if (!stateFilePath.empty()) {
        std::cout << "Attempting to load initial TimeController state from: " << stateFilePath << std::endl;
        if (!loadState(stateFilePath)) {
            // Log warning if initial load fails, but don't necessarily stop construction.
            std::cerr << "Warning: Failed to load initial TimeController state from: " << stateFilePath << std::endl;
            // Ensure state is clean even after failed load attempt
            this->currentStepPayloads.clear();
            this->nextStepPayloads.clear();
            this->operatorsToProcess.clear();
            this->currentStep = 0;
        } else {
             std::cout << "Successfully loaded initial TimeController state." << std::endl;
        }
    }
    else{
        // maybe notify user?
    }
}


/**
 * @brief Destructor for TimeController.
 * @details Currently, TimeController does not directly own dynamically allocated
 * memory for its core members (vectors/sets manage their own memory). Payloads
 * are structs, typically copied into vectors. If Payloads were pointers or if
 * other resources were allocated, cleanup would happen here.
 */
TimeController::~TimeController()
{
    // No explicit cleanup needed for current members.
}

/**
 * @brief Executes one full time step of the simulation.
 * @param None
 * @return Void.
 * @details Orchestrates the execution of the distinct simulation phases for the current step
 * in the correct order: first process flagged operators (`processOperatorChecks`),
 * then process traveling payloads (`processPayloadTraversal`).
 */
void TimeController::processCurrentStep()
{
    // order important, opposite would traverse but not allow new payloads to be made 
    // if occurred at end
    // meaning they would be stuck in waiting for processing, and

    // Phase 1: Process payloads currently traveling in this step
    processPayloadTraversal();

    // Phase 2: Check Operators flagged in the previous step and call processData
    processOperatorChecks(); // tricky state, meaning potential payloads could be waiting to be made, order important

    // Optionally, add logic here for checking simulation termination conditions.
}

/**
 * @brief Prepares the controller for the next time step.
 * @param None
 * @return Void.
 * @details Moves payloads scheduled during the current step (`nextStepPayloads`)
 * into the active processing list (`currentStepPayloads`) for the upcoming step.
 * Clears the list of newly scheduled payloads. Increments the internal step counter.
 * Note: The `operatorsToProcess` set is cleared within `processOperatorChecks`.
 */
void TimeController::advanceStep()
{
    // Append the newly scheduled payloads (from nextStepPayloads) to the end of the list
    // of payloads that are still traveling from the current step.
    // merge
    if (!nextStepPayloads.empty()) {
        currentStepPayloads.insert(
            currentStepPayloads.end(),
            std::make_move_iterator(nextStepPayloads.begin()),
            std::make_move_iterator(nextStepPayloads.end())
        );
    }
    // Clear the list that held the next step's payloads
    nextStepPayloads.clear();
    // Ensure vector capacity is managed if needed (clear might not shrink capacity)
    // nextStepPayloads.shrink_to_fit(); // Optional memory optimization

    // Increment step counter
    currentStep++;
}

/**
 * @brief Adds a newly created payload to be processed starting next step.
 * @param payload The Payload object to add. It's copied into the vector.
 * @return Void.
 * @details This method is called by `Scheduler::schedulePayloadForNextStep` when an Operator's
 * `processData` method decides to fire and create a new signal. The payload
 * is added to the `nextStepPayloads` queue.
 */
void TimeController::addToNextStepPayloads(const Payload& payload)
{
    nextStepPayloads.push_back(payload);
}

/**
 * @brief Delivers message data immediately to a target Operator and flags it for next step processing.
 * @param targetOperatorId The ID of the operator receiving the message.
 * @param messageData The integer data from the payload.
 * @return Void.
 * @details This method is called by `Scheduler::scheduleMessage` when an Operator's `traverse`
 * method determines a payload has reached a target. It uses the MetaController to get
 * the target Operator's pointer. If found, it immediately calls the Operator's `message()`
 * method to deliver the data within the current time step (Step N+1) and adds the Operator's ID
 * to the `operatorsToProcess` set, flagging it for its `processData()` method to be
 * called at the start of the next time step (Step N+2).
 */
void TimeController::deliverAndFlagOperator(uint32_t targetOperatorId, int messageData)
{
    if (metaControllerInstance.messageOp(targetOperatorId, messageData)) {
        // Operator found, and message delivered
        // Flag the operator for processing in the next step's Phase 1
        operatorsToProcess.insert(targetOperatorId);
    } else {
        // Operator not found (ID was dangling).
        // The cleanup should be triggered by the *source* Operator's traverse (not the destination operator that we just tried to access)
        // method via requestUpdate, potentially based on feedback from here or Scheduler.
        // For now, we just note that delivery failed.
        // TODO: Log this event? "Warning: Could not deliver message to non-existent Operator ID " << targetOperatorId
    }
}

/**
 * @brief Gets the current simulation time step number.
 * @return long long The current step number.
 */
long long TimeController::getCurrentStep() const
{
    return currentStep;
}

/**
 * @brief Gets the number of payloads currently active in this time step.
 * @return size_t The number of payloads in the `currentStepPayloads` vector.
 */
size_t TimeController::getCurrentStepPayloadCount() const
{
    return currentStepPayloads.size();
}


/**
 * @brief Gets the number of payloads for next time step.
 * @return size_t The number of payloads in the `nextStepPayloads` vector.
 */
size_t TimeController::getNextStepPayloadCount() const
{
    return nextStepPayloads.size();
}

// --- Private Helper Methods ---

/**
 * @brief [Private Helper] Implements Phase 1: Check Operators / Process Data.
 * @details Iterates through the `operatorsToProcess` set (containing IDs of Operators
 * that received messages in the previous step). For each ID, retrieves the
 * `Operator*` from MetaController and calls its `processData()` method.
 * Clears the `operatorsToProcess` set after processing all flagged operators.
 */
void TimeController::processOperatorChecks()
{
    // Process operators flagged in the previous step
    for (uint32_t operatorId : operatorsToProcess) {

        // metaController will call the appropriate operator and process its accumulated data
        metaControllerInstance.processOpData(operatorId);
    }

    // Clear the set for the next cycle
    operatorsToProcess.clear();
}

/**
 * @brief [Private Helper] Implements Phase 2: Process Traveling Payloads.
 * @details Iterates through the `currentStepPayloads` vector. For each active payload,
 * retrieves the managing `Operator*` from MetaController and calls its `traverse()` method,
 * passing the payload by reference so its state can be updated (timeTraveled, currentBucketIndex, active).
 * After iterating, removes any payloads that were marked as inactive by `traverse`.
 */
void TimeController::processPayloadTraversal()
{
    // Iterate through payloads currently in transit for this step
    // TODO use pointer instead?
    for (Payload& payload : currentStepPayloads) { // Use reference to allow modification by traverse
        if (!payload.active) continue; // Skip already inactive payloads
        
        // metaController will find the appropriate operator, and perform the necessary steps to traverse payload
        metaControllerInstance.traversePayload(&payload);
    }

    if(currentStepPayloads.empty()){
        std::cout << "(Before change) Current is empty: Step  " + currentStep << std::endl; 
    }
    // Cleanup: Remove payloads marked as inactive during this step's traversal
    // Using erase-remove idiom
    currentStepPayloads.erase(
        std::remove_if(currentStepPayloads.begin(), currentStepPayloads.end(),
                       [](const Payload& p){ return !p.active; }),
        currentStepPayloads.end()
    );

    if(currentStepPayloads.empty()){
        std::cout << "(After change) Current is empty: Step  " + currentStep << std::endl; 
    }
    // will still contain payloads for the next timeStep, if the payload not set to false
}


bool TimeController::hasPayloads() const {
    return currentStepPayloads.size() > 0 || nextStepPayloads.size() > 0 || operatorsToProcess.size() > 0; 
}

// --- Load State Persistence Helper Implementations ---

/**
 * public
 * @brief Loads the dynamic state (payloads, operator flags) from a file.
 * 
 */
bool TimeController::loadState(const std::string& filePath) {
    // TODO loadstate method will likely need to adopt way to load some of memory not all, as in memory size is limited. Offload to some other slower storage?
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file for loading TimeController state: " << filePath << std::endl;
        return false;
    }

    // 1. Clear existing dynamic state
    this->currentStepPayloads.clear();
    this->nextStepPayloads.clear();
    this->operatorsToProcess.clear();
    this->currentStep = 0; // Reset time step

    try {
        // 2. Read Header (Counts)
        uint64_t currentCount = 0;
        uint64_t nextCount = 0;
        uint64_t opsCount = 0;
        { // Read manually or use Serializer::readUint64BE if adapted for istream
             char headerBytes[24];
             inFile.read(headerBytes, 24);
             if (inFile.gcount() != 24) throw std::runtime_error("Failed to read header counts.");
             const std::byte* ptr = reinterpret_cast<const std::byte*>(headerBytes);
             const std::byte* end = ptr + 24;
             currentCount = Serializer::read_uint64(ptr, end); // ptr advances
             nextCount    = Serializer::read_uint64(ptr, end); // ptr advances
             opsCount     = Serializer::read_uint64(ptr, end); // ptr advances
        }


        // 3. Load Current Payloads
        this->currentStepPayloads = loadPayloads(inFile, currentCount); // inFile reads will move pointer, advancing for next partition read

        // 4. Load Next Payloads
        this->nextStepPayloads = loadPayloads(inFile, nextCount); // inFile reads will move pointer, advancing

        // 5. Load Operators To Process
        this->operatorsToProcess = loadOperatorsToProcess(inFile, opsCount); // inFile reads will move pointer, advancing

        // Optional: Check if EOF is reached or if extra data exists
        if (inFile.peek() != EOF) {
             std::cerr << "Warning: Extra data found in state file after expected sections." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: Exception during TimeController::loadState: " << e.what() << std::endl;
        // Clear potentially partially loaded state
        this->currentStepPayloads.clear();
        this->nextStepPayloads.clear();
        this->operatorsToProcess.clear();
        this->currentStep = 0;
        inFile.close();
        return false;
    }

    inFile.close();
    return true;
}


/**
 * @brief Loads a specific number of payloads from the input stream.
 * @private
 */
std::vector<Payload> TimeController::loadPayloads(std::istream& in, uint64_t count) { 
    std::vector<Payload> loadedPayloads;
    if (count > 0) {
        loadedPayloads.reserve(static_cast<size_t>(count));
    }
    
    for (uint64_t i = 0; i < count; ++i) { 
        // 1. Read the 1-byte size prefix (N) which tells us the size of the data block to follow.
        char sizeByteChar;
        in.read(&sizeByteChar, 1);
        if (in.gcount() != 1) {
             throw std::runtime_error("Failed to read 1-byte size prefix for payload " + std::to_string(i+1) + "/" + std::to_string(count));
        }
        uint8_t dataSizeN = static_cast<uint8_t>(sizeByteChar);

        if (dataSizeN == 0) {
             throw std::runtime_error("Encountered payload block with declared size 0 during loadPayloads.");
        }

        // 2. Read the next N bytes into a buffer.
        std::vector<std::byte> payloadDataBuffer(dataSizeN);
        in.read(reinterpret_cast<char*>(payloadDataBuffer.data()), dataSizeN);
        if (static_cast<uint8_t>(in.gcount()) != dataSizeN) {
            throw std::runtime_error("Failed to read expected " + std::to_string(dataSizeN) + " data bytes for payload " + std::to_string(i+1) + "/" + std::to_string(count));
        }

        // 3. The Payload constructor expects a pointer to the start of the data, NOT including the size prefix.
        //    This is exactly what we have in payloadDataBuffer.
        const std::byte* dataPtr = payloadDataBuffer.data();
        const std::byte* dataEnd = dataPtr + dataSizeN;

        Payload loadedPayload(dataPtr, dataEnd);

        if (dataPtr != dataEnd) {
             throw std::runtime_error("Payload constructor did not consume entire data block for payload " + std::to_string(i+1) + ".");
        }
        loadedPayloads.push_back(std::move(loadedPayload));
    }
    return loadedPayloads;
}


/**
 * @brief Loads a specific number of operator IDs (as uint32_t) from the input stream.
 * @private
 */
std::unordered_set<uint32_t> TimeController::loadOperatorsToProcess(std::istream& in, uint64_t count) {
    // Purpose: Read 'count' uint32_t operator IDs from the stream.
    // Parameters: stream, count.
    // Return: Set of loaded uint32_t operator IDs. Throws on error.
    // Key Logic: Loops 'count' times, reads a fixed 4-byte block for each uint32_t,
    //            and deserializes it using the appropriate Serializer method.

    std::unordered_set<uint32_t> loadedIds;
    if (count > 0) {
        loadedIds.reserve(static_cast<size_t>(count)); // Reserve space
    }

    // A buffer to hold the bytes for one uint32_t
    std::vector<std::byte> idDataBuffer(sizeof(uint32_t));

    for (uint64_t i = 0; i < count; ++i) {
        // 1. Read the fixed 4 bytes for the uint32_t operator ID.
        in.read(reinterpret_cast<char*>(idDataBuffer.data()), sizeof(uint32_t));
        if (static_cast<size_t>(in.gcount()) != sizeof(uint32_t)) {
            throw std::runtime_error("Failed to read expected " + std::to_string(sizeof(uint32_t)) 
                                     + " bytes for operator ID " + std::to_string(i+1) + "/" + std::to_string(count));
        }

        // 2. Deserialize the buffer using the direct (non-prefixed) uint32 reader.
        const std::byte* dataPtr = idDataBuffer.data();
        const std::byte* dataEnd = dataPtr + idDataBuffer.size();
        uint32_t opId = Serializer::read_uint32(dataPtr, dataEnd);

        // 3. Check for consumption and insert into the set.
        if (dataPtr != dataEnd) {
           throw std::runtime_error("Operator ID (uint32_t) deserialization did not consume entire block.");
        }
        loadedIds.insert(opId);
    }
    return loadedIds;
}


// --- Save State Persistence Helper Implementations ---


/**
 * @brief Saves the dynamic state (payloads, operator flags) to a file.
 * public 
 */
bool TimeController::saveState(const std::string& filePath) const {
    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for saving TimeController state: " << filePath << std::endl;
        return false;
    }

    try {
        // 1. Count active payloads
        uint64_t currentPayloadCount = 0;
        for(const auto& p : currentStepPayloads) if(p.active) currentPayloadCount++; // necessary, to avoid saving inactive payloads

        uint64_t nextPayloadCount = 0;
        for(const auto& p : nextStepPayloads) if(p.active) nextPayloadCount++; // Also count active in next step

        uint64_t operatorToProcessCount = operatorsToProcess.size();

        // 2. Write Header (Counts)
        std::vector<std::byte> headerBuffer; headerBuffer.reserve(24); // 3 * 8 bytes
        Serializer::write(headerBuffer, currentPayloadCount);
        Serializer::write(headerBuffer, nextPayloadCount);
        Serializer::write(headerBuffer, operatorToProcessCount);
        outFile.write(reinterpret_cast<const char*>(headerBuffer.data()), headerBuffer.size());
        if (!outFile.good()) throw std::runtime_error("Failed to write header counts.");

        // 3. Write Current Payloads
        savePayloads(outFile, currentStepPayloads);

        // 4. Write Next Payloads
        savePayloads(outFile, nextStepPayloads); // Save active ones from next step too

        // 5. Write Operators To Process
        saveOperatorsToProcess(outFile);

    } catch (const std::exception& e) {
        std::cerr << "Error: Exception during TimeController::saveState: " << e.what() << std::endl;
        outFile.close(); // Attempt to close even on error
        return false;
    }

    outFile.close();
    return outFile.good();
}


/**
 * @brief Saves active payloads from a given vector to the output stream.
 * @private
 */
void TimeController::savePayloads(std::ostream& out, const std::vector<Payload>& payloads) const {
    // Purpose: Write active payloads from vector to stream using fixed-size serialization.
    // Parameters: stream, payload vector.
    // Return: void. Throws on error.
    for (const Payload& payload : payloads) {
        if (payload.active) {
            // This now returns a fixed-size byte vector with no size prefix.
            std::vector<std::byte> payloadBytes = payload.serializeToBytes(); 
            if (!payloadBytes.empty()) {
                 out.write(reinterpret_cast<const char*>(payloadBytes.data()), payloadBytes.size());
                 if (!out.good()) {
                     throw std::runtime_error("Failed to write payload data during savePayloads.");
                 }
            }
        }
    }
}

/**
 * @brief Saves operator IDs from the operatorsToProcess set to the output stream.
 * @private
 */
void TimeController::saveOperatorsToProcess(std::ostream& out) const {
    // Purpose: Write operator IDs from set to stream using fixed-size serialization.
    // Parameters: stream.
    // Return: void. Throws on error.
    std::vector<std::byte> idBuffer; 
    for (uint32_t opId : this->operatorsToProcess) {
         idBuffer.clear();
         // Use the uint32_t overload, which writes 4 bytes directly.
         Serializer::write(idBuffer, opId);

         if (!idBuffer.empty()){
            out.write(reinterpret_cast<const char*>(idBuffer.data()), idBuffer.size());
            if (!out.good()) {
                throw std::runtime_error("Failed to write operator ID during saveOperatorsToProcess.");
            }
         }
    }
}


// --- Printing and displaying ---

/**
 * @brief Prints the current list of payloads (currentStepPayloads) in JSON array format.
 */
std::string TimeController::getCurrentPayloadsJson( bool pretty /* = false */) const {
    std::ostringstream out; 
    // Purpose: Output current payloads as a JSON array.
    // Parameters: output stream, pretty flag.
    // Return: void.
    // Key Logic: Iterate currentStepPayloads, call payload.toJsonString for each, format as JSON array.

    // Define indentation helpers
    std::string indentStep = pretty ? "  " : ""; // 2 spaces for inner levels
    std::string maybeNewline = pretty ? "\n" : "";
    std::string maybeSpace = pretty ? " " : "";

    out << "[" << maybeNewline; // Start array

    bool firstElement = true;
    for (const Payload& payload : this->currentStepPayloads) {
        if (!firstElement) {
            out << "," << maybeNewline; // Comma before subsequent elements
        }
        // Call Payload's toJsonString, passing pretty flag and indentLevel 1
        out << payload.toJsonString(pretty, (pretty ? 1 : 0));
        firstElement = false;
    }

    out << maybeNewline << "]" << std::endl; // End array (add final newline for clarity)
    return out.str();
}


/**
 * @brief Prints the next list of payloads (nextStepPayloads) in JSON array format.
 */
std::string TimeController::getNextPayloadsJson(bool pretty /* = false */) const {
    std::ostringstream out; 
    // Purpose: Output current payloads as a JSON array.
    // Parameters: output stream, pretty flag.
    // Return: void.
    // Key Logic: Iterate currentStepPayloads, call payload.toJsonString for each, format as JSON array.

    // Define indentation helpers
    std::string indentStep = pretty ? "  " : ""; // 2 spaces for inner levels
    std::string maybeNewline = pretty ? "\n" : "";
    std::string maybeSpace = pretty ? " " : "";

    out << "[" << maybeNewline; // Start array

    bool firstElement = true;
    for (const Payload& payload : this->nextStepPayloads) {
        if (!firstElement) {
            out << "," << maybeNewline; // Comma before subsequent elements
        }
        // Call Payload's toJsonString, passing pretty flag and indentLevel 1
        out << payload.toJsonString(pretty, (pretty ? 1 : 0));
        firstElement = false;
    }

    out << maybeNewline << "]" << std::endl; // End array (add final newline for clarity)
    return out.str();
}
