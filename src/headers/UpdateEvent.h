#pragma once

#include "util/Serializer.h"
#include <cstddef> // For std::byte
#include <utility> // For std::move
#include <iosfwd>  // Potentially for exceptions if they use streams
#include <vector>
#include <limits>
#include <stdexcept>
#include <string> // Optional: Include if adding string-based parameters or event IDs later

/**
 * @enum UpdateType
 * @brief Defines the different kinds of state/structural updates relevant to the Operator/Payload model.
 */
enum class UpdateType : uint16_t{
	// --- Operator Lifecycle ---
	CREATE_OPERATOR = 0, // Params: [0]=initialWeight, [1]=initialThreshold
	DELETE_OPERATOR = 1, // Params: empty

	// --- Operator State ---
	CHANGE_OPERATOR_PARAMETER = 2, // Params: [0]=paramId(0:weight, 1:threshold), [1]=newValue

	// --- Operator Connections ---
	ADD_CONNECTION = 3,	// Params: [0]=targetOpIdToAdd, [1]=distance
	REMOVE_CONNECTION = 4, // Params: [0]=targetOpIdToRemove, [1]=distance
	MOVE_CONNECTION = 5,   // Params: [0]=targetOpIdToMove, [1]=oldDistance, [2]=newDistance

	// --- Other Potential Types (Add as needed) ---
	// SET_OPERATOR_STATE, // Example: params[0]=stateId, params[1]=value
	// CUSTOM_EVENT,   	// Example: params define custom event data
};

/**
 * @struct UpdateEvent
 * @brief Represents a request/event for a state or structural change processed during the Update Loop.
 * @details Contains the type of update, the target Operator ID, and necessary parameters
 * encoded as integers. Relies on conventions for parameter parsing based on UpdateType.
 */
struct UpdateEvent {
	UpdateType type;        	// What kind of update.
	// TODO check default -1 for uint32_t
	uint32_t targetOperatorId = -1;  // ID of the Operator primarily affected.
	// int requestingOperatorId = -1; // Optional: ID of operator requesting? Useful context? TBD.

	// TODO currently updateEvent params must all be ints, therefore given IDS current type is uint32_t, must cast to int, potentially losing information and storing wrong ID
	// TODO params need to support different types, other than just "int"
	// Parameters for the update. Parsing requires convention based on UpdateType (see enum comments).
	std::vector<int> params;	// Parameters encoded as integers. Parsing logic in UpdateController is crucial.

	/**
 	* @brief Default constructor.
 	*/
	UpdateEvent() = default;

	/**
 	* @brief Constructor to initialize main fields.
 	* @param type The type of update.
 	* @param targetId The ID of the target Operator.
 	* @param parameters A vector of integer parameters specific to the update type.
 	*/
	UpdateEvent(UpdateType type, uint32_t targetId, std::vector<int> parameters = {}) :
    	type(type),
    	targetOperatorId(targetId),
    	params(std::move(parameters))
	{}

	/**
	 * @brief Deserialization Constructor (UpdateEvent Format).
	 */
	UpdateEvent(const std::byte*& current, const std::byte* end) {
		// Expects data starting AFTER 1-byte size prefix

		// Field 2: Read Update Type (uint8_t)
		this->type = static_cast<UpdateType>(Serializer::read_uint8(current, end));

		// Fields 3 & 4: Read Target Operator ID (size + value BE)
		this->targetOperatorId = Serializer::read_int(current, end);

		// Field 5: Read Number of Parameters (uint8_t)
		uint8_t paramCount = Serializer::read_uint8(current, end);

		// Field 6: Read Parameter Value Type (uint8_t = 0 for int)
		uint8_t paramTypeCode = Serializer::read_uint8(current, end);
		const uint8_t expectedParamTypeCode = 0; // Expect int
		if (paramTypeCode != expectedParamTypeCode) {
			throw std::runtime_error("Unsupported parameter type code encountered: "
									+ std::to_string(paramTypeCode));
		}

		// Field 7: Read Sequence of Parameter Values (using Serializer::read_int)
		this->params.clear();
		this->params.reserve(paramCount);
		for (uint8_t i = 0; i < paramCount; ++i) {
			// Call the Serializer method which reads size prefix + value bytes BE
			int paramValue = Serializer::read_int(current, end);
			this->params.push_back(paramValue);
		}
	}

	/**
	 * @brief Serializes the UpdateEvent state into a length-prefixed byte vector.
	 */
	std::vector<std::byte> serializeToBytes() const {
		std::vector<std::byte> dataBuffer; // Holds fields 2-7

		// Field 2: Update Type (uint8_t)
		Serializer::write(dataBuffer, static_cast<uint8_t>(this->type));

		// Fields 3 & 4: Target Operator ID (Size + Value BE)
		Serializer::write(dataBuffer, this->targetOperatorId);

		// Field 5: Number of Parameters (uint8_t)
		if (this->params.size() > std::numeric_limits<uint8_t>::max()) {
			throw std::overflow_error("UpdateEvent has too many parameters ("
									+ std::to_string(this->params.size())
									+ ") for 1-byte count serialization.");
		}
		uint8_t paramCount = static_cast<uint8_t>(this->params.size());
		Serializer::write(dataBuffer, paramCount);

		// Field 6: Parameter Value Type (uint8_t = 0 for int)
		const uint8_t paramTypeCode = 0; // 0 indicates int for now
		Serializer::write(dataBuffer, paramTypeCode);

		// Field 7: Sequence of Parameter Values (using Serializer::write(int))
		for (int paramValue : this->params) {
			// Call the Serializer method which handles size prefix + value bytes BE
			Serializer::write(dataBuffer, paramValue);
		}

		// Calculate size N and check bounds (max 255 for uint8_t prefix)
		size_t dataSize = dataBuffer.size();
		if (dataSize > std::numeric_limits<uint8_t>::max()) {
			throw std::overflow_error("Serialized UpdateEvent data size (" + std::to_string(dataSize)
									+ ") exceeds maximum representable by 1-byte length prefix (255).");
		}
		uint8_t totalDataSizeN = static_cast<uint8_t>(dataSize);

		// Create final buffer
		std::vector<std::byte> finalBuffer;
		finalBuffer.reserve(1 + totalDataSizeN);

		// Field 1: Write 1-byte size prefix N
		finalBuffer.push_back(static_cast<std::byte>(totalDataSizeN));

		// Append the data bytes
		finalBuffer.insert(finalBuffer.end(), dataBuffer.begin(), dataBuffer.end());

		return finalBuffer;
	}

	// Note: Using vector<int> is flexible but less type-safe. Requires careful handling.
	// Future enhancements could involve std::variant or specific structs per type.
};
