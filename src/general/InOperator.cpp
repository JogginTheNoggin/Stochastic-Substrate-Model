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

        // TODO bulk update, at end instead of having request for each new connection. But does that mean we would have to loop through each connection again?
        // necessary becase in future data source may not be in memory but in a database. Therefore must remain in sync. 
        this->requestUpdate(UpdateType::ADD_CONNECTION, {static_cast<int>(targetId), distance}); // TODO current cast uint32_t to int for compatibility qith UpdateEvent struct
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
    if (payloadData < 0) { // we don't handle negative
        return; 
    }

    accumulatedData.push_back(payloadData); 
}


void InOperator::message(const float payloadData) {
    if (payloadData < 0.0f) {
        return; // Ignore negative values
    }

    // Safe rounding: round first, then check limits
    float roundedFloat = std::round(payloadData);

    // Now check if it's within int range
    if (roundedFloat > static_cast<float>(std::numeric_limits<int>::max()) ||
        roundedFloat < static_cast<float>(std::numeric_limits<int>::min())) {
        return; // Overflow or underflow would occur
    }

    int rounded = static_cast<int>(roundedFloat);
    accumulatedData.push_back(rounded);
}


void InOperator::message(const double payloadData) {
    if (payloadData < 0.0) {
        return;
    }

    double roundedDouble = std::round(payloadData);

    if (roundedDouble > static_cast<double>(std::numeric_limits<int>::max()) ||
        roundedDouble < static_cast<double>(std::numeric_limits<int>::min())) {
        return;
    }

    int rounded = static_cast<int>(roundedDouble);
    accumulatedData.push_back(rounded);
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

