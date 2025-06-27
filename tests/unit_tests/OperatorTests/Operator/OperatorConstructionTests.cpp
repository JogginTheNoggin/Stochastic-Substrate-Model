#include "gtest/gtest.h"
#include "helpers/MockOperator.h" // Corrected include path
#include "headers/operators/Operator.h"
#include <memory>
#include "headers/util/DynamicArray.h"
#include <unordered_set>
#include "headers/util/Serializer.h" // Corrected include path
#include <vector>                         // For std::vector (used in new tests)
#include <cstddef>                        // For std::byte (used in new tests)

// Minimal Test fixture for Operator tests
class OperatorConstructionTest : public ::testing::Test {
};

TEST_F(OperatorConstructionTest, ConstructorAndGetId) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(123); // Changed to TestOperator
    ASSERT_NE(op, nullptr);
    EXPECT_EQ(op->getId(), 123);

    // Also test initial state of outputConnections
    const auto& connections = op->getOutputConnections();
    EXPECT_EQ(connections.maxIdx(), -1);
    EXPECT_EQ(connections.count(), 0);

    std::unique_ptr<MockOperator> op2 = std::make_unique<MockOperator>(0); // Changed to TestOperator
    ASSERT_NE(op2, nullptr);
    EXPECT_EQ(op2->getId(), 0);
}

TEST_F(OperatorConstructionTest, DeserializeNoConnections) {
    std::vector<std::byte> buffer;
    uint32_t id = 123;
    uint16_t numBuckets = 0;

    Serializer::write(buffer, id);
    Serializer::write(buffer, numBuckets);

    const std::byte* ptr = buffer.data();
    const std::byte* end_ptr = buffer.data() + buffer.size();
    MockOperator op(ptr, end_ptr);

    EXPECT_EQ(op.getId(), id);
    const auto& connections = op.getOutputConnections();
    EXPECT_EQ(connections.count(), 0);
    EXPECT_EQ(connections.maxIdx(), -1); // Default for empty DynamicArray
}

TEST_F(OperatorConstructionTest, DeserializeOneBucketOneTarget) {
    std::vector<std::byte> buffer;
    uint32_t id = 456;
    uint16_t numBuckets = 1;
    uint16_t distance1 = 5;
    uint16_t numConnectionsInBucket1 = 1;
    uint32_t targetId1_1 = 789;

    Serializer::write(buffer, id);
    Serializer::write(buffer, numBuckets);
    Serializer::write(buffer, distance1);
    Serializer::write(buffer, numConnectionsInBucket1);
    Serializer::write(buffer, targetId1_1);

    const std::byte* ptr = buffer.data();
    const std::byte* end_ptr = buffer.data() + buffer.size();
    MockOperator op(ptr, end_ptr);

    EXPECT_EQ(op.getId(), id);
    const auto& connections = op.getOutputConnections();
    EXPECT_EQ(connections.count(), 1); // One bucket
    ASSERT_GE(connections.maxIdx(), distance1);

    const auto* bucket1 = connections.get(distance1);
    ASSERT_NE(bucket1, nullptr);
    EXPECT_EQ(bucket1->size(), 1);
    EXPECT_EQ(bucket1->count(targetId1_1), 1);
}

TEST_F(OperatorConstructionTest, DeserializeMultipleBucketsMultipleTargets) {
    std::vector<std::byte> buffer;
    uint32_t id = 101;
    uint16_t numBuckets = 2;

    // Bucket 1
    uint16_t distance1 = 2;
    uint16_t numConnectionsInBucket1 = 2;
    uint32_t targetId1_1 = 201;
    uint32_t targetId1_2 = 202;

    // Bucket 2
    uint16_t distance2 = 10;
    uint16_t numConnectionsInBucket2 = 1;
    uint32_t targetId2_1 = 301;

    Serializer::write(buffer, id);
    Serializer::write(buffer, numBuckets);

    // Bucket 1 data
    Serializer::write(buffer, distance1);
    Serializer::write(buffer, numConnectionsInBucket1);
    Serializer::write(buffer, targetId1_1);
    Serializer::write(buffer, targetId1_2);

    // Bucket 2 data
    Serializer::write(buffer, distance2);
    Serializer::write(buffer, numConnectionsInBucket2);
    Serializer::write(buffer, targetId2_1);

    const std::byte* ptr = buffer.data();
    const std::byte* end_ptr = buffer.data() + buffer.size();
    MockOperator op(ptr, end_ptr);

    EXPECT_EQ(op.getId(), id);
    const auto& connections = op.getOutputConnections();
    EXPECT_EQ(connections.count(), 2); // Two buckets
    ASSERT_GE(connections.maxIdx(), distance2); // Max index should accommodate the largest distance

    // Verify Bucket 1
    const auto* bucket1 = connections.get(distance1);
    ASSERT_NE(bucket1, nullptr);
    EXPECT_EQ(bucket1->size(), 2);
    EXPECT_EQ(bucket1->count(targetId1_1), 1);
    EXPECT_EQ(bucket1->count(targetId1_2), 1);

    // Verify Bucket 2
    const auto* bucket2 = connections.get(distance2);
    ASSERT_NE(bucket2, nullptr);
    EXPECT_EQ(bucket2->size(), 1);
    EXPECT_EQ(bucket2->count(targetId2_1), 1);
}
