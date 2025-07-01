#pragma once
#include <vector>
#include <cstddef> // For std::byte
#include <cstdint> // For fixed-width types
#include <limits>  // for numeric_limits
#include <stdexcept> // For exceptions
#include <string>    // For std::string
#include <iosfwd>    // For std::ostream forward declaration
#include <sstream>   // For std::ostringstream
//#include <iostream> // Needed here for std::cout
#include "util/Serializer.h" // Assuming Serializer.h exists

/**
 * @struct Payload
 * @brief Represents a discrete packet of information actively moving through the network.
 * @details Carries message data and the state necessary to track its journey between Operators,
 * managed by the originating Operator's traverse() method and the TimeController.
 */
struct Payload {
	// --- State Data ---
	int message = 0;        	// The data value being transmitted.
	uint32_t currentOperatorId = 0; // ID of the Operator managing this payload's current journey.
	uint16_t distanceTraveled = 0;   	//DEFAULT current distance payload traveled in current Operator, used to index for operator connections
	bool active = true;     	// Is the payload still traversing? (Set false when journey ends).

	/**
 	* @brief Default constructor.
 	*/
	Payload() = default;

	/**
 	* @brief Constructor to initialize main fields.
 	* @param msg The message data.
 	* @param sourceOpId The ID of the Operator initiating this payload's journey.
 	* @param startDistance The initial distance index (usually 0).
 	* @param isActive Initial active state (usually true).
 	*/
	Payload(int msg, uint32_t sourceOpId, int startDistance, bool isActive) :
    	message(msg),
    	currentOperatorId(sourceOpId),
    	distanceTraveled(startDistance),
    	active(isActive)
	{}

	/**
 	* @brief Constructor to initialize only message and operatorId, rest is set to defaults: 0, and false respectively.
 	* @param msg The message data.
 	* @param sourceOpId The ID of the Operator initiating this payload's journey.
 	*/
	Payload(int msg, uint32_t sourceOpId) :
    	message(msg),
    	currentOperatorId(sourceOpId)
	{
		/* 
		 could overload with defaults and not use this constructor but have
		 opted for this option for slight optimization by not assign defaults, 
		 given payload defaults are already set in class definition. 
		 Small but repeated work is avoided.  
		*/

	}


	// --- NEW Deserialization Constructor ---
    /**
     * @brief Deserialization Constructor (Payload Format).
     * @param current Reference to pointer to the current position in the byte stream.
     * **MUST** point to the start of the Payload Type field (AFTER the 1-byte size prefix).
     * Will be advanced past consumed bytes for this payload block.
     * @param end Pointer defining the boundary of this payload's data block.
     * @throws std::runtime_error If data is insufficient, format is invalid, etc.
     * @throws std::overflow_error If distanceTraveled read exceeds int max (unlikely with uint16_t).
     * @details Constructs a Payload by deserializing Type, OperatorID, Message, DistanceTraveled.
     * Initializes 'active' to true. Assumes input is from saved state (active payloads only).
     */
    explicit Payload(const std::byte*& current, const std::byte* end) :
        active(true) // Payloads loaded from state are assumed to be active
    {
        // Purpose: Initialize Payload from serialized bytes (Type, OpID, Msg, Dist).
        // Parameters: current (advances), end.
        // Return: N/A. Throws on error.
        // Key Logic: Use Serializer to read fields according to format. Initialize members.

        // 1. Read Payload Type (uint16_t BE) - Field 2
        uint16_t payloadType = Serializer::read_uint16(current, end);
        const uint16_t expectedType = 0x0000; // Or define elsewhere
        if (payloadType != expectedType) {
             throw std::runtime_error("Invalid Payload Type in stream. Expected "
                                     + std::to_string(expectedType) + ", found "
                                     + std::to_string(payloadType) + ".");
        }

        // 2. Read Operator ID (size + value BE) - Fields 3 & 4
        this->currentOperatorId = Serializer::read_uint32(current, end); // TODO Payload class, this needs to be checked

        // 3. Read Message (size + value BE) - Fields 5 & 6
        this->message = Serializer::read_int(current, end);

        // 4. Read DistanceTraveled (uint16_t BE) - Field 7
        uint16_t dist16 = Serializer::read_uint16(current, end);
        // Assign to int member. Check for overflow if needed, though unlikely from uint16_t.
        if (dist16 > std::numeric_limits<int>::max()) {
             throw std::overflow_error("Deserialized distanceTraveled value exceeds int max.");
        }
        this->distanceTraveled = static_cast<int>(dist16);

        // Note: 'active' is initialized to true.
    }

    
    /**
     * @brief Compares this Payload object with another for equality.
     * @param rhs The right-hand side Payload object to compare against.
     * @return bool True if all corresponding members are equal, false otherwise.
     * @details This method performs a member-by-member comparison of the two Payload objects.
     */
    bool operator==(const Payload& rhs) const {
        return this->message == rhs.message &&
               this->currentOperatorId == rhs.currentOperatorId &&
               this->distanceTraveled == rhs.distanceTraveled &&
               this->active == rhs.active;
    }

    /**
     * @brief Compares this Payload object with another for inequality.
     * @param rhs The right-hand side Payload object to compare against.
     * @return bool True if any corresponding members are not equal, false otherwise.
     * @details This method is the logical negation of the equality operator (operator==).
     */
    bool operator!=(const Payload& rhs) const {
        return !(*this == rhs);
    }



	// --- NEW Serialization Method ---
    /**
     * @brief Serializes the Payload state into a length-prefixed byte vector.
     * @param None
     * @return std::vector<std::byte> A vector containing the 1-byte size prefix + serialized data.
     * @throws std::overflow_error If serialized data > 255 bytes or distanceTraveled > UINT16_MAX.
     * @details Serializes Type, OperatorID, Message, DistanceTraveled (as uint16_t).
     * Calculates size N, writes N (1 byte), appends data. Uses Big Endian.
     */
    std::vector<std::byte> serializeToBytes() const {
        // Purpose: Serialize Payload state (Type, OpID, Msg, Dist) with 1-byte size prefix.
        // Parameters: None.
        // Return: Length-prefixed byte vector. Throws on error.
        // Key Logic: Serialize data to temp buffer, check size N<=255, write N, append data.

        std::vector<std::byte> dataBuffer;

        // Serialize fields 2-7 into temporary buffer (Big Endian)
        // Field 2: Payload Type
        const uint16_t payloadTypeValue = 0x0000;
        Serializer::write(dataBuffer, payloadTypeValue);
        // Fields 3 & 4: Operator ID (Size + Value BE)
        Serializer::write(dataBuffer, this->currentOperatorId);
        // Fields 5 & 6: Message (Size + Value BE)
        Serializer::write(dataBuffer, this->message);
        // Field 7: DistanceTraveled (uint16_t BE)
        // ** WARNING: Potential data loss if distanceTraveled > UINT16_MAX **
        if (this->distanceTraveled < 0 || this->distanceTraveled > std::numeric_limits<uint16_t>::max()) {
             throw std::overflow_error("Payload distanceTraveled ("
                                      + std::to_string(this->distanceTraveled)
                                      + ") is out of range for uint16_t serialization format.");
        }
        uint16_t dist16 = static_cast<uint16_t>(this->distanceTraveled);
        Serializer::write(dataBuffer, dist16);

        // Calculate size N and check bounds (max 255 for uint8_t prefix)
        size_t dataSize = dataBuffer.size();
        if (dataSize > std::numeric_limits<uint8_t>::max()) {
            throw std::overflow_error("Serialized payload data size (" + std::to_string(dataSize)
                                    + ") exceeds maximum representable by 1-byte length prefix (255).");
        }
        uint8_t totalDataSizeN = static_cast<uint8_t>(dataSize);

        // Create final buffer
        std::vector<std::byte> finalBuffer;
        finalBuffer.reserve(1 + totalDataSizeN); // 1 byte for size + N bytes for data

        // Write 1-byte size prefix N
        finalBuffer.push_back(static_cast<std::byte>(totalDataSizeN));

        // Append the data bytes
        finalBuffer.insert(finalBuffer.end(), dataBuffer.begin(), dataBuffer.end());

        return finalBuffer;
    }

	/**
	 * @brief Generates a JSON string representation of the Payload object.
	 * @param pretty If true, format with indentation and newlines. If false, compact output.
	 * @param indentLevel Internal use for recursive pretty printing (tracks current indent).
	 * @return std::string The JSON representation.
	 * @details Creates a JSON object like: {"message": M, "currentOperatorId": ID, ...}
	 */
	std::string toJsonString(bool pretty, int indentLevel) const {
		// Purpose: Create JSON string for this Payload instance.
		// Parameters: pretty flag, current indentLevel.
		// Return: JSON string.
		// Key Logic: Use ostringstream, handle pretty printing (indentation, newlines).

		std::ostringstream oss;
		std::string indent = pretty ? std::string(indentLevel * 2, ' ') : ""; // 2 spaces per level
		std::string maybeNewline = pretty ? "\n" : "";
		std::string maybeSpace = pretty ? " " : "";

		oss << indent << "{" << maybeNewline; // Start object

		std::string innerIndent = pretty ? std::string((indentLevel + 1) * 2, ' ') : "";

		// Message
		oss << innerIndent << "\"message\":" << maybeSpace << this->message << "," << maybeNewline;
		// Current Operator ID
		oss << innerIndent << "\"currentOperatorId\":" << maybeSpace << this->currentOperatorId << "," << maybeNewline;
		// Distance Traveled
		oss << innerIndent << "\"distanceTraveled\":" << maybeSpace << this->distanceTraveled << "," << maybeNewline;
		// Active
		oss << innerIndent << "\"active\":" << maybeSpace << (this->active ? "true" : "false") << maybeNewline; // No comma after last element

		oss << indent << "}"; // End object (no trailing comma needed before end brace)

		return oss.str();
	}

	/**
	 * @brief Prints the JSON representation of the Payload object to an output stream.
	 * @param out The output stream (defaults to std::cout).
	 * @param pretty If true, format with indentation and newlines. If false, compact output.
	 * @param indentLevel Internal use for recursive pretty printing (tracks current indent).
	 * @return void
	 */
	void printJson(std::ostream& out /*= std::cout*/, bool pretty, int indentLevel) const { // Passing stream for flexibility, and default std::cout would require including iostream, which is large file
		// Purpose: Print JSON representation to a stream.
		// Parameters: output stream, pretty flag, indentLevel.
		// Return: void.
		// Key Logic: Call toJsonString and output to the stream.
		out << toJsonString(pretty, indentLevel);
	}


	// --- Payload ID Removed ---
	// No unique payloadId needed per latest design.
};
