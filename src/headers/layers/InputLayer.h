#pragma once

#include "Layer.h" // Include the abstract base class

// Forward declarations
class Randomizer;
struct IdRange;

class InputLayer : public Layer {
private:
    const int channelCount = 3; // max number of channels, reserved range must match this, no extra space
    // operator ids corresponding to respective output channel types
    
    /** 
     *  Each operator in this layer represents a channel.
     *  1) text
     *  2) image
     *  3) audio
     */
    const int textChannelIdOffset = 0;
    const int imgChannelIdOffset = 1;
    const int audioChannelIdOffset = 2;

    // preforms checks on layer to ensure contains right amount of channels and reserved range matches this
    void validate(); 

    /**
     * @brief for checking if the operator is of type InputOperator
     */
    void checkType(Operator* op);

public:
    /**
     * @brief Constructor for programmatic creation.
     * @param isRangeFinal Defines if the layer's ID range is static or dynamic.
     * @param initialReservedRange The pre-allocated ID range for this layer.
     */
    InputLayer(bool isRangeFinal, IdRange* initialReservedRange);

    /**
     * @brief Constructor for deserialization.
     * @param isRangeFinal The dynamic status read from the file header by MetaController.
     * @param currentPayloadData Pointer to the start of this layer's payload.
     * @param endOfPayloadData Boundary of this layer's payload.
     */
    InputLayer(bool isRangeFinal,  const std::byte*& currentPayloadData, const std::byte* endOfPayloadData);

    /**
     * @brief Implements the random initialization logic specific to an Internal Layer.
     *
     * This method will:
     * 1. Create a random number of operators (e.g., AddOperator) within its own 'reservedRange',
     * using its internal `generateNextId()` method.
     * 2. For each operator within this layer, it will call its own randomInit
     * 3. The targets for these connections will be randomly chosen from the IDs within the
     * `validConnectionRange` parameter. This allows an internal layer operator to connect
     * to other internal operators or to output layer operators.
     *
     * @param randomizer A reference to a Randomizer utility to provide random numbers.
     * @param validConnectionRange The global ID range this layer's operators are permitted to connect to.
     * This range is determined by MetaController (e.g., by combining the ID ranges of all
     * internal and output layers).
     */
    void randomInit(IdRange* validConnectionRange, Randomizer* randomizer) override;

    /**
     * @brief method to submit text into the layer, via its text channel (an operator)
     * @param text the text to be submitted into the text Operator channel
     */
    void inputText(std::string text); 

};