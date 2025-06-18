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

// Helper functions (uint16ToBytes, etc.) are removed. Serializer::write will be used directly.

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
    Serializer::write(expectedOperatorPart, static_cast<uint16_t>(AddOperator::OP_TYPE));
    // 2. Operator ID (uint32_t) - Serializer::write(buffer, this->operatorId)
    Serializer::write(expectedOperatorPart, operatorId);
    // 3. Bucket Count (uint16_t)
    Serializer::write(expectedOperatorPart, static_cast<uint16_t>(0)); // No connections

    std::vector<std::byte> expectedAddOperatorSpecificPart;
    // AddOperator specific: weight, threshold, accumulateData (each as int)
    Serializer::write(expectedAddOperatorSpecificPart, weight);
    Serializer::write(expectedAddOperatorSpecificPart, threshold);
    Serializer::write(expectedAddOperatorSpecificPart, accumulateData);

    // AddOperator::serializeToBytes combines these parts and prepends total size
    std::vector<std::byte> expectedFullDataBuffer = expectedOperatorPart;
    expectedFullDataBuffer.insert(expectedFullDataBuffer.end(), expectedAddOperatorSpecificPart.begin(), expectedAddOperatorSpecificPart.end());

    uint32_t totalDataSize = static_cast<uint32_t>(expectedFullDataBuffer.size());
    std::vector<std::byte> expectedFinalBuffer; // Create empty vector
    Serializer::write(expectedFinalBuffer, totalDataSize); // Write size prefix using Serializer
    expectedFinalBuffer.insert(expectedFinalBuffer.end(), expectedFullDataBuffer.begin(), expectedFullDataBuffer.end()); // Append data

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
    Serializer::write(expectedOperatorPart, static_cast<uint16_t>(AddOperator::OP_TYPE));
    Serializer::write(expectedOperatorPart, operatorId);
    Serializer::write(expectedOperatorPart, static_cast<uint16_t>(1)); // 1 bucket
    // For each bucket: Distance (uint16_t), Num Connections (uint16_t), Target IDs (uint32_t each)
    Serializer::write(expectedOperatorPart, static_cast<uint16_t>(2)); // Distance
    Serializer::write(expectedOperatorPart, static_cast<uint16_t>(1)); // Num Connections
    Serializer::write(expectedOperatorPart, static_cast<uint32_t>(200)); // Target ID

    std::vector<std::byte> expectedAddOperatorSpecificPart;
    Serializer::write(expectedAddOperatorSpecificPart, weight);
    Serializer::write(expectedAddOperatorSpecificPart, threshold);
    Serializer::write(expectedAddOperatorSpecificPart, accumulateData);

    std::vector<std::byte> expectedFullDataBuffer = expectedOperatorPart;
    expectedFullDataBuffer.insert(expectedFullDataBuffer.end(), expectedAddOperatorSpecificPart.begin(), expectedAddOperatorSpecificPart.end());

    uint32_t totalDataSize = static_cast<uint32_t>(expectedFullDataBuffer.size());
    std::vector<std::byte> expectedFinalBuffer;
    Serializer::write(expectedFinalBuffer, totalDataSize); // Write size prefix
    expectedFinalBuffer.insert(expectedFinalBuffer.end(), expectedFullDataBuffer.begin(), expectedFullDataBuffer.end()); // Append data

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

    std::vector<std::byte> tempExpectedBytes;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(AddOperator::OP_TYPE));
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Type"); offset += 2;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, operatorId);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "ID"); offset += 4;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(1)); // Bucket Count
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Bucket Count"); offset += 2;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(1)); // Distance
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Distance"); offset += 2;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(2)); // Num Connections
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Num Connections"); offset += 2;

    uint32_t target1 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    uint32_t target2 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    std::unordered_set<uint32_t> expectedTargets = {300, 301};
    std::unordered_set<uint32_t> actualTargets = {target1, target2};
    ASSERT_EQ(actualTargets, expectedTargets) << "Target ID set mismatch.";

    tempExpectedBytes.clear(); 
    Serializer::write(tempExpectedBytes, weight);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Weight");
    // CORRECTED: Advance by the size of the size-prefix (1) + the size of the int (4)
    offset += (1 + sizeof(int));

    tempExpectedBytes.clear(); 
    Serializer::write(tempExpectedBytes, threshold);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Threshold");
    // CORRECTED: Advance by 5 bytes
    offset += (1 + sizeof(int));

    tempExpectedBytes.clear(); 
    Serializer::write(tempExpectedBytes, accumulateData);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "AccumulateData");
    // CORRECTED: Advance by 5 bytes
    offset += (1 + sizeof(int));

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

    std::vector<std::byte> tempExpectedBytes;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(AddOperator::OP_TYPE));
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Type"); offset += 2;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, operatorId);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "ID"); offset += 4;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(2)); // Bucket Count
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Bucket Count"); offset += 2;

    // Bucket 1 (distance 0)
    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(0)); // Distance
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Dist0"); offset += 2;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(1)); // Num Connections
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "NumConn D0"); offset += 2;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint32_t>(400)); // Target ID
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Target D0"); offset += 4;

    // Bucket 2 (distance 3)
    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(3)); // Distance
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Dist3"); offset += 2;

    tempExpectedBytes.clear(); Serializer::write(tempExpectedBytes, static_cast<uint16_t>(2)); // Num Connections
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "NumConn D3"); offset += 2;

    uint32_t target1_d3 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    uint32_t target2_d3 = (static_cast<uint32_t>(actualBytes[offset+0]) << 24) | (static_cast<uint32_t>(actualBytes[offset+1]) << 16) | (static_cast<uint32_t>(actualBytes[offset+2]) << 8) | static_cast<uint32_t>(actualBytes[offset+3]);
    offset += 4;
    std::unordered_set<uint32_t> expectedTargets_d3 = {401, 402};
    std::unordered_set<uint32_t> actualTargets_d3 = {target1_d3, target2_d3};
    ASSERT_EQ(actualTargets_d3, expectedTargets_d3) << "Target ID set mismatch for distance 3.";

    tempExpectedBytes.clear(); 
    Serializer::write(tempExpectedBytes, weight);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Weight");
    // CORRECTED: Advance by the size of the size-prefix (1) + the size of the int (4)
    offset += (1 + sizeof(int));

    tempExpectedBytes.clear(); 
    Serializer::write(tempExpectedBytes, threshold);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "Threshold");
    // CORRECTED: Advance by 5 bytes
    offset += (1 + sizeof(int));

    tempExpectedBytes.clear(); 
    Serializer::write(tempExpectedBytes, accumulateData);
    expectBytesEqual(actualBytes, offset, tempExpectedBytes, "AccumulateData");
    // CORRECTED: Advance by 5 bytes
    offset += (1 + sizeof(int));

    ASSERT_EQ(offset, actualBytes.size()) << "Total length check failed, not all bytes consumed.";
}
