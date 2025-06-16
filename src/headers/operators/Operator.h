#pragma once

#include "../../headers/Payload.h"
#include "../../headers/UpdateEvent.h"
#include "../../headers/util/DynamicArray.h"
#include "../../headers/util/Randomizer.h"
#include "../../headers/util/IdRange.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <cstddef>   // For std::byte
#include <sstream>   // For std::ostringstream
#include <optional>

// Forward declaration
class Scheduler;
class UpdateScheduler;
class Serializer; // For use in derived classes, and potentially base for connections

/**
 * @class Operator
 * @brief Abstract base class for all processing units in the simulation.
 * @details Defines the common interface and universally shared properties for operators,
 * such as ID and output connections. Specific processing logic, parameters, and message
 * handling are implemented in derived classes. This class is not instantiable.
 *
 * System Role/Rationale: Provides a polymorphic base for different neuron behaviors,
 * allowing TimeController and MetaController to manage them uniformly via pointers
 * to this base type. Core of the "Possibility not Probability" philosophy by enabling
 * diverse functional units.
 *
 * Breakdown/State:
 * @property operatorId - Unique identifier for the operator (read from stream by derived/base constructor).
 * @property outputConnections - Defines timed, potentially branching connections to other Operators
 * (maps distance to a set of target Operator IDs).
 *
 * General Function/Behavior:
 * - Can be connected to other operators.
 * - Can have messages delivered to it (handled by derived classes).
 * - Can process data (logic defined by derived classes).
 * - Can have its connections modified.
 * - Can originate payloads that traverse its output connections.
 * - Can request structural or parameter updates to itself or the network.
 */
class Operator {
protected:
    uint32_t operatorId;             // Unique ID for this Operator. Read by constructor from stream.
    DynamicArray<std::unordered_set<uint32_t>> outputConnections; // Stores outgoing connections.

    // For saving UPDATE state and storing parameters for updates. 
    // TODO internal updates within system has been deferred  
    std::optional<UpdateType> updateType;
    std::vector<int> paramsForUpdate; // Only for integers for now
    int numParamsForUpdate = 0;


    /**
     * @brief [Protected Deserialization Constructor] Base constructor for deserialization.
     * @param current Reference to a pointer to the current position in the byte stream.
     * MetaController reads the "Total Operator Data Size" and "Operator Type".
     * This constructor is then called, and `current` points to the beginning of the Operator ID field.
     * This constructor will read the operatorId and then common parts like connections.
     * @param end Pointer defining the boundary of the operator's data block for this operator.
     * @note This is called by the derived class's deserialization constructor.
     */
    Operator(const std::byte*& current, const std::byte* end);

private:

    virtual void validate(); 



public:
    /**
     * @brief Constructor for creating an Operator programmatically (not through deserialization).
     * @param id Unique ID for this Operator, typically provided by MetaController.
     * @details Initializes ID. Connections are initially empty.
     */
    Operator(uint32_t id);

    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~Operator() = default;

    // --- Core Abstract Methods (must be implemented by derived classes) ---
    /**
     * @enum OperatorType
     * @brief Defines the specific type of an Operator, used for instantiation and identification.
     * @details Each value corresponds to a concrete Operator subclass.
     */
    enum class Type : uint16_t {
        ADD = 0,      // Represents an AddOperator
        SUB = 1,      // Future: Represents a SubOperator
        MUL = 2,      // Future: Represents a MulOperator
        LEFT = 3,
        RIGHT = 4,
        OUT = 5,
        IN = 6,
        // ... other types can be added here
        UNDEFINED = 0xFFFF // Default for uninitialized or error states
    };


    
    /**
     * @brief Randomly generates outgoing connections for this operator.
     * @param maxOperatorId The maximum operator ID in the network (exclusive),
     * so connections can be made to IDs from 0 to maxOperatorId-1.
     * @details Derived classes should implement logic to randomly select target
     * operator IDs and distances, then use requestUpdate() to queue ADD_CONNECTION events.
     */
    virtual void randomInit(uint32_t maxOperatorId, Randomizer* rng) = 0;


    virtual void randomInit(IdRange* connectionRange, Randomizer* rng) = 0; 

    /**
     * @brief [Pure Virtual] Receives incoming integer message data.
     * @param payloadData The integer data from the arriving payload.
     * @details Derived classes implement how this data is stored or immediately processed.
     * Called by TimeController::deliverAndFlagOperator.
     */
    virtual void message(const int payloadData) = 0;

    /**
     * @brief [Pure Virtual] Receives incoming float message data.
     * @param payloadData The float data from the arriving payload.
     * @details Derived classes implement how this data is stored or immediately processed.
     * Called by TimeController::deliverAndFlagOperator (if TimeController is updated).
     */
    virtual void message(const float payloadData) = 0;

    /**
     * @brief [Pure Virtual] Receives incoming double message data.
     * @param payloadData The double data from the arriving payload.
     * @details Derived classes implement how this data is stored or immediately processed.
     * Called by TimeController::deliverAndFlagOperator (if TimeController is updated).
     */
    virtual void message(const double payloadData) = 0;


    /**
     * @brief [Pure Virtual] Processes accumulated/received data and potentially fires/creates new payloads.
     * @details This method contains the unique logic of the specific operator type.
     * Called by TimeController during its "Check Operators" phase.
     */
    virtual void processData() = 0;

    //TODO need to perform checks on processData and message to not get overflow or underflow. 
    
    /**
     * Use to make changes to the operator, when update event occurs. 
     * Operator specific 
     */
    virtual void changeParams(const std::vector<int>& params) = 0; 

    /**
     * @brief [Pure Virtual] Gets the specific OperatorType of this concrete operator instance.
     * @return OperatorType The enum value representing the derived class's type.
     */
    virtual Type getOpType() const = 0;


    // TODO provide base class implementation, this class is at top, sub class fields can go beneath, should not close bracket??
    /**
     * @brief Generates a JSON string representation of the Operator's state.
     * @param prettyPrint If true, format the JSON with indentation and newlines for readability.
     * @param encloseInBrackets If true (default), wraps the entire output in curly braces `{}` making it a complete JSON object. If false, omits the braces, allowing derived classes to embed this content.
     * @return std::string A string containing the Operator's state formatted as JSON.
     * @note When called with `encloseInBrackets=false`, the returned string will NOT have a trailing comma. The derived class is responsible for adding it if needed.
     */
    virtual std::string toJson(bool prettyPrint = false, bool encloseInBrackets = true) const;
    
    
    
    // TODO provide base class implementation (should not calculate byte count of total operator), subclass comes after and will preform size calculation
    /**
     * @brief Serializes the Operator's state into a length-prefixed byte vector .
     * @param None (Implicit this pointer)
     * @return std::vector<std::byte> A vector containing the length-prefixed serialized binary data.
     * The first 4 bytes are the size (uint32_t BE) of the following data.
     * @details Serializes the Operator data into a
     * temporary buffer. Calculates the size of this buffer. Creates a final buffer, writes
     * the calculated size (4 bytes BE), then appends the temporary buffer content.
     * Uses Big-Endian byte order throughout.
     * @note This method is const as it only reads the operator's state.
     * DOES NOT PROVIDE BYTE SIZE OF SERIALIZATION
     */
    virtual std::vector<std::byte> serializeToBytes() const ; 


    /** @brief Gets the unique ID of this Operator. @return int The Operator's unique ID. */
    int getId() const {
        return operatorId;
    }

    /** @brief Gets a read-only reference to the output connections map. @return const DynamicArray<...>& */
    virtual const DynamicArray<std::unordered_set<uint32_t>>& getOutputConnections() const;

    // --- Common Concrete Methods See "Operator.cpp"---

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
    virtual void traverse(Payload* payload) ;


    /**
     * @brief [Private] Initiates an update request for this Operator itself.
     * @param type The type of update being requested (e.g., REMOVE_CONNECTION).
     * @param params Vector of integer parameters for the update event, specific to the type.
     * @details Creates an UpdateEvent struct targeting this Operator's ID (`this->operatorId`)
     * with the specified type and parameters. It then submits this event to the central
     * UpdateController via the UpdateScheduler static interface. Used for internal
     * self-modification requests or triggered cleanup actions (like removing dangling connections).
     */
    virtual void requestUpdate(UpdateType type, const std::vector<int>& params);


    /**
     * @brief [Internal Update] Adds a connection ID to a specific distance . Creates vector if needed.
     * @param targetOperatorId The ID of the target Operator to connect to.
     * @param distance The distance (travel time in steps) for this connection.
     * @details Called by UpdateController. Finds or creates the new vector for connections for the given `distance`.
     * Adds the `targetOperatorId` to the vector's list if not already present.
     */
    virtual void addConnectionInternal(uint32_t targetOperatorId, int distance) ;


    /**
     * @brief [Internal Update] Removes a connection ID from a specific distance bucket. Removes bucket if empty.
     * @param targetOperatorId The ID of the target Operator connection to remove.
     * @param distance The distance of the bucket from which to remove the connection.
     * @details Called by UpdateController. Finds the specified distance bucket. If found,
     * removes the `targetOperatorId` from its list. If the bucket becomes empty after
     * removal, the entire bucket is removed from the `outputDistanceBuckets` map.
     */
    virtual void removeConnectionInternal(uint32_t targetOperatorId, int distance) ;

    /**
     * @brief [Internal Update] Moves a connection ID from one distance bucket to another.
     * @param targetOperatorId The ID of the target Operator connection to move.
     * @param oldDistance The current distance of the connection.
     * @param newDistance The new distance to move the connection to.
     * @details Called by UpdateController. Effectively performs a remove from the old distance
     * bucket and an add to the new distance bucket. Uses the existing internal methods.
     */
    virtual void moveConnectionInternal(uint32_t targetOperatorId, int oldDistance, int newDistance) ;

    

    /**
     * @brief Converts an OperatorType enum to its string representation.
     * @param type The OperatorType enum value.
     * @return std::string The string name of the operator type.
     */
    static std::string typeToString(Operator::Type type);
    


    // Prevent copying/assignment
    Operator(const Operator&) = delete;
    Operator& operator=(const Operator&) = delete;
    Operator(Operator&&) = delete;
    Operator& operator=(Operator&&) = delete;
};