# MetaController: High-Level Specification

## 1. Purpose and Intent

The `MetaController` is the central component responsible for the high-level orchestration of the entire network. Its role has shifted from being a direct container for all `Operator`s to being a manager of a collection of `Layer` objects. It acts as the top-level authority on network structure, delegating all operator-specific logic to the `Layer` instances it owns.

## 2. Core Responsibilities

* **Layer Lifecycle Management**: The `MetaController` owns a collection of `Layer` objects, likely stored as a `std::vector<std::unique_ptr<Layer>>`, and manages their creation and destruction.
* **Network Configuration Persistence**: It implements `saveConfiguration` and `loadConfiguration` to handle the entire network state.
    * **Saving**: It iterates through its `Layer` objects and instructs each one to serialize itself. `MetaController` then writes these sequential data blocks to a file.
    * **Loading**: It reads a configuration file, parsing the header "envelope" of each layer block to determine its `LayerType`, `isRangeFinal` status, and size. It then instantiates the correct `Layer` subclass and passes it the corresponding data block to deserialize its own contents.
* **System-Wide Validation**: After loading a configuration, `MetaController` performs critical validation that no single `Layer` can do on its own. This includes:
    * Verifying that exactly one `Layer` is designated as dynamic (`isRangeFinal = false`).
    * Verifying that the dynamic layer's ID range comes after all static layers.
    * Verifying that no two `Layer` objects have overlapping ID ranges.
* **High-Level Operator Interaction API**: It exposes a task-based public interface (`messageOp`, `processOpData`, `traversePayload`) for other controllers like `TimeController`. It does not provide direct pointer access to operators to external systems.
* **Update Event Routing**: It serves as the primary endpoint for the `UpdateController`. When processing `UpdateEvents`, the `MetaController` finds the `Layer` containing the target `operatorId` and delegates the modification request to that `Layer` object.

## 3. Key Mechanisms & Rationale

* **Polymorphic Layer Collection**: Storing `Layer`s via `std::unique_ptr<Layer>` allows `MetaController` to manage different layer types through a common interface while ensuring proper memory management.
* **Layer-Based File Format**: The persistence format is a sequence of layer data blocks, making the file structure modular. `MetaController` parses the high-level structure, and each `Layer` is responsible for its own internal data format.
* **Delegation of Responsibility**: By delegating tasks to `Layer` objects, `MetaController`'s own logic is simplified and more robust. It is concerned with *what* layer should handle a request, not *how* the `Operator`'s state is specifically modified.

## 4. Interaction Summary

* **`UpdateController`**: Calls handler methods on `MetaController` (e.g., `handleAddConnection`). `MetaController` then finds the appropriate `Layer` and calls the corresponding method on that layer.
* **`TimeController`**: Is a client of `MetaController`'s high-level API. It drives the simulation by making calls like `metaController.processOpData(...)` and does not hold direct `Operator` pointers.
* **`Layer` Subclasses**: `MetaController` creates, owns, validates, and orchestrates `Layer` objects. It is the only component with a complete view of all layers and their relationships.

## 5. Conclusion

The `MetaController` has evolved into a true high-level orchestrator. Its design, centered around managing a collection of autonomous `Layer` objects and exposing a task-based API, makes the entire simulation architecture more modular, extensible, and robust. It cleanly separates system-wide concerns from layer-specific concerns, aligning with the project's goal of creating a flexible and powerful simulation framework.