#include "../headers/layers/InputLayer.h"
#include "../headers/operators/InOperator.h"
#include "../headers/util/Randomizer.h"
#include "../headers/util/IdRange.h"
#include "../headers/UpdateEvent.h"



InputLayer::InputLayer(bool rangeFinal, IdRange* initialReservedRange): Layer(LayerType::INPUT_LAYER, initialReservedRange, rangeFinal){
    
    uint32_t newOpId; 
    InOperator* channelOp;
    // create op channels, 
    for(int i = 0; i < channelCount; i++){
        newOpId = generateNextId(); 
        channelOp = new InOperator(newOpId);
        addNewOperator(channelOp); // add and update id's to this layer
    }

    // TODO need to perform checks to be sure all operators in layer are of OutOperator Type
    validate(); // perfrom check to ensure operators to respective channels is valid 
}


void InputLayer::validate(){
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
    for(int i = 0; i < channelCount; i++){ 
        uint32_t id = reservedRange->getMinId() + i;
        if(operators.at(i) != nullptr){ 
            checkType(operators.at(id)); // check each type is of class out operatoer 
        }
        else {
            break; // TODO we do not accept noncontiguous. this should be enforced. 
        }
    }
}


void InputLayer::checkType(Operator* op) {
    if(op == nullptr){
        throw std::runtime_error("Operator cannot be null."); 
    }
    else if (InOperator* inOp = dynamic_cast<InOperator*>(op)) {
        return; 
    }  
    else {
        throw std::runtime_error("Operator is not an Input operator. All operators within the input layer must be of type InOperator.");
    }
}

// TODO de-serialization of different channels?
InputLayer::InputLayer(bool isRangeFinal, const std::byte*& currentPayloadData,  const std::byte* endOfPayloadData)
    : Layer(LayerType::INPUT_LAYER, isRangeFinal){

    deserialize(currentPayloadData, endOfPayloadData); // use superclass to deserialize base data,
}

// Randomize the connections of the input channel, providing a pathway into other layers, and operators
void InputLayer::randomInit(IdRange* connectionRange, Randomizer* randomizer) {
    // we assume reserved range is valid, method should not be called before layer is validated

    // left for readability
    //uint32_t textChannelId = reservedRange->getMinId() + textChannelIdOffset; 
    //operators.at(textChannelId)->randomInit(connectionRange, randomizer); 
    for(int i = 0; i < operators.size(); i++){
        
        uint32_t channelId = reservedRange->getMinId() + i; 
        operators.at(channelId)->randomInit(connectionRange, randomizer);
    }
}

// input text into layers text channel (an operator)
void InputLayer::inputText(std::string text){
    // left for readability
    uint32_t textChannelId = reservedRange->getMinId() + textChannelIdOffset; 
    for (char c : text) {
        operators.at(textChannelId)->message(static_cast<int>(c)); // no need to cast
    }
}