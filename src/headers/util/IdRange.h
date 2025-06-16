#pragma once
#include <stdexcept>               // For std::runtime_error

struct IdRange {
private: 
    uint32_t minId; // inclusive
    uint32_t maxId; // inclusive

    void validate() {
        if(minId < 0 || maxId < 0 || minId > maxId){
            throw std::runtime_error("Range is not valid.");
        }
    }

public:
    // Default constructor for an invalid/uninitialized range
    IdRange(uint32_t min = 0, uint32_t max = 0) : minId(min), maxId(max) {
        validate();
    }

    uint32_t getMinId() const{
        return minId;
    }

    uint32_t getMaxId() const{
        return maxId;
    }

    void setMinId(uint32_t newMinId){
        minId = newMinId;
        validate();
    }

    void setMaxId(uint32_t newMaxId) {
        maxId = newMaxId;
        validate();
    }

    // Number of IDs in the range
    uint32_t count() const {
        return maxId - minId + 1;
    }

    bool contains(uint32_t id) const {
        return id >= minId && id <= maxId;
    }

    /**
     * @brief Checks if this range overlaps with another range.
     * @details Two ranges overlap if the start of one is less than or equal to the end of the other.
     * This check is inclusive of the boundaries.
     * @param other The other IdRange to compare against.
     * @return True if there is any overlap, false otherwise.
     */
    bool isOverlapping(const IdRange& other) const {
        // Overlap exists if the maximum of the minimums is less than or equal to the minimum of the maximums.
        return std::max(this->minId, other.minId) <= std::min(this->maxId, other.maxId);
    }

    // --- Pre-C++20 Comparison Operators ---

    bool operator==(const IdRange& other) const {
        return minId == other.minId && maxId == other.maxId;
    }

    bool operator!=(const IdRange& other) const {
        return !(*this == other);
    }

    bool operator<(const IdRange& other) const {
        if (minId < other.minId) return true;
        if (minId == other.minId) return maxId < other.maxId;
        return false;
    }

    bool operator>(const IdRange& other) const {
        return other < *this; // Implemented in terms of <
    }

    bool operator<=(const IdRange& other) const {
        return !(*this > other); // Implemented in terms of >
    }

    bool operator>=(const IdRange& other) const {
        return !(*this < other); // Implemented in terms of <
    }
};