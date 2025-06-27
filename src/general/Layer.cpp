#include "../headers/layers/Layer.h"
#include "../headers/operators/all_operators.h" // for all the different operator types
#include "../headers/Payload.h"
#include "../headers/util/Serializer.h"       // For reading primitive types
#include "../headers/util/IdRange.h"
#include <stdexcept>               // For std::runtime_error
#include <iostream>
#include <array>                   // For std::array (if needed for temporary buffers)
#include <algorithm>

/**
 * @brief Virtual destructor to ensure proper cleanup of polymorphic objects.
 * @details Deletes all Operator objects owned by the layer and the IdRange object.
 */
Layer::~Layer() {
    clearOperators();
    delete reservedRange; // The Layer is responsible for deleting the IdRange object allocated in deserialize.
}

/**
 * @brief Move constructor.
 * @details Transfers ownership of all resources from the 'other' Layer to this one.
 * The 'other' Layer is left in a valid but empty state.
 * @param other The Layer to move from.
 */
Layer::Layer(Layer&& other) noexcept
    : type(other.type),
      isRangeFinal(other.isRangeFinal),
      reservedRange(other.reservedRange), // Transfer ownership of the pointer
      currentMinId(other.currentMinId),
      currentMaxId(other.currentMaxId),
      operators(std::move(other.operators)) {
    // Leave the moved-from object in a safe, destructible state.
    other.reservedRange = nullptr;
    other.currentMinId = 0;
    other.currentMaxId = 0;
}

/**
 * @brief Move assignment operator.
 * @details Transfers ownership of all resources from the 'other' Layer to this one,
 * after properly cleaning up this Layer's existing resources.
 * @param other The Layer to move from.
 */
Layer& Layer::operator=(Layer&& other) noexcept {
    if (this != &other) {
        // 1. Clear existing resources of this object
        clearOperators();
        delete reservedRange;

        // 2. Transfer ownership from 'other'
        type = other.type;
        isRangeFinal = other.isRangeFinal;
        reservedRange = other.reservedRange;
        currentMinId = other.currentMinId;
        currentMaxId = other.currentMaxId;
        operators = std::move(other.operators);

        // 3. Leave 'other' in a safe state
        other.reservedRange = nullptr;
        other.currentMinId = 0;
        other.currentMaxId = 0;
    }
    return *this;
}





// TODO, layers may be too big to deserialize, or even serialize all at once.
void Layer::deserialize(const std::byte*& data, const std::byte* dataEnd){
    // before this the other headers should have been read by the metaController / parser
    // Checklist: Deserialize and get range (Min is before max, of course)
    int fileMinId = Serializer::read_uint32(data, dataEnd);
    int fileMaxId = Serializer::read_uint32(data, dataEnd);
    reservedRange = new IdRange(fileMinId, fileMaxId); // IdRange constructor calls validate()

    

    while (data < dataEnd) {
        uint32_t opPayloadSize = Serializer::read_uint32(data, dataEnd);
        const std::byte* currentOpDataEnd = data + opPayloadSize;
        if (currentOpDataEnd > dataEnd) { 
            throw std::runtime_error("Deserialized operator data size specified is greater than the provided data stream");    
        }

        uint16_t opTypeAsInt = Serializer::read_uint16(data, currentOpDataEnd);
        Operator::Type opType = static_cast<Operator::Type>(opTypeAsInt);
        
        Operator* newOp = nullptr;
        // ... (Switch to new specific Operator(data, endOfCurrentOperatorPayload)) ...
        // Example:
        try {
            // Switch on the type to call the correct derived constructor
            // Pass the dataPtr (which is now *after* the OperatorType field)
            switch (opType) {
                case Operator::Type::ADD:
                    newOp = new AddOperator(data, currentOpDataEnd); // Calls AddOperator deserialization constructor
                    break;
                case Operator::Type::IN:
                    newOp = new InOperator(data, currentOpDataEnd); 
                    break;
                case Operator::Type::OUT:
                    newOp = new OutOperator(data, currentOpDataEnd); 
                    break;
                default:
                    // Clean up buffer data if needed? No, buffer goes out of scope.
                    throw std::runtime_error("Unknown or unsupported OperatorType encountered in configuration file: " + std::to_string(opTypeAsInt));
            } // End switch

            if (!newOp) { throw std::runtime_error("Operator construction returned null."); }
            // Check if the constructor(s) consumed all the bytes as expected
            // data should have been advanced by base and derived constructors
            // to exactly match dataEnd.
            if (data != currentOpDataEnd) {
                // This indicates an internal error in a constructor or Serializer read helper,
                // or a mismatch between declared size N and actual serialized data structure.
                delete newOp; // Clean up partially constructed object if possible
                throw std::runtime_error("Operator constructor (Type: " + std::to_string(opTypeAsInt) 
                + ") did not consume entire data block (" + std::to_string(std::distance(data, currentOpDataEnd)) 
                + " bytes remaining). Block size mismatch likely.");
            }

        } 
        catch (const std::exception& e) { // Catch exceptions from Operator constructor or helpers
            delete newOp; // Ensure cleanup if constructor or mapping failed
            std::cerr << "Error: Exception during loadConfiguration processing: ";
            throw e; // throw the error to see show message
        }

        addNewOperator(newOp); // preform prechecks and add the operator to the layer. 
    }
    // After loop, data should be == dataEnd. 

    // Ensure data pointer reached the end of the segment it was given
    if (data != dataEnd) {
        throw std::runtime_error("Layer payload not fully consumed after deserializing operators. Remaining: " 
            + std::to_string(std::distance(data, dataEnd)));
    }
}


void Layer::addNewOperator(Operator* op) {
    if (!op) {
        // Potentially throw an error or log if adding a nullptr is an issue
        throw std::runtime_error("Add null pointer operator is not allowed.");
    }
    
    // check that ID is valid and is within the range of the layer.
    if(!isValidId(op->getId())){
        throw std::runtime_error("Operator with ID: " + std::to_string(op->getId()) 
                                 + " is not valid. Must be within range: [ " + std::to_string(reservedRange->getMinId()) 
                                 + "," + std::to_string(reservedRange->getMaxId()) + " ]");
    }
    
    if (!isIdAvailable(op->getId())) { 
        delete op; 
        throw std::runtime_error("Operator with ID: " + std::to_string(op->getId()) 
                                 + " already present in layer. Duplicates not allowed.");
    }

    updateMinMaxIds(op->getId()); 
    operators[op->getId()] = op;
    
}

// range issue if default ids were set to -1 for currentMin/Max
bool Layer::isValidId(uint32_t operatorId) { // Called by addOperator
    if(operatorId < 0){ // redundant, given we check if in range, but maybe good to keep just in case
        // added here, however, operator class also perform check, but this method only requires an int value, extra check is appropriate
        return false;
        //throw std::runtime_error("Operator ID cannot be negative.");
    }

    if(!isRangeFinal){ // meaning maxId of range can increase, (but won't decrease)
        if(operatorId < reservedRange->getMinId()){ // regardless if final, ID range does not decrease min, so ID must be greater
            return false;
            //throw std::runtime_error("Operator ID cannot be below the layers minimum ID.");
        }
    }
    else{ // range is final, operator ID must be in range
        if(!reservedRange->contains(operatorId)){ // check if in range
            return false;
            /*
            throw std::runtime_error("Operator ID " + std::to_string(operatorId) + 
                                        " is outside the layer's available range [" +
                                        std::to_string(reservedRange.getMinId()) + "-" +
                                        std::to_string(reservedRange.getMaxId()) + "].");
            */
        }
        
    }

    return true;
}

// must call isValid before hand
void Layer::updateMinMaxIds(uint32_t operatorId){
    if(operatorId < currentMinId){
        currentMinId = operatorId;
    }

    if(operatorId > currentMaxId){
        currentMaxId = operatorId;
        if(!isRangeFinal && operatorId > reservedRange->getMaxId()){
            reservedRange->setMaxId(currentMaxId); // update range if the layer is not final, can increase
        }
    }
}



uint32_t Layer::generateNextId(){
    uint32_t candidateId;

    if (isEmpty()) {
        candidateId = reservedRange->getMinId(); 
    } else {
        if (currentMaxId == std::numeric_limits<uint32_t>::max()) {
            throw std::overflow_error("Cannot generate new ID; layer is at maximum capacity for uint32_t.");
        }

        candidateId = currentMaxId + 1; // is less than numerical limit, increment
    }


    
    if (isRangeFinal) { // Static layer
        if (candidateId > reservedRange->getMaxId()) {
            throw std::overflow_error("Static Layer is full. Cannot generate ID " + std::to_string(candidateId) +
                                      ". It exceeds reserved max " + std::to_string(reservedRange->getMaxId()) + ".");
        }
    } else { // Dynamic layer
        if (candidateId > reservedRange->getMaxId()) {
            reservedRange->setMaxId(candidateId); // Extend reserved range
        }
    }
    
    return candidateId;
}

bool Layer::isIdAvailable(uint32_t id) {
    if(id < 0){ // should not be possible given unsigned nature
        return false;
    }

    return operators.find(id) == operators.end(); // what is time complexity
}

bool Layer::isEmpty() const {
    // works because default min is max uint32_t
    // return currentMinId == std::numeric_limits<uint32_t>::max();
    return operators.empty();
}


bool Layer::isFull() const {
    
    if (isRangeFinal) { // Static layer
        if(operators.empty()){ // rserved ranges must have aleast 1 space to be valid, therefore layer cannot be full if no elements
            return false;
        }
        return currentMaxId == reservedRange->getMaxId(); // not >=, as currentMaxId should never get past reserved range max Id
    }
    // Dynamic layer (rangeFinal == false) is conceptually never full unless we hit INT_MAX for IDs.
    return false; // TODO overflow, underflow potentially protections for IDS in layer, will depend on final Data type of IDS (current is 32 bit unsigned)
}

bool Layer::getIsRangeFinal() const {
    return isRangeFinal; 
}

uint32_t Layer::getOpCount(){
    return operators.size();
}

/**
 * @brief Deletes all Operator objects owned by the layer and resets tracking IDs.
 */
void Layer::clearOperators() {
    for (auto const& [id, opPtr] : operators) {
        delete opPtr;
    }
    operators.clear();
    currentMinId = std::numeric_limits<uint32_t>::max();
    currentMaxId = 0;
}

/**
 * @brief Retrieves a pointer to an Operator object by its unique ID.
 * @param operatorId The unique ID of the Operator to retrieve.
 * @return A raw pointer to the Operator object, or nullptr if not found in this layer.
 */
Operator* Layer::getOperator(uint32_t operatorId) const {
    auto it = operators.find(operatorId);
    if (it != operators.end()) {
        return it->second;
    }
    return nullptr;
}

/**
 * @brief Provides read-only access to the map of all operators in the layer.
 * @return A const reference to the unordered_map of operators.
 */
const std::unordered_map<uint32_t, Operator*>& Layer::getAllOperators() const {
    return operators;
}

// Calling operator for processing

bool Layer::messageOperator(uint32_t operatorId, int message){
    Operator* op = getOperator(operatorId);
    if(op == nullptr){
        return false;
        // TODO op does not exist, dangling id, should clean up be signalled? Op either, not in range, or in range but not created
    }

    op->message(message);
    return true; // message has been delivered. 
}


void Layer::processOperatorData(uint32_t operatorId){
    Operator* op = getOperator(operatorId);
    if(op != nullptr){
        op->processData();
    }
}

void Layer::traverseOperatorPayload(Payload* payload){
    Operator* op = getOperator(payload->currentOperatorId);
    if(op != nullptr){
        op->traverse(payload);
    }
    else{
        // operator payload is suppose to be within, does not exist, therefore is no longer active
        // The Operator responsible for this payload disappeared mid-flight!
        // Mark the payload as inactive.
        payload->active = false; 
        // TODO: Log this "orphan payload" event?
    }
}

// --- Update Handling Methods (Delegation) ---
// TODO temporary, method implementations

/**
 * @brief Changes a parameter on a specific operator within this layer.
 * @param targetOperatorId The ID of the operator to modify.
 * @param params Parameters from the UpdateEvent for the change.
 */
void Layer::changeOperatorParam(uint32_t targetOperatorId, const std::vector<int>& params) {// TODO temporary, method implementation
    Operator* op = getOperator(targetOperatorId);
    if (op) {
        op->changeParams(params);
    }
    // If op is not found, the update is silently ignored for this layer.
}


/**
 * @brief Adds a connection to an operator within this layer.
 * @param sourceOperatorId The ID of the operator to add the connection to.
 * @param params Parameters from the UpdateEvent specifying target and distance.
 */
void Layer::addOperatorConnection(uint32_t sourceOperatorId, const std::vector<int>& params) {// TODO temporary, method implementation
    if (params.size() < 2) return;

    Operator* op = getOperator(sourceOperatorId);
    if (op) {
        uint32_t targetId = static_cast<uint32_t>(params[0]);
        int distance = params[1];
        op->addConnectionInternal(targetId, distance);
    }
}

/**
 * @brief Removes a connection from an operator within this layer.
 * @param sourceOperatorId The ID of the operator to remove the connection from.
 * @param params Parameters from the UpdateEvent specifying target and distance.
 */
void Layer::removeOperatorConnection(uint32_t sourceOperatorId, const std::vector<int>& params) {// TODO temporary, method implementation
    if (params.size() < 2) return;

    Operator* op = getOperator(sourceOperatorId);
    if (op) {
        uint32_t targetId = static_cast<uint32_t>(params[0]);
        int distance = params[1];
        op->removeConnectionInternal(targetId, distance);
    }
}

/**
 * @brief Moves a connection for an operator within this layer.
 * @param sourceOperatorId The ID of the operator whose connection is to be moved.
 * @param params Parameters specifying target, old, and new distances.
 */
void Layer::moveOperatorConnection(uint32_t sourceOperatorId, const std::vector<int>& params) { // TODO temporary, method implementation
    if (params.size() < 3) return;

    Operator* op = getOperator(sourceOperatorId);
    if (op) {
        uint32_t targetId = static_cast<uint32_t>(params[0]);
        int oldDistance = params[1];
        int newDistance = params[2];
        op->moveConnectionInternal(targetId, oldDistance, newDistance);
    }
}


/**
 * @brief Creates a new operator within this layer.
 * @details This method is typically only effective on a dynamic layer (`isRangeFinal == false`).
 * It uses the layer's internal ID generation to assign a new ID and instantiates the
 * specified operator type.
 * @param params Parameters from the UpdateEvent. params[0] is expected to be the Operator::Type to create.
 */
void Layer::createOperator(const std::vector<int>& params) {  // TODO temporary, method implementation
    // This action is only permitted on dynamic layers.
    if (isRangeFinal) {
        // Optional: Log a warning that an attempt was made to add an operator to a static layer at runtime.
        // For now, we silently ignore the request.
        return;
    }

    if (params.empty()) {
        // Cannot determine which operator type to create.
        return;
    }

    Operator::Type opTypeToCreate = static_cast<Operator::Type>(params[0]);
    Operator* newOp = nullptr;
    // TODO typically an id would be present, not just generated for no reason. 
    try {
        // 1. Generate a new, valid ID from this layer's internal generator.
        // This will throw an exception if the layer is full or cannot generate an ID.
        uint32_t newOpId = generateNextId();

        // 2. Instantiate the correct operator subclass based on the type.
        switch (opTypeToCreate) {
            case Operator::Type::ADD:
                // Pass any additional parameters if the AddOperator constructor needs them.
                // For now, using the default constructor.
                newOp = new AddOperator(newOpId);
                break;
            case Operator::Type::IN:
                newOp = new InOperator(newOpId);
                break;
            case Operator::Type::OUT:
                newOp = new OutOperator(newOpId);
                break;
            default:
                // Unknown type, do nothing.
                return;
        }

        // 3. Add the fully formed operator to this layer.
        // addNewOperator handles validation (against the now-extended reservedRange) and insertion.
        addNewOperator(newOp);

    } catch (const std::exception& e) {
        // If ID generation failed (e.g., dynamic layer hit uint32_max) or another error occurred.
        // Clean up the partially created operator if it exists.
        delete newOp;
        // Optional: Log the error, e.g., std::cerr << "Failed to create operator: " << e.what() << std::endl;
        // We will not re-throw here, as one failed UpdateEvent should not halt the entire update loop.
    }
}

/**
 * @brief Deletes an operator from this layer.
 * @details Finds the operator by its ID and removes it from the map, freeing its memory.
 * This action is typically only permitted for operators within a dynamic layer to maintain
 * the integrity of static Input/Output layers.
 * @param targetOperatorId The ID of the operator to delete.
 */
void Layer::deleteOperator(uint32_t targetOperatorId) {  // TODO temporary, method implementation
    // For safety, let's enforce that operators can only be deleted from dynamic layers at runtime.
    if (isRangeFinal) {
        // Silently ignore request to delete from a static layer.
        return;
    }

    if(!reservedRange->contains(targetOperatorId)){
        return; // operator not in range
    }

    // Find the operator in this layer's map.
    auto it = operators.find(targetOperatorId);
    if (it != operators.end()) {
        // Operator found.
        Operator* opToDelete = it->second;

        // Erase the entry from the map.
        operators.erase(it);

        // Delete the operator object to free its memory.
        delete opToDelete;

        // NOTE on currentMinId / currentMaxId:
        // For performance, we do not recalculate the layer's actual min/max ID bounds on deletion.
        // If the deleted operator was the minimum or maximum, the 'currentMinId' or 'currentMaxId'
        // will now reflect a historical bound, not the tightest possible bound on the current operators.
        // This is an acceptable trade-off as 'generateNextId' will still function correctly based
        // on the historical 'currentMaxId', ensuring new IDs remain unique. Recalculating
        // would require an O(N) iteration over the map for every deletion at the boundary.
    }
    // If the operator was not found in this layer, do nothing.
}

// TODO maybe add enclosed bool option for allow subclass layers to append own layer specific data after the base class layer data.  
std::string Layer::toJson(bool prettyPrint, int depth) const{
    std::ostringstream oss;
    std::string indent = prettyPrint ? "  " : "";
    std::string newline = prettyPrint ? "\n" : "";
    std::string space = prettyPrint ? " " : "";
    int opIndentLevel = prettyPrint? 2 + depth : 0; // for proper nesting of operators json

    oss << "{" << newline;

    // --- Layer Metadata ---
    oss << indent << "\"layerType\":" << space << "\"" << static_cast<int>(this->type) << "\"," << newline;
    oss << indent << "\"isRangeFinal\":" << space << (this->isRangeFinal ? "true" : "false") << "," << newline;
    
    if (this->reservedRange) {
        oss << indent << "\"reservedRange\":" << space << "{" << newline;
        oss << indent << indent << "\"minId\":" << space << this->reservedRange->getMinId() << "," << newline;
        oss << indent << indent << "\"maxId\":" << space << this->reservedRange->getMaxId() << newline;
        oss << indent << "}," << newline;
    }
    
    oss << indent << "\"operatorCount\":" << space << this->operators.size() << "," << newline;
    
    // --- Operators Array ---
    oss << indent << "\"operators\":" << space << "[";
    if(!operators.empty()){ // only create newline if array not empty/has operator/s
        oss << newline; // open bracket
    
        // Sort operators by ID for deterministic JSON output
        std::vector<const Operator*> sortedOps;
        sortedOps.reserve(operators.size());
        for (const auto& pair : operators) {
            sortedOps.push_back(pair.second);
        }
        std::sort(sortedOps.begin(), sortedOps.end(), 
                [](const Operator* a, const Operator* b){ return a->getId() < b->getId(); });

        bool firstOp = true;
        for (const auto* op : sortedOps) {
            if (!firstOp) {
                oss << "," << newline;
            }
            
            oss << op->toJson(prettyPrint, true, opIndentLevel); // print operator and indent by 1
            firstOp = false;
        }
        oss << newline << indent; // end bracket on newline and indent
    }
    oss << "]" << newline; // End of operators array
    oss << "}"; // End of layer object

    return oss.str();
}



/* TODO check if passing the vector<byte> array is efficient, given each operator has its may have its own byte vector (which it then copies), 
instead of just adding to a stream or something */
std::vector<std::byte> Layer::serializeToBytes() const {
    std::vector<std::byte> fullLayerBlock;
    // 1. Serialize all operators into a temporary buffer
    std::vector<std::byte> operatorDataSegment;
    // For deterministic output, sort operators by ID before serializing
    std::vector<const Operator*> sortedOps; // TODO what if their are too many operators to sort, for example using a database at main storage of operators? 
    sortedOps.reserve(operators.size());
    for (const auto& pair : operators) { sortedOps.push_back(pair.second); }
    std::sort(sortedOps.begin(), sortedOps.end(), 
              [](const Operator* a, const Operator* b){ return a->getId() < b->getId(); });

    for (const Operator* op : sortedOps) { // Use sortedOps
        if (op) {
            std::vector<std::byte> opBytes = op->serializeToBytes();
            operatorDataSegment.insert(operatorDataSegment.end(), opBytes.begin(), opBytes.end());
        }
    }

    // 2. Calculate 
    uint32_t numBytesForPayload = sizeof(uint32_t)              // Size of min range
                                  + sizeof(uint32_t)            // Size of max range
                                  + operatorDataSegment.size(); // Size of all serialized operators

    // 3. Write Layer Header to fullLayerBlock
    // 3a. Layer Type
    Serializer::write(fullLayerBlock, static_cast<uint8_t>(this->type));

    // 3b. isRangeFinal Flag
    Serializer::write(fullLayerBlock, static_cast<uint8_t>(this->isRangeFinal ? 1 : 0));

    // 3c. NumBytesOfPayloadBlock
    Serializer::write(fullLayerBlock, numBytesForPayload);

    // write payload header min ID
    Serializer::write(fullLayerBlock, reservedRange->getMinId());

    // write payload header max ID
    Serializer::write(fullLayerBlock, reservedRange->getMaxId());

    // 4. Append the actual operator data segment
    if (!operatorDataSegment.empty()) {
        fullLayerBlock.insert(fullLayerBlock.end(), operatorDataSegment.begin(), operatorDataSegment.end());
    }

    return fullLayerBlock;
}


// Private member implementation of the helper function
bool Layer::compareOperatorMaps(const Layer& other) const {
    // Purpose: Perform a deep comparison of the `operators` member maps.
    // Parameters:
    // - @param other: The Layer whose map to compare against.
    // Return: True if maps contain the same keys pointing to equal Operator objects.
    // Key Logic Steps:
    // 1. Check if the map sizes are equal.
    // 2. Iterate through this layer's map.
    // 3. For each operator, check if the other map contains the same ID.
    // 4. If it does, perform a deep, polymorphic comparison on the two Operator objects.
    
    const auto& mapA = this->operators;
    const auto& mapB = other.operators;

    if (mapA.size() != mapB.size()) {
        return false;
    }

    for (const auto& pairA : mapA) {
        const uint32_t& key = pairA.first;
        const Operator* opA = pairA.second;

        auto itB = mapB.find(key);
        if (itB == mapB.end()) {
            return false; // Key from this map not found in the other.
        }
        const Operator* opB = itB->second;

        if (!(*opA == *opB)) { // This uses the polymorphic operator== for Operator
            return false;
        }
    }
    return true;
}

// Public virtual `equals` implementation
bool Layer::equals(const Layer& other) const {
    // Purpose: Compare the state of the base Layer class.
    // Parameters:
    // - @param other: The Layer to compare against. Assumes the non-member operator==
    //   has already verified that the concrete types are identical.
    // Return: True if all persistent base members are equal.
    // Key Logic Steps:
    // 1. Compare primitive members (`isRangeFinal`).
    // 2. Compare the `reservedRange` objects (handling nulls).
    // 3. Delegate the deep comparison of the `operators` map to the private helper.
    // Note: `currentMinId` and `currentMaxId` are not checked as they are derived
    // from the state of the `operators` map, which is checked more thoroughly.

    if (this->isRangeFinal != other.isRangeFinal) {
        return false;
    }

    if (this->reservedRange == nullptr || other.reservedRange == nullptr) {
        if (this->reservedRange != other.reservedRange) return false;
    } else if (*(this->reservedRange) != *(other.reservedRange)) {
        return false;
    }

    return this->compareOperatorMaps(other);
}

// Non-member operator== implementation (The "Gatekeeper")
bool operator==(const Layer& lhs, const Layer& rhs) {
    // Purpose: The primary entry point for polymorphic Layer comparison.
    // Key Logic Steps:
    // 1. Perform the essential type check. This is its main responsibility.
    // 2. Delegate to the virtual `equals` member function for detailed state comparison.
    if (lhs.getLayerType() != rhs.getLayerType()) {
        return false;
    }
    return lhs.equals(rhs);
}

// Non-member operator!= implementation
bool operator!=(const Layer& lhs, const Layer& rhs) {
    return !(lhs == rhs);
}


