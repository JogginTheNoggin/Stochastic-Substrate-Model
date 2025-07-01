#include "../headers/layers/InputLayer.h"
#include "../headers/operators/InOperator.h"
#include "../headers/util/Randomizer.h"
#include "../headers/util/IdRange.h"
#include "../headers/UpdateEvent.h"
#include "../headers/Scheduler.h"



InputLayer::InputLayer(bool rangeFinal, IdRange* initialReservedRange): Layer(LayerType::INPUT_LAYER, initialReservedRange, rangeFinal){
    // FIX: Perform precondition validation BEFORE creating operators.
    // This ensures the constructor fails immediately if the range is invalid,
    // which is the behavior the unit test correctly expects.
    checkRange(); 

    initChannels();

    validate(); // perfrom check to ensure operators to respective channels is valid 
}

// TODO de-serialization of different channels?
InputLayer::InputLayer(bool isIdRangeFinal, const std::byte*& currentPayloadData,  const std::byte* endOfPayloadData)
    : Layer(LayerType::INPUT_LAYER, isIdRangeFinal){
    
    deserialize(currentPayloadData, endOfPayloadData); // use superclass to deserialize base data,

    // FIX: Perform precondition validation BEFORE creating operators.
    // This ensures the constructor fails immediately if the range is invalid,
    // which is the behavior the unit test correctly expects.
    checkRange(); 

    // check if properly set up channels
    // if not construct them
    if(!channelsSet()){
        clearOperators(); // just reset the operators, through one or two could have been created
        initChannels();
    }
    
    validate(); // perfrom check to ensure operators to respective channels is valid 
}

void InputLayer::initChannels(){
    uint32_t newOpId; 
    InOperator* channelOp;

    

    
    // create op channels, 
    for(int i = 0; i < channelCount; i++){
        newOpId = generateNextId(); 
        channelOp = new InOperator(newOpId);
        addNewOperator(channelOp); // add and update id's to this layer
    }
}

void InputLayer::validate(){
    if(reservedRange->count() != channelCount){ // redundante current constructor will change
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

    if(!channelsSet()){
        // This should ideally not be reached if constructor and reservedRange are valid,
        // as operators for all channel IDs should have been created.
        // If reservedRange->count() != channelCount, the earlier check handles it.
        // however this scenario could occur post deserialization if operators not set
        throw std::runtime_error("Number of Operator channels not sufficient.");
    }
}


bool InputLayer::channelsSet(){
    for(int i = 0; i < channelCount; i++){ 
        uint32_t id = reservedRange->getMinId() + i;
        auto it = operators.find(id);
        if (it != operators.end() && it->second != nullptr) {
            checkType(it->second); // check each type is of class InOperator
        }
        else {
            // This should ideally not be reached if constructor and reservedRange are valid,
            // as operators for all channel IDs should have been created.
            // If reservedRange->count() != channelCount, the earlier check handles it.
            return false;
        }
    }
    return true;
}

void InputLayer::checkRange(){
    if (!reservedRange) {
        throw std::runtime_error("InputLayer requires a non-null IdRange.");
    }
    if(reservedRange->count() != channelCount){
        throw std::runtime_error("Range of layer must match channel count."); 
    }
}

void InputLayer::checkType(Operator* op) {
    if(op == nullptr){
        // This case should be caught by the check in validate() before calling checkType.
        throw std::runtime_error("Operator cannot be null (checked in checkType).");
    }
    else if (InOperator* inOp = dynamic_cast<InOperator*>(op)) {
        return; 
    }  
    else {
        throw std::runtime_error("Operator is not an Input operator. All operators within the input layer must be of type InOperator.");
    }
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
        // schedule each, which will then flag the operator for processing
        Scheduler::get()->scheduleMessage(textChannelId, static_cast<int>(c));
        //NOT correct:  operators.at(textChannelId)->message(static_cast<int>(c)); // no need to cast
    }
}