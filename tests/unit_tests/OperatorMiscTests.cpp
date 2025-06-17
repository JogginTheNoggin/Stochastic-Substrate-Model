#include "gtest/gtest.h"
#include "headers/operators/Operator.h" // For Operator::Type and Operator::typeToString
#include <string>

// No fixture needed for static method testing.

TEST(OperatorMiscTest, TypeToStringConversion) {
    EXPECT_EQ(Operator::typeToString(Operator::Type::ADD), "ADD");
    EXPECT_EQ(Operator::typeToString(Operator::Type::SUB), "SUB");
    EXPECT_EQ(Operator::typeToString(Operator::Type::MUL), "MUL");
    EXPECT_EQ(Operator::typeToString(Operator::Type::LEFT), "LEFT");
    EXPECT_EQ(Operator::typeToString(Operator::Type::RIGHT), "RIGHT");
    EXPECT_EQ(Operator::typeToString(Operator::Type::OUT), "OUT");
    EXPECT_EQ(Operator::typeToString(Operator::Type::IN), "IN");
    EXPECT_EQ(Operator::typeToString(Operator::Type::UNDEFINED), "UNDEFINED");

    // Test a hypothetical unknown type, though the switch in Operator.cpp has a default.
    // Casting an integer to Operator::Type is a way to test this, assuming the default case.
    // However, this can be risky if the integer doesn't map to any known or default behavior.
    // The current default case in Operator.cpp returns "UNKNOWN".
    // Let's use a value that's definitely not in the enum list.
    Operator::Type unknownType = static_cast<Operator::Type>(999);
    EXPECT_EQ(Operator::typeToString(unknownType), "UNKNOWN");

    // Test another value that might fall into default
    Operator::Type anotherUnknown = static_cast<Operator::Type>(100);
    EXPECT_EQ(Operator::typeToString(anotherUnknown), "UNKNOWN");
}
