#include "gtest/gtest.h"
#include "util/Serializer.h"
#include <vector>
#include <cstddef> // For std::byte
#include <limits>   // For std::numeric_limits
#include <stdexcept> // For std::runtime_error, std::length_error

// Basic test fixture for Serializer tests
class SerializerTest : public ::testing::Test {
protected:
    std::vector<std::byte> buffer;
    // Optional: Add SetUp() and TearDown() if common logic is identified later
};

// Test cases for uint8_t
TEST_F(SerializerTest, Uint8Nominal) {
    const uint8_t originalValue = 42;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint8_t readValue = Serializer::read_uint8(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr); // Ensure all bytes were consumed
}

TEST_F(SerializerTest, Uint8Zero) {
    const uint8_t originalValue = 0;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint8_t readValue = Serializer::read_uint8(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

// Test cases for uint16_t
TEST_F(SerializerTest, Uint16Nominal) {
    const uint16_t originalValue = 12345;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint16_t readValue = Serializer::read_uint16(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, Uint16Zero) {
    const uint16_t originalValue = 0;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint16_t readValue = Serializer::read_uint16(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, Uint16Max) {
    const uint16_t originalValue = std::numeric_limits<uint16_t>::max();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint16_t readValue = Serializer::read_uint16(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

// Test cases for uint32_t
TEST_F(SerializerTest, Uint32Nominal) {
    const uint32_t originalValue = 1234567890;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint32_t readValue = Serializer::read_uint32(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, Uint32Zero) {
    const uint32_t originalValue = 0;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint32_t readValue = Serializer::read_uint32(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, Uint32Max) {
    const uint32_t originalValue = std::numeric_limits<uint32_t>::max();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint32_t readValue = Serializer::read_uint32(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

// Test cases for uint64_t
TEST_F(SerializerTest, Uint64Nominal) {
    const uint64_t originalValue = 1234567890123456789ULL;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint64_t readValue = Serializer::read_uint64(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, Uint64Zero) {
    const uint64_t originalValue = 0ULL;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint64_t readValue = Serializer::read_uint64(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, Uint64Max) {
    const uint64_t originalValue = std::numeric_limits<uint64_t>::max();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    uint64_t readValue = Serializer::read_uint64(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

// Test cases for int
TEST_F(SerializerTest, IntNominalPositive) {
    const int originalValue = 12345;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    int readValue = Serializer::read_int(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, IntNominalNegative) {
    const int originalValue = -67890;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    int readValue = Serializer::read_int(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, IntZero) {
    const int originalValue = 0;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    int readValue = Serializer::read_int(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, IntMax) {
    const int originalValue = std::numeric_limits<int>::max();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    int readValue = Serializer::read_int(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, IntMin) {
    const int originalValue = std::numeric_limits<int>::min();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    int readValue = Serializer::read_int(dataPtr, endPtr);

    EXPECT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

// Test cases for float
TEST_F(SerializerTest, FloatNominal) {
    const float originalValue = 123.456f;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    float readValue = Serializer::read_float(dataPtr, endPtr);

    EXPECT_FLOAT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, FloatZero) {
    const float originalValue = 0.0f;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    float readValue = Serializer::read_float(dataPtr, endPtr);

    EXPECT_FLOAT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, FloatMax) {
    const float originalValue = std::numeric_limits<float>::max();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    float readValue = Serializer::read_float(dataPtr, endPtr);

    EXPECT_FLOAT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, FloatMin) { // Smallest positive normalized
    const float originalValue = std::numeric_limits<float>::min();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    float readValue = Serializer::read_float(dataPtr, endPtr);

    EXPECT_FLOAT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, FloatLowest) { // Most negative
    const float originalValue = std::numeric_limits<float>::lowest();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    float readValue = Serializer::read_float(dataPtr, endPtr);

    EXPECT_FLOAT_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

// Test cases for double
TEST_F(SerializerTest, DoubleNominal) {
    const double originalValue = 123456.789012;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    double readValue = Serializer::read_double(dataPtr, endPtr);

    EXPECT_DOUBLE_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, DoubleZero) {
    const double originalValue = 0.0;
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    double readValue = Serializer::read_double(dataPtr, endPtr);

    EXPECT_DOUBLE_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, DoubleMax) {
    const double originalValue = std::numeric_limits<double>::max();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    double readValue = Serializer::read_double(dataPtr, endPtr);

    EXPECT_DOUBLE_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, DoubleMin) { // Smallest positive normalized
    const double originalValue = std::numeric_limits<double>::min();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    double readValue = Serializer::read_double(dataPtr, endPtr);

    EXPECT_DOUBLE_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(SerializerTest, DoubleLowest) { // Most negative
    const double originalValue = std::numeric_limits<double>::lowest();
    Serializer::write(buffer, originalValue);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    double readValue = Serializer::read_double(dataPtr, endPtr);

    EXPECT_DOUBLE_EQ(originalValue, readValue);
    EXPECT_EQ(dataPtr, endPtr);
}

// Test for multiple writes and reads in sequence
TEST_F(SerializerTest, SequentialReadWrite) {
    const uint32_t val1 = 1024;
    const float    val2 = 3.14f;
    const int      val3 = -200;
    const uint8_t  val4 = 7;

    Serializer::write(buffer, val1);
    Serializer::write(buffer, val2);
    Serializer::write(buffer, val3);
    Serializer::write(buffer, val4);

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();

    EXPECT_EQ(Serializer::read_uint32(dataPtr, endPtr), val1);
    EXPECT_FLOAT_EQ(Serializer::read_float(dataPtr, endPtr), val2);
    EXPECT_EQ(Serializer::read_int(dataPtr, endPtr), val3);
    EXPECT_EQ(Serializer::read_uint8(dataPtr, endPtr), val4);
    EXPECT_EQ(dataPtr, endPtr); // All bytes consumed
}

// Test cases for boundary conditions and error handling

TEST_F(SerializerTest, ReadFromEmptyBuffer) {
    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();

    EXPECT_THROW(Serializer::read_uint8(dataPtr, endPtr), std::runtime_error);
    EXPECT_THROW(Serializer::read_uint16(dataPtr, endPtr), std::runtime_error);
    EXPECT_THROW(Serializer::read_uint32(dataPtr, endPtr), std::runtime_error);
    EXPECT_THROW(Serializer::read_uint64(dataPtr, endPtr), std::runtime_error);
    EXPECT_THROW(Serializer::read_int(dataPtr, endPtr), std::runtime_error); // Fails reading size prefix first
    EXPECT_THROW(Serializer::read_float(dataPtr, endPtr), std::runtime_error);
    EXPECT_THROW(Serializer::read_double(dataPtr, endPtr), std::runtime_error);
}

TEST_F(SerializerTest, ReadInsufficientData) {
    // Write 2 bytes for a uint32_t (which needs 4)
    Serializer::write(buffer, static_cast<uint16_t>(0xABCD));

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();
    EXPECT_THROW(Serializer::read_uint32(dataPtr, endPtr), std::runtime_error);

    buffer.clear();
    // Write 1 byte for a uint16_t (which needs 2)
    Serializer::write(buffer, static_cast<uint8_t>(0xAB));
    dataPtr = buffer.data();
    endPtr = buffer.data() + buffer.size();
    EXPECT_THROW(Serializer::read_uint16(dataPtr, endPtr), std::runtime_error);

    buffer.clear();
    // Write 3 bytes for an int (needs 1 for size + sizeof(int) for data)
    // Assuming sizeof(int) is 4, this will fail when reading data after size.
    // First, write a valid size prefix for int
    Serializer::write(buffer, static_cast<uint8_t>(sizeof(int)));
    // Then, write only 3 bytes of data if sizeof(int) is 4
    if (sizeof(int) > 3) {
        buffer.push_back(static_cast<std::byte>(0x01));
        buffer.push_back(static_cast<std::byte>(0x02));
        buffer.push_back(static_cast<std::byte>(0x03));
        dataPtr = buffer.data();
        endPtr = buffer.data() + buffer.size();
        EXPECT_THROW(Serializer::read_int(dataPtr, endPtr), std::runtime_error);
    }
}

TEST_F(SerializerTest, ReadIntIncorrectSizePrefix) {
    // Write a size prefix that doesn't match sizeof(int)
    uint8_t incorrectSize = sizeof(int) + 1;
    if (sizeof(int) == std::numeric_limits<uint8_t>::max()) { // unlikely edge case
        incorrectSize = sizeof(int) - 1;
    }
     // Ensure incorrectSize is not 0 if sizeof(int) is 1 to avoid issues with the test.
    if (incorrectSize == 0 && sizeof(int) == 1) {
        incorrectSize = 2; // Just needs to be different and non-zero
    }


    Serializer::write(buffer, incorrectSize); // Write incorrect size
    // Add enough dummy bytes for the incorrect size to pass checkBounds for size read itself.
    for(uint8_t i = 0; i < incorrectSize; ++i) {
        Serializer::write(buffer, static_cast<uint8_t>(0));
    }

    const std::byte* dataPtr = buffer.data();
    const std::byte* endPtr = buffer.data() + buffer.size();

    EXPECT_THROW(Serializer::read_int(dataPtr, endPtr), std::length_error);
}

// This test is more conceptual for `write(int)` as the primary check is a static_assert.
// However, there's a runtime check for `sizeof(int) > 255` in `Serializer.cpp`.
// This test would only be meaningful if sizeof(int) could actually be > 255,
// which is not the case on typical systems.
// If we wanted to simulate this, we would need to mock `sizeof(int)` or use
// a platform where this condition is true, which is beyond standard unit testing.
// For now, we acknowledge this check exists. A direct test is impractical here.
TEST_F(SerializerTest, WriteIntSizeOverflowAttempt) {
    // This test is for the check: if (intSize > std::numeric_limits<uint8_t>::max())
    // On typical systems, sizeof(int) is 4 or 8, so this won't throw.
    // If sizeof(int) were, e.g., 256, it would throw.
    // We can't change sizeof(int), so we just document this.
    if (sizeof(int) > std::numeric_limits<uint8_t>::max()) {
        // This branch will likely not be taken in test environments.
        EXPECT_THROW(Serializer::write(buffer, 123), std::length_error);
    } else {
        // Normal behavior, no throw expected from this specific check.
        EXPECT_NO_THROW(Serializer::write(buffer, 123));
    }
}
