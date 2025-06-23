#pragma once

#include "LayerType.h"
#include "../util/IdRange.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>   // For uint32_t, uint8_t
#include <cstddef>   // For std::byte
#include <limits>

// Forward declarations
class Operator;
class Payload; 
class MetaController;
class Serializer;     // Assumed to be available
class Randomizer; 
class Layer {
protected:
    LayerType type;
    bool isRangeFinal; // Flag for growing
    IdRange* reservedRange;
    // setting the defaults to both the max and min is so that any newly added operator
    // within accepted range, will replace both values, without having to perform addition checks for default cases
    uint32_t currentMinId = std::numeric_limits<uint32_t>::max(); 
    uint32_t currentMaxId = 0; 
    std::unordered_map<uint32_t, Operator*> operators;

    // not accessible for construction, as is to be treated as abstract
    explicit Layer(LayerType layerType, bool isLayerRangeFinal = true): type(layerType), isRangeFinal(isLayerRangeFinal) {} // Constructor for manual creation


    explicit Layer(LayerType layerType, IdRange* operatorReservedRange, bool isLayerRangeFinal = true ): type(layerType), 
        reservedRange(operatorReservedRange), isRangeFinal(isLayerRangeFinal) {} 

    virtual void randomInit(IdRange* validConnectionRange, Randomizer* randomizer) = 0; 


    /**
     * @brief Common logic to deserialize a stream of operator data blocks.
     * Validates operator IDs against reservedRange. Updates actualMinId, actualMaxId, nextOperatorId.
     */
    void deserialize(const std::byte*& data, const std::byte* dataEnd);

    

    void clearOperators();

    /**
     * @brief returns the next ID that can be used to assign a new operator.getAllOperators
     * @note All newly added operators will be added using the addOperator method. Therefore, even if nextId
     */
    uint32_t generateNextId();

    
    

private:
    

    // Helper to update min/max IDs when an operator is added
    bool isValidId(uint32_t operatorId);
    void updateMinMaxIds(uint32_t operatorId);


    /**
     * @brief [Private Helper] Deep compares the operator maps of this layer and another.
     * @param other The layer whose operator map should be compared against this one's.
     * @return bool True if the operator maps are identical in content.
     */
    bool compareOperatorMaps(const Layer& other) const;
    

public:
    
    /**
     * @brief [Public for Setup/Testing] Adds a new, fully constructed Operator to the layer.
     * @param op A pointer to the heap-allocated Operator to be added. The Layer takes ownership.
     * @details This method validates the operator's ID against the layer's range and checks for
     * duplicates before adding it to the internal map. It is made public to allow for the
     * programmatic construction of networks in test fixtures.
     * @throws std::runtime_error if the operator's ID is invalid for this layer or is a duplicate.
     */
    virtual void addNewOperator(Operator* op); // For manual population

    /**
     * @brief Deserialization constructor for Layer.
     * Consumes data from the provided byte pointers to reconstruct the layer,
     * including instantiating all its Operator objects.
     * @param current Reference to a pointer to the current position in the byte stream.
     * This constructor will advance 'current' past all bytes consumed for this layer.
     * @param end Pointer defining the boundary of the available data for this layer's block.
     * @throws std::runtime_error on parsing errors, unknown operator types, or insufficient data.
     */
    //Layer(LayerType layerType, bool isFinal, const std::byte*& data, const std::byte* dataEnd);

    
    
    
    ~Layer();

    // Rule of 5/3, 
    // TODO what is this
    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;
    Layer(Layer&& other) noexcept;
    Layer& operator=(Layer&& other) noexcept;

    const IdRange* getReservedIdRange() const { return reservedRange; }

    Operator* getOperator(uint32_t operatorId) const;
    const std::unordered_map<uint32_t, Operator*>& getAllOperators() const;
    

    LayerType getLayerType() const { return type; }
    uint32_t getMinOpID() const { return currentMinId; }
    uint32_t getMaxOpID() const { return currentMaxId; }
    bool isIdAvailable(uint32_t id);
    bool isFull() const; 
    bool isEmpty() const; 
    bool getIsRangeFinal() const;

    uint32_t getOpCount(); 

    // operator processing
    /**
     * @return, true if the operator exist and message has been sent, false otherwise
     */
    bool messageOperator(uint32_t operatorId, int message);

    void processOperatorData(uint32_t operatorId);

    void traverseOperatorPayload(Payload* Payload);

    // --- Update Handling ---
    // These methods are called by MetaController to delegate UpdateEvent processing.

    /**
     * @brief Creates a new operator within this layer.
     * @details This method is typically only effective on a dynamic layer. It uses the layer's
     * internal ID generation to assign a new ID and instantiates the operator.
     * @param params Parameters from the UpdateEvent, specifying operator type, etc.
     */
    virtual void createOperator(const std::vector<int>& params);

    /**
     * @brief Deletes an operator from this layer.
     * @details This action may be disallowed by certain layer subclasses (e.g., Input/Output).
     * @param targetOperatorId The ID of the operator to delete.
     */
    virtual void deleteOperator(uint32_t targetOperatorId);

    /**
     * @brief Changes a parameter on a specific operator within this layer.
     * @param targetOperatorId The ID of the operator to modify.
     * @param params Parameters from the UpdateEvent for the change.
     */
    virtual void changeOperatorParam(uint32_t targetOperatorId, const std::vector<int>& params);

    /**
     * @brief Adds a connection to an operator within this layer.
     * @param sourceOperatorId The ID of the operator to add the connection to.
     * @param params Parameters from the UpdateEvent specifying target and distance.
     */
    virtual void addOperatorConnection(uint32_t sourceOperatorId, const std::vector<int>& params);

    /**
     * @brief Removes a connection from an operator within this layer.
     * @param sourceOperatorId The ID of the operator to remove the connection from.
     * @param params Parameters from the UpdateEvent specifying target and distance.
     */
    virtual void removeOperatorConnection(uint32_t sourceOperatorId, const std::vector<int>& params);

    /**
     * @brief Moves a connection for an operator within this layer.
     * @param sourceOperatorId The ID of the operator whose connection is to be moved.
     * @param params Parameters from the UpdateEvent specifying target, old, and new distances.
     */
    virtual void moveOperatorConnection(uint32_t sourceOperatorId, const std::vector<int>& params);


    /**
     * @brief Serializes the entire layer (header and all its operators) into a byte vector.
     * The returned vector represents one complete "Layer Data Block".
     * @return std::vector<std::byte> The serialized layer data.
     * @throws std::overflow_error if data sizes exceed format limits.
     */
    std::vector<std::byte> serializeToBytes() const;


    /**
 	 * @brief Generates a JSON string representation of the Layer's state.
 	 * @param prettyPrint If true, format the JSON with indentation and newlines for readability. If false (default), output compact inline JSON.
 	 * @return std::string A string containing the Layers's state formatted as JSON.
 	 * @note Key Logic Steps: Constructs a JSON object string. Includes key-value pairs."
     */
    std::string toJson(bool prettyPrint = false) const;


    /**
     * @brief [Virtual] Compares this Layer's state members with another for equality.
     * @param other The Layer object to compare against.
     * @return bool True if the state members of this object are equal to the other.
     * @details This virtual function is the "specialist worker." It should be called ONLY
     * by the non-member `operator==` after it has verified the two objects are the
     * same concrete type. This base version compares all common Layer properties.
     */
    virtual bool equals(const Layer& other) const;
};

// --- NEW: Non-Member Equality Operator ---

/**
 * @brief [Non-Member] The primary, public-facing entry point for polymorphically comparing two Layers.
 * @param lhs The left-hand side Layer.
 * @param rhs The right-hand side Layer.
 * @return bool True if the layers are of the same concrete type and their states are equal.
 * @details This function acts as the "manager" or "gatekeeper." It first performs the essential
 * type check using `getLayerType()`. If the types match, it delegates the detailed member-wise
 * comparison to the virtual `equals()` method.
 */
bool operator==(const Layer& lhs, const Layer& rhs);

/**
 * @brief [Non-Member] The primary, public-facing entry point for polymorphic inequality comparison.
 * @return bool The logical negation of `operator==`.
 */
bool operator!=(const Layer& lhs, const Layer& rhs);