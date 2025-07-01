#pragma once

#include "gtest/gtest.h"
#include "controllers/MetaController.h"
#include "util/Randomizer.h"
#include "operators/Operator.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>
//#include "Payload.h"
//#include "UpdateEvent.h"

/**
 * @class MockMetaController
 * @brief A mock implementation of the MetaController for unit testing.
 *
 * @details
 * Purpose: This class inherits from MetaController and overrides its virtual methods.
 * It is intended to be used in unit tests for classes that depend on MetaController,
 * such as TimeController, UpdateController, and Simulator. Using this mock allows you to
 * isolate the class-under-test from the real MetaController's logic.
 *
 * Usage in Tests:
 * 1.  Instantiate this MockMetaController.
 * 2.  Pass a pointer or reference to the mock to the constructor of the class you are testing.
 * 3.  Use the mock's public tracking variables (e.g., `lastCall`, `callCount`)
 * and GTest assertions (e.g., EXPECT_EQ) to verify that the class-under-test
 * interacted with the MetaController as expected.
 */
class MockMetaController : public MetaController {
public:
    // Enum to track the last method called, for easy verification in tests.
    enum class LastCall {
        NONE,
        LOAD_CONFIG,
        SAVE_CONFIG,
        RANDOMIZE_NETWORK,
        MESSAGE_OP,
        PROCESS_OP_DATA,
        TRAVERSE_PAYLOAD,
        GET_OUTPUT,
        INPUT_TXT,
        GET_JSON,
        GET_OP_COUNT,
        PRINT,
        HANDLE_CREATE_OPERATOR,
        HANDLE_DELETE_OPERATOR,
        HANDLE_PARAM_CHANGE,
        HANDLE_ADD_CONN,
        HANDLE_REMOVE_CONN,
        HANDLE_MOVE_CONN
        // Note: Methods like getLayerCount, getAllLayers, findLayerForOperator are not tracked
        // as they are typically used by tests for setup/verification rather than being the
        // primary action under test. They can be added here if needed.
    };

    // --- Public State for Test Inspection ---
    LastCall lastCall = LastCall::NONE;
    int callCount = 0;
    
    // Parameters for method calls
    std::string lastFilePath = "NULL";
    uint32_t lastOperatorId = 0;
    int lastMessageData = 0;
    std::string lastInputText = "NULL"; 
    //Payload* lastPayloadPtr = nullptr;
    //std::vector<int> lastParams;
    const int maxId = 10; // used to define what ids are valid, allowing methods to simulate successful messages and etc. 

    // Internal state to simulate the network for testing
    std::vector<std::unique_ptr<Operator>> mockOperators; // ids are by index only, no map

    // --- Constructor & Destructor ---
    
    /**
     * @brief Default constructor for the mock.
     * @details Calls the base MetaController constructor with an empty string,
     * ensuring the base class is properly initialized in a default state.
     */
    MockMetaController(Randomizer* randomizer) : MetaController("", randomizer) {}
    virtual ~MockMetaController() override = default;

    /**
     * @brief Constructor for the mock that matches the base class.
     * @param numOperators The number of operators to initialize with (passed to base).
     * @param randomizer A pointer to the randomizer instance (passed to base).
     */
    MockMetaController(int numOperators, Randomizer* randomizer)
        : MetaController(numOperators, randomizer) {}

    // --- Helper for Test Setup ---

    /**
     * @brief Resets all tracking variables to their default state.
     */
    void reset() {
        lastCall = LastCall::NONE;
        callCount = 0;
        lastFilePath.clear();
        lastOperatorId = 0;
        lastMessageData = 0;
        mockOperators.clear();
    }



    // --- Overridden Public API of MetaController ---

    bool loadConfiguration(const std::string& filePath) override {
        callCount++;
        lastCall = LastCall::LOAD_CONFIG;
        lastFilePath = filePath;
        return true; // Simulate success
    }

    bool saveConfiguration(const std::string& filePath) const override {
        auto* nonConstThis = const_cast<MockMetaController*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::SAVE_CONFIG;
        nonConstThis->lastFilePath = filePath;
        return true;
    }

    void randomizeNetwork(int numOperators) override {
        callCount++;
        lastCall = LastCall::RANDOMIZE_NETWORK;
    }
    
    // Note: Simple getters used for test setup/verification are not mocked to track calls,
    // they just return the state of the mock.
    size_t getLayerCount() const override { return 3; /* Or a mockable value */ }
    size_t getOpCount() const override { return maxId; }



    bool messageOp(uint32_t operatorId, int message) override {
        callCount++;
        lastCall = LastCall::MESSAGE_OP;
        lastOperatorId = operatorId;
        lastMessageData = message;
        // Simulate delivery by checking our internal mock map
        if (operatorId <= maxId) {
            // simulate as if message sent
            return true;
        }

        return false; // other wise return false; 
    }

    void processOpData(uint32_t operatorId) override {
        callCount++;
        lastCall = LastCall::PROCESS_OP_DATA;
        lastOperatorId = operatorId;
    }

    void traversePayload(Payload* payload) override {
        callCount++;
        lastCall = LastCall::TRAVERSE_PAYLOAD;
    }
    
    std::string getOutput() const override{
        // can't adjust values because const. 
        // callCount++;
        // lastCall = LastCall::GET_OUTPUT;
        return "MetaController Has Output";
    }

    bool inputText(std::string input) override {
        callCount++;
        lastCall = LastCall::INPUT_TXT;
        lastInputText = input;
        return input == "" ? true: false;
    }

    /* NOT NEEDED ATM
    // --- Mocked Update Handlers ---
    void handleCreateOperator(const std::vector<int>& params) override {
        callCount++;
        lastCall = LastCall::HANDLE_CREATE_OPERATOR;
        lastParams = params;
    }

    void handleDeleteOperator(uint32_t targetOperatorId) override {
        callCount++;
        lastCall = LastCall::HANDLE_DELETE_OPERATOR;
        lastOperatorId = targetOperatorId;
    }

    void handleParameterChange(uint32_t targetOperatorId, const std::vector<int>& params) override {
        callCount++;
        lastCall = LastCall::HANDLE_PARAM_CHANGE;
        lastOperatorId = targetOperatorId;
        lastParams = params;
    }

    void handleAddConnection(uint32_t targetOperatorId, const std::vector<int>& params) override {
        callCount++;
        lastCall = LastCall::HANDLE_ADD_CONN;
        lastOperatorId = targetOperatorId;
        lastParams = params;
    }

    void handleRemoveConnection(uint32_t targetOperatorId, const std::vector<int>& params) override {
        callCount++;
        lastCall = LastCall::HANDLE_REMOVE_CONN;
        lastOperatorId = targetOperatorId;
        lastParams = params;
    }
    
    void handleMoveConnection(uint32_t targetOperatorId, const std::vector<int>& params) override {
        callCount++;
        lastCall = LastCall::HANDLE_MOVE_CONN;
        lastOperatorId = targetOperatorId;
        lastParams = params;
    }
    */

    // --- Mocked Info/Getter methods ---
    std::string getOperatorsAsJson(bool prettyPrint = true) const override {
        // can't adjust values because const. 
        // callCount++;
        // lastCall = LastCall::GET_JSON;
        return "{ \"mockNetwork\": true }";
    }

    void adjustCall(LastCall callType){
        callCount++;
        lastCall = callType;
    }

    void printOperators(bool prettyPrint = true) const override {
        // can't adjust values because const. 
        // No-op for mock
        // callCount++;
        // lastCall = LastCall::PRINT;
    }


    // -------- BASE CLASS METHOD CALLS -----------------------

    // Private

    Operator* baseGetOperatorPtr(uint32_t operatorId) const{
        return MetaController::getOperatorPtr(operatorId);
    }

    void baseClearAllLayers(){
        MetaController::clearAllLayers();
    }

    Layer* baseGetDynamicLayer(){
        return MetaController::getDynamicLayer();
    }

   
    void baseValidateLayerIdSpaces() const{
        MetaController::validateLayerIdSpaces();
    }

  
    uint32_t baseGetNextIdForNewRange() const{
        return MetaController::getNextIdForNewRange();
    }


    void baseRandomizeNetwork(int numOperators){
        MetaController::randomizeNetwork(numOperators);
    }

    // Public

	void baseRemoveOperatorPtr(int operatorId){
        MetaController::removeOperatorPtr(operatorId);
    }


    size_t baseGetLayerCount() const{
        return MetaController::getLayerCount();
    }


    size_t baseGetOpCount() const{
        return MetaController::getOpCount();
    }
    

    const std::vector<std::unique_ptr<Layer>>& baseGetAllLayers() const{
        return MetaController::getAllLayers();
    }


    void baseSortLayers(){
        MetaController::sortLayers();
    }


    Layer* baseFindLayerForOperator(uint32_t operatorId) const{
        return MetaController::findLayerForOperator(operatorId);
    }

 
    bool baseMessageOp(uint32_t operatorId, int message){
        return MetaController::messageOp(operatorId, message);
    }

    void baseProcessOpData(uint32_t operatorId){
        MetaController::processOpData(operatorId);
    }

    void baseTraversePayload(Payload* payload){
        MetaController::traversePayload(payload);
    }

    void baseHandleCreateOperator(const std::vector<int>& params){
        MetaController::handleCreateOperator(params);
    }

    void baseHandleDeleteOperator(uint32_t targetOperatorId){
        MetaController::handleDeleteOperator(targetOperatorId);
    }


    void baseHandleParameterChange(uint32_t targetOperatorId, const std::vector<int>& params){
        MetaController::handleParameterChange(targetOperatorId, params);
    }

  
    void baseHandleAddConnection(uint32_t targetOperatorId, const std::vector<int>& params){
        MetaController::handleAddConnection(targetOperatorId, params);
    }

    
    void baseHandleRemoveConnection(uint32_t targetOperatorId, const std::vector<int>& params){
        MetaController::handleRemoveConnection(targetOperatorId, params);
    }

    
    void baseHandleMoveConnection(uint32_t targetOperatorId, const std::vector<int>& params){
        MetaController::handleMoveConnection(targetOperatorId, params);
    }

    
    bool baseSaveConfiguration(const std::string& filePath) const{
        return MetaController::saveConfiguration(filePath);
    }


    bool baseLoadConfiguration(const std::string& filePath){
        return MetaController::loadConfiguration(filePath);
    }


    
    std::string baseGetOperatorsAsJson(bool prettyPrint = true) const{
        return MetaController::getOperatorsAsJson(prettyPrint);
    }

  
    void basePrintOperators(bool prettyPrint = true) const{
        MetaController::printOperators(prettyPrint);
    }

   
    std::string baseGetOutput() const{
        return MetaController::getOutput();
    }

    bool baseInputText(std::string input){
        return MetaController::inputText(input);
    }

    bool baseIsEmpty() const{
        return MetaController::isEmpty();
    }


};