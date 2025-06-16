
#include "../headers/operators/OutOperator.h"
#include <cmath>

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
    if (payloadData < 0) { // we don't handle negative
        return; 
    }

    data.push_back(payloadData); 
}


void OutOperator::message(const float payloadData) {
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
    data.push_back(rounded);
}

void OutOperator::message(const double payloadData) {
    if (payloadData < 0.0) {
        return;
    }

    double roundedDouble = std::round(payloadData);

    if (roundedDouble > static_cast<double>(std::numeric_limits<int>::max()) ||
        roundedDouble < static_cast<double>(std::numeric_limits<int>::min())) {
        return;
    }

    int rounded = static_cast<int>(roundedDouble);
    data.push_back(rounded);
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

std::string OutOperator::toJson(bool prettyPrint, bool encloseInBrackets) const{
    std::ostringstream oss;
    std::string indent = prettyPrint ? "  " : "";
    std::string newline = prettyPrint ? "\n" : "";
    std::string space = prettyPrint ? " " : "";

    if (encloseInBrackets) {
        oss << "{" << newline;
    }

    // Get base class JSON content
    oss << Operator::toJson(prettyPrint, false);
    
    // Add comma to separate base content from derived content
    oss << "," << newline;

    // Add derived class properties
    oss << indent << "\"data\":" << space << "[";
    bool first = true;
    for(const auto& val : data) {
        if (!first) {
            oss << "," << space;
        }
        oss << val;
        first = false;
    }
    oss << "]";

    if (encloseInBrackets) {
        oss << newline << "}";
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


