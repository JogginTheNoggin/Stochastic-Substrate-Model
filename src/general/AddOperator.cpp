// ===== src/general/AddOperator.cpp =====
#include "../headers/operators/AddOperator.h"
#include "../headers/Scheduler.h"   // For Scheduler::get()->schedulePayloadForNextStep
#include "../headers/Payload.h"    // For creating Payload objects
#include "../headers/util/Serializer.h" // Will be needed for deserializeParameters
#include <stdexcept>                // For std::runtime_error
#include <iostream>                 // For potential debug/error logging

// --- Constructor (Programmatic) ---
AddOperator::AddOperator(int id, int initialWeight, int initialThreshold)
    : Operator(id), // Base class constructor takes ID
      weight(initialWeight),
      threshold(initialThreshold),
      accumulateData(0) {
    // Purpose: Construct an AddOperator with specified parameters.
    // Parameters: id (for base), initialWeight, initialThreshold.
    // Return: N/A.
    // Key Logic: Call base constructor with ID. Initialize AddOperator-specific members.
}

// often used with randominit
AddOperator::AddOperator(uint32_t id): Operator(id){
}

// --- Deserialization Constructor ---
// TODO ensure general data read before weight, thresold etc, this goes for all other subclass. Subclass specific fields are to be read after general fields
AddOperator::AddOperator(const std::byte*& current, const std::byte* end)
    : Operator(current, end) { // Base constructor reads ID and common parts (e.g., connections)
    // Purpose: Initialize Operator AND connections using Serializer.
    // ... (comments remain the same) ...

    
    // 1. Read Operator Type (uint16_t BE), not needed here, handled by parent

    // 2. Read Operator ID (size + value BE) handled by parent

    // 3. Read Weight (size + value BE) 
    this->weight = Serializer::read_int(current, end);

    // 4. Read Threshold (size + value BE)
    this->threshold = Serializer::read_int(current, end);

    // 5. Read Accumulated Data (size + value BE) 
    this->accumulateData = Serializer::read_int(current, end);

}




// --- Overridden Abstract Methods ---
// Assynes operator IDs are sequential 
// min id is 0 
// TODO check comments
void AddOperator::randomInit(uint32_t maxOperatorId, Randomizer* rng) {
    // Purpose: Randomly initialize outgoing connections for this AddOperator.
    // Parameters: maxOperatorId - The exclusive upper limit for target operator IDs.
    // Return: Void.
    // Key Logic Steps:
    // 1. Check if maxOperatorId is valid and if MAX_CONNECTIONS > 0.
    // 2. Use Custom Randomizer for random number generation.
    // 3. Determine the number of connections to make (up to AddOperator::MAX_CONNECTIONS).
    // 4. Loop that many times:
    //    a. Randomly select a targetOperatorId (0 to maxOperatorId - 1). Avoid self-connection.
    //    b. Randomly select a distance (0 to AddOperator::MAX_DISTANCE).
    //    c. Call this->requestUpdate(UpdateType::ADD_CONNECTION, {targetOperatorId, distance});
    // Note: Connections are established when UpdateController processes these events.

    if (maxOperatorId <= 0 || AddOperator::MAX_CONNECTIONS <= 0) {
        return; // Not enough potential targets or no connections allowed
    }

    int connectionsToAttempt = rng->getInt(0, AddOperator::MAX_CONNECTIONS);

    if (connectionsToAttempt == 0) {
        return;
    }

    for (int i = 0; i < connectionsToAttempt; ++i) {
        uint32_t targetId = rng->getInt(0, maxOperatorId - 1);

        // Avoid self-connection if there are other operators
        if (targetId == this->operatorId && maxOperatorId > 1) {
            // Simple strategy: try to pick a different ID once.
            // A more robust approach might loop until a different ID is found or a max attempts is reached.
            targetId = rng->getInt(0, maxOperatorId - 1);
            if (targetId == this->operatorId) {
                continue; // Skip if still self-connecting
            }
        } else if (targetId == this->operatorId && maxOperatorId == 1) {
            continue; // Can only connect to self, so skip
        }

        int distance = rng->getInt(0, AddOperator::MAX_DISTANCE);
        // Ensure distance is non-negative (should be by uniform_int_distribution if MAX_DISTANCE >= 0)
        if (distance < 0) distance = 0;

        // TODO bulk update, at end instead of having request for each new connection. But does that mean we would have to loop through each connection again?
        // necessary becase in future data source may not be in memory but in a database. Therefore must remain in sync. 
        this->requestUpdate(UpdateType::ADD_CONNECTION, {static_cast<int>(targetId), distance}); // TODO current cast uint32_t to int for compatibility qith UpdateEvent struct
  
    }
}

void AddOperator::randomInit(IdRange* idRange, Randomizer* rng){
    // randomize the weights and threshold
    this->threshold = rng->getInt(0, std::numeric_limits<int>::max()); 
    this->weight = rng->getInt(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

    if (AddOperator::MAX_CONNECTIONS <= 0) {
        return; // Not enough potential targets or no connections allowed
    }

    int connectionsToAttempt = rng->getInt(0, AddOperator::MAX_CONNECTIONS);

    if (connectionsToAttempt == 0) {
        return;
    }

    for (int i = 0; i < connectionsToAttempt; ++i) {
        uint32_t targetId = rng->getInt(idRange->getMinId(), idRange->getMaxId()); // TODO range is uint32 while method is int

        // self-connection 
        

        int distance = rng->getInt(0, AddOperator::MAX_DISTANCE);
        // Ensure distance is non-negative (should be by uniform_int_distribution if MAX_DISTANCE >= 0)
        if (distance < 0) distance = 0;

        // TODO bulk update, at end instead of having request for each new connection. But does that mean we would have to loop through each connection again?
        // necessary becase in future data source may not be in memory but in a database. Therefore must remain in sync. 
        this->requestUpdate(UpdateType::ADD_CONNECTION, {static_cast<int>(targetId), distance}); // TODO current cast uint32_t to int for compatibility qith UpdateEvent struct
    }

}

/**
 * @brief Receives incoming integer message data and adds it to this AddOperator's accumulator.
 * @param payloadData The integer data from the arriving payload.
 */
void AddOperator::message(const int payloadData) {
    // Purpose: Accumulate incoming integer message data with overflow/underflow protection.
    // Parameters: payloadData - the integer data from the payload. [cite: 428]
    // Return: Void.
    // Key Logic: Add payloadData to internal accumulateData, clamping to INT_MAX/INT_MIN if overflow/underflow would occur.
    
    if (payloadData > 0) {
        if (this->accumulateData > std::numeric_limits<int>::max() - payloadData) { // TODO this is not correct
            this->accumulateData = std::numeric_limits<int>::max(); // Saturate at max
        } else {
            this->accumulateData += payloadData;
        }
    } else if (payloadData < 0) {
        if (this->accumulateData < std::numeric_limits<int>::min() - payloadData) { // becomes postive since payload < 0 , checks that has space to subtract. 
            this->accumulateData = std::numeric_limits<int>::min(); // Saturate at min
        } else {
            this->accumulateData += payloadData;
        }
    }
    // If payloadData is 0, accumulateData remains unchanged.
}

/**
 * @brief Handles incoming float message data. AddOperator currently ignores these.
 * @param payloadData The float data from the arriving payload.
 */
void AddOperator::message(const float payloadData) {
    // Purpose: Handle float messages by safely casting to int, then calling the int version of message().
    // Parameters: payloadData - the float data.
    // Return: Void.
    // Key Logic: Clamp float to int range, handle NaN/Inf, cast, then call message(int).

    if (std::isnan(payloadData) || std::isinf(payloadData)) {
        // Option: Log this event, or simply ignore (effectively treating as 0 for accumulation).
        // For now, we'll ignore by returning, meaning no change to accumulateData.
        // If you wanted to treat NaN/Inf as 0, you'd call this->message(0);
        return; 
    }

    // Clamp payloadData to a range that won't cause UB or problematic values when casting to int
    float clampedPayload = payloadData;
    if (payloadData > static_cast<float>(std::numeric_limits<int>::max())) {
        clampedPayload = static_cast<float>(std::numeric_limits<int>::max());
    } else if (payloadData < static_cast<float>(std::numeric_limits<int>::min())) {
        clampedPayload = static_cast<float>(std::numeric_limits<int>::min());
    }
    
    // Perform the cast (truncation will occur for fractional parts via std::trunc)
    int intPayloadData = static_cast<int>(std::trunc(clampedPayload));

    // Call the integer version of message() to handle accumulation and its specific saturation logic
    this->message(intPayloadData);
}

/**
 * @brief Handles incoming double message data. AddOperator currently ignores these.
 * @param payloadData The double data from the arriving payload.
 */
void AddOperator::message(const double payloadData) {
    // Purpose: Handle double messages by safely casting to int, then calling the int version of message().
    // Parameters: payloadData - the double data.
    // Return: Void.
    // Key Logic: Clamp double to int range, handle NaN/Inf, cast, then call message(int).

    if (std::isnan(payloadData) || std::isinf(payloadData)) {
        // Option: Log this event, or simply ignore.
        return; 
    }

    // Clamp payloadData to a range that won't cause UB or problematic values when casting to int
    double clampedPayload = payloadData;
    if (payloadData > static_cast<double>(std::numeric_limits<int>::max())) {
        clampedPayload = static_cast<double>(std::numeric_limits<int>::max());
    } else if (payloadData < static_cast<double>(std::numeric_limits<int>::min())) {
        clampedPayload = static_cast<double>(std::numeric_limits<int>::min());
    }
    
    // Perform the cast (truncation will occur for fractional parts via std::trunc)
    int intPayloadData = static_cast<int>(std::trunc(clampedPayload));

    // Call the integer version of message() to handle accumulation and its specific saturation logic
    this->message(intPayloadData);
}


/**
 * @brief Processes accumulated data. If threshold is met, applies weight (ADD logic)
 * and schedules new payload(s). Resets accumulator. This logic is copied from the
 * original universal Operator::processData and Operator::applyThresholdAndWeight.
 */
void AddOperator::processData() {
    // Purpose: Perform the ADD operation logic if conditions are met.
    // Parameters: None.
    // Return: Void.
    // Key Logic: Check if accumulateData > threshold. If so, calculate output,
    // create payload(s) via Scheduler, and reset accumulateData.

    
    

    int outputData = 0;
    // Calls the local helper method for AddOperator's specific logic
    if (applyThresholdAndWeight(this->accumulateData, outputData)) {
        /*
            This is where we need to apply specific filtering and modifications
            or above this


            maybe use modulus

        */




        // Threshold met, create and schedule a new payload if there are connections
        if (!outputConnections.empty()) {
            // Create the new payload, starting its journey.
            // The Payload constructor used here implies the new payload starts at distance 0 for its journey.
            Payload newPayload(outputData, this->operatorId); // Using this->operatorId as the source/manager

            // Schedule the new payload to start traveling in the *next* step
            try {
                if (Scheduler::get()) { // Ensure scheduler is available
                     Scheduler::get()->schedulePayloadForNextStep(newPayload);
                } else {
                    std::cerr << "AddOperator " << getId() << ": Scheduler instance is null. Cannot schedule payload." << std::endl;
                }
            } catch (const std::runtime_error& e) {
                std::cerr << "Error scheduling payload from AddOperator " << this->getId() << ": " << e.what() << std::endl;
            }
        }
        // Else: Threshold met but no outgoing connections, payload is simply absorbed.
    }
    // Else: Threshold not met, do nothing with output.

    // ALWAYS reset the accumulator after processing for the current step.
    this->accumulateData = 0;
}





/**
 * @brief Gets the specific OperatorType of this AddOperator.
 * @return OperatorType Always returns AddOperator::S_OP_TYPE.
 */
Operator::Type AddOperator::getOpType() const {
    // Purpose: Identify the type of this operator.
    // Parameters: None.
    // Return: The static OperatorType enum value for AddOperator.
    return AddOperator::OP_TYPE;
}


// --- AddOperator-Specific Private Helper ---
/**
 * @brief [Private Helper] Internal threshold check and weight application for ADD logic.
 * @param currentAccumulatedData The total data accumulated for this step.
 * @param outData [Output] The resulting data for the new payload if threshold is met.
 * @return bool True if threshold was met, false otherwise.
 */
bool AddOperator::applyThresholdAndWeight(int currentAccumulatedData, int& outData) {
    // Purpose: Implement the threshold check and data transformation for an ADD operation.
    // Parameters: currentAccumulatedData, outData (output reference).
    // Return: True if threshold was met, false otherwise.
    // Key Logic: If currentAccumulatedData > threshold, then outData = currentAccumulatedData + weight.
    if (currentAccumulatedData > this->threshold) { // TODO is weight really needed
        if (this->weight > 0) {
            if (currentAccumulatedData > std::numeric_limits<int>::max() - this->weight) {
                outData = std::numeric_limits<int>::max(); // Saturate at max
            } else {
                outData = currentAccumulatedData + this->weight;
            }
        } else if (this->weight < 0) { // Weight is negative, effectively a subtraction
            if (currentAccumulatedData < std::numeric_limits<int>::min() - this->weight) { // Note: -this->weight is positive
                outData = std::numeric_limits<int>::min(); // Saturate at min
            } else {
                outData = currentAccumulatedData + this->weight;
            }
        } else { // this->weight is 0
            outData = currentAccumulatedData;
        }
        return true; 
    }
    return false; 
}

// --- Internal Update Methods (Called ONLY by UpdateController) ---


// --- AddOperator-Specific Public Methods ---
/** @brief Gets the current weight of this AddOperator. @return int The current weight. */
int AddOperator::getWeight() const {
    return this->weight;
}

/** @brief Gets the current threshold of this AddOperator. @return int The current threshold. */
int AddOperator::getThreshold() const {
    return this->threshold;
}

/**
 * @brief [Internal Update] Sets the weight parameter for this AddOperator.
 * @param newWeight The new integer value for the weight.
 */
void AddOperator::setWeightInternal(int newWeight) {
    this->weight = newWeight;
}

/**
 * @brief [Internal Update] Sets the threshold parameter for this AddOperator.
 * @param newThreshold The new integer value for the threshold.
 */
void AddOperator::setThresholdInternal(int newThreshold) {
    this->threshold = newThreshold;
}

void AddOperator::changeParams(const std::vector<int>& params){
    if (params.size() < 2) {
        // TODO: Log error: Insufficient parameters
        return;
    }

    // Arbitrary for now, first id in params is attribute ID, second is Value
    int paramId = params[0];
    int newValue = params[1];

    if (paramId == 0) { // Weight
        this->setWeightInternal(newValue);
    } else if (paramId == 1) { // Threshold
        this->setThresholdInternal(newValue);
    } else {
        // TODO: Log error: Invalid parameter ID
    }
}

/**
 * @brief Generates a JSON string representation of the Operator's state.
 * @param prettyPrint If true, format the JSON with indentation and newlines for readability.
 * @param encloseInBrackets If true (default), wraps the entire output in curly braces `{}` making it a complete JSON object.
 * @return std::string A string containing the Operator's state formatted as JSON.
 * @note Key Logic Steps: Constructs a JSON object string. Calls the base `Operator::toJson` method to get common property JSON, then appends its own specific properties like "weight", "threshold", and "accumulateData".
 */
std::string AddOperator::toJson(bool prettyPrint, bool encloseInBrackets) const {
    std::ostringstream oss;
    std::string indent = prettyPrint ? "  " : "";
    std::string newline = prettyPrint ? "\n" : "";
    std::string space = prettyPrint ? " " : "";

    if (encloseInBrackets) {
        oss << "{" << newline;
    }

    // Call base class to get its part of the JSON, without enclosing brackets.
    oss << Operator::toJson(prettyPrint, false);

    // Add a comma to separate base content from derived content
    oss << "," << newline;

    // Add AddOperator-specific properties
    oss << indent << "\"weight\":" << space << this->weight << "," << newline;
    oss << indent << "\"threshold\":" << space << this->threshold << "," << newline;
    oss << indent << "\"accumulateData\":" << space << this->accumulateData; // No comma on the last property

    if (encloseInBrackets) {
        oss << newline << "}";
    }

    return oss.str();
}



/**
 * @brief Serializes the FULL AddOperator state with a 4-byte size prefix.
 * @return std::vector<std::byte> A length-prefixed byte vector representing the operator.
 * @details This method implements the full serialization logic for this concrete class.
 * It calls the base class `serializeToBytes` to get a buffer with the common data,
 * appends its own specific data to that buffer, and then prepends the final 4-byte total size.
 */
std::vector<std::byte> AddOperator::serializeToBytes() const {
    // 1. Get the serialized data from the base class by directly calling its implementation.
    // This buffer contains the Type, ID, and Connection data, but NO size prefix.
    std::vector<std::byte> dataBuffer = Operator::serializeToBytes();

    // 2. Append this derived class's specific data to the buffer.
    Serializer::write(dataBuffer, this->weight);
    Serializer::write(dataBuffer, this->threshold);
    Serializer::write(dataBuffer, this->accumulateData);

    // 3. Prepare the final buffer by prepending the calculated size of the entire data payload.
    size_t dataSize = dataBuffer.size();
    if (dataSize > std::numeric_limits<uint32_t>::max()) {
        throw std::overflow_error("Serialized operator data size exceeds uint32_t prefix limit.");
    }
    uint32_t totalDataSize = static_cast<uint32_t>(dataSize);
    std::vector<std::byte> finalBuffer;
    finalBuffer.reserve(4 + totalDataSize);

    // 4. Write the 4-byte size prefix, then insert the data payload.
    Serializer::write(finalBuffer, totalDataSize);
    finalBuffer.insert(finalBuffer.end(), dataBuffer.begin(), dataBuffer.end());

    return finalBuffer;
}