#include "gtest/gtest.h"
#include "headers/operators/AddOperator.h"
#include "headers/operators/Operator.h"
#include "headers/util/Serializer.h" // For reading back values, if needed for verification
#include <memory>
#include <vector>
#include <cstddef> // For std::byte
#include <algorithm> // For std::equal
#include <sstream>   // For std::stringstream in error reporting
#include <iomanip>   // For std::hex
#include <unordered_set> // For comparing target IDs

// Helper function to convert various types to a byte vector (Big Endian)
// These are simplified versions of what Serializer::write might do internally
// For verification purposes, we need to reconstruct what Serializer writes.

// Helper to write uint16_t as big endian bytes
std::vector<std::byte> uint16ToBytes(uint16_t val) {
    std::vector<std::byte> bytes(2);
    bytes[0] = static_cast<std::byte>((val >> 8) & 0xFF);
    bytes[1] = static_cast<std::byte>(val & 0xFF);
    return bytes;
}

// Helper to write uint32_t as big endian bytes
std::vector<std::byte> uint32ToBytes(uint32_t val) {
    std::vector<std::byte> bytes(4);
    bytes[0] = static_cast<std::byte>((val >> 24) & 0xFF);
    bytes[1] = static_cast<std::byte>((val >> 16) & 0xFF);
    bytes[2] = static_cast<std::byte>((val >> 8) & 0xFF);
    bytes[3] = static_cast<std::byte>(val & 0xFF);
    return bytes;
}

// Helper to write int (assuming Serializer writes it as a 4-byte signed int in BE).
// This matches typical direct serialization of int.
std::vector<std::byte> intToBytes(int val) {
    std::vector<std::byte> bytes(4);
    // Cast to uint32_t to ensure well-defined bitwise operations for negative numbers
    // The bit pattern of val will be preserved.
    uint32_t u_val = static_cast<uint32_t>(val);
    bytes[0] = static_cast<std::byte>((u_val >> 24) & 0xFF);
    bytes[1] = static_cast<std::byte>((u_val >> 16) & 0xFF);
    bytes[2] = static_cast<std::byte>((u_val >> 8) & 0xFF);
    bytes[3] = static_cast<std::byte>(u_val & 0xFF);
    return bytes;
}


// Fixture for Operator serializeToBytes tests
class OperatorSerializeTest : public ::testing::Test {
protected:
    // Helper to check a sub-vector
    void expectBytesEqual(const std::vector<std::byte>& full, size_t offset, const std::vector<std::byte>& expected_segment, const std::string& message = "") {
        ASSERT_GE(full.size(), offset + expected_segment.size()) << message << ": Full vector too small or offset too large.";
        bool are_equal = std::equal(expected_segment.begin(), expected_segment.end(), full.begin() + offset);
        if (!are_equal) {
            std::stringstream ss_full, ss_expected;
            ss_expected << "[";
            for(size_t i=0; i < expected_segment.size(); ++i) {
                ss_expected << (i > 0 ? " " : "") << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(expected_segment[i]);
            }
            ss_expected << "]";

            ss_full << "[";
            for(size_t i=0; i < expected_segment.size(); ++i) {
                 ss_full << (i > 0 ? " " : "") << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(full[offset+i]);
            }
            ss_full << "]";

            FAIL() << message << ": Byte segment mismatch at offset " << offset
                   << ".\nExpected: " << ss_expected.str()
                   << "\nGot:      " << ss_full.str();
        }
        SUCCEED();
    }
};

TEST_F(OperatorSerializeTest, SerializeAddOperatorNoConnections) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(123, 10, 20);
    uint32_t operatorId = 123;
    int weight = 10;
    int threshold = 20;
    int accumulateData = 0;

    std::vector<std::byte> expectedOperatorPart;
    // Operator::serializeToBytes writes:
    // 1. Operator Type (uint16_t) - from getOpType()
    auto typeBytes = uint16ToBytes(static_cast<uint16_t>(AddOperator::OP_TYPE)); // AddOperator::OP_TYPE is 0
    expectedOperatorPart.insert(expectedOperatorPart.end(), typeBytes.begin(), typeBytes.end());
    // 2. Operator ID (uint32_t) - Serializer::write(buffer, this->operatorId)
    auto idBytes = uint32ToBytes(operatorId);
    expectedOperatorPart.insert(expectedOperatorPart.end(), idBytes.begin(), idBytes.end());
    // 3. Bucket Count (uint16_t)
    auto bucketCountBytes = uint16ToBytes(0); // No connections
    expectedOperatorPart.insert(expectedOperatorPart.end(), bucketCountBytes.begin(), bucketCountBytes.end());

    std::vector<std::byte> expectedAddOperatorSpecificPart;
    // AddOperator specific: weight, threshold, accumulateData (each as int)
    auto weightBytes = intToBytes(weight);
    expectedAddOperatorSpecificPart.insert(expectedAddOperatorSpecificPart.end(), weightBytes.begin(), weightBytes.end());
    auto thresholdBytes = intToBytes(threshold);
    expectedAddOperatorSpecificPart.insert(expectedAddOperatorSpecificPart.end(), thresholdBytes.begin(), thresholdBytes.end());
    auto accDataBytes = intToBytes(accumulateData);
    expectedAddOperatorSpecificPart.insert(expectedAddOperatorSpecificPart.end(), accDataBytes.begin(), accDataBytes.end());

    // AddOperator::serializeToBytes combines these parts and prepends total size
    std::vector<std::byte> expectedFullDataBuffer = expectedOperatorPart;
    expectedFullDataBuffer.insert(expectedFullDataBuffer.end(), expectedAddOperatorSpecificPart.begin(), expectedAddOperatorSpecificPart.end());

    uint32_t totalDataSize = static_cast<uint32_t>(expectedFullDataBuffer.size());
    std::vector<std::byte> expectedFinalBuffer = uint32ToBytes(totalDataSize);
    expectedFinalBuffer.insert(expectedFinalBuffer.end(), expectedFullDataBuffer.begin(), expectedFullDataBuffer.end());

    std::vector<std::byte> actualBytes = op->serializeToBytes();

    ASSERT_EQ(actualBytes.size(), expectedFinalBuffer.size()) << "Total serialized size mismatch.";
    expectBytesEqual(actualBytes, 0, expectedFinalBuffer, "Full buffer comparison");
}

TEST_F(OperatorSerializeTest, SerializeAddOperatorOneConnection) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(124, 5, 15);
    op->addConnectionInternal(200, 2);
    uint32_t operatorId = 124;
    int weight = 5;
    int threshold = 15;
    int accumulateData = 0;

    std::vector<std::byte> expectedOperatorPart;
    auto typeBytes = uint16ToBytes(static_cast<uint16_t>(AddOperator::OP_TYPE));
    expectedOperatorPart.insert(expectedOperatorPart.end(), typeBytes.begin(), typeBytes.end());
    auto idBytes = uint32ToBytes(operatorId);
    expectedOperatorPart.insert(expectedOperatorPart.end(), idBytes.begin(), idBytes.end());
    auto bucketCountBytes = uint16ToBytes(1); // 1 bucket
    expectedOperatorPart.insert(expectedOperatorPart.end(), bucketCountBytes.begin(), bucketCountBytes.end());
    // For each bucket: Distance (uint16_t), Num Connections (uint16_t), Target IDs (uint32_t each)
    auto distBytes = uint16ToBytes(2);
    expectedOperatorPart.insert(expectedOperatorPart.end(), distBytes.begin(), distBytes.end());
    auto numConnBytes = uint16ToBytes(1);
    expectedOperatorPart.insert(expectedOperatorPart.end(), numConnBytes.begin(), numConnBytes.end());
    auto targetIdBytes = uint32ToBytes(200);
    expectedOperatorPart.insert(expectedOperatorPart.end(), targetIdBytes.begin(), targetIdBytes.end());

    std::vector<std::byte> expectedAddOperatorSpecificPart;
    auto weightBytes = intToBytes(weight);
    expectedAddOperatorSpecificPart.insert(expectedAddOperatorSpecificPart.end(), weightBytes.begin(), weightBytes.end());
    auto thresholdBytes = intToBytes(threshold);
    expectedAddOperatorSpecificPart.insert(expectedAddOperatorSpecificPart.end(), thresholdBytes.begin(), thresholdBytes.end());
    auto accDataBytes = intToBytes(accumulateData);
    expectedAddOperatorSpecificPart.insert(expectedAddOperatorSpecificPart.end(), accDataBytes.begin(), accDataBytes.end());

    std::vector<std::byte> expectedFullDataBuffer = expectedOperatorPart;
    expectedFullDataBuffer.insert(expectedFullDataBuffer.end(), expectedAddOperatorSpecificPart.begin(), expectedAddOperatorSpecificPart.end());

    uint32_t totalDataSize = static_cast<uint32_t>(expectedFullDataBuffer.size());
    std::vector<std::byte> expectedFinalBuffer = uint32ToBytes(totalDataSize);
    expectedFinalBuffer.insert(expectedFinalBuffer.end(), expectedFullDataBuffer.begin(), expectedFullDataBuffer.end());

    std::vector<std::byte> actualBytes = op->serializeToBytes();

    ASSERT_EQ(actualBytes.size(), expectedFinalBuffer.size()) << "Total serialized size mismatch.";
    expectBytesEqual(actualBytes, 0, expectedFinalBuffer, "Full buffer comparison");
}

TEST_F(OperatorSerializeTest, SerializeAddOperatorMultipleConnectionsSameDistance) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(125, 7, 25);
    op->addConnectionInternal(300, 1);
    op->addConnectionInternal(301, 1);
    uint32_t operatorId = 125;
    int weight = 7;
    int threshold = 25;
    int accumulateData = 0;

    std::vector<std::byte> actualBytes = op->serializeToBytes();

    size_t offset = 0;
    // Read total size from actual bytes
    uint32_t actualTotalSize =
        (static_cast<uint32_t>(actualBytes[0]) << 24) |
        (static_cast<uint32_t>(actualBytes[1]) << 16) |
        (static_cast<uint32_t>(actualBytes[2]) << 8)  |
        (static_cast<uint32_t>(actualBytes[3]));
    offset += 4;
    ASSERT_EQ(actualTotalSize, actualBytes.size() - 4) << "Total size prefix mismatch.";


    expectBytesEqual(actualBytes, offset, uint16ToBytes(static_cast<uint16_t>(AddOperator::OP_TYPE)), "Type"); offset += 2;
    expectBytesEqual(actualBytes, offset, uint32ToBytes(operatorId), "ID"); offset += 4;
    expectBytesEqual(actualBytes, offset, uint16ToBytes(1), "Bucket Count"); offset += 2; // 1 bucket
    expectBytesEqual(actualBytes, offset, uint16ToBytes(1), "Distance"); offset += 2;
    expectBytesEqual(actualBytes, offset, uint16ToBytes(2), "Num Connections"); offset += 2; // 2 connections

    uint32_t target1 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    uint32_t target2 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    std::unordered_set<uint32_t> expectedTargets = {300, 301};
    std::unordered_set<uint32_t> actualTargets = {target1, target2};
    ASSERT_EQ(actualTargets, expectedTargets) << "Target ID set mismatch.";

    expectBytesEqual(actualBytes, offset, intToBytes(weight), "Weight"); offset += 4;
    expectBytesEqual(actualBytes, offset, intToBytes(threshold), "Threshold"); offset += 4;
    expectBytesEqual(actualBytes, offset, intToBytes(accumulateData), "AccumulateData"); offset += 4;

    ASSERT_EQ(offset, actualBytes.size()) << "Total length check failed, not all bytes consumed.";
}


TEST_F(OperatorSerializeTest, SerializeAddOperatorMultipleConnectionsDifferentDistances) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(126, 8, 28);
    op->addConnectionInternal(400, 0);
    op->addConnectionInternal(401, 3);
    op->addConnectionInternal(402, 3);

    uint32_t operatorId = 126;
    int weight = 8;
    int threshold = 28;
    int accumulateData = 0;

    std::vector<std::byte> actualBytes = op->serializeToBytes();
    size_t offset = 0;
    uint32_t actualTotalSize =
        (static_cast<uint32_t>(actualBytes[0]) << 24) | (static_cast<uint32_t>(actualBytes[1]) << 16) |
        (static_cast<uint32_t>(actualBytes[2]) << 8)  | (static_cast<uint32_t>(actualBytes[3]));
    offset += 4;
    ASSERT_EQ(actualTotalSize, actualBytes.size() - 4) << "Total size prefix mismatch.";

    expectBytesEqual(actualBytes, offset, uint16ToBytes(static_cast<uint16_t>(AddOperator::OP_TYPE)), "Type"); offset += 2;
    expectBytesEqual(actualBytes, offset, uint32ToBytes(operatorId), "ID"); offset += 4;
    expectBytesEqual(actualBytes, offset, uint16ToBytes(2), "Bucket Count"); offset += 2; // 2 buckets

    // Bucket 1 (distance 0)
    expectBytesEqual(actualBytes, offset, uint16ToBytes(0), "Dist0"); offset += 2;
    expectBytesEqual(actualBytes, offset, uint16ToBytes(1), "NumConn D0"); offset += 2;
    expectBytesEqual(actualBytes, offset, uint32ToBytes(400), "Target D0"); offset += 4;

    // Bucket 2 (distance 3)
    expectBytesEqual(actualBytes, offset, uint16ToBytes(3), "Dist3"); offset += 2;
    expectBytesEqual(actualBytes, offset, uint16ToBytes(2), "NumConn D3"); offset += 2;
    uint32_t target1_d3 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    uint32_t target2_d3 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    std::unordered_set<uint32_t> expectedTargets_d3 = {401, 402};
    std::unordered_set<uint32_t> actualTargets_d3 = {target1_d3, target2_d3};
    ASSERT_EQ(actualTargets_d3, expectedTargets_d3) << "Target ID set mismatch for distance 3.";

    expectBytesEqual(actualBytes, offset, intToBytes(weight), "Weight"); offset += 4;
    expectBytesEqual(actualBytes, offset, intToBytes(threshold), "Threshold"); offset += 4;
    expectBytesEqual(actualBytes, offset, intToBytes(accumulateData), "AccumulateData"); offset += 4;

    ASSERT_EQ(offset, actualBytes.size()) << "Total length check failed, not all bytes consumed.";
}
