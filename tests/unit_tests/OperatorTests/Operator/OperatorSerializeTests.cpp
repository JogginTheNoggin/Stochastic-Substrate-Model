#include "gtest/gtest.h"
#include "helpers/TestOperator.h" // Changed
#include "headers/operators/Operator.h"
#include "headers/util/Serializer.h" // Required for TestOperator connection setup (addConnectionInternal)
#include <memory>
#include <vector>
#include <cstddef>      // For std::byte
#include <string>
#include <fstream>      // For std::ifstream
#include <stdexcept>    // For std::runtime_error
#include <filesystem>   // For checking file existence if needed (not strictly required by problem for read func)

// Golden file directory (relative to build directory where tests run)
const std::string MOCK_FILE_DIR = "../tests/unit_tests/OperatorTests/Operator/golden_files/";

// Helper function to read a binary file into a vector of bytes
std::vector<std::byte> readBinaryGoldenFile(const std::string& relativePath) {
    // Construct path relative to the current working directory of the test executable
    // This assumes tests are run from the 'build' directory.
    std::string filePath = relativePath; // Path is already relative to build dir as per MOCK_FILE_DIR

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open golden file: " + filePath +
                                 ". Make sure the path is correct and the file exists. "
                                 "CWD: " + std::filesystem::current_path().string());
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read golden file: " + filePath);
    }
    return buffer;
}

class OperatorSerializeTest : public ::testing::Test {
    // No specific protected members needed for these tests anymore
};

TEST_F(OperatorSerializeTest, SerializeNoConnections) {
    auto op = std::make_unique<TestOperator>(123); // TestOperator ID 123

    std::vector<std::byte> actualBytes = op->baseClassSerializeToBytes(); // Calls Operator::serializeToBytes

    std::string goldenFilePath = MOCK_FILE_DIR + "operator_serialize_no_connections.bin";
    std::vector<std::byte> expectedBytes = readBinaryGoldenFile(goldenFilePath);

    EXPECT_EQ(actualBytes, expectedBytes);
}

TEST_F(OperatorSerializeTest, SerializeOneConnection) {
    auto op = std::make_unique<TestOperator>(124); // TestOperator ID 124
    op->addConnectionInternal(200, 2); // Target 200, distance 2

    std::vector<std::byte> actualBytes = op->baseClassSerializeToBytes();

    std::string goldenFilePath = MOCK_FILE_DIR + "operator_serialize_one_connection.bin";
    std::vector<std::byte> expectedBytes = readBinaryGoldenFile(goldenFilePath);

    EXPECT_EQ(actualBytes, expectedBytes);
}

TEST_F(OperatorSerializeTest, SerializeMultipleConnectionsSameDistance) {
    auto op = std::make_unique<TestOperator>(125); // TestOperator ID 125
    // Add in specific order to test sorting of target IDs by Operator::serializeToBytes
    op->addConnectionInternal(301, 1);
    op->addConnectionInternal(300, 1);

    std::vector<std::byte> actualBytes = op->baseClassSerializeToBytes();

    std::string goldenFilePath = MOCK_FILE_DIR + "operator_serialize_multi_conn_same_dist.bin";
    std::vector<std::byte> expectedBytes = readBinaryGoldenFile(goldenFilePath);

    EXPECT_EQ(actualBytes, expectedBytes);
}

TEST_F(OperatorSerializeTest, SerializeMultipleConnectionsDifferentDistances) {
    auto op = std::make_unique<TestOperator>(126); // TestOperator ID 126
    // Add in specific order to test sorting of distance buckets by Operator::serializeToBytes
    op->addConnectionInternal(401, 3);
    op->addConnectionInternal(402, 3); // Second target for distance 3
    op->addConnectionInternal(400, 0);


    std::vector<std::byte> actualBytes = op->baseClassSerializeToBytes();

    std::string goldenFilePath = MOCK_FILE_DIR + "operator_serialize_multi_conn_multi_dist.bin";
    std::vector<std::byte> expectedBytes = readBinaryGoldenFile(goldenFilePath);

    EXPECT_EQ(actualBytes, expectedBytes);
}
