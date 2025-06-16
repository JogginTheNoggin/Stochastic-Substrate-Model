# Connection and Delay Mechanism (Distance Buckets)

## 1. Concept

[cite_start]In the abstract Operator/Payload model, direct connections between `Operator`s and their associated signal delay are managed using a specialized dynamic array mechanism stored within the source `Operator`. [cite_start]This system replaces the explicit pathway nodes or map-based structures used in previous designs.

## 2. Structure

* [cite_start]**`Operator::outputConnections`**: Each `Operator` object contains a member variable, `outputConnections`, of type `DynamicArray<std::unordered_set<int>*>`.
* [cite_start]**Index as Distance**: The **index** used to access this array directly corresponds to the **distance**—an integer representing the number of time steps for a signal to travel.
* [cite_start]**Value as Target Set**: The **value** stored at a given index `D` is a **pointer to a heap-allocated `std::unordered_set<int>`**. [cite_start]This set contains the unique IDs of all target `Operator`s reached after exactly `D` time steps. [cite_start]A `nullptr` at an index indicates no connections exist at that distance.
* **Memory Management**: The `Operator` class is solely responsible for managing the lifetime of the heap-allocated `std::unordered_set<int>` objects. It must `new` sets when adding the first connection at a given distance and `delete` these sets in its destructor or when a distance becomes empty.
* [cite_start]**Note**: The intermediate `DistanceBucket` struct is no longer used.

## 3. Functionality

* [cite_start]**Encoding Delay**: The delay is implicitly encoded by the array **index**. [cite_start]A payload targeting distance `D` implies a delay of `D` time steps from the source. [cite_start]The `Operator::traverse` method manages the payload's progression towards this target distance.
* **Encoding Connections**: The set of `Operator` IDs at a specific distance index `D` defines all the targets for that delay. Using a `std::unordered_set` automatically ensures target uniqueness.
* [cite_start]**Encoding Branching**: If a set at index `D` contains multiple `Operator` IDs, it represents a branching point. [cite_start]When a payload reaches distance `D`, its message data is scheduled for delivery to **all** listed targets simultaneously.
* **Ordered Processing**: The progression logic is handled within `Operator::traverse`. When a payload reaches its target distance, `traverse` calls a helper method (e.g., `findNextConnectionDistance`) to iterate through subsequent indices of the `DynamicArray` to find the **next populated distance** for the payload to target.

## 4. Rationale

* [cite_start]**Simplification**: This model eliminates the intermediate `DistanceBucket` struct, simplifying the data structures involved.
* [cite_start]**Memory Efficiency**: By heap-allocating sets of target IDs, the model can be memory-efficient for networks with sparse connections at varying distances.
* **Functional Focus**: The structure directly represents the functional requirement: "send data X to target(s) Y after Z time steps," using the array index `Z` to store a pointer to the set of targets `Y`.
* [cite_start]**ID-Based Robustness**: Storing `OperatorID`s instead of raw `Operator*` pointers makes the connection data robust against target `Operator` deletion, although this creates "dangling IDs" that require lazy cleanup during traversal.

## 5. Management

[cite_start]All modifications to connections—such as adding or removing target IDs, or changing target distances—are handled via `UpdateEvents` processed by the `UpdateController`. [cite_start]The `UpdateController` calls the appropriate internal methods on the source `Operator` object (e.g., `addConnectionInternal`, `removeConnectionInternal`), which contain the logic for manipulating the `DynamicArray` and managing the associated heap-allocated sets.