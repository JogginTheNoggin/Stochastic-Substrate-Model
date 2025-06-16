# State and Update Management

## 1. Overview

[cite_start]A core principle of the simulator is the strict separation of rapid signal processing (handled by the Time Loop) from slower, more complex state and structural changes (handled by the Update Loop). [cite_start]This division ensures that signal flow within any given time step is consistent and deterministic. [cite_start]The `MetaController`, `UpdateController`, and `UpdateScheduler` work together to manage these updates.

## 2. State Management: A Partitioned Responsibility

[cite_start]The management of the network's state is partitioned across the `MetaController` and the `Layer` objects it orchestrates.

### MetaController's Role

[cite_start]The `MetaController` is no longer the direct container for operator parameters. [cite_start]Its responsibility is now at a higher level of abstraction. [cite_start]It manages the collection of `Layer` objects and enforces system-wide rules, such as ensuring that the ID ranges assigned to each layer do not overlap.

### Layer's Role

[cite_start]Each `Layer` object is now responsible for the direct state management of the `Operator`s it contains. [cite_start]It owns the `Operator` instances, manages their lifecycle, and controls its own reserved ID range.

### Operator's Role

[cite_start]The dynamic operational state, such as an `AddOperator`'s `accumulateData`, resides solely within the live `Operator` objects, which are in turn owned and managed by a `Layer`.

## 3. The Update Mechanism

The system uses a formal event queue to manage all runtime modifications.

### `UpdateEvent` Struct

[cite_start]This is a data structure that defines a requested change.
* [cite_start]**`type`**: The kind of update (e.g., `CHANGE_OPERATOR_PARAMETER`, `ADD_CONNECTION`, `DELETE_OPERATOR`).
* [cite_start]**`targetOperatorId`**: The unique ID of the `Operator` affected.
* [cite_start]**`params`**: A `std::vector<int>` that encodes the necessary parameters for the update, requiring careful parsing based on the event type.

### `UpdateScheduler` (Static Helper Class)

* [cite_start]**Purpose**: Provides the sole public interface for an `Operator` to submit `UpdateEvent`s. [cite_start]It decouples the event generator from the processing queue, adhering to the principle of least privilege.
* [cite_start]**Method**: `static void Submit(const UpdateEvent& event)`.
* [cite_start]**Usage**: An `Operator` instance calls this method when it needs to request a change to its own state or connections.

### `UpdateController` (Static Class)

* [cite_start]**Purpose**: Manages the central queue of `UpdateEvent`s and orchestrates their processing during the Update Loop phase, which occurs between time steps.
* **Method**: `static void ProcessUpdates()`
    * [cite_start]This method is called by the main simulation loop after the `TimeController` finishes its step.
    * [cite_start]It iterates through all events in its queue.
    * [cite_start]For each event, it initiates a call chain: `UpdateController` -> `MetaController` -> `Layer` -> `Operator`. [cite_start]The `MetaController` routes the request to the correct `Layer`, which then finds the specific `Operator` and calls an internal update method on it (e.g., `operatorPtr->addConnectionInternal(...)`).
    * [cite_start]Finally, it clears the queue.
* [cite_start]**Rationale**: This centralizes the application of all state and structural changes, ensuring they happen atomically between time steps.

## 4. Update Origin and Timing

[cite_start]A critical distinction exists between how network modifications are handled at setup versus at runtime.

* [cite_start]**Setup-Time Modifications**: Changes made during the initial, programmatic creation of the network (e.g., within a `Layer`'s `randomInit` method) are performed **directly and immediately**. [cite_start]For instance, `randomInit` can call an `Operator`'s `addConnectionInternal` method directly. [cite_start]This is done for efficiency before the simulation loop begins, as there is no risk of creating inconsistent states.
* [cite_start]**Run-Time Modifications**: Any change requested **after** the simulation has started, typically triggered by an `Operator`'s own processing logic, **must** be formalized as an `UpdateEvent` and submitted via the `UpdateScheduler`. [cite_start]This ensures the change is deferred and applied safely by the `UpdateController` between time steps, maintaining simulation consistency.

## 5. Conclusion

[cite_start]The state and update management system has evolved to support the new `Layer`-based architecture. [cite_start]State management is clearly partitioned, with `MetaController` handling the high-level layer structure and rules, while each `Layer` manages its own operators. [cite_start]The update processing flow follows a clean chain of delegation from `UpdateController` -> `MetaController` -> `Layer` -> `Operator`. [cite_start]This, combined with the clear distinction between immediate setup-time changes and deferred run-time updates, creates a robust, modular, and consistent system for managing an evolving network.