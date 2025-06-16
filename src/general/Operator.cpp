#include "../headers/operators/Operator.h"
#include "../headers/Scheduler.h"
#include "../headers/UpdateScheduler.h"
#include "../headers/util/DynamicArray.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <cstddef>   // For std::byte
#include <sstream>   // For std::ostringstream

//TODO  Incorrect 
Operator::Operator(const std::byte*& current, const std::byte* end) {
    // Purpose: Construct the base part of an Operator from a byte stream.
    // Parameters: current - Reference to a pointer to the current position in the byte stream.
    //                     Expected to point at "Operator ID Size" field. Will be advanced.
    //             end - Pointer defining the boundary of the operator's data block.
    // Return: N/A.
    // Key Logic: Reads operatorId and outputConnections from the stream.
    //            This constructor is called by derived class deserialization constructors.

    // 1. Read Operator ID (Size + Value)
    // Corresponds to Fields 3 & 4 of the revised serialization format
    this->operatorId = Serializer::read_uint32(current, end);

    // 2. Read Number of Connection Buckets
    // Corresponds to Field 5 of the revised serialization format
    uint16_t numBuckets = Serializer::read_uint16(current, end);

    // 3. Read Each Bucket's Data
    // Corresponds to Field 6 of the revised serialization format
    for (uint16_t i = 0; i < numBuckets; ++i) {
        // 6.a. Read Distance
        uint16_t distance_u16 = Serializer::read_uint16(current, end);
        int distance = static_cast<int>(distance_u16);

        // 6.b. Read Number of Connections in this Bucket
        uint16_t numConnectionsInBucket = Serializer::read_uint16(current, end);

        if (numConnectionsInBucket > 0) {
            std::unordered_set<uint32_t>* targetIdsPtr = new std::unordered_set<uint32_t>();
            try {
                targetIdsPtr->reserve(numConnectionsInBucket);
                // 6.c. Read Each Target Connection's Data
                for (uint16_t j = 0; j < numConnectionsInBucket; ++j) {
                    // 6.c.i & 6.c.ii Read Target Operator ID (Size + Value)
                    uint32_t targetId = Serializer::read_uint32(current, end);
                    targetIdsPtr->insert(targetId);
                }
                this->outputConnections.set(distance, targetIdsPtr);
            } catch (...) {
                delete targetIdsPtr; // Clean up allocated set on exception
                throw; // Re-throw
            }
        } else {
            // If numConnectionsInBucket is 0, we might still want to represent an empty bucket
            // or ensure the distance exists in the DynamicArray structure if it implies that.
            // For now, if it's 0, we don't create a set for it, outputConnections.get(distance) will be nullptr.
            // If the DynamicArray requires explicit null setting for non-existent buckets,
            // that would happen by default or by `outputConnections.set(distance, nullptr);`
        }
    }

    validate(); // ensure deserialized operator is valid
}


Operator::Operator(uint32_t id) : operatorId(id) {
    // Purpose: Construct an Operator with a given ID when created programmatically.
    // Parameters: id - The unique identifier for this operator.
    // Return: N/A.
    // Key Logic: Initializes operatorId. outputConnections is default-initialized (empty).
}

void Operator::validate(){

    if(operatorId < 0){
        throw std::runtime_error("Operator ID cannot be negative.");
    }

    // TODO maybe add more for validation 

}


/**
 * @brief Manages the traversal progression of an outgoing payload originating from this Operator.
 * @param payload The payload currently traversing (passed by reference to modify its state).
 * @details Called by TimeController during the "Process Traveling Payloads" phase (Phase 2 of Step N+1).
 * Increments the payload's `timeTraveled`. Checks if the time matches the distance
 * of the `currentBucketIndex` stored in the payload. If reached:
 * 1. Calls `Scheduler::get()->scheduleMessage()` for all target Operator IDs in that bucket.
 * 2. Finds the next distance bucket for the payload to travel towards.
 * 3. Updates the payload's `currentBucketIndex` or marks it inactive if it was the last bucket.
 * 4. Includes logic hint for requesting cleanup of dangling IDs via `requestUpdate`.
 * @note Does NOT require MetaController reference. Relies on Scheduler for message dispatch.
 */
void Operator::traverse(Payload* payload) {
    if (payload == nullptr || !payload->active || payload->distanceTraveled < 0) {
        return; // Don't process inactive payloads
    }

    // Increment distance traveled for this step

    const std::unordered_set<uint32_t>* targetIdsPtr = outputConnections.get(payload->distanceTraveled);

    // Check if the destination distance for the current bucket is reached
    if (targetIdsPtr != nullptr && !targetIdsPtr->empty()) {
        // Current distance has connection/s, Schedule messages to all targets.
        for (uint32_t targetId : *targetIdsPtr) {
            try {
                    Scheduler::get()->scheduleMessage(targetId, payload->message);
                    // Note: Dangling ID detection happens implicitly if TimeController::deliverAndFlagOperator
                    // fails to find the target Operator*. We rely on requestUpdate for cleanup.
            } catch (const std::runtime_error& e) {
                // Handle error if Scheduler instance is not available (e.g., log)
                // std::cerr << "Error scheduling message: " << e.what() << std::endl;
            }
                // TODO: Implement mechanism for traverse to know if delivery failed
                // for a specific targetId (e.g., return value from scheduleMessage or
                // a callback system) to reliably trigger requestUpdate below.
                // For now, we assume delivery might fail silently at TimeController.
                // The requestUpdate for cleanup needs a reliable trigger.
                // If(delivery_failed_for_targetId){
                //     requestUpdate(UpdateType::REMOVE_CONNECTION, {targetId, currentBucket.distance});
                // }
        }

        // Move payload forward
        
        // TODO an update request could be call just after this, making the payload useful and active but at different time, should this remain? 
        if (payload->distanceTraveled == outputConnections.maxIdx()) { // no 
            // no more buckets to check
            payload->active = false;

        } else {
            payload->distanceTraveled++; // move forward
        }
    }
    // Else: Distance not yet reached, payload continues traveling towards currentBucketIndex.
    // TODO check that payload after processData, is able to continue, may need to be added to timeController timeStep again, for nextTimeStep
}


/**
 * @brief [Private] Initiates an update request for this Operator itself.
 * @param type The type of update being requested (e.g., REMOVE_CONNECTION).
 * @param params Vector of integer parameters for the update event, specific to the type.
 * @details Creates an UpdateEvent struct targeting this Operator's ID (`this->operatorId`)
 * with the specified type and parameters. It then submits this event to the central
 * UpdateController via the UpdateScheduler static interface. Used for internal
 * self-modification requests or triggered cleanup actions (like removing dangling connections).
 */
void Operator::requestUpdate(UpdateType type, const std::vector<int>& params){
    UpdateEvent event(type, this->operatorId, params);
    
    try {
        UpdateScheduler::get()->Submit(event);
    } catch (const std::runtime_error& e) {
        // Handle error if UpdateScheduler instance is not available (e.g., log)
        // std::cerr << "Error submitting update event: " << e.what() << std::endl;
    }
}


/**
 * @brief [Internal Update] Adds a connection ID to a specific distance . Creates vector if needed.
 * @param targetOperatorId The ID of the target Operator to connect to.
 * @param distance The distance (travel time in steps) for this connection.
 * @details Called by UpdateController. Finds or creates the new vector for connections for the given `distance`.
 * Adds the `targetOperatorId` to the vector's list if not already present.
 */
void Operator::addConnectionInternal(uint32_t targetOperatorId, int distance) {
    // Find the vector for the given distance. If it doesn't exist, operator[] creates it.
    if (distance < 0) return; // Or throw? Invalid distance index.

    std::unordered_set<uint32_t>* targetsPtr = outputConnections.get(distance);
    // TODO bucket distance is not need, only payload needs to know distance, the array index represents the distance enough

    if (targetsPtr == nullptr) {
        // Bucket doesn't exist, create it
        std::unordered_set<uint32_t>* newTargets;
        newTargets->insert(targetOperatorId);
        outputConnections.set(distance, newTargets); // Set new set
    } else {
        // Bucket exists, insert target (set handles duplicates)
        targetsPtr->insert(targetOperatorId);
            // If 'get' returned a copy (unlikely for pointer), need to 'set' modified set back
            // outputConnections.set(distance, *targetsPtr); // If needed
    }
}


/**
 * @brief [Internal Update] Removes a connection ID from a specific distance bucket. Removes bucket if empty.
 * @param targetOperatorId The ID of the target Operator connection to remove.
 * @param distance The distance of the bucket from which to remove the connection.
 * @details Called by UpdateController. Finds the specified distance bucket. If found,
 * removes the `targetOperatorId` from its list. If the bucket becomes empty after
 * removal, the entire bucket is removed from the `outputDistanceBuckets` map.
 */
void Operator::removeConnectionInternal(uint32_t targetOperatorId, int distance) {

    if (distance < 0 || distance <= outputConnections.maxIdx()) return;

    std::unordered_set<uint32_t>* targetsPtr = outputConnections.get(distance);

    if (targetsPtr != nullptr) {
        targetsPtr->erase(targetOperatorId); // Remove element using set's erase

        // If the set is now empty, potentially remove it from DynamicArray
        // if (targetsPtr->empty()) {
        //     outputConnections.remove(distance); // Assuming remove exists
        // } else {
        //    // If 'get' returned a copy, need to 'set' the modified set back
        //    // outputConnections.set(distance, *targetsPtr);
        // }
    }
}

/**
 * @brief [Internal Update] Moves a connection ID from one distance bucket to another.
 * @param targetOperatorId The ID of the target Operator connection to move.
 * @param oldDistance The current distance of the connection.
 * @param newDistance The new distance to move the connection to.
 * @details Called by UpdateController. Effectively performs a remove from the old distance
 * bucket and an add to the new distance bucket. Uses the existing internal methods.
 */
void Operator::moveConnectionInternal(uint32_t targetOperatorId, int oldDistance, int newDistance) {
    // TODO check efficiency
    // First, remove the connection from the old distance bucket
    removeConnectionInternal(targetOperatorId, oldDistance);
    // Then, add the connection to the new distance bucket
    addConnectionInternal(targetOperatorId, newDistance);
}






const DynamicArray<std::unordered_set<uint32_t>>& Operator::getOutputConnections() const{
    return this->outputConnections; 
}


std::string Operator::typeToString(Operator::Type type) {
    switch (type) {
        case Operator::Type::ADD: return "ADD";
        case Operator::Type::SUB: return "SUB";
        case Operator::Type::MUL: return "MUL";
        case Operator::Type::LEFT: return "ADD";
        case Operator::Type::RIGHT: return "SUB";
        case Operator::Type::OUT: return "MUL";
        case Operator::Type::IN: return "MUL";
        case Operator::Type::UNDEFINED: return "UNDEFINED";
        default: return "UNKNOWN";
    }
}


/**
 * @brief Generates a JSON string representation of the base Operator's state.
 * @param prettyPrint If true, format the JSON with indentation and newlines.
 * @param encloseInBrackets If true, wraps the output in {}. Allows derived classes to embed this content.
 * @return std::string A JSON formatted string of the base operator's properties.
 * @note Key Logic Steps: Constructs a JSON object string. Includes "operatorId" and "outputDistanceBuckets". The `encloseInBrackets` flag allows for composition by derived classes.
 */
std::string Operator::toJson(bool prettyPrint, bool encloseInBrackets) const {
    std::ostringstream oss;
    std::string base_indent = prettyPrint && encloseInBrackets ? "  " : "";
    std::string deeper_indent = prettyPrint ? base_indent + "  " : "";
    std::string deepest_indent = prettyPrint ? deeper_indent + "  " : "";
    std::string newline = prettyPrint ? "\n" : "";
    std::string space = prettyPrint ? " " : "";

    if (encloseInBrackets) {
        oss << "{" << newline;
    }

    // Add base properties
    oss << base_indent << "\"opType\":" << space << "\"" << Operator::typeToString(this->getOpType()) << "\"," << newline;
    oss << base_indent << "\"operatorId\":" << space << this->operatorId << "," << newline;
    oss << base_indent << "\"outputDistanceBuckets\":" << space << "[" << newline;

    bool firstBucket = true;
    if (outputConnections.maxIdx() >= 0) {
        for (int i = 0; i <= this->outputConnections.maxIdx(); ++i) {
            const std::unordered_set<uint32_t>* bucket = outputConnections.get(i);
            if (bucket != nullptr && !bucket->empty()) {
                if (!firstBucket) {
                    oss << "," << newline;
                }
                firstBucket = false;

                oss << deeper_indent << "{" << newline;
                oss << deepest_indent << "\"distance\":" << space << i << "," << newline;
                oss << deepest_indent << "\"targetOperatorIds\":" << space << "[";

                bool firstId = true;
                for (uint32_t targetId : *bucket) {
                    if (!firstId) {
                        oss << "," << space;
                    }
                    firstId = false;
                    oss << targetId;
                }
                oss << "]" << newline;
                oss << deeper_indent << "}";
            }
        }
    }

    oss << newline << base_indent << "]";

    if (encloseInBrackets) {
        oss << newline << "}";
    }

    return oss.str();
}



/**
 * @brief Serializes the base Operator fields into a byte vector WITHOUT a size prefix.
 * @return std::vector<std::byte> A byte vector containing the serialized base class data.
 * @details This virtual method is intended to be called by derived class overrides. It serializes
 * the Type, ID, and connection data, which is common to all Operator types.
 * @note DOES NOT PROVIDE BYTE SIZE OF SERIALIZATION
 */
std::vector<std::byte> Operator::serializeToBytes() const {
    std::vector<std::byte> buffer;

    // Serialize Operator Type (2 bytes)
    Serializer::write(buffer, static_cast<uint16_t>(this->getOpType()));

    // Serialize Operator ID (n bytes, size-prefixed)
    Serializer::write(buffer, this->operatorId);

    // --- Serialize Connections Efficiently ---
    // 1. Get the exact count of active buckets directly from the DynamicArray.
    const uint16_t bucketCount = static_cast<uint16_t>(this->outputConnections.count());

    if (bucketCount > std::numeric_limits<uint16_t>::max()){
        throw std::overflow_error("Operator " + std::to_string(operatorId) + " has too many non-empty buckets for serialization format.");
    }

    Serializer::write(buffer, bucketCount);

    // 2. If there are buckets, perform a single loop to find and serialize them.
    if (bucketCount > 0) {
        for (int d = 0; d <= this->outputConnections.maxIdx(); ++d) {
            const std::unordered_set<uint32_t>* targetsPtr = outputConnections.get(d);
            if (targetsPtr != nullptr && !targetsPtr->empty()) {
                const std::unordered_set<uint32_t>& targetIds = *targetsPtr;

                if (d < 0 || d > std::numeric_limits<uint16_t>::max()) {
                     throw std::range_error("Operator " + std::to_string(operatorId) + " has distance " + std::to_string(d) + " out of range for serialization format.");
                }
                Serializer::write(buffer, static_cast<uint16_t>(d)); // Distance

                if (targetIds.size() > std::numeric_limits<uint16_t>::max()) {
                     throw std::overflow_error("Operator " + std::to_string(operatorId) + ", Distance " + std::to_string(d) + " has too many connections for serialization format.");
                }
                Serializer::write(buffer, static_cast<uint16_t>(targetIds.size())); // Num Connections

                for (int targetId : targetIds) {
                    Serializer::write(buffer, targetId); // Target IDs
                }
            }
        }
    }

    return buffer;
}