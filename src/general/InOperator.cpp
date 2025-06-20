#include "../headers/operators/InOperator.h"
#include "../headers/Scheduler.h"   // For Scheduler::get()->schedulePayloadForNextStep
#include "../headers/Payload.h"    // For creating Payload objects
#include "../headers/util/Serializer.h" // Will be needed for deserializeParameters
#include <stdexcept>                // For std::runtime_error
#include <iostream>                 // For potential debug/error logging
#include <cmath>

// TODO better comments
InOperator::InOperator(uint32_t id): Operator(id){
    
}

// just uses default super class constructor, for now
InOperator::InOperator(const std::byte*& current, const std::byte* end): Operator(current, end){
    // no custom deserialization for this operator
    // accumulated data is not preserved, as we can just send new data in
}

// Must have given virtual method of base class
void InOperator::randomInit(uint32_t maxOperatorId, Randomizer* rng){

}

void InOperator::randomInit(IdRange* idRange, Randomizer* rng){
    if (MAX_CONNECTIONS <= 0) {
        return; // Not enough potential targets or no connections allowed
    }

    int connectionsToAttempt = rng->getInt(0, MAX_CONNECTIONS);

    if (connectionsToAttempt == 0) {
        return;
    }

    for (int i = 0; i < connectionsToAttempt; ++i) {
        uint32_t targetId = rng->getInt(idRange->getMinId(), idRange->getMaxId()); // TODO range is uint32 while method is int

        // self-connection 
        

        int distance = rng->getInt(0, MAX_DISTANCE);
        // Ensure distance is non-negative (should be by uniform_int_distribution if MAX_DISTANCE >= 0)
        if (distance < 0) distance = 0;

        addConnectionInternal(targetId, distance); // add the connection, no update call
        
    }

}


void InOperator::processData() {

    // only send if actually output connections
    if (!outputConnections.empty()) {

        for(int value: accumulatedData){ // for each message value, create a payload to its output connections
            // Create the new payload, starting its journey.
            // The Payload constructor used here implies the new payload starts at distance 0 for its journey.
            Payload newPayload(value, this->operatorId); // Using this->operatorId as the source/manager

            // Schedule the new payload to start traveling in the *next* step
            try {
                if (Scheduler::get()) { // Ensure scheduler is available
                     Scheduler::get()->schedulePayloadForNextStep(newPayload);
                } else {
                    std::cerr << "InOperator " << getId() << ": Scheduler instance is null. Cannot schedule payload." << std::endl;
                }
            } catch (const std::runtime_error& e) {
                std::cerr << "Error scheduling payload from InOperator " << this->getId() << ": " << e.what() << std::endl;
            }


        }
    }

    accumulatedData.clear(); // clear the accumulated data for next message
}



/**
 * @brief This operator has no specific parameters (like weight or threshold) to change.
 * This method does nothing.
 */
void InOperator::changeParams(const std::vector<int>& params) {
    // Purpose: Fulfill the pure virtual function requirement from the base class.
    // Parameters: params - A vector of parameters (ignored).
    // Return: Void.
    // Key Logic: OutOperator has no configurable parameters. This method is a no-op.
}

void InOperator::message(const int payloadData){
    if (payloadData < 0) { // we don't do negative numbers
        accumulatedData.push_back(0); // submit as zero input, low
        // TODO consider is zero acceptable, what about audio, can't audio values be negative
        return; 
    }

    accumulatedData.push_back(payloadData); 
}


/**
 * @brief Handles incoming float data by rounding, clamping, and adding it to the accumulator.
 * @param payloadData The float data from the arriving payload.
 * @details This method standardizes float inputs to integers. It ignores non-numeric
 * values (NaN, Infinity). Out-of-range values are clamped to INT_MAX/INT_MIN,
 * and valid values are rounded to the nearest integer before being added to the internal vector.
 */
void InOperator::message(const float payloadData) {
    // Purpose: Handle float messages by safely rounding, clamping, and adding to the vector.
    // Parameters: @param payloadData The float data to process.
    // Return: Void.
    // Key Logic: Ignores NaN/Inf. Checks if the float value is outside the representable
    //            range of an int. If so, clamps the value to INT_MAX/MIN.
    //            Otherwise, rounds the float and casts it to an int. The result is added to the vector.

    if (std::isnan(payloadData) || std::isinf(payloadData)) {
        return;
    }

    int intPayloadData;
    // Perform rounding using double to preserve precision for comparison
    double rounded_payload_as_double = std::round(static_cast<double>(payloadData));

    if (rounded_payload_as_double >= static_cast<double>(std::numeric_limits<int>::max())) {
        intPayloadData = std::numeric_limits<int>::max();
    } else if (rounded_payload_as_double <= static_cast<double>(std::numeric_limits<int>::min())) {
        intPayloadData = std::numeric_limits<int>::min();
    } else {
        // It's safe to cast to int now as it's within range
        intPayloadData = static_cast<int>(rounded_payload_as_double);
    }

    this->accumulatedData.push_back(intPayloadData);
}


/**
 * @brief Handles incoming double data by rounding, clamping, and adding it to the accumulator.
 * @param payloadData The double data from the arriving payload.
 * @details This method standardizes double inputs to integers. It ignores non-numeric
 * values (NaN, Infinity). Out-of-range values are clamped to INT_MAX/INT_MIN,
 * and valid values are rounded to the nearest integer before being added to the internal vector.
 */
void InOperator::message(const double payloadData) {
    // Purpose: Handle double messages by safely rounding, clamping, and adding to the vector.
    // Parameters: @param payloadData The double data to process.
    // Return: Void.
    // Key Logic: Ignores NaN/Inf. Checks if the double value is outside the representable
    //            range of an int. If so, clamps the value to INT_MAX/MIN.
    //            Otherwise, rounds the double and casts it to an int. The result is added to the vector.

    if (std::isnan(payloadData) || std::isinf(payloadData)) {
        return;
    }

    int intPayloadData;
    // Rounding is already done using double type for payloadData
    double rounded_payload_double = std::round(payloadData);

    if (rounded_payload_double >= static_cast<double>(std::numeric_limits<int>::max())) {
        intPayloadData = std::numeric_limits<int>::max();
    } else if (rounded_payload_double <= static_cast<double>(std::numeric_limits<int>::min())) {
        intPayloadData = std::numeric_limits<int>::min();
    } else {
        // It's safe to cast to int now as it's within range
        intPayloadData = static_cast<int>(rounded_payload_double);
    }

    this->accumulatedData.push_back(intPayloadData);
}

// TODO consider adding implementation to super class and simply modifying the op_type in subclass
/**
 * @brief Gets the specific OperatorType of this AddOperator.
 * @return OperatorType Always returns AddOperator::S_OP_TYPE.
 */
Operator::Type InOperator::getOpType() const {
    // Purpose: Identify the type of this operator.
    // Parameters: None.
    // Return: The static OperatorType enum value for AddOperator.
    return InOperator::OP_TYPE;
}

bool InOperator::equals(const Operator& other) const {
    // Purpose: Compare this InOperator to another Operator for equality.
    // Return: True if they are functionally equivalent InOperators.
    // Key Logic Steps:
    // 1. Call the base class `equals` method.
    // 2. The `accumulatedData` vector is transient operational state and is not
    //    serialized. Therefore, it is not part of an equality check that
    //    verifies persistent state. The base class comparison is sufficient.

    return Operator::equals(other);
}

std::string InOperator::toJson(bool prettyPrint, bool encloseInBrackets) const{
    std::ostringstream oss;
    std::string indent = prettyPrint ? "  " : "";
    std::string newline = prettyPrint ? "\n" : "";
    std::string space = prettyPrint ? " " : "";

    if (encloseInBrackets) {
        oss << "{" << newline;
    }

    // Get base class JSON content (without brackets)
    oss << Operator::toJson(prettyPrint, false);
    
    // Add comma to separate base content from derived content
    oss << "," << newline;

    // Add derived class properties
    oss << indent << "\"accumulatedData\":" << space << "[";
    bool first = true;
    for(const auto& data : accumulatedData) {
        if (!first) {
            oss << "," << space;
        }
        oss << data;
        first = false;
    }
    oss << "]";

    if (encloseInBrackets) {
        oss << newline << "}";
    }

    return oss.str();
}

std::vector<std::byte> InOperator::serializeToBytes() const{
    // Get the serialized data from the base class by directly calling its implementation.
    // This buffer contains the Type, ID, and Connection data, but NO size prefix.
    std::vector<std::byte> dataBuffer = Operator::serializeToBytes();

    // This operator type does not append any of its own data to the binary stream.

    // Prepare the final buffer by prepending the calculated size of the entire data payload.
    size_t dataSize = dataBuffer.size();
    if (dataSize > std::numeric_limits<uint32_t>::max()) {
        throw std::overflow_error("Serialized operator data size exceeds uint32_t prefix limit.");
    }
    uint32_t totalDataSize = static_cast<uint32_t>(dataSize);
    std::vector<std::byte> finalBuffer;
    finalBuffer.reserve(4 + totalDataSize);

    // Write the 4-byte size prefix, then insert the data payload.
    Serializer::write(finalBuffer, totalDataSize);
    finalBuffer.insert(finalBuffer.end(), dataBuffer.begin(), dataBuffer.end());

    return finalBuffer;
}

