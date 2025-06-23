#include "../headers/layers/InternalLayer.h"
#include "../headers/layers/Layer.h"
#include "../headers/util/Randomizer.h"
#include "../headers/util/IdRange.h"
#include "../headers/operators/AddOperator.h"
#include "../headers/UpdateEvent.h"

InternalLayer::InternalLayer(bool isLayerRangeFinal, const std::byte*& currentPayloadData,  const std::byte* endOfPayloadData)
    : Layer(LayerType::INTERNAL_LAYER, isLayerRangeFinal){

    deserialize(currentPayloadData, endOfPayloadData); // use superclass to deserialize base data,
}


InternalLayer::InternalLayer(bool isLayerRangeFinal, IdRange* initialReservedRange): Layer(LayerType::INTERNAL_LAYER, initialReservedRange, isLayerRangeFinal){
    
}


void InternalLayer::randomInit(IdRange* connectionRange, Randomizer* randomizer) {
    if (!reservedRange || !randomizer) {
        return; // Cannot initialize without a range or randomizer
    }
    // This method populates an empty, programmatically-created layer.

    // 1. Create Operators for this layer.
    int capacity = reservedRange->count();
    if (capacity <= 0) {
        return;
    }
    int numOpsToCreate = (capacity > 1) ? randomizer->getInt(capacity / 2, capacity) : capacity; // half to full capacity
    
    std::vector<Operator*> justCreatedOperators; // Keep track of new ops to connect them
    justCreatedOperators.reserve(numOpsToCreate);

    for (int i = 0; i < numOpsToCreate; ++i) {
        try {
            // Get a valid ID from the layer's own ID generator
            uint32_t newOpId = generateNextId(); 
            // TODO add switch to support randomization of which type of operator to use 
            // For now, we create AddOperator instances. This could be more flexible later.
            Operator* newOp = new AddOperator(newOpId);
            newOp->randomInit(connectionRange, randomizer); // randomize the operators details
            
            // TODO add more operator types in the future. 
            // Add the new operator to this layer. The addNewOperator method handles validation, and ID update for next operator.
            addNewOperator(newOp);

        } catch (const std::overflow_error& e) {
            // This happens if a static layer becomes full. Stop creating operators.
            // Optional: Log this event.
            // std::cerr << "Layer with range [" << reservedRange.getMinId() << "-" << reservedRange.getMaxId() 
            //           << "] is full. Stopped creating operators at " << i << " of " << numOpsToCreate << "." << std::endl;
            break; 
        }
    }
}