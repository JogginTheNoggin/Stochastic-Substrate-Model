# UpdateScheduler: High-Level Specification

## 1. Purpose and Intent

[cite_start]The `UpdateScheduler` is a helper class designed as the **sole public interface** for submitting `UpdateEvent`s into the simulation's update system. [cite_start]Its purpose is to decouple the components that *request* updates (like `Operator`s performing self-modification) from the component that *manages and processes* those updates (the `UpdateController`). [cite_start]It adheres to the principle of least privilege.

## 2. Core Responsibilities

* [cite_start]**Interface Provision**: Exposes static methods (`get`, `Submit`) allowing controlled access to the update submission function.
* [cite_start]**Decoupling**: Hides the implementation details of the `UpdateController` and its internal queue from the update requestors.
* [cite_start]**Instance Management**: Manages access to an underlying `UpdateScheduler` instance which holds the connection to the `UpdateController` instance.
* [cite_start]**Event Forwarding**: Internally calls the appropriate queuing method on the correct `UpdateController` instance to enqueue the event.

## 3. Key Mechanisms & Rationale

* [cite_start]**Static Access Pattern**: Similar to the `Scheduler`, this class uses a specific static management pattern to avoid passing pointers and avoid true global state.
    * [cite_start]**Private Constructor**: The constructor is private.
    * [cite_start]**Static Factory (`CreateInstance`)**: A static method, `UpdateScheduler* CreateInstance(UpdateController* controller)`, is the sole entry point for creating `UpdateScheduler` objects. [cite_start]It links the new instance to a specific `UpdateController`.
    * [cite_start]**Static Accessor (`get`)**: A static method, `UpdateScheduler* get()`, provides access to the default `UpdateScheduler` instance. [cite_start]`Operator`s use this to retrieve a pointer (e.g., `UpdateScheduler::get()->Submit(...)`).
* **Key Interface Methods**:
    * `void Submit(const UpdateEvent& event)`: An instance method called via `get()`. [cite_start]This is the public method used by any component needing to request a state or structural change. [cite_start]It takes a fully formed `UpdateEvent` struct and calls the queuing method on its associated `UpdateController` instance (e.g., `updateControllerInstance->AddToQueue(event)`). [cite_start]This enforces that requestors only need to know *what* change they want, not *how* it happens.

## 4. Conclusion

[cite_start]The `UpdateScheduler` serves as a simple, crucial gateway for all network modification requests. [cite_start]By providing a restricted, static interface accessed via `UpdateScheduler::get()`, it promotes modularity, hides implementation details of the `UpdateController`, and ensures that update requests enter the system through a single, well-defined point.