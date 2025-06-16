#pragma once

#include <vector>
#include <queue> // Using queue might be slightly more appropriate than vector
#include <string>
#include <iosfwd>

// Forward Declarations
class MetaController; // Required for dependency injection
struct UpdateEvent;   // Required for the queue

/**
 * @class UpdateController
 * @brief Manages the processing of state and structural updates between Time Loop steps.
 * @details Instantiated externally, requires MetaController. Processes a queue
 * of UpdateEvents submitted via UpdateScheduler. Ensures atomic updates.
 */
class UpdateController {
private:
	// Injected dependency (set via constructor)
	MetaController& metaControllerInstance;

	// Queue of pending update requests
	std::queue<UpdateEvent> updateQueue;

public:
	/**
 	 * @brief Constructor for UpdateController.
 	 * @param metaController A reference to the simulation's MetaController instance.
 	 */
	UpdateController(MetaController& metaController);


	/**
     * @brief Constructor for UpdateController (Attempts initial state load).
     * @param metaController A reference to the simulation's MetaController instance.
     * @param stateFilePath Path to the state file containing queued UpdateEvents to load.
     * @details If stateFilePath is provided, calls loadState(). Logs warning on failure.
     * State File Format (Condensed - Big Endian): Sequence of UpdateEvent Blocks.
     * - UpdateEvent Block: [uint8_t Size N][N bytes of Event Data] (See UpdateEvent format)
     */
    explicit UpdateController(MetaController& metaController, const std::string& stateFilePath);

	/**
 	 * @brief Destructor for UpdateController.
 	 */
	virtual ~UpdateController();

	/**
 	 * @brief Adds an UpdateEvent to the processing queue.
 	 * @param event The UpdateEvent to be queued.
 	 * @return Void.
 	 * @note Called by UpdateScheduler::Submit.
 	 */
	void AddToQueue(const UpdateEvent& event);

	/**
 	 * @brief Processes all events currently in the update queue.
 	 * @param None
 	 * @return Void.
 	 * @details Iterates through the queue, coordinates with MetaController for
 	 * lifecycle events, retrieves Operator pointers, and calls internal
 	 * update methods on Operators for parameter/connection changes.
 	 * Clears the queue afterwards. Should be called between Time steps.
 	 */
	void ProcessUpdates();

	/**
 	 * @brief Checks if the update queue is empty.
 	 * @return bool True if the queue is empty, false otherwise.
 	 */
	bool IsQueueEmpty() const;

	/**
 	 * @brief Gets the current number of events in the update queue.
 	 * @return size_t The number of events waiting to be processed.
 	 */
	size_t QueueSize() const;


	// --- NEW State Persistence Methods ---

    /**
     * @brief Saves the current queue of UpdateEvents to a file.
     * @param filePath The path to the file where the queue state should be saved.
     * @return bool True if saving was successful, false otherwise.
     * @details Writes UpdateEvents sequentially using their serialization format. No header/count.
     * State File Format (Condensed - Big Endian): Sequence of UpdateEvent Blocks.
     * - UpdateEvent Block: [uint8_t Size N][N bytes of Event Data] (See UpdateEvent format)
     */
    bool saveState(const std::string& filePath) const;

    /**
     * @brief Loads a queue of UpdateEvents from a file.
     * @param filePath The path to the file containing the queue state.
     * @return bool True if loading was successful, false otherwise.
     * @details Clears the existing queue. Reads the file until EOF, creating UpdateEvents
     * from the data blocks and adding them to the queue.
     * State File Format (Condensed - Big Endian): Sequence of UpdateEvent Blocks.
     * - UpdateEvent Block: [uint8_t Size N][N bytes of Event Data] (See UpdateEvent format)
     */
    bool loadState(const std::string& filePath);

	// Prevent copying/assignment
	UpdateController(const UpdateController&) = delete;
	UpdateController& operator=(const UpdateController&) = delete;
};
