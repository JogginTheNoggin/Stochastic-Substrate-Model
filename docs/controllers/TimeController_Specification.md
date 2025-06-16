# TimeController: High-Level Specification

## 1. Purpose and Intent

[cite_start]The `TimeController` is the engine managing the progression of the simulation through discrete time steps. [cite_start]Its primary purpose is to orchestrate the processing of active `Payload` objects and `Operator` states by making high-level requests to the `MetaController`, ensuring consistent and deterministic execution within each step.

## 2. Core Responsibilities

* [cite_start]**Time Step Management**: Maintains the state for the current time step (`currentStepPayloads`), accumulates payloads for the next time step (`nextStepPayloads`), and tracks `Operator`s needing processing (`operatorsToProcess`).
* [cite_start]**Simulation Phase Orchestration**: Executes the distinct processing phases within a single time step in the correct order:
    1.  [cite_start]**Phase 1: Check Operators / Process Data**: Triggers processing for `Operator`s that were flagged in the previous step.
    2.  [cite_start]**Phase 2: Process Traveling Payloads**: Manages the step-by-step traversal of payloads currently in transit.
* [cite_start]**State Advancement**: Moves the simulation forward from one discrete step to the next via its `advanceStep` method.
* **Initiating Actions via MetaController**: The `TimeController` no longer resolves `operatorId`s to pointers itself. [cite_start]Instead, it interacts with operators by calling high-level, task-based methods on its `MetaController` instance (e.g., `messageOp`, `processOpData`, `traversePayload`).
* [cite_start]**Immediate Message Delivery**: Provides the `deliverAndFlagOperator` method, called by the `Scheduler`, to immediately deliver message data to target `Operator`s and flag them for processing in the next step.

## 3. Key Mechanisms & Rationale

* [cite_start]**Dependency Injection**: The `TimeController` receives a reference to the simulation's `MetaController` instance via its constructor, enabling it to make the necessary high-level requests.
* [cite_start]**Multi-Phase Processing**: The separation of phases prevents causality violations.
    * In **Phase 1**, `Operator::processData()` is called for operators flagged in the previous step (Step N). [cite_start]This processes data accumulated in Step N and may schedule new payloads for a future step (e.g., Step N+2).
    * In **Phase 2**, the `traverse` method on existing payloads is called. If a payload reaches its destination, it triggers `Scheduler::scheduleMessage`, which in turn calls `TimeController::deliverAndFlagOperator`. This immediately delivers the message data in the current step (Step N+1) and flags the recipient to be processed in the next step (Step N+2).
    * [cite_start]This ensures that data is delivered in the step it arrives, while the processing of that data happens in the following step, after all inputs for a given step have been received.
* [cite_start]**Payload Management**: It tracks `currentStepPayloads` and `nextStepPayloads`. [cite_start]It collects newly created payloads into `nextStepPayloads` via its `addToNextStepPayloads` method, which is called by the `Scheduler`.

## 4. Interaction Summary

[cite_start]The `TimeController` is instantiated with a `MetaController` reference. [cite_start]Its `processCurrentStep` method is called by the main simulation loop.
* During **Phase 1**, it iterates `operatorsToProcess` and calls `MetaController::processOpData`. [cite_start]The resulting `Operator::processData` call may trigger `Scheduler::schedulePayloadForNextStep`, which in turn calls `TimeController::addToNextStepPayloads`.
* During **Phase 2**, it iterates `currentStepPayloads` and calls `MetaController::traversePayload`. The `Operator::traverse` method might call `Scheduler::scheduleMessage`, which calls `TimeController::deliverAndFlagOperator`. [cite_start]This method then uses `MetaController::messageOperator` to deliver the message and, if successful, adds the target ID back to `operatorsToProcess` for the next cycle.

## 5. Conclusion

[cite_start]The `TimeController` acts as the central clockwork and orchestrator for the simulation's dynamic activity. [cite_start]Its refined multi-phase processing ensures logical consistency: signals are delivered immediately upon arrival, accumulated during that step, and then processed at the start of the subsequent step. [cite_start]Its reliance on an injected `MetaController` instance facilitates high-level access to operators without breaking encapsulation.