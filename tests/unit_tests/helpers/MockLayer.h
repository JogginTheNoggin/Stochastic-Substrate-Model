#pragma once

#include "headers/layers/Layer.h"
#include "headers/util/IdRange.h"
#include "headers/util/Randomizer.h"
#include "headers/layers/LayerType.h"

/**
 * @class TestLayer
 * @brief A minimal, concrete subclass of Layer for testing the base class's functionality.
 * @details The only purpose of this class is to allow instantiation of the abstract Layer
 * class so we can write unit tests for its concrete public methods (e.g., addNewOperator,
 * serializeToBytes, equals, etc.) in isolation from the more complex logic of subclasses
 * like InternalLayer or InputLayer.
 */
class MockLayer : public Layer {
public:
    /**
     * @brief Constructor that passes arguments up to the protected Layer base class constructor.
     * @param type The functional type of the layer.
     * @param range A pointer to the heap-allocated IdRange for this layer.
     * @param isRangeFinal A flag indicating if the layer's ID range is static or dynamic.
     */
    MockLayer(LayerType type, IdRange* range, bool isRangeFinal)
        : Layer(type, range, isRangeFinal) {}

    // NEW: Deserialization constructor for testing
    MockLayer(LayerType type, bool isRangeFinal, const std::byte*& data, const std::byte* dataEnd)
        : Layer(type, isRangeFinal) {
        // This constructor calls the protected Layer constructor and then
        // calls the protected deserialize method to populate the object.
        deserialize(data, dataEnd);
    }

    /**
     * @brief Provides a minimal implementation for the pure virtual method from the base class.
     * @details The body is intentionally empty because we do not need to test random
     * initialization logic when validating the base Layer's other methods.
     */
    void randomInit(IdRange* validConnectionRange, Randomizer* randomizer) override {
        // No-op for testing base class functionality.
    }

    /**
     * @brief [Override] Compares this TestLayer's state with another for equality.
     * @param other The Layer object to compare against.
     * @return bool True if the base Layer state is equal.
     * @details This class has no unique persistent state beyond what is in the
     * base Layer, so equality is determined entirely by the base class `equals` method.
     */
    bool equals(const Layer& other) const override {
        return Layer::equals(other);
    }

     /**
     * @brief [Public for Testing] Exposes the protected deserialize method for testing purposes.
     * @param data Pointer to the byte stream data.
     * @param dataEnd Pointer to the end of the byte stream data.
     */
    void test_deserialize(const std::byte*& data, const std::byte* dataEnd) {
        // This public method calls the protected base class method.
        // It's specifically for allowing white-box testing of deserialization.
        Layer::deserialize(data, dataEnd);
    }


    // Access the protected method for testing
    uint32_t generateNextId(){
        return Layer::generateNextId(); 
    }
};
