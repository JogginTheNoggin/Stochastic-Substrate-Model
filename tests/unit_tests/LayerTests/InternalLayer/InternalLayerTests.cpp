#include "gtest/gtest.h"
#include "headers/layers/InternalLayer.h"
#include "headers/util/IdRange.h"
#include "helpers/MockRandomizer.h" // Will be created if not existing, or use existing
#include "helpers/JsonTestHelpers.h" // For potential future use or complex state verification
#include "headers/operators/AddOperator.h"
#include "headers/operators/InOperator.h" // Needed for equality test
#include "helpers/TestOperator.h"       // For verifying operator interactions if needed
#include "headers/util/Serializer.h" // For manually crafting byte streams

#include <vector>
#include <memory> // For std::unique_ptr
#include <set>    // For robust iteration checking

// Test fixture for InternalLayer tests
class InternalLayerTest : public ::testing::Test {
protected:
    IdRange* defaultLayerRange;
    IdRange* defaultConnectionRange;
    MockRandomizer* mockRandomizer;

    InternalLayerTest() {
        defaultLayerRange = new IdRange(100, 200);
        defaultConnectionRange = new IdRange(100, 300); // Example connection range
        mockRandomizer = new MockRandomizer();
    }

    ~InternalLayerTest() override {
        delete defaultLayerRange;
        delete defaultConnectionRange;
        delete mockRandomizer;
    }
};

namespace {
    // Define a path for golden files, this is how OperatorJsonTests.cpp does it.
    std::string MOCK_GOLDEN_FILES_DIR = "../tests/unit_tests/LayerTests/InternalLayer/golden_files/";
}

// --- Programmatic Constructor Tests ---
TEST_F(InternalLayerTest, ProgrammaticConstructor_SetsTypeAndRange_Dynamic) {
    IdRange* customRange = new IdRange(50, 150);
    bool isFinal = false;
    InternalLayer layer(isFinal, customRange);

    EXPECT_EQ(layer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(layer.getIsRangeFinal(), isFinal);
    ASSERT_NE(layer.getReservedIdRange(), nullptr);
    EXPECT_EQ(layer.getReservedIdRange()->getMinId(), 50);
    EXPECT_EQ(layer.getReservedIdRange()->getMaxId(), 150);
    EXPECT_TRUE(layer.isEmpty());
    EXPECT_EQ(layer.getOpCount(), 0);
}

TEST_F(InternalLayerTest, ProgrammaticConstructor_SetsTypeAndRange_Static) {
    IdRange* customRange = new IdRange(10, 20);
    bool isFinal = true;
    InternalLayer layer(isFinal, customRange);

    EXPECT_EQ(layer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(layer.getIsRangeFinal(), isFinal);
    ASSERT_NE(layer.getReservedIdRange(), nullptr);
    EXPECT_EQ(layer.getReservedIdRange()->getMinId(), 10);
    EXPECT_EQ(layer.getReservedIdRange()->getMaxId(), 20);
    EXPECT_TRUE(layer.isEmpty());
    EXPECT_EQ(layer.getOpCount(), 0);
}

TEST_F(InternalLayerTest, ProgrammaticConstructor_NullRange) {
    bool isFinal = false;
    InternalLayer layer(isFinal, nullptr);

    EXPECT_EQ(layer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(layer.getIsRangeFinal(), isFinal);
    EXPECT_EQ(layer.getReservedIdRange(), nullptr);
    EXPECT_TRUE(layer.isEmpty());
    EXPECT_EQ(layer.getOpCount(), 0);
}

// --- randomInit Tests ---
// TODO currently only works with AddOperator given the number of values needed to generate
// TODO add support for TestOperator if possible instead of relying on AddOperator (not essential)
TEST_F(InternalLayerTest, RandomInit_BasicInitialization_DynamicLayer) {
    IdRange* layerRange = new IdRange(100, 110);
    InternalLayer dynamicLayer(false, layerRange);

    mockRandomizer->setNextInt(5); // 1. Create 5 operators.

    // 2. For each AddOperator's randomInit, provide mocks for all random calls.
    for (int i = 0; i < 5; ++i) {
        mockRandomizer->setNextInt(10);  // for threshold
        mockRandomizer->setNextInt(20);  // for weight
        mockRandomizer->setNextInt(1);   // for number of connections
        mockRandomizer->setNextInt(200); // for target ID
        mockRandomizer->setNextInt(1);   // for distance
    }

    IdRange* connectionRange = new IdRange(200, 210);
    dynamicLayer.randomInit(connectionRange, mockRandomizer);

    ASSERT_EQ(dynamicLayer.getOpCount(), 5);
    EXPECT_FALSE(dynamicLayer.isEmpty());

    // Verify operator IDs without assuming iteration order
    std::set<uint32_t> expectedIds = {100, 101, 102, 103, 104};
    const auto& operators = dynamicLayer.getAllOperators();
    for (const auto& pair : operators) {
        EXPECT_EQ(expectedIds.count(pair.first), 1) << "Unexpected operator ID found: " << pair.first;
        expectedIds.erase(pair.first);
        ASSERT_NE(pair.second, nullptr);
        EXPECT_EQ(pair.second->getOpType(), Operator::Type::ADD);
    }
    EXPECT_TRUE(expectedIds.empty()) << "Not all expected operator IDs were created.";

    // 1 call for numOps, 5 calls per operator
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 1 + (5 * 5));

    delete connectionRange;
}


// TODO currently only works with AddOperator given the number of values needed to generate
// TODO add support for TestOperator if possible instead of relying on AddOperator (not essential)
TEST_F(InternalLayerTest, RandomInit_StaticLayer_RespectsCapacity) {
    IdRange* layerRange = new IdRange(10, 12);
    InternalLayer staticLayer(true, layerRange);

    mockRandomizer->setNextInt(3);

    for (int i = 0; i < 3; ++i) {
        mockRandomizer->setNextInt(10); // for threshold
        mockRandomizer->setNextInt(20); // for weight
        mockRandomizer->setNextInt(0);  // for number of connections
    }

    IdRange* connectionRange = new IdRange(20, 30);
    staticLayer.randomInit(connectionRange, mockRandomizer);

    ASSERT_EQ(staticLayer.getOpCount(), 3);

    // Check that the created operator IDs are correct without assuming order
    std::set<uint32_t> expectedIds = {10, 11, 12};
    const auto& operators = staticLayer.getAllOperators();
    for(const auto& pair : operators){
        ASSERT_NE(pair.second, nullptr);
        EXPECT_EQ(expectedIds.count(pair.first), 1);
    }
    
    // 1 call for numOps, 3 calls per operator
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 1 + (3 * 3));

    delete connectionRange;
}


TEST_F(InternalLayerTest, RandomInit_NullConnectionRange) {
    IdRange* layerRange = new IdRange(500, 502);
    InternalLayer layer(false, layerRange);

    mockRandomizer->setNextInt(1); // Create 1 operator

    // Provide mocks for AddOperator::randomInit calls.
    // It will still randomize threshold and weight.
    mockRandomizer->setNextInt(10); // Mock for threshold
    mockRandomizer->setNextInt(20); // Mock for weight
    // It then tries to create connections. If it asks for 0, it won't access the null range.
    mockRandomizer->setNextInt(0);  // Mock for connectionsToAttempt = 0

    layer.randomInit(nullptr, mockRandomizer);

    EXPECT_EQ(layer.getOpCount(), 1);
    ASSERT_NE(layer.getOperator(500), nullptr);
    EXPECT_EQ(layer.getOperator(500)->getOpType(), Operator::Type::ADD);
    
    // Expected calls: 1 (numOps) + 3 for the operator (thresh, weight, num_conn)
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 1 + 3);
}

// --- Deserialization Constructor Tests ---
TEST_F(InternalLayerTest, DeserializeConstructor_EmptyLayer_Dynamic) {
    bool isFinalFromFile = false;
    uint32_t rangeMin = 10, rangeMax = 20;

    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    InternalLayer layer(isFinalFromFile, dataPtr, endPtr);

    EXPECT_EQ(layer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(layer.getIsRangeFinal(), isFinalFromFile);
    ASSERT_NE(layer.getReservedIdRange(), nullptr);
    EXPECT_EQ(layer.getReservedIdRange()->getMinId(), rangeMin);
    EXPECT_EQ(layer.getReservedIdRange()->getMaxId(), rangeMax);
    EXPECT_TRUE(layer.isEmpty());
    EXPECT_EQ(layer.getOpCount(), 0);
    EXPECT_EQ(dataPtr, endPtr) << "Deserialization did not consume all provided data.";
}

// --- serializeToBytes Tests ---
TEST_F(InternalLayerTest, SerializeToBytes_ThenDeserialize_TypeIntegrity) {
    IdRange* range = new IdRange(1000, 1010);
    InternalLayer originalLayer(false, range);

    AddOperator* op1 = new AddOperator(1001);
    originalLayer.addNewOperator(op1);

    std::vector<std::byte> serializedBytes = originalLayer.serializeToBytes();
    ASSERT_FALSE(serializedBytes.empty());

    // --- Begin Deserialization Process ---
    const std::byte* dataPtr = serializedBytes.data();
    const std::byte* endPtr = dataPtr + serializedBytes.size();

    LayerType fileLayerType = static_cast<LayerType>(Serializer::read_uint8(dataPtr, endPtr));
    bool fileIsRangeFinal = static_cast<bool>(Serializer::read_uint8(dataPtr, endPtr));
    uint32_t payloadSize = Serializer::read_uint32(dataPtr, endPtr);
    
    ASSERT_LE(dataPtr + payloadSize, endPtr);

    const std::byte* payloadStartPtr = dataPtr;
    InternalLayer deserializedLayer(fileIsRangeFinal, payloadStartPtr, payloadStartPtr + payloadSize);

    // --- Verification ---
    EXPECT_EQ(originalLayer, deserializedLayer);
    EXPECT_EQ(payloadStartPtr, endPtr);
}

// --- Equality Tests (operator==) ---
Operator* createAddOp(uint32_t id) {
    return new AddOperator(id);
}

TEST_F(InternalLayerTest, Equality_IdenticalLayers) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(createAddOp(15));

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2);
    layer2.addNewOperator(createAddOp(15));

    EXPECT_TRUE(layer1 == layer2);
    EXPECT_FALSE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentIsRangeFinal) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(true, range2);

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentReservedRange) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);

    IdRange* range2 = new IdRange(30, 40);
    InternalLayer layer2(false, range2);

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentOperators_Count) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(createAddOp(15));

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2);

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentOperators_Id) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(createAddOp(15));

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2);
    layer2.addNewOperator(createAddOp(16));

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentOperators_Type) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(new AddOperator(15));

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2);
    layer2.addNewOperator(new InOperator(15));

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentLayerType) {
    class MockInputLayer : public Layer {
    public:
        MockInputLayer(bool isFinal, IdRange* range) : Layer(LayerType::INPUT_LAYER, range, isFinal) {}
        void randomInit(IdRange*, Randomizer*) override {}
    };

    IdRange* range1 = new IdRange(10, 20);
    InternalLayer internalL(false, range1);

    IdRange* range2 = new IdRange(10, 20);
    MockInputLayer inputL(false, range2);

    EXPECT_FALSE(internalL == static_cast<const Layer&>(inputL));
    EXPECT_TRUE(internalL != static_cast<const Layer&>(inputL));
}

TEST_F(InternalLayerTest, DeserializeConstructor_EmptyLayer_Static) {
    bool isFinalFromFile = true;
    uint32_t rangeMin = 100, rangeMax = 120;

    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    InternalLayer layer(isFinalFromFile, dataPtr, endPtr);

    EXPECT_EQ(layer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(layer.getIsRangeFinal(), isFinalFromFile);
    ASSERT_NE(layer.getReservedIdRange(), nullptr);
    EXPECT_EQ(layer.getReservedIdRange()->getMinId(), rangeMin);
    EXPECT_EQ(layer.getReservedIdRange()->getMaxId(), rangeMax);
    EXPECT_TRUE(layer.isEmpty());
}

TEST_F(InternalLayerTest, DeserializeConstructor_WithOneOperator) {
    bool isFinalFromFile = false;
    uint32_t layerRangeMin = 1, layerRangeMax = 10;

    std::vector<std::byte> layerPayloadBytes;
    Serializer::write(layerPayloadBytes, layerRangeMin);
    Serializer::write(layerPayloadBytes, layerRangeMax);

    AddOperator opToSerialize(5);
    opToSerialize.addConnectionInternal(20, 1);
    
    std::vector<std::byte> operatorBlock = opToSerialize.serializeToBytes();
    layerPayloadBytes.insert(layerPayloadBytes.end(), operatorBlock.begin(), operatorBlock.end());

    const std::byte* dataPtr = layerPayloadBytes.data();
    const std::byte* endPtr = dataPtr + layerPayloadBytes.size();

    InternalLayer layer(isFinalFromFile, dataPtr, endPtr);

    ASSERT_EQ(layer.getOpCount(), 1);
    Operator* deserializedOp = layer.getOperator(opToSerialize.getId());
    ASSERT_NE(deserializedOp, nullptr);
    
    EXPECT_TRUE(*deserializedOp == opToSerialize);
    EXPECT_EQ(dataPtr, endPtr);
}
