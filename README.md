# Stochastic Substrate Model Simulator

## Overview

The Stochastic Substrate Model Simulator project is a C++ simulation framework designed as a testbed for exploring **emergent computational properties** within adaptive networks. The guiding philosophy is **"Possibility not Probability,"** meaning the primary objective is not to perfectly replicate known biological neural functions but to construct a system with the inherent **potential** for complex, perhaps unexpected, behaviors like self-organization, learning, and adaptation to arise naturally from the interaction of its defined components and rules. The framework aims to provide the necessary building blocks and flexibility for such possibilities to be investigated, prioritizing a more **abstract, functional model** over exact biological mimicry.

## Core Concepts

The simulation model is built around three main components:

*   **Layers (`Layer` class hierarchy):** Layers are containers that own and manage a collection of Operators within a specific, reserved ID range. They provide architectural structure, separating the network into functional areas (e.g., input, output, internal processing). Concrete subclasses like `InputLayer`, `OutputLayer`, and `InternalLayer` provide specialized functionality.
*   **Operators (`Operator` abstract base class):** The Operator is the sole active processing element in the simulation. It integrates incoming signals, applies logic (e.g., threshold, weight), and initiates outgoing signals (Payloads). Concrete subclasses like `AddOperator` (sums data), `InOperator` (distributes input), and `OutOperator` (collects output) implement specific behaviors. Operators manage timed, branching connections to other Operators using a "distance bucket" model, where the index of an array corresponds to the delay in time steps.
*   **Payloads (`Payload` struct):** A Payload represents a discrete packet of information (`message`) actively traversing the network. It carries the message data and state necessary to track its multi-step journey between Operators, including its `currentOperatorId` and `distanceTraveled`. Payloads are not copied for branching; instead, the message data is delivered to all targets at a connection point, and the original payload continues or becomes inactive.

## Simulation Flow

The simulation progresses in discrete time steps managed by the `TimeController`. Each time step involves distinct logical phases to ensure consistency:

1.  **Phase 1: Check Operators / Process Data (Start of Step N+1)**
    *   The `TimeController` triggers `Operator::processData()` for all Operators that received messages in the previous step (Step N).
    *   `processData()`:
        *   Checks accumulated data against a threshold.
        *   If the threshold is met, it creates new `Payload`(s).
        *   New Payloads are initialized with their first target distance and scheduled to start their journey in a *subsequent* time step (e.g., Step N+2) via the `Scheduler`.
        *   The Operator's accumulator is reset.
    *   This phase processes inputs from Step N, decides on "firing," and launches new signals for a future step.

2.  **Phase 2: Process Traveling Payloads (Main part of Step N+1)**
    *   The `TimeController` triggers `Operator::traverse(Payload&)` for each active Payload currently in transit.
    *   `traverse(Payload&)`:
        *   Checks if the Payload has reached its target distance.
        *   **If reached:**
            *   It schedules the Payload's `message` for immediate delivery to all target Operators at that distance via `Scheduler::scheduleMessage()`. This flags target Operators for processing in Step N+2.
            *   It finds the next connection distance for the Payload, updates `payload.distanceTraveled`, or marks the Payload inactive if no more connections exist.
        *   **If no connection at target distance:** It attempts to find the next valid connection distance to "skip" the empty bucket or deactivates the payload.
    *   This phase manages payload progression, handles arrivals by triggering data delivery, and transitions Payloads to their next state.

Signal data is delivered (`Operator::message()`) within the same step it conceptually arrives, while the processing of that data (`Operator::processData()`) happens in the following step.

## State and Update Management

The system separates rapid signal processing from slower state and structural changes.

*   **State Management:**
    *   `MetaController`: Manages the collection of `Layer` objects and enforces system-wide rules (e.g., non-overlapping ID ranges for Layers).
    *   `Layer`: Owns and manages the `Operator` instances within its designated ID range.
    *   `Operator`: Holds its dynamic operational state (e.g., accumulated data).

*   **(NOT YET IMPLEMENTED) Update Mechanism:**
    *   **`UpdateEvent`**: A struct defining a requested change (e.g., add connection, change parameter).
    *   **`UpdateScheduler`**: A static helper class providing the sole public interface (`UpdateScheduler::get()->Submit(event)`) for an `Operator` to submit `UpdateEvent`s.
    *   **`UpdateController`**: Manages a central queue of `UpdateEvent`s. Its `ProcessUpdates()` method is called by the main simulation loop *between* time steps. It iterates through queued events, dispatching them to the `MetaController` for execution. This ensures changes are applied atomically.
    *   **Setup-Time vs. Run-Time Modifications**:
        *   Changes during initial network setup (e.g., in `Layer::randomInit`) are performed directly.
        *   Changes after the simulation starts *must* be submitted as `UpdateEvent`s.

## Serialization

The Stochastic Substrate Model Simulator has a defined framework for persisting and reloading the network state. This includes:

*   **Network Configuration:** The static structure and parameters of the network, managed by `MetaController` and `Layer` objects. This involves serializing Layer types, ID ranges, Operator data, and connection details.
*   **Dynamic Simulation State:** Active `Payload`s and processing flags, managed by `TimeController`.
*   **Pending Updates:** The queue of `UpdateEvent`s, managed by `UpdateController`.

All multi-byte numerical fields use Big-Endian byte order. For more details, refer to `docs/Serialization_Framework.md`.

## Directory Structure

The project's code is organized as follows:

*   `src/`: Contains the core source code.
    *   `general/`: Implementation files (.cpp) for core classes.
    *   `headers/`: Header files (.h) defining classes and utilities.
        *   `cli/`, `controllers/`, `layers/`, `operators/`, `util/`: Subdirectories for specific component types.
*   `tests/`: Contains unit tests.
*   `docs/`: Contains detailed design documentation and specifications for various components and concepts. A good starting point for understanding the code structure is `docs/_current_code_dir_structure_.md`.

## Building and Running

The following are general instructions. You may need to adjust them based on your specific environment and the project's `CMakeLists.txt`.

### Prerequisites

*   **CMake**: Version 3.10 or higher (or as specified in `CMakeLists.txt`).
*   **C++ Compiler**: A compiler supporting C++17 (e.g., GCC 7+, Clang 5+).


### Building

The project uses CMake for building.

1.  **Create a build directory**:
    It's good practice to build out-of-source.
    ```bash
    mkdir build
    cd build
    ```

2.  **Configure the project**:
    Run CMake from the build directory to generate build files.
    ```bash
    cmake ..

3.  **Build the project**:
    Use the generated build files (e.g., Makefile on Linux/macOS).
    ```bash
    make
    ```
    Or, for potentially faster parallel builds (using the number of available processors):
    ```bash
    make -j$(nproc)
    ```

### Running the Simulator

After building, the executables will be located in the `build/` or `build/AthenaTests/` directories (or similar, depending on `CMakeLists.txt` configuration).


*   **Unit Tests**:
    Unit tests are separate executables. Check `CMakeLists.txt` for test targets.
    ```bash
    # Example (replace 'unit_tests' with the actual test executable name)
    ```
    You can also often run tests via CTest if configured:
    ```bash
    cd build
    ctest
    ```

Consult the `CMakeLists.txt` file for the definitive names of executable targets and any specific build options.

The previous content of this README regarding VSCode tasks has been preserved in `docs/VSCODE_HELPER_GUIDE.md`.
