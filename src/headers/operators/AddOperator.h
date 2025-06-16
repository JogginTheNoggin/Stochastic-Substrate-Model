// ===== src/headers/AddOperator.h =====
#pragma once

#include "Operator.h" // Base class
#include <vector>
#include <string>
#include <cstddef>   // For std::byte
#include <sstream>   // For std::ostringstream

// Forward declaration if needed
class Serializer;

/**
 * @class AddOperator
 * @brief A concrete Operator that accumulates incoming integer messages and, if a threshold
 * is met, adds a weight to the accumulated sum to produce an output message.
 *
 * System Role/Rationale: Implements a basic excitatory neuron model where integer inputs sum up,
 * and firing strength is modulated by a weight if a threshold is passed.
 *
 * Breakdown/State:
 * @property operatorId (inherited) - Unique identifier.
 * @property outputConnections (inherited) - Defines outgoing connections.
 * @property weight - The integer value added to accumulatedData if threshold is met during firing.
 * @property threshold - The integer value accumulatedData must exceed for the operator to fire.
 * @property accumulateData - Stores the sum of integer message data received in the current time step.
 *
 * General Function/Behavior:
 * - Receives integer messages and sums them into `accumulateData`.
 * - Ignores float/double messages (or could be made to cast/error).
 * - When `processData` is called:
 * - Checks if `accumulateData` > `threshold`.
 * - If so, calculates output as `accumulateData + weight`.
 * - Creates new Payload(s) with this integer output message for connected operators.
 * - Resets `accumulateData` to 0.
 */
class AddOperator : public Operator {
private:
    static constexpr int MAX_CONNECTIONS = 5;
    static constexpr int MAX_DISTANCE = 10; 
    int weight;
    int threshold;
    int accumulateData; // Specific to AddOperator for integer accumulation
    bool pending = false; // TODO this may have to do with serialization and state safe, look into what was its intended purpose
    /**
     * @brief [Private Helper] Internal threshold check and weight application for ADD logic.
     * @param currentAccumulatedData The total data accumulated for this step.
     * @param outData [Output] The resulting data for the new payload if threshold is met.
     * @return bool True if threshold was met, false otherwise.
     */
    bool applyThresholdAndWeight(int currentAccumulatedData, int& outData);


    

public:
    /**
     * @brief Static constant representing the OperatorType for AddOperator.
     */
    static constexpr Operator::Type OP_TYPE = Operator::Type::ADD;

    /**
     * @brief Constructor for creating an AddOperator programmatically.
     * @param id Unique ID for this Operator.
     * @param initialWeight Initial weight parameter. Default = 1.
     * @param initialThreshold Initial threshold parameter. Default = 0.
     */
    AddOperator(int id, int initialWeight = 1, int initialThreshold = 0);

    /**
     * @brief Deserialization Constructor for AddOperator.
     * @param current Reference to a pointer to the current position in the byte stream.
     * MetaController reads "Total Operator Data Size" and "Operator Type".
     * This constructor is then called. `current` points to the Operator ID field.
     * This constructor will call the base Operator constructor to read ID and common parts,
     * then it will call its own `deserializeParameters` to read AddOperator-specific parts.
     * @param end Pointer defining the boundary of the operator's data block for this operator.
     */
    AddOperator(const std::byte*& current, const std::byte* end);


    AddOperator(uint32_t id); 

    // TODO add randomize constructor, which contains max operatorID parameter in order to select within valid values for output Connections
    // TODO maybe even have a mutant option. 
    


    ~AddOperator() override = default;

    

    // --- Overridden Abstract Methods from Operator ---
    // TODO comment for method
    void randomInit(uint32_t maxOperatorId, Randomizer* rng) override;

    // note self-connection is allowed, self modulation
    void randomInit(IdRange* idRange, Randomizer* rng) override;

    void message(const int payloadData) override;
    void message(const float payloadData) override; // AddOperator might ignore or cast these
    void message(const double payloadData) override; // AddOperator might ignore or cast these


    /**
     * @brief Processes accumulated data from the PREVIOUS step and potentially fires.
     * @details Called by TimeController during the "Check Operator" phase (Phase 1 of Step N+2).
     * It checks the total `accumulateData` (received via `message()` calls in Step N+1)
     * against the `threshold`. If the threshold is met, it applies the `weight` to calculate
     * output data, creates a new Payload, and schedules it via `Scheduler::get()->schedulePayloadForNextStep()`
     * to begin its journey in Step N+3. Finally, it resets `accumulateData` to zero,
     * preparing it for accumulation during Step N+2.
     */
    void processData() override; 


    Operator::Type getOpType() const override; 
    std::vector<std::byte> serializeToBytes() const override;

    /**
     * @brief Generates a JSON string representation of the Operator's state.
     * @param prettyPrint If true, format the JSON with indentation and newlines for readability.
     * @param encloseInBrackets If true (default), wraps the entire output in curly braces `{}` making it a complete JSON object. If false, omits the braces, allowing other classes to embed this content.
     * @return std::string A string containing the Operator's state formatted as JSON.
     * @note Key Logic Steps: Constructs a JSON object string by calling the base class `toJson` method to get common properties, then appending its own specific properties like "weight", "threshold", and "accumulateData".
     */
    std::string toJson(bool prettyPrint = false, bool encloseInBrackets = true) const override;


    void changeParams(const std::vector<int>& params) override; // TODO may not want method to be of const type 



    // --- AddOperator-Specific Methods ---
    int getWeight() const;
    int getThreshold() const;
    void setWeightInternal(int newWeight);
    void setThresholdInternal(int newThreshold);

    /**
     * @brief [Override] Compares this AddOperator's state with another for equality.
     * @param other The Operator object to compare against.
     * @return bool True if the base Operator state is equal AND AddOperator-specific
     * members (weight, threshold) are also equal.
     * @details First, it invokes the base class `Operator::equals`. If that succeeds,
     * it safely casts `other` to an `AddOperator` and compares its own persistent fields.
     */
    bool equals(const Operator& other) const override;


    // Prevent copying/assignment
    AddOperator(const AddOperator&) = delete;
    AddOperator& operator=(const AddOperator&) = delete;
    AddOperator(AddOperator&&) = delete;
    AddOperator& operator=(AddOperator&&) = delete;
};