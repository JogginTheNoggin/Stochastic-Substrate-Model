# UpdateController: High-Level Specification

## 1. Purpose and Intent

[cite_start]The `UpdateController` is the component responsible for the orderly execution of all runtime modifications to the network's state and structure. [cite_start]Its primary purpose is to manage a queue of `UpdateEvent` structs, ensuring that requested changes are applied atomically between the main simulation time steps. [cite_start]It decouples the components that *request* updates (typically `Operator`s) from the high-level system that *executes* them (`MetaController` and `Layer` objects).

## 2. Core Responsibilities

* [cite_start]**Event Queuing**: It exposes an `AddToQueue` method for the `UpdateScheduler` to add `UpdateEvent`s to its internal queue.
* [cite_start]**Atomic Execution**: Its `ProcessUpdates` method processes the entire queue of events at a specific point in the simulation's main loop (between time steps), ensuring all pending modifications are applied as a single, atomic transaction.
* **Event Dispatching**: For each event in the queue, the `UpdateController` dispatches the request to the `MetaController`. [cite_start]It does not resolve operatorIds to pointers itself. [cite_start]Instead, it calls the appropriate high-level handler method on `MetaController` (e.g., `metaController->handleAddConnection(...)`).

## 3. Key Mechanisms & Rationale

* [cite_start]**`updateQueue` (`std::queue<UpdateEvent>`)**: A standard FIFO queue that stores pending `UpdateEvent`s, guaranteeing they are processed in the order they were submitted.
* **`ProcessUpdates()` Method**: This is the central method of the `UpdateController`. It drains the `updateQueue` and, for each event, uses a `switch` statement on the event's type to call the corresponding `handle...` method on its `metaControllerInstance`, passing the event's parameters.
* [cite_start]**Rationale**: This revised logic makes the `UpdateController` simpler and more robust. [cite_start]Its sole responsibility is to manage the queue and dispatch tasks to the `MetaController`, which acts as the single entry point for all network modifications.

## 4. Interaction Summary

* [cite_start]**`UpdateScheduler`**: The `UpdateScheduler` remains the sole public interface for submitting events. [cite_start]It calls `UpdateController::AddToQueue` to enqueue an `UpdateEvent`.
* [cite_start]**`MetaController`**: The `UpdateController` acts as a client to the `MetaController`'s service interface. It calls public handler methods (e.g., `handleDeleteOperator`) on `MetaController`, telling it *what* change is requested. [cite_start]`MetaController` is then responsible for figuring out *where* (which `Layer`) and *how* to apply it.
* [cite_start]**`Operator`**: The interaction is indirect. The `UpdateController` never calls methods on an `Operator` object directly. [cite_start]Its actions trigger a call chain (`UpdateController` -> `MetaController` -> `Layer` -> `Operator`) that ultimately results in an internal `Operator` method being invoked.

## 5. Conclusion

[cite_start]The `UpdateController`'s role is refined as the dedicated manager and dispatcher of a transactional update queue. [cite_start]By delegating the execution logic to `MetaController`, its own implementation is simplified, and it is fully decoupled from the network's internal `Layer` and `Operator` structure. [cite_start]This reinforces its role as a key component for ensuring that all runtime network modifications occur in an orderly and consistent manner between simulation steps.