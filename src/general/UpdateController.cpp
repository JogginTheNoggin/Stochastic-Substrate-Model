// UpdateController.cpp

#include "../headers/controllers/UpdateController.h"
#include "../headers/controllers/MetaController.h" // Required for coordinating updates
#include "../headers/UpdateEvent.h"    // Required for event type and queue
#include "../headers/util/Serializer.h"     // For reading size byte during load
#include <queue>
#include <fstream>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <stdexcept> // For parameter validation (though validation is now in MetaController)

/**
 * @brief Constructor for UpdateController.
 * @param metaController A reference to the simulation's MetaController instance.
 * @details Stores a reference to the MetaController to enable dispatching of
 * update requests to the appropriate handler methods within MetaController.
 */
UpdateController::UpdateController(MetaController& metaController) :
    metaControllerInstance(metaController)
{
    // updateQueue is default-initialized (empty)
}

/**
 * @brief Constructor for UpdateController (Attempts initial state load).
 */
UpdateController::UpdateController(MetaController& metaController, const std::string& stateFilePath) :
    metaControllerInstance(metaController) // Initialize reference
{
    // Purpose: Initialize UpdateController, associate with MetaController, load initial queue state.
    // Parameters: metaController, stateFilePath.
    // Return: N/A.
    // Key Logic: Store metaController, call loadState if path given, log warning on failure.
    if (!stateFilePath.empty()) {
        std::cout << "Attempting to load initial UpdateController state from: " << stateFilePath << std::endl;
        if (!loadState(stateFilePath)) {
            std::cerr << "Warning: Failed to load initial UpdateController state from: " << stateFilePath << std::endl;
            // Ensure queue is empty if load failed
            std::queue<UpdateEvent> emptyQueue;
            std::swap(updateQueue, emptyQueue); // Clear queue efficiently
        } else {
            std::cout << "Successfully loaded initial UpdateController state. Queue size: " << updateQueue.size() << std::endl;
        }
    }
}

/**
 * @brief Destructor for UpdateController.
 */
UpdateController::~UpdateController()
{
    // No explicit cleanup needed for current members
}

/**
 * @brief Adds an UpdateEvent to the processing queue.
 * @param event The UpdateEvent to be queued.
 * @return Void.
 * @details Called by UpdateScheduler::Submit. Pushes event onto the internal queue.
 */
void UpdateController::AddToQueue(const UpdateEvent& event)
{
    updateQueue.push(event);
}

/**
 * @brief Processes all events currently in the update queue by dispatching to MetaController.
 * @param None
 * @return Void.
 * @details Iterates through the queue. For each event, it calls the corresponding
 * public handler method on the `MetaController` instance, passing the necessary
 * parameters (target ID, params vector). `MetaController` now contains the logic
 * for interacting with the actual `Operator` objects. Clears the queue afterwards.
 */
void UpdateController::ProcessUpdates()
{
    while (!updateQueue.empty()) {
        UpdateEvent event = updateQueue.front();
        updateQueue.pop();

        // Dispatch event handling to MetaController
        try { // Optional: Add try-catch around MetaController calls if handlers can throw
            switch (event.type) {
                case UpdateType::CREATE_OPERATOR:
                    metaControllerInstance.handleCreateOperator(event.params);
                    break;

                case UpdateType::DELETE_OPERATOR:
                    metaControllerInstance.handleDeleteOperator(event.targetOperatorId);
                    break;

                case UpdateType::CHANGE_OPERATOR_PARAMETER:
                    metaControllerInstance.handleParameterChange(event.targetOperatorId, event.params);
                    break;

                case UpdateType::ADD_CONNECTION:
                    metaControllerInstance.handleAddConnection(event.targetOperatorId, event.params);
                    break;

                case UpdateType::REMOVE_CONNECTION:
                    metaControllerInstance.handleRemoveConnection(event.targetOperatorId, event.params);
                    break;

                 case UpdateType::MOVE_CONNECTION:
                    metaControllerInstance.handleMoveConnection(event.targetOperatorId, event.params);
                    break;

                default: // Add cases for other UpdateTypes as needed
                    // TODO: Log error or warning: Unhandled UpdateType
                    break;
            } // end switch
        } catch (const std::exception& e) {
             // TODO: Log error from MetaController handler: e.what()
             // Decide if processing should continue or stop on error
        }
    } // end while queue not empty
}

/**
 * @brief Checks if the update queue is empty.
 * @return bool True if the queue contains no UpdateEvents, false otherwise.
 */
bool UpdateController::IsQueueEmpty() const
{
    return updateQueue.empty();
}

/**
 * @brief Gets the current number of events in the update queue.
 * @return size_t The number of UpdateEvents waiting to be processed.
 */
size_t UpdateController::QueueSize() const
{
    return updateQueue.size();
}


// --- NEW State Persistence Method Implementations ---

/**
 * @brief Saves the current queue of UpdateEvents to a file.
 */
bool UpdateController::saveState(const std::string& filePath) const { // const ensures not going to modify data
    // Purpose: Save queued UpdateEvents to file.
    // Parameters: filePath.
    // Return: True on success, False otherwise.
    // Key Logic: Open file, copy queue, iterate copy, serialize event, write block, close.

    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for saving UpdateController state: " << filePath << std::endl;
        return false;
    }

    // Copy the queue to iterate without modifying the original during save
    std::queue<UpdateEvent> queueCopy = updateQueue; // TODO copying not quite ideal, for memory
    // necessary to pop etc, would have to reinsert
    // but we could just, track an index, and re-push once we pop
    try {
        while (!queueCopy.empty()) {
            const UpdateEvent& event = queueCopy.front(); // Get ref to front

            std::vector<std::byte> eventBytes = event.serializeToBytes(); // Includes 1-byte size prefix

            if (!eventBytes.empty()) { // Should always have size byte
                 outFile.write(reinterpret_cast<const char*>(eventBytes.data()), eventBytes.size());
                 if (!outFile.good()) {
                     throw std::runtime_error("Failed to write UpdateEvent data.");
                 }
            }
            queueCopy.pop(); // Remove element from copy
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Exception during UpdateController::saveState: " << e.what() << std::endl;
        outFile.close();
        return false;
    }

    outFile.close();
    return outFile.good();
}

/**
 * @brief Loads a queue of UpdateEvents from a file.
 */
bool UpdateController::loadState(const std::string& filePath) {
    // Purpose: Load UpdateEvents from file into queue.
    // Parameters: filePath.
    // Return: True on success, False otherwise.
    // Key Logic: Clear queue, open file, loop: read 1-byte size N, read N bytes, create Event, AddToQueue.

    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file for loading UpdateController state: " << filePath << std::endl;
        return false;
    }

    // 1. Clear the existing queue
    // std::queue<UpdateEvent>().swap(updateQueue); // Efficient way to clear
    // TODO is swap really efficient? 
     std::queue<UpdateEvent> emptyQueue;
     std::swap(updateQueue, emptyQueue);


    try {
         std::streampos startPos;
         while (inFile.peek() != EOF) { // Check before reading size
            startPos = inFile.tellg(); // does this affect the read 

            // 1. Read 1-byte size prefix (N)
            char sizeByteChar;
            inFile.read(&sizeByteChar, 1); // isn't this going to affect the eventDataBuffer, isn't size suppose to contain this value to, 
             if (inFile.gcount() != 1) {
                 if (inFile.eof()) break; // Normal EOF
                 else throw std::runtime_error("Failed to read 1-byte size prefix for UpdateEvent at pos " + std::to_string(startPos));
             }
            uint8_t dataSizeN = static_cast<uint8_t>(sizeByteChar);

            if (dataSizeN == 0) {
                 throw std::runtime_error("Encountered UpdateEvent block with declared size 0 at pos " + std::to_string(startPos));
            }

            // 2. Read the next N bytes
            std::vector<std::byte> eventDataBuffer(dataSizeN);
            inFile.read(reinterpret_cast<char*>(eventDataBuffer.data()), dataSizeN);
            if (static_cast<uint8_t>(inFile.gcount()) != dataSizeN) {
                throw std::runtime_error("Failed to read expected " + std::to_string(dataSizeN) + " data bytes for UpdateEvent at pos " + std::to_string(startPos));
            }

            // 3. Parse the Data Block & Create Event
            const std::byte* dataPtr = eventDataBuffer.data(); // Start of data (Type field)
            const std::byte* dataEnd = dataPtr + dataSizeN;    // End of data

            UpdateEvent loadedEvent(dataPtr, dataEnd); // Use deserialization constructor

            // Check consumption
            if (dataPtr != dataEnd) {
                 throw std::runtime_error("UpdateEvent constructor did not consume entire data block. Size mismatch. Pos " + std::to_string(startPos));
            }

            // 4. Add the successfully loaded event to the queue
            // Use AddToQueue if it does more than just push (e.g., logging)
            // AddToQueue(loadedEvent);
            // Or just push directly if AddToQueue is simple
            updateQueue.push(std::move(loadedEvent));

        } // End while loop

    } catch (const std::exception& e) {
        std::cerr << "Error: Exception during UpdateController::loadState: " << e.what() << std::endl;
        // Clear potentially partially loaded state
        std::queue<UpdateEvent> emptyQueueOnError;
        std::swap(updateQueue, emptyQueueOnError);
        inFile.close();
        return false; // Loading failed
    }

    inFile.close();
    return true; // Loading successful
}
