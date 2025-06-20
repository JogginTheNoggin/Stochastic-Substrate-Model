#pragma once
#include "headers/operators/Operator.h"
#include <memory>
#include <string>
// --- Test-Only Subclass ---
// The only purpose of this class is to allow us to create an instance of the
// abstract Operator class so we can test its concrete public methods in isolation.
class TestOperator : public Operator {
public:
    // Inherit the base class constructor
    using Operator::Operator;

    // Provide minimal implementations for all pure virtual methods
    Operator::Type getOpType() const override { return Operator::Type::UNDEFINED; }
    void message(const int) override {}
    void message(const float) override {}
    void message(const double) override {}
    void processData() override {}
    void changeParams(const std::vector<int>&) override {}
    void randomInit(uint32_t, Randomizer*) override {}
    void randomInit(IdRange*, Randomizer*) override {}
    bool equals(const Operator& other) const override {return false;};

    
    // This override is the key: it just calls the base class implementation directly,
    // allowing us to test Operator::toJson without any subclass interference.
    std::string toJson(bool prettyPrint, bool encloseInBrackets) const override {
        return Operator::toJson(prettyPrint, encloseInBrackets);
    }

    
};