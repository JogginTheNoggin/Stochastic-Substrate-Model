#include "gtest/gtest.h"
#include "headers/layers/InternalLayer.h"
#include "headers/util/IdRange.h"
#include "helpers/MockRandomizer.h" // Will be created if not existing, or use existing
#include "helpers/JsonTestHelpers.h" // For potential future use or complex state verification
#include "headers/operators/AddOperator.h"
#include "helpers/TestOperator.h"      // For verifying operator interactions if needed
#include "headers/util/Serializer.h" // For manually crafting byte streams

#include <vector>
#include <memory> // For std::unique_ptr

// Test fixture for InternalLayer tests
class InternalLayerTest : public ::testing::Test {
protected:
    // Define a path for golden files if needed, e.g. for serialize/deserialize tests later
    // static const std::string GOLDEN_FILES_DIR;
    // GOLDEN_FILES_DIR = "../tests/unit_tests/LayerTests/InternalLayer/golden_files/";

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

    // Helper to create a simple byte stream for an empty layer for deserialization tests
    std::vector<std::byte> createEmptyLayerByteStream(LayerType type, bool isRangeFinal, uint32_t rangeMin, uint32_t rangeMax) {
        std::vector<std::byte> stream;
        // LayerType (1 byte)
        stream.push_back(static_cast<std::byte>(type));
        // isRangeFinal (1 byte)
        stream.push_back(static_cast<std::byte>(isRangeFinal));

        // Payload Size (4 bytes) - for reserved range only, no operators
        // Size of IdRange (min + max = 4 + 4 = 8 bytes)
        uint32_t payloadSize = 8;
        Serializer::write(stream, payloadSize);

        // ReservedRange (min ID, max ID)
        Serializer::write(stream, rangeMin); // minId for reserved range
        Serializer::write(stream, rangeMax); // maxId for reserved range

        return stream;
    }
};

// const std::string InternalLayerTest::GOLDEN_FILES_DIR = "../tests/unit_tests/LayerTests/InternalLayer/golden_files/";

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
    // Layer takes ownership of customRange, so it shouldn't be deleted here.
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
    // Layer takes ownership of customRange
}

TEST_F(InternalLayerTest, ProgrammaticConstructor_NullRange) {
    // Test behavior if a null IdRange is passed, though Layer constructor might handle/disallow this.
    // Based on Layer.h, IdRange* is taken, so it could be null.
    // Let's assume the Layer base class constructor handles the IdRange pointer correctly
    // (e.g. it might store it, or expect it to be valid).
    // If InternalLayer has specific logic for a null range, that should be tested.
    // For now, assume it's passed to Layer base.
    bool isFinal = false;
    InternalLayer layer(isFinal, nullptr);

    EXPECT_EQ(layer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(layer.getIsRangeFinal(), isFinal);
    EXPECT_EQ(layer.getReservedIdRange(), nullptr); // Expecting Layer to store the nullptr
    EXPECT_TRUE(layer.isEmpty());
    EXPECT_EQ(layer.getOpCount(), 0);
}

// --- randomInit Tests ---
TEST_F(InternalLayerTest, RandomInit_BasicInitialization_DynamicLayer) {
    IdRange* layerRange = new IdRange(100, 110); // Capacity for 11 operators (100-110 inclusive)
    InternalLayer dynamicLayer(false, layerRange); // isRangeFinal = false

    // MockRandomizer setup
    // 1. Number of operators to create: let's say 5. Max is 11.
    //    getInt(capacity / 2, capacity) -> getInt(5, 11)
    mockRandomizer->setNextInt(5);

    // 2. For each AddOperator's randomInit(connectionRange, randomizer):
    //    AddOperator::randomInit creates 0 to MAX_INITIAL_TARGETS_PER_OPERATOR connections.
    //    Let's assume it asks for number of connections (e.g. getInt(0, X))
    //    And then for each connection, it asks for target ID (e.g. getInt(min, max))
    //    And then for distance (e.g. getInt(min, max))
    // For simplicity in this test, we'll assume AddOperator::randomInit is called,
    // but won't deeply inspect its internals via mocks here. We'll check operator properties.
    // We will mock the calls that AddOperator's randomInit makes if needed.
    // For now, provide enough mock values if AddOperator's randomInit uses randomizer.
    // Assuming AddOperator::randomInit calls getInt for num_connections, targetId, distance.
    for (int i = 0; i < 5; ++i) { // For 5 operators
        mockRandomizer->setNextInt(1); // Each operator creates 1 connection
        mockRandomizer->setNextInt(200); // Target ID for that connection
        mockRandomizer->setNextInt(1);   // Distance for that connection
    }

    IdRange* connectionRange = new IdRange(200, 210); // External valid connection range
    dynamicLayer.randomInit(connectionRange, mockRandomizer);

    ASSERT_EQ(dynamicLayer.getOpCount(), 5);
    EXPECT_FALSE(dynamicLayer.isEmpty());

    const auto& operators = dynamicLayer.getAllOperators();
    uint32_t expectedId = 100;
    for (const auto& pair : operators) {
        Operator* op = pair.second;
        ASSERT_NE(op, nullptr);
        EXPECT_EQ(op->getId(), expectedId++);
        EXPECT_EQ(op->getOpType(), Operator::Type::ADD); // InternalLayer currently creates AddOperator

        // Verify that AddOperator's randomInit was indirectly effective
        // For example, check if it has connections (if TestOperator could expose this)
        // or rely on AddOperator's own tests. Here, we primarily test InternalLayer's role.
        // We can check the number of connections if the Operator class allows.
        // As Operator::getConnections is not public, we'd need a TestOperator or similar
        // to verify Operator::randomInit calls.
        // For now, we trust AddOperator::randomInit works if called.
    }
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 1 + (5 * 3)); // 1 for numOps, 3 per op for AddOp's randomInit

    delete connectionRange;
    // dynamicLayer owns layerRange
}


TEST_F(InternalLayerTest, RandomInit_StaticLayer_RespectsCapacity) {
    IdRange* layerRange = new IdRange(10, 12); // Capacity for 3 operators (10, 11, 12)
    InternalLayer staticLayer(true, layerRange); // isRangeFinal = true

    // MockRandomizer setup
    // Try to create 5 operators, but capacity is 3.
    // getInt(capacity / 2, capacity) -> getInt(1, 3)
    mockRandomizer->setNextInt(5); // Ask for 5, but should be capped by internal logic or generateNextId failure

    // Provide enough random numbers for the actual number of ops that will be created (3)
    for (int i = 0; i < 3; ++i) {
        mockRandomizer->setNextInt(0); // Each operator creates 0 connections for simplicity
    }

    IdRange* connectionRange = new IdRange(20, 30);
    staticLayer.randomInit(connectionRange, mockRandomizer);

    ASSERT_EQ(staticLayer.getOpCount(), 3); // Should only create up to capacity

    uint32_t expectedId = 10;
    for(const auto& pair : staticLayer.getAllOperators()){
        ASSERT_NE(pair.second, nullptr);
        EXPECT_EQ(pair.second->getId(), expectedId++);
    }
    // The loop in InternalLayer::randomInit breaks when generateNextId throws overflow
    // So, the randomizer calls for operator params might be less than 5.
    // It's 1 (for numOpsToCreate) + 3 * (1 for AddOp's num_connections)
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 1 + 3);


    delete connectionRange;
    // staticLayer owns layerRange
}

TEST_F(InternalLayerTest, RandomInit_ZeroOperatorsToCreate) {
    IdRange* layerRange = new IdRange(1, 5); // Capacity for 5
    InternalLayer layer(false, layerRange);

    // MockRandomizer setup: request 0 operators
    // getInt(capacity / 2, capacity) -> getInt(2, 5). Let's say it returns 2.
    // Then the internal logic of randomInit will be numOpsToCreate = randomizer->getInt(capacity / 2, capacity)
    // To test "zero" operators, we need to make getInt return 0 for the number of operators.
    // However, the call is randomizer->getInt(capacity / 2, capacity). Min is capacity/2.
    // So, if capacity > 1, it will always try to create at least 1.
    // If capacity is 1, it will try to create 1.
    // The only way numOpsToCreate is 0 is if capacity is 0.
    // Let's test with capacity 0.

    IdRange* zeroCapacityRange = new IdRange(1, 0); // Invalid range, count = 0
    InternalLayer zeroCapLayer(false, zeroCapacityRange);

    // No calls to randomizer should happen if capacity is 0.
    // Or, if it calls getInt(0,0), we set it to return 0.
    mockRandomizer->setNextInt(0); // For the getInt(0,0) call.

    IdRange* connectionRange = new IdRange(20, 30);
    zeroCapLayer.randomInit(connectionRange, mockRandomizer);

    EXPECT_EQ(zeroCapLayer.getOpCount(), 0);
    EXPECT_TRUE(zeroCapLayer.isEmpty());
    // If capacity is 0, numOpsToCreate = randomizer->getInt(0,0) -> let's say this is 0.
    // The loop for creating ops won't run.
    // So, getIntCallCount should be 1 if capacity is 0 and getInt(0,0) is called.
    // If capacity is 0, `numOpsToCreate` is assigned `capacity` (which is 0), so no call to randomizer.
    // The code is: int numOpsToCreate = (capacity > 1) ? randomizer->getInt(capacity / 2, capacity) : capacity;
    // If capacity = 0, numOpsToCreate = 0. No call to randomizer.
    // If capacity = 1, numOpsToCreate = 1. Call to randomizer for operator params.
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 0);


    delete connectionRange;
    // layer owns layerRange, zeroCapLayer owns zeroCapacityRange
}


TEST_F(InternalLayerTest, RandomInit_NullConnectionRange) {
    // Test how randomInit behaves if the global connectionRange is null.
    // Operators' randomInit should handle this.
    IdRange* layerRange = new IdRange(500, 502); // Capacity 3
    InternalLayer layer(false, layerRange);

    mockRandomizer->setNextInt(1); // Create 1 operator
    // AddOperator::randomInit will be called with null connectionRange.
    // It should handle this gracefully (e.g., create no connections).
    // We need to provide mocks for AddOperator's randomInit if it uses the randomizer
    // even with a null connection range (e.g. to decide number of connections = 0).
    mockRandomizer->setNextInt(0); // AddOperator creates 0 connections

    layer.randomInit(nullptr, mockRandomizer);

    EXPECT_EQ(layer.getOpCount(), 1);
    ASSERT_NE(layer.getOperator(500), nullptr);
    EXPECT_EQ(layer.getOperator(500)->getOpType(), Operator::Type::ADD);
    // Expect 1 call for numOpsToCreate, 1 for AddOp's num_connections
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 1 + 1);
}

// --- Deserialization Constructor Tests ---
TEST_F(InternalLayerTest, DeserializeConstructor_EmptyLayer_Dynamic) {
    bool isFinalFromFile = false; // This would be read by MetaController and passed in
    uint32_t rangeMin = 10, rangeMax = 20;

    // Construct the byte stream for the Layer's payload part (excluding Type and isRangeFinal flag from file header)
    // The InternalLayer deserialization constructor expects currentPayloadData to point to the start of its *payload*.
    // The Layer::deserialize method, which it calls, expects to read:
    // - ReservedRange (min, max)
    // - Then operators.
    // The `isRangeFinal` is passed directly to the constructor, not read from the stream by this constructor.
    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);
    // No operators in this stream.

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
    InternalLayer originalLayer(false, range); // Dynamic layer

    // Add an operator to make serialization non-trivial
    // Use TestOperator for simplicity, but an AddOperator would be more true to InternalLayer's current randomInit
    // However, for just testing Layer's serialization of type, any operator works.
    // The key is that Layer::serializeToBytes prepends its own type.
    TestOperator* op1 = new TestOperator(1001, Operator::Type::ADD);
    originalLayer.addNewOperator(op1);

    // Serialize the layer
    std::vector<std::byte> serializedBytes = originalLayer.serializeToBytes();
    ASSERT_FALSE(serializedBytes.empty());

    // --- Begin Deserialization Process (mimicking MetaController) ---
    const std::byte* dataPtr = serializedBytes.data();
    const std::byte* endPtr = dataPtr + serializedBytes.size();

    // 1. Read LayerType (as MetaController would from file header)
    LayerType fileLayerType = static_cast<LayerType>(*dataPtr);
    dataPtr++;
    ASSERT_EQ(fileLayerType, LayerType::INTERNAL_LAYER) << "Serialized LayerType is incorrect at the start of the stream.";

    // 2. Read isRangeFinal (as MetaController would from file header)
    bool fileIsRangeFinal = static_cast<bool>(*dataPtr);
    dataPtr++;
    ASSERT_EQ(fileIsRangeFinal, originalLayer.getIsRangeFinal()) << "Serialized isRangeFinal is incorrect.";

    // 3. Read payloadSize (as MetaController would)
    uint32_t payloadSize;
    memcpy(&payloadSize, dataPtr, sizeof(uint32_t)); // Assumes direct memory copy is safe here
    dataPtr += sizeof(uint32_t);

    // Boundary check for payload
    ASSERT_LE(dataPtr + payloadSize, endPtr) << "Payload size exceeds buffer.";

    // 4. Construct the layer using the payload
    // The InternalLayer constructor takes 'isRangeFinal' as an argument read previously.
    // It expects dataPtr to point to the beginning of its specific payload (which is after layer type, isfinal, payloadsize).
    // So, dataPtr is already correctly positioned by the reads above.
    const std::byte* payloadStartPtr = dataPtr;
    InternalLayer deserializedLayer(fileIsRangeFinal, payloadStartPtr, payloadStartPtr + payloadSize);

    // --- Verification ---
    EXPECT_EQ(deserializedLayer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(deserializedLayer.getIsRangeFinal(), originalLayer.getIsRangeFinal());
    ASSERT_NE(deserializedLayer.getReservedIdRange(), nullptr);
    EXPECT_EQ(deserializedLayer.getReservedIdRange()->getMinId(), originalLayer.getReservedIdRange()->getMinId());
    EXPECT_EQ(deserializedLayer.getReservedIdRange()->getMaxId(), originalLayer.getReservedIdRange()->getMaxId());

    EXPECT_EQ(deserializedLayer.getOpCount(), originalLayer.getOpCount());
    Operator* deserializedOp = deserializedLayer.getOperator(op1->getId());
    ASSERT_NE(deserializedOp, nullptr);
    EXPECT_EQ(deserializedOp->getId(), op1->getId());
    EXPECT_EQ(deserializedOp->getOpType(), op1->getOpType());
    // Note: op1 is owned by originalLayer and will be cleaned up by its destructor.
    // deserializedOp is owned by deserializedLayer.

    // Check if the pointer advanced correctly after Layer's deserialize consumed its part
    EXPECT_EQ(payloadStartPtr, endPtr) << "Layer deserialization did not consume the entire payload block.";
}

// --- Equality Tests (operator==) ---
// Helper to create a TestOperator easily for equality tests
Operator* createTestOp(uint32_t id) {
    return new TestOperator(id, Operator::Type::ADD); // Using ADD type as InternalLayer creates these
}

TEST_F(InternalLayerTest, Equality_IdenticalLayers) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(createTestOp(15));

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2);
    layer2.addNewOperator(createTestOp(15));

    EXPECT_TRUE(layer1 == layer2);
    EXPECT_FALSE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentIsRangeFinal) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1); // Dynamic

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(true, range2);  // Static

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentReservedRange) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);

    IdRange* range2 = new IdRange(30, 40); // Different range
    InternalLayer layer2(false, range2);

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentOperators_Count) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(createTestOp(15));

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2); // No operators

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentOperators_Id) {
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(createTestOp(15));

    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2);
    layer2.addNewOperator(createTestOp(16)); // Different operator ID

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}

TEST_F(InternalLayerTest, Equality_DifferentOperators_Type) {
    // This relies on Layer::equals comparing operator types if operators have same ID.
    // The base Operator::equals compares opType.
    IdRange* range1 = new IdRange(10, 20);
    InternalLayer layer1(false, range1);
    layer1.addNewOperator(new TestOperator(15, Operator::Type::ADD));


    IdRange* range2 = new IdRange(10, 20);
    InternalLayer layer2(false, range2);
    // To test this properly, we need an operator type that InternalLayer wouldn't normally create,
    // but can still be added via addNewOperator.
    layer2.addNewOperator(new TestOperator(15, Operator::Type::IN)); // Different op type

    EXPECT_FALSE(layer1 == layer2);
    EXPECT_TRUE(layer1 != layer2);
}


TEST_F(InternalLayerTest, Equality_DifferentLayerType) {
    // This test requires a way to compare with another Layer of a different concrete type.
    // We can use TestLayer for this if it allows setting a different LayerType.
    // Let's assume TestLayer.h exists and can be used for this.
    // #include "helpers/TestLayer.h" // Make sure this is included if used.
    // For now, we'll skip if TestLayer isn't readily usable for this specific comparison.

    // The base `bool operator==(const Layer& lhs, const Layer& rhs)` already checks
    // `lhs.getLayerType() == rhs.getLayerType()`. So, if types differ, it's false.
    // This means an InternalLayer will never be equal to an InputLayer, for example.
    // We can demonstrate this by trying to compare with a default-constructed Layer
    // if its type is different, or a TestLayer if available.

    // If we had a TestLayer:
    // TestLayer otherTypeLayer(LayerType::INPUT_LAYER, false, new IdRange(1,2));
    // InternalLayer internalLayer(false, new IdRange(1,2));
    // EXPECT_FALSE(internalLayer == otherTypeLayer);

    // Since direct instantiation of another type might be complex or not desired here,
    // and the base operator== handles type checking, we can trust that works.
    // A more direct test would be:
    class MockInputLayer : public Layer { // Minimal mock for type difference
    public:
        MockInputLayer(bool isFinal, IdRange* range) : Layer(LayerType::INPUT_LAYER, range, isFinal) {}
        void randomInit(IdRange*, Randomizer*) override {} // Dummy
    };

    IdRange* range1 = new IdRange(10, 20);
    InternalLayer internalL(false, range1);

    IdRange* range2 = new IdRange(10, 20); // Same range for fair comparison on other aspects
    MockInputLayer inputL(false, range2);

    // This relies on the global operator==(const Layer&, const Layer&)
    // which should first check getLayerType().
    EXPECT_FALSE(internalL == static_cast<const Layer&>(inputL));
    EXPECT_TRUE(internalL != static_cast<const Layer&>(inputL));
}

TEST_F(InternalLayerTest, DeserializeConstructor_EmptyLayer_Static) {
    bool isFinalFromFile = true; // Static
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
    EXPECT_EQ(layer.getOpCount(), 0);
    EXPECT_EQ(dataPtr, endPtr) << "Deserialization did not consume all provided data.";
}


TEST_F(InternalLayerTest, DeserializeConstructor_WithOneOperator) {
    bool isFinalFromFile = false;
    uint32_t layerRangeMin = 1, layerRangeMax = 10;

    // Create payload for Layer::deserialize
    std::vector<std::byte> layerPayloadBytes;
    Serializer::write(layerPayloadBytes, layerRangeMin); // ReservedRange Min
    Serializer::write(layerPayloadBytes, layerRangeMax); // ReservedRange Max

    // Create a TestOperator and serialize it (this will be part of layerPayloadBytes)
    // Operator serialization format (from Operator::serializeToBytes):
    // OpType (1 byte), OpId (4 bytes), NumDistBuckets (2 bytes), [Dist (2 bytes), NumTargets (2 bytes), Targets (4*N bytes)]*
    // The TestOperator::serializeToBytes() prepends total size (4 bytes) + then the above.
    // Layer::deserialize expects a stream of these blocks.

    TestOperator opToSerialize(5, Operator::Type::ADD); // ID 5, within layer range
    opToSerialize.addConnectionInternal(20, 1); // Add a connection to make serialization non-trivial

    // Get the operator data block *as Layer::deserialize expects it*:
    // Type (1), ID (4), isOverride (1), payload_size (4), payload_data(N)
    // This is what Operator::serializeToBytes(std::vector<std::byte>& buffer) in Layer.cpp writes.
    // Let's manually construct this part for an AddOperator.
    // The `Operator::serializeToBytes()` in `Operator.cpp` (base class version)
    // does: opType, opID, then connection data.
    // The `Layer::deserialize` loop reads:
    //   1. Operator Type (std::byte)
    //   2. Operator ID (uint32_t)
    //   3. isOverride (std::byte) - THIS IS NOT IN Operator.cpp's serializeToBytes()
    //   4. payloadSize (uint32_t) - THIS IS NOT IN Operator.cpp's serializeToBytes()
    //   5. payload (std::vector<std::byte>(payloadSize))
    // This indicates a mismatch or a specific format expected by Layer::deserialize.
    // The `Operator::serializeToBytes()` in `Operator.cpp` is `virtual std::vector<std::byte> serializeToBytes() const;`
    // This returns a buffer.
    // `Layer::serializeToBytes()` calls `op->serializeToBytes()` and prepends opType, opId, isOverride=0, size.
    // So, we need to mimic that structure for the test.

    std::vector<std::byte> operatorBlock;
    // 1. OperatorType
    operatorBlock.push_back(static_cast<std::byte>(Operator::Type::ADD));
    // 2. OperatorID
    Serializer::write(operatorBlock, opToSerialize.getId());
    // 3. isOverride (bool, 1 byte) - Assuming false for normal operators
    operatorBlock.push_back(static_cast<std::byte>(false));
    // 4. PayloadSize and Payload from operator's own serialization (excluding type and ID)
    //    The Operator::serializeToBytes() in Operator.cpp is the one that returns the actual "payload"
    //    for the Layer's serialization context. It includes connections.
    //    TestOperator::baseClassSerializeToBytes() gives us this.
    std::vector<std::byte> operatorInternalPayload = opToSerialize.baseClassSerializeToBytes();
    Serializer::write(operatorBlock, static_cast<uint32_t>(operatorInternalPayload.size())); // PayloadSize
    operatorBlock.insert(operatorBlock.end(), operatorInternalPayload.begin(), operatorInternalPayload.end()); // Payload

    // Append this operator block to the layer's payload
    layerPayloadBytes.insert(layerPayloadBytes.end(), operatorBlock.begin(), operatorBlock.end());

    const std::byte* dataPtr = layerPayloadBytes.data();
    const std::byte* endPtr = dataPtr + layerPayloadBytes.size();

    InternalLayer layer(isFinalFromFile, dataPtr, endPtr);

    EXPECT_EQ(layer.getLayerType(), LayerType::INTERNAL_LAYER);
    EXPECT_EQ(layer.getIsRangeFinal(), isFinalFromFile);
    ASSERT_NE(layer.getReservedIdRange(), nullptr);
    EXPECT_EQ(layer.getReservedIdRange()->getMinId(), layerRangeMin);
    EXPECT_EQ(layer.getReservedIdRange()->getMaxId(), layerRangeMax);

    ASSERT_EQ(layer.getOpCount(), 1);
    Operator* deserializedOp = layer.getOperator(opToSerialize.getId());
    ASSERT_NE(deserializedOp, nullptr);
    EXPECT_EQ(deserializedOp->getId(), opToSerialize.getId());
    EXPECT_EQ(deserializedOp->getOpType(), Operator::Type::ADD); // Deserialized as AddOperator

    // Check if connections were deserialized (requires inspecting deserializedOp)
    // This depends on AddOperator's deserialization logic and Operator base deserialization.
    // For now, primarily testing InternalLayer's ability to trigger this.
    // If OperatorJsonTests cover connection deserialization, we rely on that.
    // Here we check if the TestOperator's connection is present by comparing with original.
    // This requires operator== to be well-defined or checking specific connection counts.
    // The base Operator::equals compares connections.
    // We need to downcast to AddOperator to compare if it's truly an AddOperator.
    AddOperator* addOp = dynamic_cast<AddOperator*>(deserializedOp);
    ASSERT_NE(addOp, nullptr);

    // To properly check connections, we'd need to query 'addOp'.
    // For simplicity, we'll assume if the operator is there and of the right type,
    // Layer::deserialize did its job of calling the correct Operator constructor.

    EXPECT_EQ(dataPtr, endPtr) << "Deserialization did not consume all provided data.";
}
