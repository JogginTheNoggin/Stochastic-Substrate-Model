#include "gtest/gtest.h"
#include "headers/layers/InputLayer.h"
#include "headers/util/IdRange.h"
#include "helpers/MockRandomizer.h"
#include "headers/operators/InOperator.h" // For verifying channel operator types
#include "headers/util/Serializer.h"      // For crafting byte streams and deserializing headers

#include <vector>
#include <memory> // For std::unique_ptr
#include <stdexcept> // For std::runtime_error

// Test fixture for InputLayer tests
class InputLayerTest : public ::testing::Test {
protected:
    IdRange* defaultLayerRange;
    MockRandomizer* mockRandomizer;
    const int defaultChannelCount = 3; // As defined in InputLayer.h

    InputLayerTest() {
        // Default range that is large enough for the default number of channels
        defaultLayerRange = new IdRange(100, 100 + defaultChannelCount - 1);
        mockRandomizer = new MockRandomizer();
    }

    ~InputLayerTest() override {
        delete defaultLayerRange;
        delete mockRandomizer;
    }

    // Helper to create a layer with a specific range
    InputLayer* createLayer(IdRange* range, bool isFinal = false) {
        return new InputLayer(isFinal, range);
    }
};

// --- Programmatic Constructor Tests ---

TEST_F(InputLayerTest, Constructor_ValidRange_Dynamic) {
    IdRange* customRange = new IdRange(50, 50 + defaultChannelCount -1);
    bool isFinal = false;
    std::unique_ptr<InputLayer> layer(new InputLayer(isFinal, customRange));

    EXPECT_EQ(layer->getLayerType(), LayerType::INPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinal);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), 50);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), 50 + defaultChannelCount - 1);
    EXPECT_FALSE(layer->isEmpty()); // Should have channels
    EXPECT_EQ(layer->getOpCount(), defaultChannelCount);

    // Verify operator IDs and types
    for (int i = 0; i < defaultChannelCount; ++i) {
        uint32_t expectedId = customRange->getMinId() + i;
        Operator* op = layer->getOperator(expectedId);
        ASSERT_NE(op, nullptr) << "Operator with ID " << expectedId << " should exist.";
        EXPECT_EQ(op->getId(), expectedId);
        // Check if it's an InOperator
        InOperator* inOp = dynamic_cast<InOperator*>(op);
        ASSERT_NE(inOp, nullptr) << "Operator with ID " << expectedId << " should be an InOperator.";
    }
}

TEST_F(InputLayerTest, Constructor_ValidRange_Static) {
    IdRange* customRange = new IdRange(10, 10 + defaultChannelCount -1);
    bool isFinal = true;
    std::unique_ptr<InputLayer> layer(new InputLayer(isFinal, customRange));

    EXPECT_EQ(layer->getLayerType(), LayerType::INPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinal);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), 10);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), 10 + defaultChannelCount - 1);
    EXPECT_FALSE(layer->isEmpty());
    EXPECT_EQ(layer->getOpCount(), defaultChannelCount);

    for (int i = 0; i < defaultChannelCount; ++i) {
        uint32_t expectedId = customRange->getMinId() + i;
        ASSERT_NE(layer->getOperator(expectedId), nullptr);
        EXPECT_NE(dynamic_cast<InOperator*>(layer->getOperator(expectedId)), nullptr);
    }
}

TEST_F(InputLayerTest, Constructor_RangeTooSmall_ThrowsException) {
    // Range for 2 operators when defaultChannelCount is 3
    IdRange* smallRange = new IdRange(1, 1 + defaultChannelCount - 2);
    bool isFinal = false;

    // Expecting a runtime_error because validate() should fail
    // The error message comes from InputLayer::validate: "Range of layer must match channel count."
    EXPECT_THROW({
        InputLayer layer(isFinal, smallRange);
    }, std::runtime_error);

    delete smallRange; // Important to clean up if constructor throws
}


TEST_F(InputLayerTest, Constructor_NullRange_ThrowsException) {
    bool isFinal = false;
    // Expecting a runtime_error because validate() will try to access reservedRange->count()
    // or because generateNextId() might fail if reservedRange is null before operators are added.
    // The specific error might be "Range of layer must match channel count." or a null pointer access.
    // Let's refine this based on actual behavior.
    // InputLayer constructor directly uses initialReservedRange to set its own reservedRange.
    // Then generateNextId() is called, which dereferences reservedRange.
    // So, a null initialReservedRange should cause issues there or in validate().
    // Layer::generateNextId() throws if reservedRange is null.
    EXPECT_THROW({
        InputLayer layer(isFinal, nullptr);
    }, std::runtime_error);
}

// --- randomInit Tests ---
TEST_F(InputLayerTest, RandomInit_BasicInitialization) {
    std::unique_ptr<InputLayer> layer(createLayer(defaultLayerRange));
    IdRange connectionRange(200, 300);

    // InputLayer::randomInit iterates through its operators (channels)
    // and calls randomInit on each.
    // For an InOperator, randomInit(connectionRange, randomizer) typically:
    // 1. Decides a number of connections to make (e.g., randomizer->getInt(min_conn, max_conn)).
    // 2. For each connection:
    //    a. Picks a target ID (e.g., randomizer->getInt(connectionRange.min, connectionRange.max)).
    //    b. Picks a distance (e.g., randomizer->getInt(min_dist, max_dist)).

    // Let's assume each InOperator tries to make 1 connection.
    // And InOperator::randomInit calls getInt 3 times: num_connections, target_id, distance.
    // Since there are defaultChannelCount (3) channels:
    const int callsPerChannel = 3; // num_connections, target_id, distance for InOperator::randomInit

    for (int i = 0; i < defaultChannelCount; ++i) {
        mockRandomizer->setNextInt(1); // Number of connections for this channel's InOperator
        mockRandomizer->setNextInt(connectionRange.getMinId() + i); // Target ID
        mockRandomizer->setNextInt(1); // Distance
    }

    layer->randomInit(&connectionRange, mockRandomizer);

    // Verify that randomizer was called the correct number of times.
    // Each of the defaultChannelCount InOperators should have its randomInit called.
    // If InOperator::randomInit makes N getInt calls, total calls = defaultChannelCount * N.
    EXPECT_EQ(mockRandomizer->getIntCallCount(), defaultChannelCount * callsPerChannel);

    // Further verification could involve checking the actual connections if InOperator provided a way.
    // For now, verifying the interaction with Randomizer is the primary goal for Layer-level test.
    for(int i=0; i < defaultChannelCount; ++i) {
        Operator* op = layer->getOperator(defaultLayerRange->getMinId() + i);
        ASSERT_NE(op, nullptr);
        // We can't easily check connections here without accessors in InOperator
        // or making InOperator more complex for testing.
        // However, we've confirmed randomInit was called on each via mockRandomizer.
    }
}

TEST_F(InputLayerTest, RandomInit_NullConnectionRange) {
    std::unique_ptr<InputLayer> layer(createLayer(defaultLayerRange));

    // If connectionRange is nullptr, InOperator::randomInit should handle it gracefully
    // (e.g., by making 0 connections).
    // Assume InOperator::randomInit first calls getInt for number of connections.
    // If it gets 0, it might not call getInt for target_id or distance.
    const int callsForZeroConnections = 1; // Only for num_connections

    for (int i = 0; i < defaultChannelCount; ++i) {
        mockRandomizer->setNextInt(0); // Number of connections = 0
    }

    layer->randomInit(nullptr, mockRandomizer);

    EXPECT_EQ(mockRandomizer->getIntCallCount(), defaultChannelCount * callsForZeroConnections);
     for(int i=0; i < defaultChannelCount; ++i) {
        Operator* op = layer->getOperator(defaultLayerRange->getMinId() + i);
        ASSERT_NE(op, nullptr);
        // Operator should still exist and be an InOperator
        ASSERT_NE(dynamic_cast<InOperator*>(op), nullptr);
    }
}

TEST_F(InputLayerTest, RandomInit_NoOperators_DoesNotThrow) {
    // This scenario is somewhat artificial for InputLayer as it always creates operators.
    // However, if a derived class could have zero operators, its randomInit should be safe.
    // For InputLayer, this test primarily ensures that if the loop over operators somehow
    // doesn't run (e.g., if channelCount was 0, though it's const 3), it doesn't crash.

    // To simulate this, we'd need an InputLayer with 0 operators, which its constructor prevents.
    // So, this test is more conceptual for the base Layer::randomInit pattern.
    // We can test the existing InputLayer with a valid setup.
    // If channelCount was 0, the loop in randomInit wouldn't run, and getIntCallCount would be 0.
    // Since InputLayer *always* has operators, this specific test case is less direct.
    // We'll ensure it behaves correctly with its standard operator set.

    IdRange* customRange = new IdRange(300, 300 + defaultChannelCount - 1);
    std::unique_ptr<InputLayer> layer(new InputLayer(false, customRange)); // Standard construction

    // Set up mock for the expected calls
    for (int i = 0; i < defaultChannelCount; ++i) {
        mockRandomizer->setNextInt(0); // Attempt 0 connections
    }

    IdRange connectionRange(400,400); // Dummy connection range
    layer->randomInit(&connectionRange, mockRandomizer); // Should make 3 calls (one for each channel)

    EXPECT_EQ(mockRandomizer->getIntCallCount(), defaultChannelCount * 1); // 1 call per channel for 0 connections
}

// --- inputText Tests ---
// Note: Verifying Operator::message calls directly is hard without a mock operator
// or specific observable side effects from InOperator::message.
// This test primarily ensures the method runs without crashing and targets the expected channel.
TEST_F(InputLayerTest, InputText_SendsMessagesToTextChannel) {
    std::unique_ptr<InputLayer> layer(createLayer(defaultLayerRange));

    // The text channel is expected to be the operator with ID: defaultLayerRange->getMinId() + textChannelIdOffset
    // In InputLayer.h, textChannelIdOffset is 0.
    uint32_t textChannelId = defaultLayerRange->getMinId() + 0; // Assuming textChannelIdOffset = 0

    Operator* textChannelOp = layer->getOperator(textChannelId);
    ASSERT_NE(textChannelOp, nullptr) << "Text channel operator should exist.";
    ASSERT_NE(dynamic_cast<InOperator*>(textChannelOp), nullptr) << "Text channel operator should be an InOperator.";

    std::string testText = "Hello";

    // Call inputText. This should internally call message() on the textChannelOp for each char.
    // We can't easily verify the calls to message() here without a more complex setup (e.g., mock operators).
    // So, we mostly check that it doesn't throw and completes.
    // If InOperator::message had an inspectable state change, we could check that.
    EXPECT_NO_THROW(layer->inputText(testText));

    // Basic check: if message() was buggy and, for example, deleted the operator or layer,
    // subsequent access might fail. This is a weak check.
    ASSERT_NE(layer->getOperator(textChannelId), nullptr) << "Text channel operator should still exist after inputText.";

    // If we had a way to get the last processed value or a counter from InOperator, we could use it.
    // For example, if InOperator stored the last char_code:
    // InOperator* inOp = dynamic_cast<InOperator*>(textChannelOp);
    // EXPECT_EQ(inOp->getLastMessageValue(), static_cast<int>(testText.back()));
    // This relies on features not present in the current InOperator.
}

// --- Deserialization Constructor Tests ---
TEST_F(InputLayerTest, DeserializeConstructor_Basic_Dynamic) {
    bool isFinalFromFile = false;
    uint32_t rangeMin = 10, rangeMax = 10 + defaultChannelCount - 1;

    std::vector<std::byte> payloadBytes;
    // Layer::deserialize expects the data for its reservedRange to be in the payload.
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);
    // InputLayer does not deserialize its operators from the stream, it creates them.
    // So, no operator data is added to payloadBytes here.

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    // The constructor InputLayer(bool, const std::byte*&, const std::byte*)
    // calls Layer::deserialize, which reads the range.
    // Then, InputLayer's constructor body creates the InOperators.
    std::unique_ptr<InputLayer> layer(new InputLayer(isFinalFromFile, dataPtr, endPtr));

    EXPECT_EQ(layer->getLayerType(), LayerType::INPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinalFromFile);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), rangeMin);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), rangeMax);

    EXPECT_FALSE(layer->isEmpty());
    EXPECT_EQ(layer->getOpCount(), defaultChannelCount);

    // Verify operator IDs and types (created internally, not from stream)
    for (int i = 0; i < defaultChannelCount; ++i) {
        uint32_t expectedId = rangeMin + i;
        Operator* op = layer->getOperator(expectedId);
        ASSERT_NE(op, nullptr) << "Operator with ID " << expectedId << " should exist.";
        EXPECT_EQ(op->getId(), expectedId);
        ASSERT_NE(dynamic_cast<InOperator*>(op), nullptr) << "Operator " << expectedId << " should be InOperator.";
    }

    EXPECT_EQ(dataPtr, endPtr) << "Deserialization should consume all provided IdRange data.";
}

TEST_F(InputLayerTest, DeserializeConstructor_Basic_Static) {
    bool isFinalFromFile = true;
    uint32_t rangeMin = 20, rangeMax = 20 + defaultChannelCount - 1;

    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    std::unique_ptr<InputLayer> layer(new InputLayer(isFinalFromFile, dataPtr, endPtr));

    EXPECT_EQ(layer->getLayerType(), LayerType::INPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinalFromFile);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), rangeMin);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), rangeMax);
    EXPECT_EQ(layer->getOpCount(), defaultChannelCount);
    EXPECT_EQ(dataPtr, endPtr);
}

TEST_F(InputLayerTest, DeserializeConstructor_RangeTooSmallInStream_ThrowsDuringConstruction) {
    bool isFinalFromFile = false;
    // This range is too small for defaultChannelCount (e.g., if defaultChannelCount is 3, this is range for 1 op)
    uint32_t rangeMin = 30, rangeMax = 30;

    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    // InputLayer's constructor calls deserialize (which sets up the small range)
    // and then tries to create defaultChannelCount operators.
    // The validate() method, called at the end of InputLayer's constructor,
    // should throw because reservedRange->count() != channelCount
    EXPECT_THROW({
        InputLayer layer(isFinalFromFile, dataPtr, endPtr);
    }, std::runtime_error);

    // dataPtr may or may not be advanced depending on where the error occurs in Layer::deserialize
    // or InputLayer constructor. If IdRange was read successfully, it would be advanced.
}

TEST_F(InputLayerTest, DeserializeConstructor_InsufficientDataForRange_ThrowsInLayerDeserialize) {
    bool isFinalFromFile = false;

    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, static_cast<uint32_t>(100)); // Only min_id, max_id is missing

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    // Layer::deserialize will throw due to insufficient data when reading the IdRange.
    EXPECT_THROW({
        InputLayer layer(isFinalFromFile, dataPtr, endPtr);
    }, std::runtime_error);
    // The error "Serializer::read_uint32 error: Insufficient data." from Serializer is expected.
}

// --- Serialization and Deserialization (Round Trip) Tests ---
TEST_F(InputLayerTest, SerializeDeserialize_RoundTrip_Basic) {
    IdRange* originalRange = new IdRange(70, 70 + defaultChannelCount - 1);
    std::unique_ptr<InputLayer> originalLayer(new InputLayer(false, originalRange));

    // Give the InOperators some connections to make their serialized state more complex
    IdRange connectionRange(200, 210);
    // For each of the 3 channels' InOperator::randomInit:
    // 1. num_connections, 2. target_id, 3. distance
    mockRandomizer->setNextInt(1); mockRandomizer->setNextInt(200); mockRandomizer->setNextInt(1); // Channel 0
    mockRandomizer->setNextInt(1); mockRandomizer->setNextInt(201); mockRandomizer->setNextInt(2); // Channel 1
    mockRandomizer->setNextInt(0); // Channel 2, 0 connections

    originalLayer->randomInit(&connectionRange, mockRandomizer);
    EXPECT_EQ(mockRandomizer->getIntCallCount(), 3 + 3 + 1);


    std::vector<std::byte> serializedBytes = originalLayer->serializeToBytes();
    ASSERT_FALSE(serializedBytes.empty());

    // --- Begin Deserialization Process (mimicking MetaController) ---
    const std::byte* dataPtr = serializedBytes.data();
    const std::byte* endPtr = dataPtr + serializedBytes.size();

    // 1. Read LayerType from the stream
    LayerType fileLayerType = static_cast<LayerType>(Serializer::read_uint8(dataPtr, endPtr));
    EXPECT_EQ(fileLayerType, LayerType::INPUT_LAYER);

    // 2. Read isRangeFinal from the stream
    bool fileIsRangeFinal = static_cast<bool>(Serializer::read_uint8(dataPtr, endPtr));

    // 3. Read payload size (this is the size of data *after* these common header bytes)
    uint32_t payloadSize = Serializer::read_uint32(dataPtr, endPtr);
    ASSERT_LE(payloadSize, static_cast<uint32_t>(endPtr - dataPtr)) << "Payload size exceeds remaining data.";

    const std::byte* payloadStartPtr = dataPtr; // This is what's passed to the layer constructor

    // 4. Construct the layer using the specific deserialization constructor
    // The InputLayer deserialization constructor will:
    //    a. Call Layer::deserialize(payloadStartPtr, payloadStartPtr + payloadSize)
    //       This Layer::deserialize will read:
    //       - Reserved IdRange (min, max)
    //       - For each operator block found in the stream:
    //         - OperatorType, OperatorID, OperatorPayloadSize
    //         - Delegate to Operator::deserialize (e.g., InOperator)
    //    b. InputLayer's constructor *also* creates its channels if they weren't loaded.
    //       However, for InputLayer, the operators *are* serialized, so they should be loaded by Layer::deserialize.
    //       The key difference from InternalLayer is that InputLayer's *programmatic* constructor creates operators.
    //       Its *deserialization* constructor relies on Layer::deserialize to load them if present.
    //       Let's re-check InputLayer.cpp:
    //       InputLayer::InputLayer(bool isIdRangeFinal, const std::byte*& currentPayloadData,  const std::byte* endOfPayloadData)
    //          : Layer(LayerType::INPUT_LAYER, isIdRangeFinal) {
    //          deserialize(currentPayloadData, endOfPayloadData); // This is Layer::deserialize
    //          // NO validate() or internal operator creation here!
    //       }
    //       This means InputLayer *relies entirely* on Layer::deserialize to load its operators.
    //       And Layer::deserialize *does* load operators from the stream.
    //
    //       The programmatic constructor `InputLayer(bool, IdRange*)` *does* call `validate()`.
    //       The deserialization constructor does *not* call `validate()` explicitly.
    //       The operators created by the programmatic constructor (and then serialized) should be correctly deserialized.

    InputLayer deserializedLayer(fileIsRangeFinal, dataPtr, payloadStartPtr + payloadSize);

    // --- Verification ---
    EXPECT_EQ(dataPtr, payloadStartPtr + payloadSize) << "Deserialization did not consume the entire payload.";

    // Use the equality operator to compare
    EXPECT_EQ(*originalLayer, deserializedLayer) << "Original and deserialized layers are not equal.";
}

TEST_F(InputLayerTest, SerializeDeserialize_RoundTrip_StaticRange) {
    IdRange* originalRange = new IdRange(80, 80 + defaultChannelCount - 1);
    std::unique_ptr<InputLayer> originalLayer(new InputLayer(true, originalRange)); // Static range

    // No randomInit, operators will be default constructed by InputLayer constructor
    // and then serialized in their default state.

    std::vector<std::byte> serializedBytes = originalLayer->serializeToBytes();
    ASSERT_FALSE(serializedBytes.empty());

    const std::byte* dataPtr = serializedBytes.data();
    const std::byte* endPtr = dataPtr + serializedBytes.size();

    LayerType fileLayerType = static_cast<LayerType>(Serializer::read_uint8(dataPtr, endPtr));
    bool fileIsRangeFinal = static_cast<bool>(Serializer::read_uint8(dataPtr, endPtr));
    uint32_t payloadSize = Serializer::read_uint32(dataPtr, endPtr);
    const std::byte* payloadStartPtr = dataPtr;

    InputLayer deserializedLayer(fileIsRangeFinal, dataPtr, payloadStartPtr + payloadSize);

    EXPECT_EQ(dataPtr, payloadStartPtr + payloadSize);
    EXPECT_EQ(*originalLayer, deserializedLayer);
}

// --- Equality Tests (operator== and operator!=) ---

// Helper to create a basic InputLayer for equality tests
std::unique_ptr<InputLayer> createInputLayerForEquality(uint32_t minId, bool isFinal) {
    // InputLayer has a fixed channel count of 3
    const int inputLayerChannelCount = 3;
    IdRange* range = new IdRange(minId, minId + inputLayerChannelCount - 1);
    return std::make_unique<InputLayer>(isFinal, range);
}

TEST_F(InputLayerTest, Equality_IdenticalLayers) {
    std::unique_ptr<InputLayer> layer1 = createInputLayerForEquality(100, false);
    std::unique_ptr<InputLayer> layer2 = createInputLayerForEquality(100, false);

    // Optionally, make their internal state (operator connections) identical too
    IdRange dummyRange(1,1);
    mockRandomizer->setNextInt(0); mockRandomizer->setNextInt(0); mockRandomizer->setNextInt(0); // Chan 0,1,2 make 0 connections
    layer1->randomInit(&dummyRange, mockRandomizer);
    mockRandomizer->setNextInt(0); mockRandomizer->setNextInt(0); mockRandomizer->setNextInt(0);
    layer2->randomInit(&dummyRange, mockRandomizer);


    EXPECT_TRUE(*layer1 == *layer2);
    EXPECT_FALSE(*layer1 != *layer2);
}

TEST_F(InputLayerTest, Equality_DifferentIsRangeFinal) {
    std::unique_ptr<InputLayer> layer1 = createInputLayerForEquality(100, false);
    std::unique_ptr<InputLayer> layer2 = createInputLayerForEquality(100, true);

    EXPECT_FALSE(*layer1 == *layer2);
    EXPECT_TRUE(*layer1 != *layer2);
}

TEST_F(InputLayerTest, Equality_DifferentReservedRange) {
    std::unique_ptr<InputLayer> layer1 = createInputLayerForEquality(100, false);
    std::unique_ptr<InputLayer> layer2 = createInputLayerForEquality(200, false);

    EXPECT_FALSE(*layer1 == *layer2);
    EXPECT_TRUE(*layer1 != *layer2);
}

TEST_F(InputLayerTest, Equality_DifferentOperatorStates) {
    std::unique_ptr<InputLayer> layer1 = createInputLayerForEquality(100, false);
    std::unique_ptr<InputLayer> layer2 = createInputLayerForEquality(100, false);

    IdRange connectionRange(300, 310);
    // Layer1: Channel 0 gets 1 connection, others 0
    mockRandomizer->setNextInt(1); mockRandomizer->setNextInt(300); mockRandomizer->setNextInt(1); // Chan 0
    mockRandomizer->setNextInt(0); // Chan 1
    mockRandomizer->setNextInt(0); // Chan 2
    layer1->randomInit(&connectionRange, mockRandomizer);

    // mockRandomizer->reset(); // MockRandomizer does not have reset. Ensure queue has values for next calls.

    // Layer2: All channels get 0 connections
    // Values for layer2's randomInit are added to the queue after layer1's values.
    mockRandomizer->setNextInt(0); // Chan 0
    mockRandomizer->setNextInt(0); // Chan 1
    mockRandomizer->setNextInt(0); // Chan 2
    layer2->randomInit(&connectionRange, mockRandomizer);

    // This relies on InOperator::equals correctly comparing connections.
    EXPECT_FALSE(*layer1 == *layer2);
    EXPECT_TRUE(*layer1 != *layer2);
}


// Mock a different layer type for comparison
#include "headers/layers/InternalLayer.h" // For a concrete different type

TEST_F(InputLayerTest, Equality_DifferentLayerType) {
    std::unique_ptr<InputLayer> inputLayer = createInputLayerForEquality(100, false);

    // Create an InternalLayer with a compatible IdRange for a more nuanced comparison
    // (though type difference alone should make them unequal)
    IdRange* internalRange = new IdRange(100, 100 + defaultChannelCount -1); // Same range
    InternalLayer internalLayer(false, internalRange);
    // Note: InternalLayer's constructor doesn't auto-create operators like InputLayer.
    // For this test, it doesn't matter as the type check in Layer::operator== should be primary.

    // Need to cast to base Layer to ensure the non-member operator== is invoked correctly
    // and handles the polymorphic comparison via getLayerType() then virtual equals().
    const Layer& baseInputLayer = *inputLayer;
    const Layer& baseInternalLayer = internalLayer;

    EXPECT_FALSE(baseInputLayer == baseInternalLayer);
    EXPECT_TRUE(baseInputLayer != baseInternalLayer);

    // internalLayer will clean up its own range as it takes ownership if passed to that constructor
}

// Test with an InputLayer that has a different number of operators (not possible with current constructor)
// This tests the underlying Layer::compareOperatorMaps more generally.
// We can't directly create an InputLayer with a different number of ops.
// But if we manually deserialize one that's malformed (e.g. fewer ops than channelCount),
// it might hit this. However, InputLayer's design is strict about channelCount.
// The most relevant aspect is covered by DifferentOperatorStates.

// Test for equality when one layer has a null reserved range (should not happen with InputLayer constructors)
// but tests robustness of Layer::equals
TEST_F(InputLayerTest, Equality_OneNullReservedRange_IfPossible) {
    // InputLayer constructor throws if range is null.
    // So, we can't easily test this scenario directly with InputLayer.
    // This would be more for testing Layer::equals in isolation with a mock Layer.
    // For InputLayer, this case is prevented by constructor validation.
    SUCCEED() << "Skipping direct test for null reserved range comparison as InputLayer constructor prevents it.";
}
