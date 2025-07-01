
#include "../headers/operators/OutOperator.h"
#include <cmath>
#include <iostream>

// TODO better comments

OutOperator::OutOperator(uint32_t id): Operator(id){
    
}

// just uses default super class constructor, for now
OutOperator::OutOperator(const std::byte*& current, const std::byte* end): Operator(current, end){
    // Purpose: Construct an OutOperator from a byte stream.
    // Details: The base constructor first reads common data (ID, Connections). This
    // constructor then reads the data specific to the OutOperator.
    
    // Read the count of data elements
    uint16_t dataCount = Serializer::read_uint16(current, end);

    // Reserve space and read each element
    this->data.reserve(dataCount);
    for (uint16_t i = 0; i < dataCount; ++i) {
        this->data.push_back(Serializer::read_int(current, end));
    }

}


/**
 * @brief This operator does not process data to fire new payloads; it only accumulates.
 * This method does nothing.
 */
void OutOperator::processData() {
    // Purpose: Fulfill the pure virtual function requirement from the base class.
    // Parameters: None.
    // Return: Void.
    // Key Logic: This operator is a terminal node; its function is to collect data,
    // not to process it and fire new payloads. Therefore, this method is intentionally empty.
}


/**
 * @brief This operator has no specific parameters (like weight or threshold) to change.
 * This method does nothing.
 */
void OutOperator::changeParams(const std::vector<int>& params) {
    // Purpose: Fulfill the pure virtual function requirement from the base class.
    // Parameters: params - A vector of parameters (ignored).
    // Return: Void.
    // Key Logic: OutOperator has no configurable parameters. This method is a no-op.
}

void OutOperator::message(const int payloadData){
    std::cout << " Output recieved. " << std::endl; // TODO temporary for testing
    data.push_back(payloadData); 
}


/**
 * @brief Handles incoming float data by rounding, clamping, and storing it.
 * @param payloadData The float data from the arriving payload.
 * @details This method standardizes float inputs to integers. It ignores non-numeric
 * values (NaN, Infinity). Out-of-range values are clamped to INT_MAX/INT_MIN,
 * and valid values are rounded to the nearest integer before being stored in the internal data vector.
 */
void OutOperator::message(const float payloadData) {
    // Purpose: Handle float messages by safely rounding, clamping, and adding to the vector.
    // Parameters: @param payloadData The float data to process.
    // Return: Void.
    // Key Logic: Ignores NaN/Inf. Checks if the float value is outside the representable
    //            range of an int. If so, clamps the value to INT_MAX/MIN.
    //            Otherwise, rounds the float and casts it to an int. The result is added to the data vector.

    if (std::isnan(payloadData) || std::isinf(payloadData)) {
        return;
    }

    int intPayloadData;

    if (payloadData >= static_cast<float>(std::numeric_limits<int>::max())) {
        intPayloadData = std::numeric_limits<int>::max();
    } else if (payloadData <= static_cast<float>(std::numeric_limits<int>::min())) {
        intPayloadData = std::numeric_limits<int>::min();
    } else {
        intPayloadData = static_cast<int>(std::round(payloadData));
    }

    data.push_back(intPayloadData);
}


/**
 * @brief Handles incoming double data by rounding, clamping, and storing it.
 * @param payloadData The double data from the arriving payload.
 * @details This method standardizes double inputs to integers. It ignores non-numeric
 * values (NaN, Infinity). Out-of-range values are clamped to INT_MAX/INT_MIN,
 * and valid values are rounded to the nearest integer before being stored in the internal data vector.
 */
void OutOperator::message(const double payloadData) {
    // Purpose: Handle double messages by safely rounding, clamping, and adding to the vector.
    // Parameters: @param payloadData The double data to process.
    // Return: Void.
    // Key Logic: Ignores NaN/Inf. Checks if the double value is outside the representable
    //            range of an int. If so, clamps the value to INT_MAX/MIN.
    //            Otherwise, rounds the double and casts it to an int. The result is added to the data vector.

    if (std::isnan(payloadData) || std::isinf(payloadData)) {
        return;
    }

    int intPayloadData;

    if (payloadData >= static_cast<double>(std::numeric_limits<int>::max())) {
        intPayloadData = std::numeric_limits<int>::max();
    } else if (payloadData <= static_cast<double>(std::numeric_limits<int>::min())) {
        intPayloadData = std::numeric_limits<int>::min();
    } else {
        intPayloadData = static_cast<int>(std::round(payloadData));
    }

    data.push_back(intPayloadData);
}

bool OutOperator::hasOutput(){
    return data.size() > 0; 
}

/**
 * @brief Converts the stored integer data into an ASCII string and clears the internal buffer.
 * @return std::string A string where each character is derived from an integer in the data buffer.
 */
std::string OutOperator::getDataAsString() {
    // Purpose: Convert buffered integers to an ASCII string using scaling and clear the buffer.
    // Parameters: None.
    // Return: A string representing the scaled intensity of the buffered data.
    // Key Logic: Pre-allocate a string. Iterate data vector, scale each int to a char
    //            using bit-shifting, append to string, then clear the data vector.

    if (data.empty()) {
        return "";
    }

    // Pre-allocate memory for the output string to avoid reallocations. This is more efficient.
    std::string out;
    out.reserve(data.size());

    // The number of value bits in a positive integer (e.g., 31 for a 32-bit int).
    constexpr int INT_VALUE_BITS = std::numeric_limits<int>::digits;
    // We are scaling down to the 8 bits of a standard char.
    constexpr int CHAR_BITS = 8;
    // The amount to shift right to perform the scaling.
    constexpr int SHIFT_AMOUNT = INT_VALUE_BITS - CHAR_BITS;

    for (int value : data) {
        // Defensively handle negative values, though message() should prevent them.
        int non_negative_value = (value < 0) ? 0 : value;

        char ch;
        // Use if constexpr to resolve the shift logic at compile time.
        if constexpr (SHIFT_AMOUNT > 0) {
            // Scale the int from [0, INT_MAX] to [0, 255] by taking the most significant 8 value-bits.
            // Casting to unsigned ensures a logical right shift, which is best practice here.
            ch = static_cast<char>(static_cast<unsigned int>(non_negative_value) >> SHIFT_AMOUNT);
        } else {
            // This case handles systems where int is 8 bits or less. Simply truncate.
            ch = static_cast<char>(non_negative_value & 0xFF); // truncate bottom 8 bits
        }
        out.push_back(ch);
    }

    data.clear(); // Important: Reset the buffer after reading its contents.
    return out;
}


// TODO consider adding implementation to super class and simply modifying the op_type in subclass
/**
 * @brief Gets the specific OperatorType of this AddOperator.
 * @return OperatorType Always returns AddOperator::S_OP_TYPE.
 */
Operator::Type OutOperator::getOpType() const {
    // Purpose: Identify the type of this operator.
    // Parameters: None.
    // Return: The static OperatorType enum value for AddOperator.
    return OutOperator::OP_TYPE;
}

void OutOperator::randomInit(uint32_t maxId, Randomizer* rng) {
    // CORRECTED: This implementation is required.
    // OutOperator does not create outgoing connections, so this method is intentionally empty.
    // It fulfills the abstract base class's requirement.
}


void OutOperator::randomInit(IdRange* validConnectionRange, Randomizer* rng) {
    // CORRECTED: This implementation is required.
    // OutOperator does not create outgoing connections, so this method is intentionally empty.
    // It fulfills the abstract base class's requirement.
}

bool OutOperator::equals(const Operator& other) const {
    // Purpose: Compare this OutOperator to another Operator for equality.
    // Return: True if they are functionally equivalent OutOperators.
    // Key Logic Steps:
    // 1. Call the base class `equals` method. If it fails, return false.
    // 2. Cast `other` to a `const OutOperator&`.
    // 3. Compare the internal `data` vectors, which are persisted during serialization.

    if (!Operator::equals(other)) {
        return false;
    }

    const auto& otherOutOp = static_cast<const OutOperator&>(other);

    return this->data == otherOutOp.data;
}

// In general/OutOperator.cpp

/**
 * @brief Generates a JSON string representation of the OutOperator's state.
 * @param prettyPrint If true, format the JSON for human readability.
 * @param encloseInBrackets If true, wrap the entire output in curly braces.
 * @param indentLevel The current level of indentation for pretty-printing.
 * @return A string containing the Operator's state formatted as JSON.
 */
std::string OutOperator::toJson(bool prettyPrint, bool encloseInBrackets, int indentLevel) const { // 
    std::ostringstream oss;

    std::string current_indent = prettyPrint ? std::string(indentLevel * 2, ' ') : "";
    std::string inner_indent = prettyPrint ? std::string((indentLevel + 1) * 2, ' ') : "";
    std::string array_element_indent = prettyPrint ? std::string((indentLevel + 2) * 2, ' ') : "";
    std::string newline = prettyPrint ? "\n" : "";
    std::string space = prettyPrint ? " " : "";

    if (encloseInBrackets) {
        oss << current_indent << "{" << newline;
    }

    // Get the base class JSON content as a fragment, passing the current indentLevel.
    std::string baseJson = Operator::toJson(prettyPrint, false, indentLevel);
    oss << baseJson;

    // Add a comma to separate base content from derived content
    oss << "," << newline;

    // Add OutOperator-specific properties
    oss << inner_indent << "\"data\":" << space << "[";
    
    if (!data.empty()) {
        if (prettyPrint) {
            oss << newline;
            for (size_t i = 0; i < data.size(); ++i) {
                oss << array_element_indent << data[i] << (i == data.size() - 1 ? "" : ",") << newline;
            }
            oss << inner_indent;
        } else { // Compact version
            for (size_t i = 0; i < data.size(); ++i) {
                oss << data[i] << (i == data.size() - 1 ? "" : ",");
            }
        }
    }

    oss << "]"; // No comma on the last property

    if (encloseInBrackets) {
        oss << newline << current_indent << "}";
    }

    return oss.str();
}


std::vector<std::byte> OutOperator::serializeToBytes() const{
    // 1. Get the base data from the base class implementation.
    std::vector<std::byte> dataBuffer = Operator::serializeToBytes();

    // 2. Append this derived class's specific data to the buffer.
    // First, write the count of elements in the data vector.
    if (data.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::overflow_error("OutOperator data vector size exceeds uint16_t limit.");
    }
    Serializer::write(dataBuffer, static_cast<uint16_t>(data.size()));

    // Then, write each element.
    for (int val : data) {
        Serializer::write(dataBuffer, val);
    }

    // 3. Prepare the final buffer by prepending the calculated size.
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


