#include "../headers/layers/OutputLayer.h"
#include "../headers/layers/LayerType.h"
#include "../headers/util/Randomizer.h"
#include "../headers/util/IdRange.h"
#include "../headers/operators/AddOperator.h"
#include "../headers/operators/OutOperator.h"
#include "../headers/UpdateEvent.h"
#include <stdexcept>               // For std::runtime_error


OutputLayer::OutputLayer(bool isIdRangeFinal, IdRange* initialReservedRange): Layer(LayerType::OUTPUT_LAYER, initialReservedRange, isIdRangeFinal){



    uint32_t newOpId; 
    OutOperator* channelOp;
    // create op channels, 
    for(int i = 0; i < channelCount; i++){
        newOpId = generateNextId(); 
        channelOp = new OutOperator(newOpId);
        addNewOperator(channelOp); // add and update id's to this layer
    }
    
    validate(); //
}


OutputLayer::OutputLayer(bool isIdRangeFinal, const std::byte*& currentPayloadData,  const std::byte* endOfPayloadData)
    : Layer(LayerType::OUTPUT_LAYER, isRangeFinal){

    deserialize(currentPayloadData, endOfPayloadData); // use superclass to deserialize base data,
    /*  De-serializing the different channels is tricky given we don't know the ids.
        However, we know the id range and this range should only consist of 3 available ids.
        Therefore, we enforce and ordering of these ids, from least to greatest.
        1) text channel
        2) image channel
        3) audio channel

        future channels cans be accomodated by adopting a different mapping. 

    */
        
    // TODO need to perform checks to be sure all operators in layer are of OutOperator Type
    validate(); // perform checks to ensure operators and channels correct

}

bool OutputLayer::hasTextOutput(){
    uint32_t textChannelId = reservedRange->getMinId() + textChannelIdOffset; 
    return static_cast<OutOperator*>(operators.at(textChannelId))->hasOutput();  // static cast because we know it must be Out Operator
}


// get the layers text output
std::string OutputLayer::getTextOutput(){
        // left for readability
        uint32_t textChannelId = reservedRange->getMinId() + textChannelIdOffset; 
        // TODO should we not do dynamic cast and just save as subclass type, given only a few operators? or to remain sync follow the common interface?
        return (static_cast<OutOperator*>(operators.at(textChannelId)))->getDataAsString(); // index 0 text channel
}





void OutputLayer::validate(){
    if(reservedRange->count() != channelCount){
        throw std::runtime_error("Range of layer must match channel count."); 
    }
    else if(operators.empty()){
        throw std::runtime_error("Layer does not contain any operators for channels.");
    }
    else if(operators.size() > channelCount){
        // this could be handled more softly, not quite necessary. (currently strict)
        throw std:: runtime_error("Number of operators exceed the amount of available channels"); 
    }
    else if (operators.size() < channelCount){ // TODO consider implications of not enforcing this 
        /*  may not enforce needing to have multiple channels, 
            however, if other layers connect to operators IDs that don't exist
            should be fine but maybe not ideal
        */
    }

    // check op for each channel type
    for(int i = 0; i < channelCount; i++){ // check for each channel type
        uint32_t id = reservedRange->getMinId() + i;
        if(operators.at(i) != nullptr){ 
            checkType(operators.at(id)); // check each type is of class out operatoer 
        }
        else {
            break; // TODO we do not accept noncontiguous. this should be enforced. 
        }
    }

}

void OutputLayer::checkType(Operator* op) {
    if(op == nullptr){
        throw std::runtime_error("Operator cannot be null."); 
    }
    else if (OutOperator* outOp = dynamic_cast<OutOperator*>(op)) {
        return; 
    }  
    else {
        throw std::runtime_error("Operator is not an Output operator. All operators within the output layer must be of type OutOperator.");
    }
}

void OutputLayer::randomInit(IdRange* connectionRange, Randomizer* randomizer) {
    // This method is not used by the layer
    // Output layr typically has only 1 operator?? 
}