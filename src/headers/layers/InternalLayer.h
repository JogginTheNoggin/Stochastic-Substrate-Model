#pragma once

#include "Layer.h" // Include the abstract base class

// Forward declarations
class Randomizer;
struct IdRange;

class InternalLayer : public Layer {
public:
    /**
     * @brief Constructor for programmatic creation.
     * @param isRangeFinal Defines if the layer's ID range is static or dynamic.
     * @param initialReservedRange The pre-allocated ID range for this layer.
     */
    InternalLayer(bool isRangeFinal, IdRange* initialReservedRange);

    /**
     * @brief Constructor for deserialization.
     * @param isRangeFinal The dynamic status read from the file header by MetaController.
     * @param currentPayloadData Pointer to the start of this layer's payload.
     * @param endOfPayloadData Boundary of this layer's payload.
     */
    InternalLayer(bool isRangeFinal, 
                  const std::byte*& currentPayloadData, 
                  const std::byte* endOfPayloadData);


    InternalLayer(bool isRangeFinal);

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
};