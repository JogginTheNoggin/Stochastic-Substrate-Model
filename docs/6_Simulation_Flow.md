# Simulation Flow: Time Loop & Processing Phases

## 1. Overview

[cite_start]The simulation progresses in discrete time steps managed by the `TimeController`. [cite_start]A key design principle is the separation of concerns within each step, dividing processing into distinct logical phases to ensure consistency and deterministic behavior. [cite_start]This process is distinct from the separate "Update Loop," which handles all structural and parameter changes *between* time steps.

## 2. Time Step Phases (Executed by `TimeController::processCurrentStep()`)

[cite_start]Each iteration of the main loop executes the following logical phases in order.

### Phase 1: Check Operators / Process Data (Start of Step N+1)

* [cite_start]**Trigger**: The `TimeController` iterates through its `operatorsToProcess` set. [cite_start]This set contains the IDs of `Operator`s that successfully received messages during the previous step (Step N).
* [cite_start]**Action**: For each `operatorId`, the `TimeController` makes a high-level call to the `MetaController`: `metaControllerInstance.processOpData(operatorId);`. [cite_start]The `MetaController` then routes this request to the correct `Layer`, which in turn calls the `processData()` method on the specific `Operator`.
* **Inside `Operator::processData()`**:
    1.  [cite_start]The method checks the `accumulateData` (summed inputs from Step N) against the `Operator`'s threshold.
    2.  [cite_start]If the threshold is met, it finds the **first available connection distance** (`D_first`) and creates one or more new `Payload` objects.
    3.  [cite_start]Each new payload is initialized with its initial target distance set: `distanceTraveled = D_first`.
    4.  [cite_start]These new payloads are scheduled to start their journey in a *subsequent* time step (e.g., Step N+2) by calling `Scheduler::get()->schedulePayloadForNextStep()`.
    5.  [cite_start]Finally, `processData` resets `accumulateData` to zero.
* [cite_start]**Cleanup**: The `operatorsToProcess` set is cleared after the iteration is complete.
* [cite_start]**Purpose**: To process the total accumulated input from the *previous* step, make the "firing" decision, launch new signals for a *future* step, and clear the accumulator for the *current* step's new inputs.

### Phase 2: Process Traveling Payloads (Main part of Step N+1)

* [cite_start]**Trigger**: After Phase 1, the `TimeController` iterates through the list of `Payload` objects currently active and in transit (`currentStepPayloads`).
* [cite_start]**Action**: For each active `Payload`, the `TimeController` makes a high-level call: `metaControllerInstance.traversePayload(&payload);`. [cite_start]The `MetaController` routes this request to the `Layer` responsible for the managing `Operator` (`payload->currentOperatorId`), which then calls `managingOp->traverse(&payload)`. [cite_start]If the managing operator was deleted mid-flight, the `Layer` marks the payload as inactive.
* **Inside `Operator::traverse(Payload&)`**:
    1.  [cite_start]The method uses the payload's current target distance, `D_target = payload.distanceTraveled`, to look up the connection set in its `outputConnections` array.
    2.  **If the target distance is reached** (i.e., a valid, non-empty connection set exists):
        * [cite_start]It iterates through each `targetId` in the set and calls `Scheduler::get()->scheduleMessage(targetId, payload.message)`. [cite_start]This triggers an **immediate** data delivery to the target `Operator`'s `message()` method and adds the `targetId` to the `operatorsToProcess` set for Step N+2.
        * [cite_start]It then finds the **next available connection distance** (`nextDistance`) greater than the current one.
        * [cite_start]It updates the payload's state: `payload.distanceTraveled = nextDistance`, or if no further connections exist, it sets `payload.active = false`.
    3.  [cite_start]**If the target distance has no connections** (or the pointer is null), it still attempts to find the next available connection distance and updates the payload's state to "skip" the empty bucket, or deactivates it if none exist.
* [cite_start]**Cleanup**: After the loop, the `TimeController` may remove any payloads that were marked as inactive.
* **Purpose**: To manage the progression of payloads already in the network. [cite_start]It handles arrivals by triggering immediate data delivery and flagging recipients for next-step processing, implicitly managing branching, and transitioning payloads to their next target distance or marking them inactive.

## 3. Time Advancement (Executed by `TimeController::advanceStep`)

[cite_start]This method is called by the main loop *after* `processCurrentStep()` and *after* `UpdateController::ProcessUpdates()`. It performs the following actions:

* [cite_start]Moves newly scheduled payloads from `nextTimeStepPayloads` to `currentStepPayloads`.
* [cite_start]Clears `nextStepPayloads`.
* Increments the master step counter.

## 4. Rationale

This revised multi-phase approach ensures that:

* [cite_start]An `Operator`'s decision to fire (`processData`) is based on the complete input received and accumulated in the **previous** step.
* [cite_start]Signal data is delivered (`message()`) **within the same step** it conceptually arrives, mediated by the `Scheduler` -> `TimeController` call chain.
* [cite_start]Newly created payloads (from `processData`) do not start their journey until a **subsequent** step, preventing interference with payloads already in transit.
* [cite_start]The logic for payload progression (`traversePayload`), data delivery (`messageOperator`), and firing logic (`processOperatorData`) are handled in distinct, causally consistent phases, which provides determinism and consistency within each discrete time step.