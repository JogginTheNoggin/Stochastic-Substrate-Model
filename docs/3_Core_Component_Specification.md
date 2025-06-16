# Core Component Specification: Operator and Payload

## 1. Introduction

This document details the core components of the Abstract Operator/Payload Model: the `Operator` (the processing unit), the `Payload` (the information carrier), and the `Scheduler` (the interface for initiating their interaction). This model represents a shift towards functional abstraction, prioritizing simplified component types and scalable state management over direct biological analogy to support the project's goal of enabling emergent network behaviors.

## 2. `Payload` Struct (The Information Carrier)

* [cite_start]**Purpose**: The `Payload` is a discrete packet of information that actively moves through the network. [cite_start]It carries both the message data and the state necessary to track its multi-step journey between Operators.

* **State**:
    * [cite_start]`message` (int): The actual data value being transmitted.
    * [cite_start]`currentOperatorId` (uint32_t): The ID of the Operator currently managing this payload's journey.
    * [cite_start]`distanceTraveled` (int): The distance the payload has traveled, which also acts as the index into the managing Operator's `outputConnections` array.
    * [cite_start]`active` (bool): Tracks if the payload is still in transit.

* **Lifecycle & Rationale**:
    * [cite_start]**Creation**: New `Payload` instances are created *only* by an `Operator` within its `processData()` method when a processing threshold is met. [cite_start]This decouples payload creation from the arrival of individual messages.
    * [cite_start]**Traversal**: The payload's state is managed by the logic within the `Operator::traverse` method, which is called by the `TimeController`.
    * [cite_start]**Branching (No Copying)**: `Payload` objects are **not copied** when a connection specifies multiple targets. [cite_start]When a target distance is reached, only the `message` data is scheduled for delivery to all target Operators.
    * [cite_start]**Continuation**: The original `Payload` object then updates its `distanceTraveled` state to target the next available connection distance within the same source Operator and continues its journey.
    * [cite_start]**Termination**: A payload becomes inactive (`active = false`) only after it has triggered message scheduling for the final connection distance of its managing Operator. [cite_start]This lifecycle avoids the significant memory overhead of object copying during branching.

## 3. `Operator` Abstract Base Class (The Processing Unit)

* [cite_start]**Purpose**: The `Operator` is the sole active processing unit in the simulation. [cite_start]It is an abstract base class that defines a common interface for all processing nodes. [cite_start]Its function is to accumulate incoming data (`message`), process it (`processData`), initiate new payloads if a threshold is met, and manage the timed journey (`traverse`) of payloads originating from it.

* **State**:
    * [cite_start]`operatorId` (uint32_t): A unique identifier for the `Operator` instance.
    * [cite_start]`outputConnections` (`DynamicArray<std::unordered_set<uint32_t>*>`): A specialized data structure for outgoing connections.
        * [cite_start]The **index** of the array corresponds directly to the **distance** (delay in time steps).
        * [cite_start]The value at a given index is a **pointer to a heap-allocated `std::unordered_set<uint32_t>`** containing the IDs of all target operators at that distance.
        * **Memory Management**: The `Operator` instance is responsible for the lifetime of these heap-allocated sets, performing `new` when a connection is first added to a distance and `delete` in its destructor or when a connection is removed.

* **Interface**: It defines a set of `virtual` and `pure virtual` methods that all concrete subclasses must implement or can override.
    * `virtual void randomInit(...) = 0;` 
    * [cite_start]`virtual void message(...) = 0;` 
    * [cite_start]`virtual void processData() = 0;` 
    * `virtual Type getOpType() const = 0;` 
    * [cite_start]`virtual std::vector<std::byte> serializeToBytes() const;` 
    * [cite_start]The base class also provides concrete `virtual` implementations for `traverse(Payload&)`, `requestUpdate(...)`, and internal connection management methods.

## 4. Concrete Operator Subclasses

* **`AddOperator`**: A standard processing unit that sums incoming integer data.
    * [cite_start]**State**: Adds `weight`, `threshold`, and `accumulateData` integer members.
    * [cite_start]**Behavior**: `message()` adds data to `accumulateData`. [cite_start]`processData()` checks if `accumulateData > threshold`; if true, it calculates an output (`accumulatedData + weight`) and creates a new `Payload`. [cite_start]`accumulateData` is always reset after processing.

* **`InOperator`**: Acts as an "input channel" or data distributor for the network.
    * [cite_start]**State**: Stores incoming data in a `std::vector<int> accumulatedData`.
    * [cite_start]**Behavior**: `message()` appends data to the vector. [cite_start]`processData()` iterates through the vector and creates a new, separate `Payload` for **each** integer value, then clears the vector.

* [cite_start]**`OutOperator`**: Acts as a "terminal node" or data sink for external observation.
    * **State**: Stores incoming data in a `std::vector<int> data`.
    * [cite_start]**Behavior**: `message()` appends data to its `data` vector. [cite_start]Its `processData()` method is intentionally empty, as it only collects data and does not fire new signals. [cite_start]It has no outgoing connections.

## 5. Core Processing Logic & Rationale (Multi-Phase Flow)

* **`message(const int payloadData)` (Called during Step N)**
    * [cite_start]**Role**: Simply accumulates incoming data.
    * **Logic**: Called immediately by the `TimeController` when a predecessor's `traverse` method schedules a message. It adds the data to the operator's internal accumulator (e.g., `this->accumulateData += payloadData;`). The `TimeController` also flags this operator to be processed in the next time step (Step N+1).
    * [cite_start]**Rationale**: Decouples signal arrival from signal processing, ensuring all inputs for a given step are summed before evaluation.

* **`processData()` (Called at Start of Step N+1)**
    * [cite_start]**Role**: Processes the total accumulated data from the previous step, checks against a threshold, and creates/schedules *new* initial payloads.
    * **Logic**: If the threshold is met, it calculates an output value, finds the *first* available connection distance (`D_first`), and creates a new `Payload` with its initial target distance set (`distanceTraveled = D_first`). This new payload is scheduled via `Scheduler::get()->schedulePayloadForNextStep()` to begin its journey in a subsequent step (e.g., N+2). The accumulator is then reset to zero.
    * [cite_start]**Rationale**: Ensures the "firing" decision is based on the complete input from the previous step and separates new payload creation from the traversal of existing payloads.

* **`traverse(Payload* payload)` (Called during Main part of Step N+1)**
    * **Role**: Manages the journey of a payload already in transit that originated from this `Operator`.
    * [cite_start]**Logic**: It retrieves the payload's current target distance (`D_target = payload->distanceTraveled`) and checks for connections at that distance index in its `outputConnections` array.
        * **If Reached**: It calls `Scheduler::get()->scheduleMessage()` for all `targetId`s in the connection set, delivering the data immediately. It then finds the *next* available connection distance (`nextDistance`) and updates the payload's state (`payload->distanceTraveled = nextDistance`) or marks it inactive if no further connections exist.
        * [cite_start]**If No Connection**: It still attempts to find the next valid connection distance to "skip" the empty bucket and updates the payload's state accordingly.
    * [cite_start]**Rationale**: This encapsulates the stateful progression logic, handles branching by triggering message delivery to multiple targets, and manages payload inactivation. [cite_start]It also includes a placeholder for flagging dangling ID connections for cleanup.

## 6. Update Handling

[cite_start]Operators can request changes to their own parameters or connections by creating `UpdateEvent` structs and submitting them via the static `UpdateScheduler::get()->Submit()` interface. [cite_start]These events are processed atomically between time steps by the `UpdateController`.

## 7. `Scheduler` Static Helper Class

* [cite_start]**Purpose**: Provides a restricted, decoupled interface for `Operator`s to interact with the `TimeController`'s scheduling mechanisms.
* **Access**: Uses a static `get()` accessor, so `Operator`s don't need to store a pointer to it.
* **Interface**:
    * `schedulePayloadForNextStep(const Payload& payload)`: Called by `processData` to launch a new payload.
    * [cite_start]`scheduleMessage(uint32_t targetOperatorId, int messageData)`: Called by `traverse` to deliver data when a payload reaches a target.
* **Rationale**: Enforces least privilege and decouples `Operator` logic from `TimeController` implementation details.

## 8. Design Rationale Summary & Trade-offs

* [cite_start]**Rationale**: This model simplifies the component types to `Operator` and `Payload`, focusing on function over form. [cite_start]It improves memory efficiency by avoiding payload copying and using ID-based connections to simplify deletion management.
* **Strengths**: Fewer C++ classes, potential memory savings, clear separation of concerns, and no payload object copying for branching.
* [cite_start]**Quirks/Characteristics**: The model is highly abstract, with complex logic concentrated in the `Operator` methods and the `TimeController`.
* [cite_start]**Trade-offs**: Sacrifices biological structural detail for potential implementation simplicity. [cite_start]Complexity is shifted from managing a graph of many node types to managing payload state transitions and the logic within the `Operator` class.

## 9. Conclusion

The refined Abstract Operator/Payload model offers a streamlined approach focused on processing nodes and stateful information packets. [cite_start]Its success hinges on the efficient implementation of the distinct `Operator` logic (`message`, `processData`, `traverse`) and the `TimeController`'s management of the multi-phase time step. [cite_start]While abstract, it provides a potentially scalable and manageable foundation for exploring emergent network behaviors.