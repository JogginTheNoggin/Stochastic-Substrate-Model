#pragma once

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

// Forward Declarations
class Operator;
class Layer;      // The abstract base class for layers
class Payload;
class Randomizer; 
struct UpdateEvent;
struct IdRange;

/**
 * @class MetaController
 * @brief Acts as the high-level orchestrator for Layer objects and enforces system-wide rules.
 * @details The MetaController is a manager of a collection of Layer objects. 
 * It is responsible for loading/saving the entire network configuration by
 * orchestrating the serialization of individual layers, validating inter-layer consistency (e.g.,
 * non-overlapping ID ranges), and delegating runtime operator modifications to the appropriate Layer instance.
 */
class MetaController {
private:
    Randomizer* rand; 
    // --- Core State ---
    
    /**
     * @brief A collection of unique pointers to the polymorphic Layer objects that constitute the network.
     * @details MetaController owns the Layer objects. Using std::unique_ptr ensures that when a Layer is
     * removed from this vector or when MetaController is destroyed, the Layer's destructor is called,
     * which in turn cleans up all the Operators it owns.
     */
    std::vector<std::unique_ptr<Layer>> layers;

    /**
     * @brief Retrieves a pointer to an Operator object by its unique ID.
     * @details This method now delegates the search to the contained layers. It iterates through
     * each layer and asks it for the operator. To optimize, it may first check if the ID
     * falls within a layer's known reserved ID range before querying it.
     * @param operatorId The unique ID of the Operator to retrieve.
     * @return A raw pointer to the Operator object, or nullptr if no operator with that ID is found in any layer.
     */
    Operator* getOperatorPtr(uint32_t operatorId) const;

    // --- Private Helper Methods ---

    /**
     * @brief Deletes all managed Layer objects and resets the MetaController's state.
     * @details Called by the destructor or before loading a new configuration to ensure a clean slate.
     * Iterates through the `layers` vector, which automatically calls the destructor for each
     * Layer object due to `std::unique_ptr`, thereby deleting all contained operators.
     */
    void clearAllLayers();

    /**
     * @brief Returns last layer (and therefore the dynamic layer) in the sorted list of layers. 
     * @return A raw pointer to the dynamic layer (`isRangeFinal == false`), or nullptr if no dynamic layer is found.
     * @throws std::runtime_error if more than one dynamic layer is found, as this violates a core system rule.
     * @note This corresponds to the layer with the highest min and max ID, there by being the only layer which can extends its ID range.  
     */
    Layer* getDynamicLayer();

    /**
     * @brief Performs system-wide validation checks after all layers have been loaded from a configuration.
     * @details This is a crucial step after deserialization to ensure network integrity. It enforces rules that
     * cannot be checked by any single layer in isolation, such as:
     * 1. There is exactly one dynamic layer.
     * 2. The dynamic layer's ID range is "after" all static layers' ID ranges.
     * 3. No two static layers have overlapping ID ranges.
     * @throws std::runtime_error if any validation check fails.
     */
    void validateLayerIdSpaces() const;

    
    /**
     * @brief Calculates the next available ID to start a new layer's range.
     * @details This method should be called whenever a new layer is to be programmatically created.
     * It ensures the starting ID is always after the highest current ID in the entire system,
     * accounting for any growth in the dynamic layer.
     * @return The next valid, globally unique ID to start a new range.
     */
    uint32_t getNextIdForNewRange() const;

public:

    MetaController(int numOperators);


    /**
     * @brief Constructor for the MetaController.
     * @details Initializes an empty MetaController. If a configuration file path is provided,
     * it immediately attempts to load the network configuration from that file.
     * @param configPath Optional path to a network configuration file to load on startup.
     */
    MetaController(const std::string& configPath = "");
    
    /**
     * @brief Destructor.
     * ensures that all owned Layer objects (and consequently, all Operators they own) are properly deleted.
     */
    virtual ~MetaController();


    /**
     * @brief Generates a network with a specified number of operators of a given type
     * and randomizes their connections.
     * @param numOperators The total number of operators to create.
     * @param randomizer The randomizer for generating a random configuration. 
     * @details Creates layers with respective ranges, then calls their randomizeInit method on the appropriate ones.
     * Note: Modifications to the layer and its operators do not notify other intermediate class such as updateControllers etc. 
     * This may change in the future.
     */
    void randomizeNetwork(int numOperators, Randomizer* randomizer);

    // --- Public Operator Access ---



    /**
 	 * @brief Removes an Operator pointer from a specific layer. 
 	 * @param operatorId The ID of the Operator pointer to remove.
 	 * @return Void.
 	 * @note Cannot delete operators in input or output layers. (Maybe only allow )
 	 */
	void removeOperatorPtr(int operatorId);

    // --- Layer & Network Info ---

    /**
     * @brief Gets the number of layers currently managed by the MetaController.
     * @return size_t The number of layers.
     */
    size_t getLayerCount() const;

    /**
     * @brief Gets the total number of operators across all layers in the network.
     * @return size_t The total operator count.
     */
    size_t getTotalOperatorCount() const;
    
    /**
     * @brief Provides read-only access to the collection of layers.
     * @details Useful for systems that need to inspect or interact with layers based on their type
     * or other properties (e.g., a system for providing input to all InputLayers).
     * @return A const reference to the vector of unique_ptrs to Layer objects.
     */
    const std::vector<std::unique_ptr<Layer>>& getAllLayers() const;


    void sortLayers(); 


    Layer* findLayerForOperator(uint32_t operatorId) const;

    /**
     * @return, true if the layer with operator exist and message has been sent, false otherwise
     */
    bool messageOp(uint32_t operatorId, int message);

    void processOpData(uint32_t operatorId);

    void traversePayload(Payload* payload);




    // --- Update Event Handling ---

    /**
     * @brief Handles a CREATE_OPERATOR UpdateEvent.
     * @details Gets the single dynamic layer and delegates the operator creation request to it.
     * The dynamic layer is responsible for generating and assigning the new operator's ID.
     * @param params A vector of integers containing parameters for the new operator, such as its type.
     */
    void handleCreateOperator(const std::vector<int>& params);

    /**
     * @brief Handles a DELETE_OPERATOR UpdateEvent.
     * @details Finds which layer contains the target operator and delegates the deletion request to that layer.
     * This action is typically only permitted for operators within a dynamic layer.
     * @param targetOperatorId The ID of the operator to delete.
     */
    void handleDeleteOperator(uint32_t targetOperatorId);

    /**
     * @brief Delegates a parameter change request to the appropriate layer and operator.
     * @param targetOperatorId The ID of the operator to modify.
     * @param params A vector of integers specifying the parameter to change and its new value.
     */
    void handleParameterChange(uint32_t targetOperatorId, const std::vector<int>& params);

    /**
     * @brief Delegates a request to add a connection to the appropriate layer and operator.
     * @param targetOperatorId The ID of the operator that will be the source of the connection.
     * @param params A vector of integers specifying the target operator ID and distance for the new connection.
     */
    void handleAddConnection(uint32_t targetOperatorId, const std::vector<int>& params);

    /**
     * @brief Delegates a request to remove a connection.
     * @param targetOperatorId The ID of the operator from which the connection originates.
     * @param params A vector of integers specifying the target operator ID and distance to remove.
     */
    void handleRemoveConnection(uint32_t targetOperatorId, const std::vector<int>& params);

    /**
     * @brief Delegates a request to move a connection.
     * @param targetOperatorId The ID of the operator whose connection is being moved.
     * @param params A vector specifying the target ID, old distance, and new distance.
     */
    void handleMoveConnection(uint32_t targetOperatorId, const std::vector<int>& params);

    // --- Persistence ---

    /**
     * @brief Saves the entire network configuration to a file.
     * @details Orchestrates the serialization process by iterating through its Layer objects. For each layer,
     * it calls the layer's `serializeToBytes()` method to get a complete data block for that layer,
     * and writes that block to the specified file.
     * @param filePath The path to the file where the configuration will be saved.
     * @return True if saving was successful, false otherwise.
     */
    bool saveConfiguration(const std::string& filePath) const;

    /**
     * @brief Loads a complete network configuration from a file, replacing any existing state.
     * @details Clears any current layers. Reads the file sequentially, expecting a series of Layer Data Blocks.
     * For each block, it reads the header envelope (Type, Dynamic Status, Payload Size), instantiates the
     * correct Layer subclass, and passes the payload data to the layer's constructor for deserialization.
     * After all layers are loaded, it performs crucial system-wide validation via `validateLayerIdSpaces()`.
     * @param filePath The path to the file containing the configuration.
     * @return True if loading and validation were successful, false otherwise.
     */
    bool loadConfiguration(const std::string& filePath);

    // --- JSON Output ---

    /**
     * @brief Generates a JSON string representing the entire network, grouped by layer.
     * @details Iterates through all layers and calls their respective `toJson()` methods
     * to compose a single JSON array containing all layer and operator data.
     * The layers are sorted by their ID range to ensure a deterministic output.
     * @param prettyPrint If true, format the JSON for human readability.
     * @return A std::string containing the JSON representation of the network.
     */
    std::string getOperatorsAsJson(bool prettyPrint = true) const;

    /**
     * @brief Prints a JSON representation of the entire network to standard output.
     * @param prettyPrint If true, format the JSON for human readability.
     */
    void printOperators(bool prettyPrint = true) const;

    // Prevent copying to ensure clear ownership of layers and operators
    MetaController(const MetaController&) = delete;
    MetaController& operator=(const MetaController&) = delete;
};