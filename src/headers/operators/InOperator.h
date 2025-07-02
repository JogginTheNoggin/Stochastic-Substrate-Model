#pragma once

#include "Operator.h"
#include <vector>

// TODO better comments
class InOperator: public Operator{
private:
    std::vector<int> accumulatedData; // TODO does order matter? Will it be preserved 

public:
    static constexpr Operator::Type OP_TYPE = Operator::Type::IN;
    static constexpr int MAX_CONNECTIONS = 2;
    static constexpr int MAX_DISTANCE = 2; 


    InOperator(uint32_t id) ;

    // should just call super class constructor of similar type for deserialize 
    InOperator(const std::byte*& current, const std::byte* end); 

    ~InOperator() override = default;

    // --- Overridden Abstract Methods from Operator ---
    void randomInit(uint32_t maxOperatorId, Randomizer* rng) override;

    // note self-connection is allowed, self modulation
    void randomInit(IdRange* idRange, Randomizer* rng) override;

    void message(const int payloadData) override;
    void message(const float payloadData) override; // InOperator might ignore or cast these
    void message(const double payloadData) override; // InOperator might ignore or cast these


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
     * @param indentLevel The current level of indentation (number of indent steps) for pretty-printing.
     * @return std::string A string containing the Operator's state formatted as JSON.
     * @note Key Logic Steps: Constructs a JSON object string by calling the base class `toJson` method to get common properties, then appending its own specific properties like "weight", "threshold", and "accumulateData".
     */
    std::string toJson(bool prettyPrint = false, bool encloseInBrackets = true, int indentLevel = 0) const override;
    void changeParams(const std::vector<int>& params) override; // TODO may not want method to be of const type, and needs support for different data Types

    /**
     * @brief [Override] Compares this InOperator's state with another for equality.
     * @param other The Operator object to compare against.
     * @return bool True if the base Operator state is equal.
     * @details This class has no unique persistent state, so equality is
     * determined entirely by the base class `equals` method. 
     */
    bool equals(const Operator& other) const override;

    // Prevent copying/assignment
    InOperator(const InOperator&) = delete;
    InOperator& operator=(const InOperator&) = delete;
    InOperator(InOperator&&) = delete;
    InOperator& operator=(InOperator&&) = delete;
};