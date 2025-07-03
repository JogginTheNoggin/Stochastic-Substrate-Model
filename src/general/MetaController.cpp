#include "../headers/controllers/MetaController.h"
#include "../headers/layers/Layer.h" // Needed for iterating and calling layer methods
#include "../headers/operators/Operator.h" // For the return type of getOperatorPtr
#include "../headers/Payload.h"
#include "../headers/layers/InputLayer.h"
#include "../headers/layers/OutputLayer.h"
#include "../headers/layers/InternalLayer.h"
#include "../headers/util/Randomizer.h"
#include "../headers/util/PseudoRandomSource.h"
#include <fstream>
#include <vector>
#include <algorithm> // For std::sort in validation
#include <iostream>  // For std::cout used in printOperators

// --- Constructor & State Management ---
// custom randomizer, primarily for testing
MetaController::MetaController(int numOperators, Randomizer* randomizer): rand(randomizer) {
    randomizeNetwork(numOperators); 


    // below is unnecessary based on how randomizeNetwork is but is done for safe keeping
    // (Below is Repeated Code, also in load configure) 
    // Step 1: Sort the newly loaded layers into their canonical order.
    sortLayers(); 

    // Step 2: Perform system-wide validation on the sorted layers.
    try {
        validateLayerIdSpaces();
    } catch (const std::runtime_error& e) {
        clearAllLayers(); // Validation failed, do not keep the invalid state.
        throw; // Re-throw the validation error.
    }

}



/*
MetaController::MetaController(int numOperators){
    rand = new Randomizer();
    randomizeNetwork(numOperators, rand); 


    // below is unnecessary based on how randomizeNetwork is but is done for safe keeping
    // (Below is Repeated Code, also in load configure) 
    // Step 1: Sort the newly loaded layers into their canonical order.
    sortLayers(); 

    // Step 2: Perform system-wide validation on the sorted layers.
    try {
        validateLayerIdSpaces();
    } catch (const std::runtime_error& e) {
        clearAllLayers(); // Validation failed, do not keep the invalid state.
        throw; // Re-throw the validation error.
    }
}; 
*/

/**
 * @brief Constructor for the MetaController.
 * @param configPath Optional path to a network configuration file to load on startup.
 */
MetaController::MetaController(const std::string& configPath, Randomizer* randomizer): rand(randomizer){
    if (!configPath.empty()) {
        try {
            loadConfiguration(configPath);
        } catch (const std::exception& e) {
            // If loading fails in the constructor, clear any partial state and re-throw
            // so the creator of MetaController knows initialization failed.
            clearAllLayers();
            throw std::runtime_error("Failed to initialize MetaController from configuration file: " + std::string(e.what()));
        }
    }
}

MetaController::~MetaController() = default;

void MetaController::randomizeNetwork(int numInternalOperators) {
    if (numInternalOperators < 0 ) {
        throw std::invalid_argument("Number of internal operators cannot be negative.");
    }
    else if(rand == nullptr){
        // default to pseudo random
        rand = new Randomizer(std::make_unique<PseudoRandomSource>());
    }


    // 1. Start with a clean slate
    clearAllLayers();

    // 2. Define the ID Ranges for each layer based on your specification.
    // We allocate these on the heap because the Layer constructors take ownership of the pointers.
    // input layer has 3 operators or channels, text, image, audio (for now)
    // output layer is similar but handled slightly different
    // magic numbers work for now
    IdRange* inputRange = new IdRange(0, 2);
    IdRange* outputRange = new IdRange(3, 5);
    uint32_t internalMinId = 6; // start id of internal layer must be past last layer
    uint32_t internalMaxId = internalMinId + numInternalOperators - 1; // calculate what the maxID would be if we wanted space for numInternalOperators
    // Handle case where numInternalOperators is 0
    // zero internal operators simply means input layer will only be able to directly connect to output layer. (still valid, but simply like copying out values)
    if (numInternalOperators == 0) {
        // this would still allow 1 operator to be created though it is set to zero, 
        // therefore to pass validation, we create a valid non overlapping range
        // but we do not call to randomize the internal layer
        internalMaxId = internalMinId; // Creates an empty but valid range
    }
    IdRange* internalRange = new IdRange(internalMinId, internalMaxId);

    // 3. Define the connection ranges.
    // Both Input and Internal layers can connect to the Output and Internal layers.
    uint32_t connectionMinId = outputRange->getMinId();
    uint32_t connectionMaxId = internalRange->getMaxId();
    IdRange* fullConnectionRange = new IdRange(connectionMinId, connectionMaxId);

    // 4. Instantiate the concrete Layer subclasses.
    // Input and Output layers have static ranges (`isRangeFinal = true`).
    // The Internal layer has a dynamic range (`isRangeFinal = false`).
    auto inputLayer = std::make_unique<InputLayer>(true, inputRange);
    auto outputLayer = std::make_unique<OutputLayer>(true, outputRange);
    auto internalLayer = std::make_unique<InternalLayer>(false, internalRange);

    // 5. Call randomInit() on  respective layers to create operators(only internal layer) and queue connection events (both input and internal layer).
    // Note: The `randomInit` method populates the layer with operators and then calls
    // changes are reflected immediately, as layers do not need to call updateController or notify other intermediate classes (may change in the future)
    inputLayer->randomInit(fullConnectionRange, rand); // randomly connects input channels to a number of operators

    // Only initialize the internal layer if operators were requested for it.
    if (numInternalOperators > 0) {
        internalLayer->randomInit(fullConnectionRange, rand); // randomly connects internal operators to other internal operators and or output channels
        // output layer is not randomized, as has predefined operators and no outbound connections
    }


    // 6. Add the fully initialized layers to the MetaController's main vector.
    // We use std::move to transfer ownership of the unique_ptr.
    layers.push_back(std::move(inputLayer));
    layers.push_back(std::move(outputLayer));
    layers.push_back(std::move(internalLayer));

    // 7. Clean up the connection range object as it's no longer needed.
    // The layers themselves now own their respective reservedRange objects.
    delete fullConnectionRange;
}

/**
 * @brief Deletes all managed Layer objects and resets the MetaController's state.
 */
void MetaController::clearAllLayers() {
    // Purpose: Ensure the MetaController is in a clean, empty state.
    // Parameters: None.
    // Return: Void.
    // Key Logic: Clearing the vector of std::unique_ptr automatically calls the destructor
    // for each contained Layer object, which in turn is responsible for deleting all
    // the Operator objects it owns. This ensures no memory leaks.
    layers.clear();
}


// --- Operator Access ---

/**
 * @brief Retrieves a pointer to an Operator object by its unique ID.
 * @param operatorId The unique ID of the Operator to retrieve.
 * @return A raw pointer to the Operator object, or nullptr if not found.
 */
Operator* MetaController::getOperatorPtr(uint32_t operatorId) const {
    // Purpose: Find and return a specific operator from anywhere in the network.
    // Parameters: operatorId - The ID of the operator to find.
    // Return: A pointer to the found Operator, or nullptr.
    // Key Logic: Delegate the search to each layer. Iterate through all managed layers.
    // For each layer, ask it if it contains the operator. An optimization is to
    // first check if the ID falls within the layer's reserved range.
    

    // could use binary search but overkill given expected layer count
    for (const auto& layerPtr : layers) {
        if (layerPtr) { // Ensure the unique_ptr is not null
            // Optimization: Check if the ID is within the layer's known ID range before searching its map.
            // This can prevent searching in layers where the ID could not possibly exist.
            const IdRange* range = layerPtr->getReservedIdRange();
            if (range && range->contains(operatorId)) {
                Operator* op = layerPtr->getOperator(operatorId);
                if (op) {
                    // Operator found, return it immediately.
                    return op;
                }
            }
            // If range is not valid or ID not in range, we continue to the next layer.
            // A broader search could be done if ranges might not be perfectly maintained,
            // but checking the range first is more efficient.
            // Operator* op = layerPtr->getOperator(operatorId); // Broader search without range check
            // if (op) return op;
        }
    }

    // If the loop completes, the operator was not found in any layer.
    return nullptr;
}


// --- Layer & Network Info ---

/**
 * @brief Gets the number of layers currently managed by the MetaController.
 * @return size_t The number of layers.
 */
size_t MetaController::getLayerCount() const {
    return layers.size();
}

/**
 * @brief Gets the total number of operators across all layers in the network.
 * @return size_t The total operator count.
 */
size_t MetaController::getOpCount() const {
    // Purpose: Calculate the total number of operators in the entire network.
    // Parameters: None.
    // Return: The total count.
    // Key Logic: Iterate through each layer and sum the number of operators from each one.
    size_t totalCount = 0;
    for (const auto& layerPtr : layers) {
        if (layerPtr) {
            totalCount += layerPtr->getOpCount();
        }
    }
    return totalCount;
}

/**
 * @brief Provides read-only access to the collection of layers.
 * @return A const reference to the vector of unique_ptrs to Layer objects.
 */
const std::vector<std::unique_ptr<Layer>>& MetaController::getAllLayers() const {
    return layers;
}



// --- Private Helper Implementations ---


/**
 * @brief Performs system-wide validation checks on the (assumed to be sorted) layers.
 * @details This is a crucial const method called after loading and sorting. It enforces:
 * 1. There is exactly one dynamic layer in the entire configuration.
 * 2. The dynamic layer is the last element in the sorted list (i.e., has the highest ID range).
 * 3. No two layers have overlapping ID ranges.
 * @throws std::runtime_error if any validation check fails.
 */
void MetaController::validateLayerIdSpaces() const {
    if (layers.empty()) {
        return; // Nothing to validate
    }

    // --- Step 1: Find the single dynamic layer and verify its count ---
    int dynamicLayerCount = 0;
    const Layer* dynamicLayer = nullptr;
    for (const auto& layerPtr : layers) {
        if (layerPtr && !layerPtr->getIsRangeFinal()) {
            dynamicLayerCount++;
            dynamicLayer = layerPtr.get();
        }
    }

    if (dynamicLayerCount != 1) {
        throw std::runtime_error("Validation Failed: Configuration must contain exactly one dynamic layer. Found: " + std::to_string(dynamicLayerCount));
    }

    // --- Step 2: Check for overlaps between adjacent layers in the sorted vector ---
    // This is now an efficient O(N) pass.
    for (size_t i = 0; i < layers.size() - 1; ++i) {
        const IdRange* rangeA = layers[i]->getReservedIdRange();
        const IdRange* rangeB = layers[i+1]->getReservedIdRange();
        
        if (rangeA && rangeB ) {
            // Use the new isOverlapping method for clarity
            if (rangeA->isOverlapping(*rangeB)) {
                 throw std::runtime_error("Validation Failed: Overlapping ID ranges detected between sorted layers. Range [" +
                                     std::to_string(rangeA->getMinId()) + "-" + std::to_string(rangeA->getMaxId()) + "]" +
                                     " overlaps with subsequent range [" + std::to_string(rangeB->getMinId()) + "-" + std::to_string(rangeB->getMaxId()) + "].");
            }
        }
    }

    // --- Step 3: Verify the dynamic layer is the last element in the sorted vector ---
    // This confirms the rule that the dynamic layer's minId must be greater than all static layers' maxId.
    if (!layers.empty() && layers.back().get() != dynamicLayer) {
        throw std::runtime_error("Validation Failed: The dynamic layer does not have the highest ID range and is not last after sorting, which is required for safe expansion.");
    }
}


uint32_t MetaController::getNextIdForNewRange() const {
    if (layers.empty()) {
        return 0;
    }

    // Because the layers are sorted, the last layer has the highest ID range.
    const auto& lastLayer = layers.back();
    if (lastLayer) {
        return static_cast<uint32_t>(lastLayer->getReservedIdRange()->getMaxId()) + 1;
    }

    return 0; // Fallback for an empty or invalid last layer
}

/**
 * @brief [Private Helper] Sorts the internal 'layers' vector in place.
 * @details Sorts the layers based on their reserved ID range, primarily using the
 * minId of the range. This establishes a canonical order for the layers, which
 * simplifies subsequent validation checks. This is a state-modifying method.
 */
void MetaController::sortLayers() {
    std::sort(this->layers.begin(), this->layers.end(), 
        [](const std::unique_ptr<Layer>& a, const std::unique_ptr<Layer>& b) {
            // Ensure we handle layers that might have invalid or uninitialized ranges (Unlikely)
            if (!a || !a->getReservedIdRange()) return true; // Invalid ranges are "less than" valid ones
            if (!b || !b->getReservedIdRange()) return false;
            
            // Use the overloaded operator< from the IdRange struct for comparison
            return *(a->getReservedIdRange()) < *(b->getReservedIdRange());
        }
    );
}

/**
 * @brief Finds the layer that contains a given operator ID.
 * @param operatorId The ID of the operator to find.
 * @return A raw pointer to the Layer that should contain the operator, or nullptr if not found.
 */
Layer* MetaController::findLayerForOperator(uint32_t operatorId) const {
    for (const auto& layerPtr : layers) {
        if (layerPtr) {
            const IdRange* range = layerPtr->getReservedIdRange();
            if (range && range->contains(operatorId)) {
                return layerPtr.get();
            }
        }
    }
    return nullptr;
}

Layer* MetaController::getDynamicLayer(){
    return layers.back().get();
}


OutputLayer* MetaController::getOutputLayer(){
    for (const auto& layerPtr : layers) { // TODO not efficient but good enough with only 3 layers
        if (auto* outputLayer = dynamic_cast<OutputLayer*>(layerPtr.get())) {
            return outputLayer;
        }
    }
    return nullptr;
}

// operator processing

bool MetaController::messageOp(uint32_t operatorId, int message) {
    Layer* layer = findLayerForOperator(operatorId);
    if(layer == nullptr){
        return false;
        // Operator not found (ID was dangling).
        // If cleanup is triggered, should it occur here? How will it check all layers for this operator connection? Should it? 
        // TODO: Log this event? "Warning: Could not deliver message to non-existent Operator ID " << targetOperatorId
    }

    // use the layer to message its respective operator
    // returns whether successful or not. 
    // TODO would layer messageOp returning false mean clean up could be signaled? 
    return layer->messageOperator(operatorId, message);

}

void MetaController::processOpData(uint32_t operatorId){
    Layer* layer = findLayerForOperator(operatorId);
    if(layer == nullptr){
        return; // op does not exist
        // TODO log event? 
    }

    // use the layer to process data of its respective operator
    layer->processOperatorData(operatorId);
}

void MetaController::traversePayload(Payload* payload){
    Layer* layer = findLayerForOperator(payload->currentOperatorId);
    if(layer == nullptr){
        return ;
        // paylod pointed to non existant operator
        // TODO log failed attempt? 
    }

    // use the layer to send payload its respective operator
    // TODO would layer messageOp returning false mean clean up could be signaled? 
    layer->traverseOperatorPayload(payload);
}


// --- Update Event Handling ---

/**
 * @brief Handles a CREATE_OPERATOR UpdateEvent.
 * @details Finds the single dynamic layer and delegates the operator creation request to it.
 */
void MetaController::handleCreateOperator(const std::vector<int>& params) {
    // TODO this is good enough for now but likely want it to allow creation in any layer so long as not past its reserved range
    Layer* dynamicLayer = getDynamicLayer(); // Using your renamed getDynamicLayer()
    if (dynamicLayer) {
        dynamicLayer->createOperator(params);
    } else {
        // Optional: Log warning - no dynamic layer found to create operator in.
    }
}

/**
 * @brief Handles a DELETE_OPERATOR UpdateEvent.
 * @details Finds which layer contains the target operator and delegates the deletion request.
 */
void MetaController::handleDeleteOperator(uint32_t targetOperatorId) {
    Layer* targetLayer = findLayerForOperator(targetOperatorId);
    if (targetLayer) {
        targetLayer->deleteOperator(targetOperatorId);
    } else {
        // Optional: Log warning - could not find layer for operator to be deleted.
    }
}

/**
 * @brief Delegates a parameter change request to the appropriate layer.
 */
void MetaController::handleParameterChange(uint32_t targetOperatorId, const std::vector<int>& params) {
    Layer* targetLayer = findLayerForOperator(targetOperatorId);
    if (targetLayer) {
        targetLayer->changeOperatorParam(targetOperatorId, params);
    }
}

/**
 * @brief Delegates a request to add a connection to the appropriate layer.
 */
void MetaController::handleAddConnection(uint32_t targetOperatorId, const std::vector<int>& params) {
    Layer* targetLayer = findLayerForOperator(targetOperatorId);
    if (targetLayer) {
        targetLayer->addOperatorConnection(targetOperatorId, params);
    }
}

/**
 * @brief Delegates a request to remove a connection from an operator in the appropriate layer.
 */
void MetaController::handleRemoveConnection(uint32_t targetOperatorId, const std::vector<int>& params) {
    Layer* targetLayer = findLayerForOperator(targetOperatorId);
    if (targetLayer) {
        targetLayer->removeOperatorConnection(targetOperatorId, params);
    }
}

/**
 * @brief Delegates a request to move a connection for an operator in the appropriate layer.
 */
void MetaController::handleMoveConnection(uint32_t targetOperatorId, const std::vector<int>& params) {
    Layer* targetLayer = findLayerForOperator(targetOperatorId);
    if (targetLayer) {
        targetLayer->moveOperatorConnection(targetOperatorId, params);
    }
}

int MetaController::getTextCount(){
    OutputLayer* outputLayer = getOutputLayer();
    if(outputLayer == nullptr){
        return 0; 
    }
    return outputLayer->hasTextOutput()? outputLayer->getTextCount(): 0;
}


void MetaController::clearTextOutput(){
    OutputLayer* outputLayer = getOutputLayer();
    if(outputLayer == nullptr){
        return; 
    }
    outputLayer->clearTextOutput();
}

// input and output
std::string MetaController::getOutput() {
    OutputLayer* outputLayer = getOutputLayer();
    if(outputLayer == nullptr){
        return "[ No Output Layer. ]"; 
    }
    return outputLayer->hasTextOutput()? outputLayer->getTextOutput() : "[ No New Output Text. ]";
}

void MetaController::setTextBatchSize(int size){
    OutputLayer* outputLayer = getOutputLayer();
    if(outputLayer == nullptr){
        return; 
    }
    outputLayer->setTextBatchSize(size);
}


bool MetaController::inputText(std::string text){
    for (const auto& layerPtr : layers) { // TODO not efficient but good enough with only 3 layers
        if (auto* inputLayer = dynamic_cast<InputLayer*>(layerPtr.get())) {
            inputLayer->inputText(text);
            return true;
        }
    }
    return false;
}


bool MetaController::isEmpty() const {
    return getOpCount() == 0; 
}


// --- Persistence ---

/**
 * @brief Saves the entire network configuration to a file.
 * @param filePath The path to the file where the configuration will be saved.
 * @return True if saving was successful, false otherwise.
 */
bool MetaController::saveConfiguration(const std::string& filePath) const {
    // Purpose: Persist the entire network state by serializing each layer sequentially.
    // Parameters: filePath - The destination for the configuration file.
    // Return: True on success, false on file I/O error.
    // Key Logic: Opens a binary file stream. For each layer in the 'layers' vector,
    // it calls that layer's serializeToBytes() method. The resulting byte vector, which
    // is a self-contained block for that layer, is written directly to the file.

    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        std::cout << "Could not open path specified in metaController" << std::endl; 
        // Optional: Log an error here.
        return false;
    }

    // Iterate through all layers and ask each one to serialize itself.
    for (const auto& layerPtr : layers) {
        if (layerPtr) {
            std::vector<std::byte> layerBlockBytes = layerPtr->serializeToBytes();
            if (!layerBlockBytes.empty()) {
                outFile.write(reinterpret_cast<const char*>(layerBlockBytes.data()), layerBlockBytes.size());
                if (!outFile.good()) {
                    // File write error occurred.
                    outFile.close();
                    std::cout << "Error writting to path specified in metaController" << std::endl; 
                    return false;
                }
            }
        }
    }

    outFile.close();
    return outFile.good();
}


/**
 * @brief Loads a complete network configuration from a file, replacing any existing state.
 * @param filePath The path to the file containing the configuration.
 * @return True if loading and validation were successful, false otherwise.
 */
bool MetaController::loadConfiguration(const std::string& filePath) {
    clearAllLayers(); // Always start with a clean slate

    std::ifstream inFile(filePath, std::ios::binary | std::ios::ate); // Open at end to get size
    if (!inFile.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
        // Optional: Log error
        return false;
    }

    std::streamsize totalFileSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    if (totalFileSize == 0) {
        return true; // Empty file is a valid empty configuration
    }

    // Read the entire file into a buffer for safer parsing.
    // TODO Acknowledges that this isn't suitable for multi-gigabyte files.
    std::vector<std::byte> fileBuffer(totalFileSize);
    if (!inFile.read(reinterpret_cast<char*>(fileBuffer.data()), totalFileSize)) {
        throw std::runtime_error("Failed to read file into buffer: " + filePath);
        return false; // Failed to read file into buffer
    }
    inFile.close();

    const std::byte* current = fileBuffer.data();
    const std::byte* endOfAllData = current + totalFileSize;

    // Loop to read each Layer block from the buffer until it's fully consumed.
    while (current < endOfAllData) {
        // --- 1. Read the Layer "Envelope" Header ---
        // This is the part MetaController is responsible for parsing.
        
        LayerType fileLayerType = static_cast<LayerType>(Serializer::read_uint8(current, endOfAllData));
        bool fileIsRangeFinal = (Serializer::read_uint8(current, endOfAllData) == 1);
        uint32_t numBytesOfPayload = Serializer::read_uint32(current, endOfAllData);

        // Define the boundaries of the payload data that will be passed to the Layer's constructor.
        const std::byte* payloadEnd = current + numBytesOfPayload;
        if (payloadEnd > endOfAllData) {
            throw std::runtime_error("Layer payload size specified in header exceeds remaining file data.");
        }

        // --- 2. Instantiate Correct Layer Subclass ---
        std::unique_ptr<Layer> newLayer = nullptr;

        try {
            switch (fileLayerType) {
                case LayerType::INPUT_LAYER:
                    newLayer = std::make_unique<InputLayer>(fileIsRangeFinal, current, payloadEnd);
                    break;
                case LayerType::OUTPUT_LAYER:
                    newLayer = std::make_unique<OutputLayer>(fileIsRangeFinal, current, payloadEnd);
                    break;
                case LayerType::INTERNAL_LAYER:
                    newLayer = std::make_unique<InternalLayer>(fileIsRangeFinal, current, payloadEnd);
                    break;
                default:
                    throw std::runtime_error("Unknown LayerType (" + std::to_string(static_cast<int>(fileLayerType)) + ") found in file.");
            }

            // The Layer constructor must have consumed its entire payload segment.
            if (current != payloadEnd) {
                throw std::runtime_error("Layer constructor for type " + std::to_string(static_cast<int>(fileLayerType)) + " did not consume its entire payload.");
            }

        } catch (const std::exception& e) {
            clearAllLayers(); // Ensure partial state is cleaned on failure
            // Optional: Re-throw with more context
            throw std::runtime_error("Failed during layer deserialization: " + std::string(e.what()));
        }

        layers.push_back(std::move(newLayer));
    }

    // --- 3. Post-Load System-Wide Validation ---
    // Step 1: Sort the newly loaded layers into their canonical order.
    sortLayers(); 

    // Step 2: Perform system-wide validation on the sorted layers.
    try {
        validateLayerIdSpaces();
    } catch (const std::runtime_error& e) {
        clearAllLayers(); // Validation failed, do not keep the invalid state.
        throw; // Re-throw the validation error.
    }


    return true;
}


// TODO formatting likely off.
std::string MetaController::getOperatorsAsJson(bool prettyPrint) const{
    std::ostringstream oss;
    std::string newline = prettyPrint ? "\n" : "";

    // The layers vector is assumed to be sorted by the constructor/load methods,
    // which is crucial for deterministic output.
    
    oss << "[" << newline; // Start of the main array of layers

    bool firstLayer = true;
    for (const auto& layerPtr : layers) {
        if (!layerPtr) continue;

        if (!firstLayer) {
            oss << "," << newline;
        }

        // Call the layer's toJson method and append its output to the stream.
        // The Layer's own toJson handles its internal formatting.
        oss << layerPtr->toJson(prettyPrint, 1);
        
        firstLayer = false;
    }

    oss << newline << "]"; // End of the main array

    return oss.str();
}


void MetaController::printOperators(bool prettyPrint) const{
    // This method simply calls getOperatorsAsString and prints the result to std::cout.
    std::cout << getOperatorsAsJson(prettyPrint) << std::endl;
}

