#include "../headers/util/Serializer.h"
#include <stdexcept>
#include <limits>
#include <iterator>
#include <array>     // For std::array
#include <vector>    // For std::vector
#include <cstddef>   // For std::byte
#include <type_traits> // For make_unsigned_t

// Include system headers for endian detection if needed by macros
#if defined(__linux__) || defined(__APPLE__)
#include <endian.h>
#elif defined(_WIN32) || defined(_WIN64)
// Windows is typically Little Endian, __BYTE_ORDER__ might be defined by compiler (GCC/Clang)
// If using MSVC, may need different checks or assume LE.
#endif

#if __has_include(<bit>)
#include <bit> // For std::endian
#define HAS_STD_ENDIAN 1
#else
#define HAS_STD_ENDIAN 0
#endif

// --- Private Static Helper Method Implementation ---
void Serializer::checkBounds(const std::byte* current, const std::byte* end, size_t needed) {
    if (current == nullptr || end == nullptr || current > end) {
         throw std::runtime_error("[Serializer::checkBounds] Invalid buffer pointers.");
    }
    if (static_cast<size_t>(std::distance(current, end)) < needed) {
        throw std::runtime_error("[Serializer::checkBounds] Insufficient data. Needed "
                                 + std::to_string(needed) + ", have "
                                 + std::to_string(std::distance(current, end)) + ".");
    }
}

// --- Public Write Method Implementations ---

void Serializer::write(std::vector<std::byte>& buffer, uint8_t value) {
    buffer.push_back(static_cast<std::byte>(value));
}

void Serializer::write(std::vector<std::byte>& buffer, uint16_t value) {
    Serializer::appendUnsignedBE_Internal(buffer, value); // Call static private helper
}

void Serializer::write(std::vector<std::byte>& buffer, uint32_t value) {
    Serializer::appendUnsignedBE_Internal(buffer, value); // Call static private helper
}

void Serializer::write(std::vector<std::byte>& buffer, uint64_t value) {
    Serializer::appendUnsignedBE_Internal(buffer, value); // Call static private helper
}

void Serializer::write(std::vector<std::byte>& buffer, float value) {
     Serializer::appendFloatBE_Internal(buffer, value); // Call static private helper
}

void Serializer::write(std::vector<std::byte>& buffer, double value) {
     Serializer::appendFloatBE_Internal(buffer, value); // Call static private helper
}

void Serializer::write(std::vector<std::byte>& buffer, int value) {
    constexpr size_t intSize = sizeof(int);
    if (intSize > std::numeric_limits<uint8_t>::max()) {
         throw std::length_error("[Serializer::write(int)] sizeof(int) > 255.");
    }
    // Write 1-byte size prefix using the uint8_t overload
    Serializer::write(buffer, static_cast<uint8_t>(intSize));

    // Write value bytes using the helper (cast to unsigned for safety)
    Serializer::appendUnsignedBE_Internal(buffer, static_cast<std::make_unsigned_t<int>>(value));
}


// --- Public Read Method Implementations ---

uint8_t Serializer::read_uint8(const std::byte*& current, const std::byte* end) {
    checkBounds(current, end, 1);
    return static_cast<uint8_t>(*current++);
}

uint16_t Serializer::read_uint16(const std::byte*& current, const std::byte* end) {
    // Call static private helper
    return Serializer::readUnsignedBE_Internal<uint16_t>(current, end);
}

uint32_t Serializer::read_uint32(const std::byte*& current, const std::byte* end) {
    // Call static private helper
    return Serializer::readUnsignedBE_Internal<uint32_t>(current, end);
}

uint64_t Serializer::read_uint64(const std::byte*& current, const std::byte* end) {
    // Call static private helper
    return Serializer::readUnsignedBE_Internal<uint64_t>(current, end);
}

int Serializer::read_int(const std::byte*& current, const std::byte* end) {
    // Read 1-byte size prefix using the uint8_t reader
    uint8_t sizeN = Serializer::read_uint8(current, end);

    // Validate size against the system's sizeof(int)
    constexpr size_t expectedSize = sizeof(int);
    if (sizeN != expectedSize) {
        throw std::length_error("[Serializer::read_int] Size mismatch. File says "
                                + std::to_string(sizeN) + ", but expected "
                                + std::to_string(expectedSize) + ".");
    }

    // Read N bytes using the unsigned helper
    auto unsignedValue = Serializer::readUnsignedBE_Internal<std::make_unsigned_t<int>>(current, end);

    // Cast/reinterpret back to signed int
    return static_cast<int>(unsignedValue); // Assumes 2's complement
}

float Serializer::read_float(const std::byte*& current, const std::byte* end) {
     // Call static private helper
     return Serializer::readFloatBE_Internal<float>(current, end);
}

double Serializer::read_double(const std::byte*& current, const std::byte* end) {
     // Call static private helper
     return Serializer::readFloatBE_Internal<double>(current, end);
}

