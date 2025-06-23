#include "gtest/gtest.h"
#include "headers/layers/OutputLayer.h"
#include "headers/util/IdRange.h"
#include "headers/operators/OutOperator.h" // For direct interaction and casting
#include "headers/util/Serializer.h"     // For crafting byte streams for deserialization tests
#include "headers/layers/LayerType.h"    // For LayerType enum
#include "headers/operators/Operator.h"  // For Operator::Type
#include "helpers/MockRandomizer.h"

#include <vector>
#include <memory>    // For std::unique_ptr
#include <stdexcept> // For std::runtime_error
#include <string>
#include <limits>    // For std::numeric_limits

// Test fixture for OutputLayer tests
class OutputLayerTest : public ::testing::Test {
protected:
    IdRange* defaultLayerRange;
    const int channelCount = 3; // As defined in OutputLayer.h
    const int textChannelIdOffset = 0;
    const int imgChannelIdOffset = 1;
    const int audioChannelIdOffset = 2;

    OutputLayerTest() {
        // Default range that is large enough for the default number of channels
        defaultLayerRange = new IdRange(100, 100 + channelCount - 1);
    }

    ~OutputLayerTest() override {
        delete defaultLayerRange;
    }

    // Helper to create a layer with a specific range
    // Takes ownership of the IdRange pointer
    OutputLayer* createLayer(IdRange* range, bool isFinal = false) {
        return new OutputLayer(isFinal, range);
    }

    // Helper to get the text OutOperator from a layer
    OutOperator* getTextOutOperator(OutputLayer* layer) {
        if (!layer || !layer->getReservedIdRange()) return nullptr;
        uint32_t textOpId = layer->getReservedIdRange()->getMinId() + textChannelIdOffset;
        Operator* op = layer->getOperator(textOpId);
        return dynamic_cast<OutOperator*>(op);
    }

    // Helper to convert int to char based on OutOperator's logic for testing getTextOutput
    char scaleIntToChar(int value) {
        if (value < 0) value = 0;
        constexpr int INT_VALUE_BITS = std::numeric_limits<int>::digits;
        constexpr int CHAR_BITS = 8;
        constexpr int SHIFT_AMOUNT = INT_VALUE_BITS - CHAR_BITS;

        if constexpr (SHIFT_AMOUNT > 0) {
            return static_cast<char>(static_cast<unsigned int>(value) >> SHIFT_AMOUNT);
        } else {
            return static_cast<char>(value & 0xFF);
        }
    }

    /**
     * @brief Calculates an integer that, when processed by OutOperator's scaling logic,
     * results in the target character. This makes tests robust to changes in the
     * scaling implementation.
     * @param target_char_val The character we want the OutOperator to produce.
     * @return The integer value to pass to `OutOperator::message()`.
     */
    int inputValueForChar(unsigned char target_char_val) {
        constexpr int INT_VALUE_BITS = std::numeric_limits<int>::digits;
        constexpr int CHAR_BITS = 8;
        // This logic mirrors the scaling performed in OutOperator::getDataAsString 
        constexpr int SHIFT_AMOUNT = (INT_VALUE_BITS > CHAR_BITS) ? (INT_VALUE_BITS - CHAR_BITS) : 0;

        if constexpr (SHIFT_AMOUNT > 0) {
            // Shift the char value left to find the corresponding input integer.
            return static_cast<int>(static_cast<unsigned int>(target_char_val) << SHIFT_AMOUNT);
        } else {
            // If int is 8 bits or less, no shift happens, so the value is direct.
            return static_cast<int>(target_char_val);
        }
    }
};

// --- Programmatic Constructor Tests (`OutputLayer(bool, IdRange*)`) ---

TEST_F(OutputLayerTest, Constructor_ValidRange_Dynamic) {
    // IdRange will be owned by the OutputLayer
    IdRange* customRange = new IdRange(50, 50 + channelCount - 1);
    bool isFinal = false;
    std::unique_ptr<OutputLayer> layer(createLayer(customRange, isFinal));

    EXPECT_EQ(layer->getLayerType(), LayerType::OUTPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinal);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), 50);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), 50 + channelCount - 1);
    EXPECT_FALSE(layer->isEmpty()); // Should have channels (operators)
    EXPECT_EQ(layer->getOpCount(), channelCount);

    // Verify operator IDs and types
    for (int i = 0; i < channelCount; ++i) {
        uint32_t expectedId = customRange->getMinId() + i;
        Operator* op = layer->getOperator(expectedId);
        ASSERT_NE(op, nullptr) << "Operator with ID " << expectedId << " should exist.";
        EXPECT_EQ(op->getId(), expectedId);
        OutOperator* outOp = dynamic_cast<OutOperator*>(op);
        ASSERT_NE(outOp, nullptr) << "Operator with ID " << expectedId << " should be an OutOperator.";
        EXPECT_EQ(outOp->getOpType(), Operator::Type::OUT);
    }
}

TEST_F(OutputLayerTest, Constructor_ValidRange_Static) {
    IdRange* customRange = new IdRange(10, 10 + channelCount - 1);
    bool isFinal = true;
    std::unique_ptr<OutputLayer> layer(createLayer(customRange, isFinal));

    EXPECT_EQ(layer->getLayerType(), LayerType::OUTPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinal);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), 10);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), 10 + channelCount - 1);
    EXPECT_FALSE(layer->isEmpty());
    EXPECT_EQ(layer->getOpCount(), channelCount);

    for (int i = 0; i < channelCount; ++i) {
        uint32_t expectedId = customRange->getMinId() + i;
        Operator* op = layer->getOperator(expectedId);
        ASSERT_NE(op, nullptr);
        EXPECT_NE(dynamic_cast<OutOperator*>(op), nullptr);
    }
}

TEST_F(OutputLayerTest, Constructor_RangeTooSmall_ThrowsException) {
    // Create a range that is too small for the channel count.
    // IdRange will be attempted to be owned by OutputLayer, and deleted by its destructor on throw.
    IdRange* smallRange = new IdRange(1, 1 + channelCount - 2); // Count is 2, expected 3
    bool isFinal = false;

    EXPECT_THROW({
        OutputLayer layer(isFinal, smallRange);
        // layer will go out of scope and delete smallRange if constructor succeeds
        // If constructor throws, OutputLayer's destructor handles smallRange
    }, std::runtime_error);
    // smallRange should be deleted by the OutputLayer destructor if it was assigned,
    // or if an exception is thrown within the Layer base class constructor before full OutputLayer construction.
    // If OutputLayer(isFinal, smallRange) itself throws and smallRange was passed, it should be deleted.
    // If the test framework catches the exception, smallRange might leak if not handled by OutputLayer's error path.
    // OutputLayer's constructor: `Layer(LayerType::OUTPUT_LAYER, initialReservedRange, isIdRangeFinal)`
    // Layer's constructor: `reservedRange = initialReservedRange;`
    // So, Layer owns it. If Layer constructor completes and then OutputLayer specific code throws, Layer destructor runs.
    // If `checkRange()` in OutputLayer constructor throws before `Layer` base is fully set,
    // then `initialReservedRange` (which is `smallRange`) needs to be managed.
    // OutputLayer constructor: `OutputLayer(bool isIdRangeFinal, IdRange* initialReservedRange): Layer(LayerType::OUTPUT_LAYER, initialReservedRange, isIdRangeFinal)`
    // The `Layer` base class takes ownership of `initialReservedRange`. If `OutputLayer` constructor throws (e.g. in `checkRange`),
    // the `Layer` destructor will be called and delete `reservedRange`.
}

TEST_F(OutputLayerTest, Constructor_RangeNull_ThrowsException) {
    bool isFinal = false;
    // Expecting a runtime_error because validate() or checkRange() will access reservedRange.
    // OutputLayer's `checkRange()` is called first.
    EXPECT_THROW({
        OutputLayer layer(isFinal, nullptr);
    }, std::runtime_error);
}

// --- Deserialization Constructor Tests (`OutputLayer(bool, const std::byte*&, const std::byte*)`) ---
TEST_F(OutputLayerTest, DeserializeConstructor_ValidData_Dynamic) {
    // ARRANGE
    bool isFinalFromFile = false;
    uint32_t rangeMin = 200;
    uint32_t rangeMax = rangeMin + channelCount - 1;

    // This vector holds the data for the layer payload, which Layer::deserialize() expects.
    std::vector<std::byte> payloadBytes;
    
    // 1. Serialize the layer's IdRange first.
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);

    // 2. Serialize each operator correctly.
    for (int i = 0; i < channelCount; ++i) {
        uint32_t opId = rangeMin + i;
        OutOperator opToSerialize(opId); // Create an operator instance.

        // For this test, give the text channel operator some data before serializing.
        if (i == textChannelIdOffset) {
            opToSerialize.message(inputValueForChar('H')); // Represents 'H'
            opToSerialize.message(inputValueForChar('I')); // Represents 'I'
        }

        // Get the complete, size-prefixed data block for the operator.
        std::vector<std::byte> operatorBlock = opToSerialize.serializeToBytes();

        // Append the entire valid block to the payload stream.
        payloadBytes.insert(payloadBytes.end(), operatorBlock.begin(), operatorBlock.end());
    }

    // ACT: Deserialize the correctly formatted byte stream.
    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();
    std::unique_ptr<OutputLayer> layer(new OutputLayer(isFinalFromFile, dataPtr, endPtr));

    // ASSERT: Verify the deserialized layer's properties.
    EXPECT_EQ(layer->getLayerType(), LayerType::OUTPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinalFromFile);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), rangeMin);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), rangeMax);
    EXPECT_EQ(layer->getOpCount(), channelCount);

    // Verify operator-specific details, particularly the data in the text channel.
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);
    EXPECT_TRUE(textOp->hasOutput());

    // Check the content from the text channel. The values 72 & 73 are small enough
    // not to be distorted by the scaling logic.
    std::string expectedText;
    expectedText += 'H';
    expectedText += 'I';
    EXPECT_EQ(layer->getTextOutput(), expectedText);
    EXPECT_FALSE(layer->hasTextOutput()) << "Buffer should be cleared after reading.";

    // Ensure the entire payload buffer was consumed.
    EXPECT_EQ(dataPtr, endPtr) << "Deserialization should consume all provided data.";
}

TEST_F(OutputLayerTest, DeserializeConstructor_RangeTooSmallInStream_Throws) {
    bool isFinalFromFile = false;
    uint32_t rangeMin = 300;
    // This range is too small (count = 1)
    uint32_t rangeMax = rangeMin;

    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);
    // No operator data needed as it should throw before/during operator processing.

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    // OutputLayer's constructor calls Layer::deserialize, then its own checkRange() and validate().
    // checkRange() or validate() should throw.
    EXPECT_THROW({
        OutputLayer layer(isFinalFromFile, dataPtr, endPtr);
    }, std::runtime_error);
}

TEST_F(OutputLayerTest, DeserializeConstructor_InsufficientDataForRange_Throws) {
    bool isFinalFromFile = false;
    std::vector<std::byte> payloadBytes;
    Serializer::write(payloadBytes, static_cast<uint32_t>(400)); // Only min_id, max_id is missing

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    // Layer::deserialize will throw due to insufficient data when reading the IdRange.
    EXPECT_THROW({
        OutputLayer layer(isFinalFromFile, dataPtr, endPtr);
    }, std::runtime_error);
    // Error message might be "Serializer::read_uint32 error: Insufficient data."
}

TEST_F(OutputLayerTest, DeserializeConstructor_MissingOperatorData_InitializesChannels) {
    bool isFinalFromFile = false;
    uint32_t rangeMin = 500;
    uint32_t rangeMax = rangeMin + channelCount - 1;

    std::vector<std::byte> payloadBytes;
    // 1. Serialize a valid IdRange
    Serializer::write(payloadBytes, rangeMin);
    Serializer::write(payloadBytes, rangeMax);
    // 2. *Do not* serialize any operator data.
    //    Layer::deserialize will read the range and find no operator blocks.

    const std::byte* dataPtr = payloadBytes.data();
    const std::byte* endPtr = dataPtr + payloadBytes.size();

    // OutputLayer's deserialization constructor:
    // - Calls Layer::deserialize(...); // reads range, finds no operators
    // - Calls checkRange(); // should pass
    // - Calls channelsSet(); // should return false because no operators were loaded
    // - Calls clearOperators();
    // - Calls initChannels(); // should create the 3 OutOperators
    // - Calls validate(); // should now pass
    std::unique_ptr<OutputLayer> layer(new OutputLayer(isFinalFromFile, dataPtr, endPtr));

    EXPECT_EQ(layer->getLayerType(), LayerType::OUTPUT_LAYER);
    EXPECT_EQ(layer->getIsRangeFinal(), isFinalFromFile);
    ASSERT_NE(layer->getReservedIdRange(), nullptr);
    EXPECT_EQ(layer->getReservedIdRange()->getMinId(), rangeMin);
    EXPECT_EQ(layer->getReservedIdRange()->getMaxId(), rangeMax);
    EXPECT_EQ(layer->getOpCount(), channelCount); // Should have been initialized

    // Verify that initChannels created the operators
    for (int i = 0; i < channelCount; ++i) {
        uint32_t expectedId = rangeMin + i;
        Operator* op = layer->getOperator(expectedId);
        ASSERT_NE(op, nullptr) << "Operator with ID " << expectedId << " should have been created by initChannels.";
        EXPECT_EQ(op->getId(), expectedId);
        ASSERT_NE(dynamic_cast<OutOperator*>(op), nullptr) << "Operator " << expectedId << " should be an OutOperator.";
    }
    // dataPtr should be advanced past the IdRange data.
    // Expected advancement: sizeof(rangeMin) + sizeof(rangeMax)
    EXPECT_EQ(dataPtr, payloadBytes.data() + sizeof(uint32_t) * 2);
}

// --- hasTextOutput() Tests ---

TEST_F(OutputLayerTest, HasTextOutput_NoOutput_ReturnsFalse) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    EXPECT_FALSE(layer->hasTextOutput());
}

TEST_F(OutputLayerTest, HasTextOutput_WithOutput_ReturnsTrue) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);

    textOp->message(123); // Add some data

    EXPECT_TRUE(layer->hasTextOutput());
}

TEST_F(OutputLayerTest, HasTextOutput_OutputCleared_ReturnsFalse) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);

    textOp->message(123); // Add some data
    ASSERT_TRUE(layer->hasTextOutput()); // Pre-condition

    layer->getTextOutput(); // This call should clear the OutOperator's internal buffer

    EXPECT_FALSE(layer->hasTextOutput());
}

TEST_F(OutputLayerTest, HasTextOutput_OtherChannelsWithOutput_NotAffected) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    
    // Get image operator (assuming imgChannelIdOffset = 1)
    uint32_t imgOpId = layer->getReservedIdRange()->getMinId() + imgChannelIdOffset;
    Operator* op = layer->getOperator(imgOpId);
    OutOperator* imgOp = dynamic_cast<OutOperator*>(op);
    ASSERT_NE(imgOp, nullptr);

    imgOp->message(456); // Add data to image channel

    // Text channel should still be empty
    EXPECT_FALSE(layer->hasTextOutput());

    // Add data to text channel now
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);
    textOp->message(123);

    EXPECT_TRUE(layer->hasTextOutput()); // Now text has output
    EXPECT_TRUE(imgOp->hasOutput()); // Image channel should also still have its output
}

// --- getTextOutput() Tests ---

TEST_F(OutputLayerTest, GetTextOutput_NoOutput_ReturnsEmptyString) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    EXPECT_EQ(layer->getTextOutput(), "");
    EXPECT_FALSE(layer->hasTextOutput()) << "Buffer should remain empty and cleared.";
}

TEST_F(OutputLayerTest, GetTextOutput_WithOutput_ReturnsCorrectStringAndClears) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);

    // Values that should map to 'H' and 'i' using scaleIntToChar helper
    // This requires knowing how OutOperator scales. Assuming scaleIntToChar is accurate.
    // For simplicity, let's use values that are already small enough to avoid complex scaling issues
    // if int is much larger than char.
    // Or, more robustly, use the scaleIntToChar to find appropriate input values.
    // Example: find an int X such that scaleIntToChar(X) == 'H'.
    // For now, let's assume direct ASCII values if they are small enough not to be drastically scaled.
    // OutOperator::getDataAsString uses: (static_cast<unsigned int>(non_negative_value) >> SHIFT_AMOUNT)
    // where SHIFT_AMOUNT = std::numeric_limits<int>::digits - 8.
    // To get 'H' (ASCII 72), we need an input `val_H` such that `(val_H >> SHIFT_AMOUNT) == 72`.
    // So, `val_H` could be `72 << SHIFT_AMOUNT`.
    constexpr int INT_VALUE_BITS = std::numeric_limits<int>::digits;
    constexpr int CHAR_BITS = 8;
    constexpr int SHIFT_AMOUNT = INT_VALUE_BITS - CHAR_BITS;

    int val_H = static_cast<int>(static_cast<unsigned int>('H') << SHIFT_AMOUNT);
    // Ensure val_H doesn't overflow int if 'H' is too large for the shift on a small int type
    // (unlikely for typical int sizes and ASCII values).
    if (SHIFT_AMOUNT > 0 && (static_cast<unsigned int>('H') > (static_cast<unsigned int>(std::numeric_limits<int>::max()) >> SHIFT_AMOUNT))) {
        val_H = std::numeric_limits<int>::max(); // or handle error, this is for test setup
    }


    int val_i = static_cast<int>(static_cast<unsigned int>('i') << SHIFT_AMOUNT);
    if (SHIFT_AMOUNT > 0 && (static_cast<unsigned int>('i') > (static_cast<unsigned int>(std::numeric_limits<int>::max()) >> SHIFT_AMOUNT))) {
        val_i = std::numeric_limits<int>::max();
    }


    textOp->message(val_H);
    textOp->message(val_i);

    ASSERT_TRUE(layer->hasTextOutput()); // Pre-condition: has output

    std::string expectedString = "";
    expectedString += scaleIntToChar(val_H); // Use our test fixture's helper
    expectedString += scaleIntToChar(val_i);

    EXPECT_EQ(layer->getTextOutput(), expectedString);
    EXPECT_FALSE(layer->hasTextOutput()) << "Buffer should be cleared after getTextOutput().";
    EXPECT_EQ(layer->getTextOutput(), "") << "Subsequent call should return empty string.";
}

TEST_F(OutputLayerTest, GetTextOutput_MultipleChars_ScalingTest) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);

    constexpr int INT_VALUE_BITS = std::numeric_limits<int>::digits;
    constexpr int CHAR_BITS = 8;
    constexpr int SHIFT_AMOUNT = INT_VALUE_BITS - CHAR_BITS;

    int val_A = static_cast<int>(static_cast<unsigned int>('A') << SHIFT_AMOUNT);
    int val_B = static_cast<int>(static_cast<unsigned int>('B') << SHIFT_AMOUNT);
    int val_C = static_cast<int>(static_cast<unsigned int>('C') << SHIFT_AMOUNT);
     // Handle potential overflow for test values on smaller int systems, though unlikely for A,B,C
    if (SHIFT_AMOUNT > 0) {
        if (static_cast<unsigned int>('A') > (static_cast<unsigned int>(std::numeric_limits<int>::max()) >> SHIFT_AMOUNT)) val_A = std::numeric_limits<int>::max();
        if (static_cast<unsigned int>('B') > (static_cast<unsigned int>(std::numeric_limits<int>::max()) >> SHIFT_AMOUNT)) val_B = std::numeric_limits<int>::max();
        if (static_cast<unsigned int>('C') > (static_cast<unsigned int>(std::numeric_limits<int>::max()) >> SHIFT_AMOUNT)) val_C = std::numeric_limits<int>::max();
    }


    textOp->message(val_A);
    textOp->message(val_B);
    textOp->message(val_C);

    std::string expected = "";
    expected += scaleIntToChar(val_A);
    expected += scaleIntToChar(val_B);
    expected += scaleIntToChar(val_C);

    EXPECT_EQ(layer->getTextOutput(), expected);
    EXPECT_FALSE(layer->hasTextOutput());
}

// --- randomInit(IdRange*, Randomizer*) Test ---
// OutputLayer::randomInit is documented as a no-op. This test confirms that.
// Also, OutOperator::randomInit (which would be called by Layer::randomInit if it iterated operators)
// is also a no-op.
// NOTE: This test was re-added as per user feedback for completeness, was step 6 in original plan.
TEST_F(OutputLayerTest, RandomInit_NoOp) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    MockRandomizer mockRandomizer; // Not strictly needed as method is empty, but good for verification
    IdRange connectionRange(2000, 2001); 

    int initialOpCount = layer->getOpCount();
    Operator* firstOpBefore = layer->getOperator(defaultLayerRange->getMinId());

    layer->randomInit(&connectionRange, &mockRandomizer);

    EXPECT_EQ(mockRandomizer.getIntCallCount(), 0);
    EXPECT_EQ(layer->getOpCount(), initialOpCount);
    EXPECT_EQ(layer->getOperator(defaultLayerRange->getMinId()), firstOpBefore);
}


// --- Serialization/Deserialization (Round Trip) Tests ---
// This is Step 6 in the revised plan (was 7).
TEST_F(OutputLayerTest, SerializeDeserialize_RoundTrip_Basic) {
    // 1. ARRANGE: Create and configure the original layer.
    IdRange* originalRange = new IdRange(770, 770 + channelCount - 1);
    std::unique_ptr<OutputLayer> originalLayer(createLayer(originalRange, false)); // Dynamic

    // Add data to its text channel using the inputValueForChar helper for robust testing.
    OutOperator* originalTextOp = getTextOutOperator(originalLayer.get());
    ASSERT_NE(originalTextOp, nullptr);
    originalTextOp->message(inputValueForChar('X'));
    originalTextOp->message(inputValueForChar('Y'));

    // 2. SERIALIZE: Get the full byte block for the original layer.
    std::vector<std::byte> serializedBytes = originalLayer->serializeToBytes();
    ASSERT_FALSE(serializedBytes.empty());

    // 3. DESERIALIZE: Mimic MetaController's process.
    const std::byte* dataPtr = serializedBytes.data();
    const std::byte* streamEnd = dataPtr + serializedBytes.size();

    // a. Read the layer "envelope" header.
    LayerType fileLayerType = static_cast<LayerType>(Serializer::read_uint8(dataPtr, streamEnd));
    bool fileIsRangeFinal = static_cast<bool>(Serializer::read_uint8(dataPtr, streamEnd));
    uint32_t payloadSize = Serializer::read_uint32(dataPtr, streamEnd);
    
    // b. Define the boundaries of the payload data.
    const std::byte* payloadEnd = dataPtr + payloadSize;
    ASSERT_LE(payloadEnd, streamEnd) << "Payload size in header exceeds available data.";

    // c. Construct the new layer from the payload data.
    OutputLayer deserializedLayer(fileIsRangeFinal, dataPtr, payloadEnd);

    // 4. VERIFY
    // a. Check that deserialization consumed the entire payload.
    EXPECT_EQ(dataPtr, payloadEnd) << "Deserialization did not consume the entire payload.";

    // b. Use the virtual operator== to compare the original and deserialized layers.
    EXPECT_EQ(*originalLayer, deserializedLayer) << "Original and deserialized layers are not equal.";

    // c. As a final check, verify the specific output text using the fixture's helper.
    std::string expectedText;
    // Use the correct helper function 'scaleIntToChar' from your test fixture.
    expectedText += scaleIntToChar(inputValueForChar('X'));
    expectedText += scaleIntToChar(inputValueForChar('Y'));
    
    std::string deserializedText = deserializedLayer.getTextOutput();
    EXPECT_EQ(deserializedText, expectedText);
    EXPECT_FALSE(deserializedLayer.hasTextOutput()) << "Text should be cleared after reading from deserialized layer.";
}


TEST_F(OutputLayerTest, GetTextOutput_NegativeValuesBecomeZeroCharEquivalent) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);

    textOp->message(-100);
    textOp->message(-1);

    // Negative values are treated as 0 by OutOperator::getDataAsString's scaling part
    std::string expected = "";
    expected += scaleIntToChar(0); // scaleIntToChar(-100) -> scaleIntToChar(0)
    expected += scaleIntToChar(0); // scaleIntToChar(-1)   -> scaleIntToChar(0)
    
    EXPECT_EQ(layer->getTextOutput(), expected);
    EXPECT_FALSE(layer->hasTextOutput());
}

TEST_F(OutputLayerTest, GetTextOutput_MaxValueInt) {
    std::unique_ptr<OutputLayer> layer(createLayer(new IdRange(*defaultLayerRange)));
    OutOperator* textOp = getTextOutOperator(layer.get());
    ASSERT_NE(textOp, nullptr);

    textOp->message(std::numeric_limits<int>::max());
    
    std::string expected = "";
    expected += scaleIntToChar(std::numeric_limits<int>::max());
    
    EXPECT_EQ(layer->getTextOutput(), expected);
    EXPECT_FALSE(layer->hasTextOutput());
}
