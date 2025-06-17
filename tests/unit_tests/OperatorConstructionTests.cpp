#include "gtest/gtest.h"
#include "headers/operators/AddOperator.h" // Required for AddOperator
#include "headers/operators/Operator.h"    // Base class of AddOperator
#include <memory>                         // For std::unique_ptr
#include "headers/util/DynamicArray.h"     // For getOutputConnections (used by ConstructorAndGetId)
#include <unordered_set>                  // For connection bucket type (though not directly used by ConstructorAndGetId, getOutputConnections returns it)

// Minimal Test fixture for Operator tests
class OperatorConstructionTest : public ::testing::Test { // Renamed fixture for clarity
};

TEST_F(OperatorConstructionTest, ConstructorAndGetId) { // Adjusted to new fixture name
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(123);
    ASSERT_NE(op, nullptr);
    EXPECT_EQ(op->getId(), 123);

    // Also test initial state of outputConnections
    const auto& connections = op->getOutputConnections();
    EXPECT_EQ(connections.maxIdx(), -1);
    EXPECT_EQ(connections.count(), 0);

    std::unique_ptr<AddOperator> op2 = std::make_unique<AddOperator>(0);
    ASSERT_NE(op2, nullptr);
    EXPECT_EQ(op2->getId(), 0);
}
