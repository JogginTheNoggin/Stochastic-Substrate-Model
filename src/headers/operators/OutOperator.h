#pragma once

#include "Operator.h"
#include <deque> 

// TODO better comments
class OutOperator: public Operator{

private: 
    std::deque<int> data; 

public:
    static constexpr Operator::Type OP_TYPE = Operator::Type::OUT;
    static constexpr size_t MAX_DATA_BUFFER_SIZE = 8192000;
    size_t output_batch_size = 512; // batch size for operator 
    // TODO batch size may not be relevant for say image channel

    OutOperator(uint32_t id) ;

    // should just call super class constructor of similar type for deserialize 
    OutOperator(const std::byte*& current, const std::byte* end); 


    // --- Overridden Abstract Methods from Operator ---
    // TODO comment for method
    void randomInit(uint32_t maxOperatorId, Randomizer* rng) override;

    // note self-connection is allowed, self modulation
    void randomInit(IdRange* idRange, Randomizer* rng) override;

    void message(const int payloadData) override;
    void message(const float payloadData) override; // AddOperator might ignore or cast these
    void message(const double payloadData) override; // AddOperator might ignore or cast these

    /**
     * @brief used to check if there is output to read
     * 
     */
    bool hasOutput();  // possible to make this base class virtual method, which would just checks accumulated data if it exist.  

    
    int getOutputCount();

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
     * @brief [Override] Compares this OutOperator's state with another for equality.
     * @param other The Operator object to compare against.
     * @return bool True if the base state is equal and the `data` vectors are identical.
     * @details Invokes the base `Operator::equals` method first, then compares the
     * content of the internal data buffer. 
     */
    bool equals(const Operator& other) const override;

    // --- OutOperator-Specific Methods ---
    /**
     * @brief Converts the stored integer data into an ASCII string and clears the internal buffer.
     * @return std::string A string where each character is derived from an integer in the data buffer.
     * @details For each integer `v` in the internal `data` vector, this method scales it from the
     * range of a positive integer `[0, INT_MAX]` down to the ASCII range `[0, 255]`. It uses an
     * efficient bit-shift operation `(v >> (INT_BITS - 8))` to approximate `(v * 255) / INT_MAX`.
     * After generating the string, the internal data buffer is cleared.
     */
    std::string getDataAsString();

    void clearData();

    void setBatchSize(int size);

};