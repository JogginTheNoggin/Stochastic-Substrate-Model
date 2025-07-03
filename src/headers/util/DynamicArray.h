// DynamicArray.h
// -------------------------------------------------------------
// A lightweight, fixed‑capacity container that mimics a subset of
// std::vector’s interface while avoiding dynamic allocations.
//
// * Design goals
//   ‑ Simplicity: header‑only, no dependencies besides <cstddef> and
//     <stdexcept>.
//   ‑ Predictability: capacity is a compile‑time constant, so memory
//     footprint is known at compile time and the array lives on the
//     stack (or inside the owning object) rather than the heap.
//   ‑ Safety: bounds‑checked element access and overflow checks on add().
//
// * Typical usage
//       DynamicArray<int, 16> ints;  // max 16 ints
//       ints.add(3);
//       for (std::size_t i = 0; i < ints.size(); ++i) {
//           std::cout << ints[i] << "\n";
//       }
//
//   Note: The template parameter MAX_SIZE becomes both the static constexpr
//   field `kMAX_SIZE` *and* the actual length of the internal C‑array.
// -------------------------------------------------------------
#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include <cstddef>      // std::size_t
#include <stdexcept>    // std::overflow_error, std::out_of_range
#include <cstdint>      // std::int16_t 
#include "Constants.h"
/**
 * @tparam T         element type
 * @tparam MAX_SIZE  compile-time capacity (≤ 65 535 by default)
 *
 * A fixed-capacity, *sparse* array.  The class does NOT track which slots are
 * “occupied”; it only records the highest index ever written (maxElementIdx).
 *
 * Typical usage:
 *     DynamicArray<int> a;
 *     a.set(0, 10);      // ok
 *     a.set(5, 42);      // ok – gaps 1-4 are caller’s concern
 *     int x = a.get(5);  // 42
 *
 */
template <typename T>
class DynamicArray { // TODO check DynamicArray Memory leaks, not going to be dynamic for now (static network)
public:
    static constexpr std::int16_t MAX_SIZE = 2 << Constants::NETWORK_SIZE;

    using value_type = T;
    using size_type  = std::int16_t;        // matches MAX_SIZE’s range

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------
    constexpr DynamicArray() noexcept {}

    // ------------------------------------------------------------------
    // Introspection
    // ------------------------------------------------------------------
    /** Logical size is defined as capacity; gaps are allowed. */
    [[nodiscard]] constexpr size_type size() const noexcept    { return MAX_SIZE; }

    [[nodiscard]] constexpr size_type capacity() const noexcept{ return MAX_SIZE; }

    /** Highest index that has ever been written, or −1 when empty. */
    [[nodiscard]] constexpr size_type maxIdx() const noexcept  { return maxElementIdx; }


    /**
     * @brief Returns the number of non-null elements in the array.
     * @return size_type The total count of active elements.
     */
    [[nodiscard]] constexpr size_type count() const noexcept    { return elementCount; }


    // ------------------------------------------------------------------
    // Modifiers
    // ------------------------------------------------------------------
    /**
     * @brief Write `value` to `idx`, updating the element count. Overwrites any previous contents.
     * @param idx The index to write to.
     * @param value A pointer to the value to set.
     */
    void set(size_type idx,  T* value) {
        if (!inRange(idx))
            throw std::out_of_range{"DynamicArray index out of range"};
        
        // Update the count based on whether we are adding, removing, or replacing an element.
        const bool wasNull = (elements[idx] == nullptr);
        const bool isNull = (value == nullptr);

        if (wasNull && !isNull) {
            elementCount++; // A new element is being added to a previously empty slot.
        } else if (!wasNull && isNull) {
            elementCount--; // An existing element is being removed.
        }

        elements[idx] = value;
        if (idx > maxElementIdx) maxElementIdx = idx;
    }

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------
    /** Return a mutable reference to slot `idx` (bounds-checked only). */
    [[nodiscard]] T* get(size_type idx) {
        if (!inRange(idx))
            throw std::out_of_range{"DynamicArray index out of range"};
        return elements[idx];
    }

    /** Const overload of get(). */
    // confusing but makes returned point not modifiable
    [[nodiscard]] const T* get(size_type idx) const {
        if (!inRange(idx))
            throw std::out_of_range{"DynamicArray index out of range"};
        return elements[idx];
    }
    


    /**
     * @brief Sets the element at `idx` to null and decrements the count if it wasn't already null.
     * @param idx The index of the element to remove.
     */
    void remove(size_type idx) { // TODO this wouldn't work if was element with multiple elements
        if (!inRange(idx))
            throw std::out_of_range{"DynamicArray index out of range"};
        
        // Only decrement the count if there was an element to remove.
        if (elements[idx] != nullptr) {
            elementCount--;
        }
        elements[idx] = nullptr;
        //TODO maxIdx update
    }

    /**
     * @brief Checks if the array contains any non-null elements.
     * @return bool True if there are no active elements, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return elementCount == 0;
    }

private:
    T*          elements[MAX_SIZE]{};        // raw storage (default-initialised)
    size_type  maxElementIdx = static_cast<int16_t>(0);               // highest slot ever written
    size_type   elementCount = 0;                     // Count of non-null elements
    // TODO in dynamicArray the inconsistent use of int16_t and uint16_t maybe a problem 

    [[nodiscard]] const bool inRange(int idx) const noexcept{ // const to avoid editing and make compatible with other const functions
        return !(idx < 0 || idx >= MAX_SIZE);
    }
};

#endif // DYNAMICARRAY_H
