#include "../headers/operators/Operator.h"
#include "../headers/Scheduler.h"
#include "../headers/UpdateScheduler.h"
#include "../headers/util/DynamicArray.h"
#include <algorithm> // Required for std::sort
#include <vector>    // Required for std::vector
#include <utility>   // Required for std::pair
#include <stdexcept> // Required for std::range_error, std::overflow_error
#include <limits>    // Required for std::numeric_limits
#include <unordered_set>
#include <cstddef>   // For std::byte
#include <sstream>   // For std::ostringstream
#include <iostream>

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
    if (payload == nullptr) {
        return; // Don't process inactive payloads
    }
    else if(!payload->active || payload->distanceTraveled < 0){
        payload->active = false; // dont process inactive or invalid distanced payloads
        return;
    }
    else if(payload->currentOperatorId != this->operatorId){ 
        return; // do not process if not owned. 
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

        
    }
    // Move payload forward
        
    // TODO an update request could be call just after this, making the payload useful and active but at different time, should this remain? 
    if (payload->distanceTraveled >= outputConnections.maxIdx()) { // no 
        // no more buckets to check
        payload->active = false;

    } else {
        payload->distanceTraveled++; // move forward
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
        // Bucket doesn't exist, create it.
        // CORRECTED: Allocate a new set on the heap.
        targetsPtr = new std::unordered_set<uint32_t>();
        outputConnections.set(distance, targetsPtr);
    } 

    // THE BUG: This 'insert' is called on the OLD value of targetsPtr if it wasn't null.
    // The pointer that was freshly created is not used here.
    // If targetsPtr was NOT null, it inserts into that. If it WAS null, it inserts into a garbage pointer.
    targetsPtr->insert(targetOperatorId);
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

    if (distance < 0 || distance > outputConnections.maxIdx()) return;

    std::unordered_set<uint32_t>* targetsPtr = outputConnections.get(distance); // mutable

    if (targetsPtr != nullptr) {
        targetsPtr->erase(targetOperatorId); // Remove element using set's erase
        // TODO update maxID ?
        // If the set is now empty, remove it from DynamicArray
        if (targetsPtr->empty()) {
            outputConnections.remove(distance); 
        } 
        else{
            outputConnections.set(distance, targetsPtr);
        }


    }

    // else don't have element
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
    if(oldDistance < 0 || newDistance < 0){ // negative distances are invalid
        return; 
    }
    const std::unordered_set<uint32_t>* oldTargetsPtr = outputConnections.get(oldDistance);

    // Check if the bucket at the old distance exists and if the target ID is in it.
    if (oldTargetsPtr != nullptr && oldTargetsPtr->count(targetOperatorId) > 0) {
        // The connection exists, so we can proceed with the move.
        
        // First, remove the connection from the old distance bucket
        removeConnectionInternal(targetOperatorId, oldDistance);
        
        // Then, add the connection to the new distance bucket
        addConnectionInternal(targetOperatorId, newDistance);
    }
    // If the connection does not exist at the old distance, do nothing.
}






const DynamicArray<std::unordered_set<uint32_t>>& Operator::getOutputConnections() const{
    return this->outputConnections; 
}


std::string Operator::typeToString(Operator::Type type) {
    switch (type) {
        case Operator::Type::ADD: return "ADD";
        case Operator::Type::SUB: return "SUB";
        case Operator::Type::MUL: return "MUL";
        case Operator::Type::LEFT: return "LEFT";
        case Operator::Type::RIGHT: return "RIGHT";
        case Operator::Type::OUT: return "OUT";
        case Operator::Type::IN: return "IN";
        case Operator::Type::UNDEFINED: return "UNDEFINED";
        default: return "UNKNOWN";
    }
}


/**
 * @brief Generates a JSON string representation of the base Operator's state.
 * @param prettyPrint If true, format the JSON with indentation and newlines.
 * @param encloseInBrackets If true, wraps the output in {}. Allows derived classes to embed this content.
 * @param indentLevel The current level of indentation for pretty-printing.
 * @return std::string A JSON formatted string of the base operator's properties.
 * @note Key Logic Steps: Constructs a JSON object string based on the provided indent level. Includes "operatorId" and "outputDistanceBuckets".
 */
std::string Operator::toJson(bool prettyPrint, bool encloseInBrackets, int indentLevel) const {
    std::ostringstream oss;
    
    std::string base_indent = prettyPrint ? std::string(indentLevel * 2, ' ') : "";
    std::string deeper_indent = prettyPrint ? std::string((indentLevel + 1) * 2, ' ') : "";
    std::string deepest_indent = prettyPrint ? std::string((indentLevel + 2) * 2, ' ') : "";
    std::string even_deeper_indent = prettyPrint ? std::string((indentLevel + 3) * 2, ' ') : "";
    std::string newline = prettyPrint ? "\n" : "";
    std::string space = prettyPrint ? " " : "";

    if (encloseInBrackets) {
        oss << base_indent << "{" << newline;
    }

    oss << deeper_indent << "\"opType\":" << space << "\"" << Operator::typeToString(this->getOpType()) << "\"," << newline;
    oss << deeper_indent << "\"operatorId\":" << space << this->operatorId << "," << newline;
    oss << deeper_indent << "\"outputDistanceBuckets\":" << space << "[";

    std::vector<std::pair<int, const std::unordered_set<uint32_t>*>> sortedBuckets;
    for (int d = 0; d <= this->outputConnections.maxIdx(); ++d) {
        const auto* targetsPtr = outputConnections.get(d);
        if (targetsPtr != nullptr && !targetsPtr->empty()) {
            sortedBuckets.push_back({d, targetsPtr});
        }
    }
    std::sort(sortedBuckets.begin(), sortedBuckets.end(), [](const auto& a, const auto& b){ return a.first < b.first; });

    if (!sortedBuckets.empty()) {
        oss << newline;
    }

    for (size_t i = 0; i < sortedBuckets.size(); ++i) {
        const auto& pair = sortedBuckets[i];
        int distance = pair.first;
        const auto& bucket = *pair.second;
        
        oss << deepest_indent << "{" << newline;
        oss << even_deeper_indent << "\"distance\":" << space << distance << "," << newline;
        oss << even_deeper_indent << "\"targetOperatorIds\":" << space << "[";

        if (prettyPrint && !bucket.empty()) {
            oss << newline;
            
            std::vector<uint32_t> sortedTargets(bucket.begin(), bucket.end());
            std::sort(sortedTargets.begin(), sortedTargets.end());

            for (size_t j = 0; j < sortedTargets.size(); ++j) {
                oss << even_deeper_indent << "  " << sortedTargets[j] << (j == sortedTargets.size() - 1 ? "" : ",") << newline;
            }
            oss << even_deeper_indent;

        } else { // Compact version of the nested array
            std::vector<uint32_t> sortedTargets(bucket.begin(), bucket.end());
            std::sort(sortedTargets.begin(), sortedTargets.end());
            for (size_t j = 0; j < sortedTargets.size(); ++j) {
                oss << sortedTargets[j] << (j == sortedTargets.size() - 1 ? "" : ",");
            }
        }

        oss << "]" << newline;
        oss << deepest_indent << "}" << (i == sortedBuckets.size() - 1 ? "" : ",") << newline;
    }

    if (!sortedBuckets.empty()) {
        oss << deeper_indent;
    }
    oss << "]";

    if (encloseInBrackets) {
        oss << newline << base_indent << "}";
    }

    return oss.str();
}


/**
 * @brief Serializes the base Operator fields into a byte vector WITHOUT a size prefix.
 * @return std::vector<std::byte> A byte vector containing the serialized base class data.
 * @details This virtual method is intended to be called by derived class overrides. It serializes
 * the Type, ID, and connection data, which is common to all Operator types. The connections are serialized 
 * in sorted order for deterministic output and repeatabilitiy.
 * @note !!!! DOES NOT PROVIDE BYTE SIZE OF SERIALIZATION !!! 
 * MEANING ANY SUBCLASSES MUST PROVIDED THIS INFORMATION INORDER TO WORK PROPERLY
 */
std::vector<std::byte> Operator::serializeToBytes() const {
    // Purpose: Serialize the base Operator fields into a byte vector without a size prefix.
    //          This is intended to be called by the derived class's serializeToBytes method.
    // Return: std::vector<std::byte> - A byte vector containing the serialized base class data.
    // Key Logic Steps:
    // 1. Serialize the operator's type and ID.
    // 2. Collect all valid, non-empty connection buckets.
    // 3. Sort the buckets by distance to ensure deterministic output.
    // 4. Write the count of valid buckets.
    // 5. Iterate through the sorted buckets, performing safety checks before writing the
    //    distance, connection count, and sorted target IDs for each.

    std::vector<std::byte> buffer;
    
    // Serialize Operator Type (2 bytes)
    Serializer::write(buffer, static_cast<uint16_t>(this->getOpType()));
    // Serialize Operator ID (4 bytes, direct write)
    Serializer::write(buffer, this->operatorId);

    // --- Serialize Connections Deterministically and Safely ---

    // 1. Collect all valid buckets into a temporary vector to be sorted.
    std::vector<std::pair<int, const std::unordered_set<uint32_t>*>> validBuckets;
    for (int d = 0; d <= this->outputConnections.maxIdx(); ++d) {
        const auto* targetsPtr = outputConnections.get(d);
        if (targetsPtr != nullptr && !targetsPtr->empty()) {
            validBuckets.push_back({d, targetsPtr});
        }
    }

    // 2. Sort the collected buckets by distance. This guarantees a deterministic byte stream.
    std::sort(validBuckets.begin(), validBuckets.end(), 
              [](const auto& a, const auto& b) {
                  return a.first < b.first;
              });

    
    if (validBuckets.size() > std::numeric_limits<uint16_t>::max()){
        throw std::overflow_error("Operator " + std::to_string(operatorId) + " has too many non-empty buckets for serialization format.");
    }
    // 3. Write the final count of buckets that will be serialized.
    Serializer::write(buffer, static_cast<uint16_t>(validBuckets.size()));

    // 4. Iterate through the sorted, valid buckets and serialize their data.
    for (const auto& pair : validBuckets) {
        int distance = pair.first;
        const auto& targetIds = *pair.second;

        // NEW: Add range check for distance before serializing. 
        if (distance < 0 || distance > std::numeric_limits<uint16_t>::max()) {
            throw std::range_error("Operator " + std::to_string(operatorId) + " has distance " + std::to_string(distance) + " out of range for uint16_t serialization format.");
        }
        Serializer::write(buffer, static_cast<uint16_t>(distance)); // Write Distance

        // NEW: Add overflow check for the number of connections in a single bucket. 
        if (targetIds.size() > std::numeric_limits<uint16_t>::max()) {
            throw std::overflow_error("Operator " + std::to_string(operatorId) + ", Distance " + std::to_string(distance) + " has too many connections for uint16_t serialization format.");
        }
        Serializer::write(buffer, static_cast<uint16_t>(targetIds.size())); // Write Num Connections

        // To ensure target IDs are also deterministic, sort them before writing.
        std::vector<uint32_t> sortedTargets(targetIds.begin(), targetIds.end());
        std::sort(sortedTargets.begin(), sortedTargets.end());

        for (uint32_t targetId : sortedTargets) {
            Serializer::write(buffer, targetId); // Write Target IDs
        }
    }
    
    return buffer;
}

// Private member implementation of the helper function
bool Operator::compareConnections(const Operator& other) const {
    // Purpose: Perform a deep comparison of the outputConnections members.
    // Parameters:
    // - @param other: The Operator whose connections to compare against.
    // Return: True if connection structures are identical.
    // Key Logic Steps:
    // 1. Compare the count of active buckets and the max index.
    // 2. Iterate through each possible distance.
    // 3. For each distance, get the connection sets from both operators.
    // 4. Handle all cases: both null, one null, or both valid (in which case the sets are compared).

    const auto& connA = this->outputConnections;
    const auto& connB = other.outputConnections;

    if (connA.maxIdx() != connB.maxIdx() || connA.count() != connB.count()) {
        return false;
    }

    if (connA.maxIdx() == -1) { // Both are empty
        return true;
    }

    for (int i = 0; i <= connA.maxIdx(); ++i) {
        const auto* setA = connA.get(i);
        const auto* setB = connB.get(i);

        if (setA == nullptr && setB == nullptr) {
            continue;
        }
        if (setA == nullptr || setB == nullptr) {
            return false;
        }
        if (*setA != *setB) {
            return false;
        }
    }
    return true;
}

// Public virtual `equals` implementation
bool Operator::equals(const Operator& other) const {
    // Purpose: Compare the state of the base Operator class.
    // Parameters:
    // - @param other: The Operator to compare against. Assumes the non-member operator== has
    //   already verified the types are identical.
    // Return: True if base members are equal.
    // Key Logic Steps:
    // 1. Compare the operatorId.
    // 2. Delegate the deep comparison of connections to the private helper method.

    if (this->operatorId != other.operatorId) {
        return false;
    }
    return this->compareConnections(other);
}

// Non-member operator== implementation (The "Manager")
bool operator==(const Operator& lhs, const Operator& rhs) {
    // Purpose: Perform the primary polymorphic equality check.
    // Return: True if types and values are equal.
    // Key Logic Steps:
    // 1. Perform the essential type check. This is the main responsibility of this function.
    // 2. If types match, delegate to the virtual `equals` member function for the detailed comparison.
    if (lhs.getOpType() != rhs.getOpType()) {
        return false;
    }
    return lhs.equals(rhs);
}

// Non-member operator!= implementation
bool operator!=(const Operator& lhs, const Operator& rhs) {
    return !(lhs == rhs);
}