#pragma once

#include <vector>
#include <cstddef> // For std::byte
#include <cstdint> // For uint8_t, uint16_t, uint32_t, uint64_t, int types
#include <string>  // For exception messages potentially
#include <type_traits> // For helpers like is_unsigned_v etc.
#include <cstring>   // For memcpy

/**
 * @class Serializer
 * @brief Provides static utility methods for serializing and deserializing primary
 * data types according to the project's defined binary format (Big Endian).
 * @details Handles specific encoding rules like size-prefixing for 'int' types
 * and fixed-size representation for floats and specific uint types. Uses private
 * static helpers for internal logic.
 */
class Serializer {
private:
    Serializer() = delete; // Static class

    // --- Private Static Helpers ---

    /**
     * @brief Checks if enough bytes remain in the buffer segment.
     */
    static void checkBounds(const std::byte* current, const std::byte* end, size_t needed);

    /**
     * @brief Internal helper template to append bytes of unsigned integers BE.
     */
    template<typename UnsignedIntT>
    static void appendUnsignedBE_Internal(std::vector<std::byte>& buffer, UnsignedIntT value);

    /**
     * @brief Internal helper template to read bytes into unsigned integers BE.
     */
    template<typename UnsignedIntT>
    static UnsignedIntT readUnsignedBE_Internal(const std::byte*& current, const std::byte* end);

    /**
      * @brief Internal helper template to append bytes of floating point types BE.
      */
    template<typename FloatT>
    static void appendFloatBE_Internal(std::vector<std::byte>& buffer, FloatT value);

    /**
     * @brief Internal helper template to read bytes into floating point types BE.
     */
    template<typename FloatT>
    static FloatT readFloatBE_Internal(const std::byte*& current, const std::byte* end);


public:
    // --- Write Methods (Overloaded Public API) ---

    static void write(std::vector<std::byte>& buffer, uint8_t value);
    static void write(std::vector<std::byte>& buffer, uint16_t value);
    static void write(std::vector<std::byte>& buffer, uint32_t value);
    static void write(std::vector<std::byte>& buffer, uint64_t value);
    static void write(std::vector<std::byte>& buffer, int value);
    static void write(std::vector<std::byte>& buffer, float value);
    static void write(std::vector<std::byte>& buffer, double value);

    // --- Read Methods (Specific Public API - Reader must know type) ---

    static uint8_t read_uint8(const std::byte*& current, const std::byte* end);
    static uint16_t read_uint16(const std::byte*& current, const std::byte* end);
    static uint32_t read_uint32(const std::byte*& current, const std::byte* end);
    static uint64_t read_uint64(const std::byte*& current, const std::byte* end);
    static int read_int(const std::byte*& current, const std::byte* end);
    static float read_float(const std::byte*& current, const std::byte* end);
    static double read_double(const std::byte*& current, const std::byte* end);

    

}; // End class Serializer

// --- Template Implementations for Helpers ---
// Template definitions usually need to be in the header file or included
// directly/indirectly by files that use them, unless explicit instantiation is used.
// Let's define them here for simplicity as they are private implementation details
// called by public methods defined in Serializer.cpp (which includes this header).

template<typename UnsignedIntT>
void Serializer::appendUnsignedBE_Internal(std::vector<std::byte>& buffer, UnsignedIntT value) {
    static_assert(std::is_unsigned_v<UnsignedIntT>, "Template only for unsigned integers");
    constexpr size_t typeSize = sizeof(UnsignedIntT);
    for (size_t i = 0; i < typeSize; ++i) {
        size_t shift = 8 * (typeSize - 1 - i);
        buffer.push_back(static_cast<std::byte>((value >> shift) & 0xFF));
    }
}

template<typename UnsignedIntT>
UnsignedIntT Serializer::readUnsignedBE_Internal(const std::byte*& current, const std::byte* end) {
    static_assert(std::is_unsigned_v<UnsignedIntT>, "Template only for unsigned integers");
    constexpr size_t typeSize = sizeof(UnsignedIntT);
    Serializer::checkBounds(current, end, typeSize); // Call static private method
    UnsignedIntT value = 0;
    for (size_t i = 0; i < typeSize; ++i) {
        value = (value << 8) | static_cast<UnsignedIntT>(static_cast<uint8_t>(*current++));
    }
    return value;
}

//TODO definitely check if big endian based on system. 
template<typename FloatT>
void Serializer::appendFloatBE_Internal(std::vector<std::byte>& buffer, FloatT value) {
    static_assert(std::is_floating_point_v<FloatT>, "Template only for float/double");
    constexpr size_t typeSize = sizeof(FloatT);
    std::array<std::byte, typeSize> bytes;
    std::memcpy(bytes.data(), &value, typeSize);

    #if HAS_STD_ENDIAN // Assuming HAS_STD_ENDIAN defined based on <bit> inclusion elsewhere
        if constexpr (std::endian::native == std::endian::little) {
             buffer.insert(buffer.end(), bytes.rbegin(), bytes.rend());
        } else {
             buffer.insert(buffer.end(), bytes.begin(), bytes.end());
        }
    #elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        buffer.insert(buffer.end(), bytes.rbegin(), bytes.rend());
    #else
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    #endif
 }

template<typename FloatT>
FloatT Serializer::readFloatBE_Internal(const std::byte*& current, const std::byte* end) {
     static_assert(std::is_floating_point_v<FloatT>, "Template only for float/double");
     constexpr size_t typeSize = sizeof(FloatT);
     Serializer::checkBounds(current, end, typeSize); // Call static private method
     std::array<std::byte, typeSize> bytes;

    #if HAS_STD_ENDIAN
        if constexpr (std::endian::native == std::endian::little) {
             for (size_t i = 0; i < typeSize; ++i) { bytes[typeSize - 1 - i] = *current++; }
        } else {
             for (size_t i = 0; i < typeSize; ++i) { bytes[i] = *current++; }
        }
    #elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
         for (size_t i = 0; i < typeSize; ++i) { bytes[typeSize - 1 - i] = *current++; }
    #else
         for (size_t i = 0; i < typeSize; ++i) { bytes[i] = *current++; }
    #endif

    FloatT value;
    std::memcpy(&value, bytes.data(), typeSize);
    return value;
}
