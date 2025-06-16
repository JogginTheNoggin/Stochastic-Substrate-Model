# Scheduler: High-Level Specification

## 1. Purpose and Intent

[cite_start]The `Scheduler` is a helper class providing a restricted, decoupled interface for other components (primarily `Operator`s) to schedule simulation events managed by the `TimeController`. [cite_start]Its main purpose is to enforce the principle of least privilege and maintain a clean separation of concerns. [cite_start]It allows access to necessary scheduling functionality without requiring `Scheduler` instances to be passed to or stored within every `Operator`.

## 2. Core Responsibilities

* [cite_start]**Interface Provision**: Exposes static methods that allow controlled access to event scheduling functions.
* [cite_start]**Decoupling**: Hides the implementation details of the `TimeController` from the components that need to schedule events.
* [cite_start]**Instance Management**: Manages access to an underlying `Scheduler` instance which holds the connection to the `TimeController` instance.
* [cite_start]**Event Forwarding**: Internally calls the appropriate methods on the `TimeController` instance to queue or trigger the requested events.

## 3. Key Mechanisms & Rationale

* [cite_start]**Static Access Pattern**: To avoid the overhead of passing `Scheduler` pointers, the class uses a specific static management pattern.
    * [cite_start]**Private Constructor**: The constructor is private to prevent direct instantiation.
    * [cite_start]**Static Factory (`CreateInstance`)**: A static method, `Scheduler* CreateInstance(TimeController* controller)`, is the sole entry point for creating `Scheduler` objects. [cite_start]It links the new `Scheduler` instance to a specific `TimeController`.
    * [cite_start]**Static Accessor (`get`)**: A static method, `Scheduler* get()`, provides access to the default `Scheduler` instance. [cite_start]`Operator`s use this to retrieve a pointer (e.g., `Scheduler::get()->scheduleMessage(...)`).
    * [cite_start]**Rationale**: This pattern provides global-like access (`Scheduler::get()`) without true global state issues and avoids redundant storage of pointers in every `Operator`.
* **Key Interface Methods**:
    * `void schedulePayloadForNextStep(const Payload& payload)`: An instance method called via `get()`. [cite_start]It allows an `Operator` (typically in `processData`) to launch a newly created `Payload` into the simulation, starting its journey in the *next* time step. [cite_start]It works by calling `timeControllerInstance->addToNextStepPayloads(payload)`.
    * `void scheduleMessage(int targetOperatorId, int messageData)`: An instance method called via `get()`. [cite_start]It allows an `Operator` (typically in `traverse`) to signal that a payload's message data should be delivered to a target `Operator` immediately. [cite_start]It works by calling `timeControllerInstance->deliverAndFlagOperator(...)`. [cite_start]This decouples `Operator::traverse` from having to perform `MetaController` lookups or direct `message()` calls.

## 4. Conclusion

[cite_start]The `Scheduler` acts as a crucial intermediary, providing controlled access to time-step-related event scheduling. [cite_start]Its static access pattern (`get()`) allows `Operator`s to easily schedule payload launches and message deliveries without direct coupling to the `TimeController` or requiring scheduler instances to be passed around. [cite_start]This promotes modularity and hides the implementation details of the `TimeController`'s complex multi-phase step processing.