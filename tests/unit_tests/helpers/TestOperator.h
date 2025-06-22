#pragma once
#include "headers/operators/Operator.h"
#include <memory>
#include <string>
#include <vector> // Required for changeParams
// --- Test-Only Subclass ---
// The only purpose of this class is to allow us to create an instance of the
// abstract Operator class so we can test its concrete public methods in isolation.
class TestOperator : public Operator {
public:
    // Inherit the base class constructor
    using Operator::Operator;

    enum class CalledMethod {
        NONE,
        MESSAGE_INT,
        MESSAGE_FLOAT,
        MESSAGE_DOUBLE,
        PROCESS_DATA,
        CHANGE_PARAMS,
        RANDOM_INIT_UINT32,
        RANDOM_INIT_IDRANGE,
        TRAVERSE,
        ADD_CONNECTION_INTERNAL,
        REMOVE_CONNECTION_INTERNAL,
        MOVE_CONNECTION_INTERNAL

    };
    
    CalledMethod lastMethodCalled = CalledMethod::NONE;
    int lastIntParam = 0;
    int prevLastIntParam = 0; // needed for move connection method.
    float lastFloatParam = 0.0f;
    double lastDoubleParam = 0.0;
    uint32_t lastUint32Param = 0;
    Payload* lastPayloadParam = nullptr; 
    IdRange* lastRangeParam = nullptr;
    // --- Constructors ---
    // Constructor for programmatic creation (matches Operator(uint32_t id))
    // Also allows setting a type for tests that might need it (e.g., equality checks)
    TestOperator(uint32_t id, Type type = Operator::Type::UNDEFINED) 
        : Operator(id), mock_type(type) {}

    // Deserialization constructor (matches Operator(const std::byte*&, const std::byte*))
    TestOperator(const std::byte*& current, const std::byte* end) 
        : Operator(current, end), mock_type(Operator::Type::UNDEFINED) {
        // Note: The actual type would be read by Operator's deserialization constructor
        // if it were part of the base class's serialized data. Here, we just give it a default.
        // If the type is determined during base deserialization and stored, this might need adjustment.
        // For now, assume Operator base constructor doesn't set a readable 'type' member that getOpType() uses.
        // The pure virtual getOpType() means derived classes *must* define it.
    }

    // --- Mocked Methods ---
    // Pure virtual methods from Operator that need mocking or minimal implementation
    Operator::Type getOpType() const override { return Operator::Type::UNDEFINED; }
    void message(const int payloadData) override {
        lastMethodCalled = CalledMethod::MESSAGE_INT;
        lastIntParam = payloadData;
    }
    void message(const float payloadData) override {
        lastMethodCalled = CalledMethod::MESSAGE_FLOAT;
        lastFloatParam = payloadData;
    }
    void message(const double payloadData) override {
        lastMethodCalled = CalledMethod::MESSAGE_DOUBLE;
        lastDoubleParam = payloadData;
    }

    void processData() override {
        lastMethodCalled = CalledMethod::PROCESS_DATA;
    }
    void changeParams(const std::vector<int>& params) override {
        lastMethodCalled = CalledMethod::CHANGE_PARAMS;
    }
    void randomInit(uint32_t maxId, Randomizer* rng) override {
        lastMethodCalled = CalledMethod::RANDOM_INIT_UINT32;
        lastUint32Param = maxId;
    }
    void randomInit(IdRange* range, Randomizer* rng) override {
        lastMethodCalled = CalledMethod::RANDOM_INIT_IDRANGE;
        lastRangeParam = range; 
    }
    
    // Virtual methods from Operator that are not pure, but we want to mock for interaction testing
    // therefore call the base implementation as well
    void traverse(Payload* payload) override{
        lastMethodCalled = CalledMethod::TRAVERSE;
        lastPayloadParam = payload;
        Operator::traverse(payload);
    };
    void addConnectionInternal(uint32_t targetOperatorId, int distance) override {
        lastMethodCalled = CalledMethod::ADD_CONNECTION_INTERNAL;
        lastIntParam = distance;
        lastUint32Param= targetOperatorId;
        Operator::addConnectionInternal(targetOperatorId, distance);
    };
    void removeConnectionInternal(uint32_t targetOperatorId, int distance) override {
        lastMethodCalled = CalledMethod::REMOVE_CONNECTION_INTERNAL;
        lastIntParam = distance;
        lastUint32Param = targetOperatorId;
        Operator::removeConnectionInternal(targetOperatorId, distance);
    };
    void moveConnectionInternal(uint32_t targetOperatorId, int oldDistance, int newDistance) override {
        lastMethodCalled = CalledMethod::MOVE_CONNECTION_INTERNAL;
        prevLastIntParam = oldDistance;
        lastIntParam = newDistance;
        lastUint32Param = targetOperatorId;
        Operator::moveConnectionInternal(targetOperatorId, oldDistance, newDistance); 
    };

    // Correctly delegate to base class equals for comparing Operator properties
    bool equals(const Operator& other) const override {
        // This virtual equals is called when types are known to be compatible
        // OR it's part of the polymorphic comparison started by Operator::operator==.
        // Operator::operator== already checks getOpType().
        return Operator::equals(other);
    }

    // --- Correct Serialization Implementation ---

    /**
     * @brief [Override] Serializes the TestOperator to a self-contained, size-prefixed block.
     * @return A std::vector<std::byte> containing the full serialized object.
     * @details This method follows the correct pattern for subclass serialization:
     * 1. Call the base class Operator::serializeToBytes to get the raw data (no prefix).
     * 2. Append any derived-class-specific data (none for this test class).
     * 3. Prepend the final total size of the data block as a 4-byte prefix.
     */
    std::vector<std::byte> serializeToBytes() const override {
        // 1. Get the raw serialized data from the base Operator class.
        // no byte size prefix
        std::vector<std::byte> dataBuffer = Operator::serializeToBytes();

        // 2. A real subclass would append its own data here. TestOperator has none.

        // 3. Create the final buffer and prepend the 4-byte size of the data payload.
        std::vector<std::byte> finalBuffer;
        finalBuffer.reserve(4 + dataBuffer.size());
        Serializer::write(finalBuffer, static_cast<uint32_t>(dataBuffer.size()));
        finalBuffer.insert(finalBuffer.end(), dataBuffer.begin(), dataBuffer.end());

        return finalBuffer;
    }

    /**
     * @brief for checking the validing of base operator class serializer, without byte size prefix that subclass will append
     * @return A std::vector<std::byte> containing the serialized object, without byte size prefix.
     */
    std::vector<std::byte> baseClassSerializeToBytes() const {
        return Operator::serializeToBytes();
    }
    
    // This override is the key: it just calls the base class implementation directly,
    // allowing us to test Operator::toJson without any subclass interference.
    std::string toJson(bool prettyPrint, bool encloseInBrackets, int indentLevel = 0) const override {
        return Operator::toJson(prettyPrint, encloseInBrackets, indentLevel);
    }

private: 
    Operator::Type mock_type;
};