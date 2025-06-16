# Neuron Simulator Serialization Framework

## 1. Purpose

[cite_start]This document specifies the conceptual framework and encoding principles for persisting the Neuron Simulator's state, divided into three categories: Network Configuration, Dynamic Simulation State, and Pending Updates.

## 2. General Encoding Conventions

* [cite_start]**Endianness**: All multi-byte numerical fields MUST use **Big-Endian** byte order.
* **Data Types**: The format uses fixed-size integer types (e.g., `uint8_t`, `uint16_t`, `uint32_t`) for cross-platform consistency. [cite_start]`Operator` and `Layer` IDs are `uint32_t`.
* [cite_start]**Encoding**: A `Serializer` utility class should be used for consistent encoding.

## 3. Section 1: Network Configuration File Format (`MetaController`)

[cite_start]**Purpose**: Persists the static structure and parameters of the entire network, organized by layers. [cite_start]`MetaController` is solely responsible for saving and loading this file.

**File Structure**: A contiguous sequence of **Layer Data Blocks**. [cite_start]The file is read sequentially until EOF.

### 3.1. Layer Data Block

[cite_start]The top-level block in the configuration file, produced by `Layer::serializeToBytes()`.

| Field                 | Size (Bytes) | Data Type / Representation | Description                                                                                    |
| --------------------- | ------------ | -------------------------- | ---------------------------------------------------------------------------------------------- |
| 1. Layer Type         | 1            | `uint8_t`                  | [cite_start]Numeric ID of the layer's type (e.g., 0=Input, 1=Output, 3=Internal).               |
| 2. isRangeFinal Flag  | 1            | `uint8_t`                  | [cite_start]Flag indicating if the layer's ID range is dynamic (0) or static (1).               |
| 3. NumBytesOfPayload  | 4            | `uint32_t` (Big Endian)    | [cite_start]Total byte size of the payload section (Field 4) that follows this header.          |
| 4. Layer Payload      | Variable     | Sequence of bytes          | [cite_start]The payload containing the layer's ID range and all of its operator data.         |

### 3.2. Layer Payload Format

[cite_start]This defines the structure of the "Layer Payload" (Field 4) from the table above.

| Field                     | Size (Bytes) | Data Type / Representation | Description                                                                                 |
| ------------------------- | ------------ | -------------------------- | ------------------------------------------------------------------------------------------- |
| 1. Reserved Min ID        | 4            | `uint32_t` (Big Endian)    | [cite_start]The minimum ID of the range reserved for this layer.                           |
| 2. Reserved Max ID        | 4            | `uint32_t` (Big Endian)    | [cite_start]The maximum ID of the range reserved for this layer.                           |
| 3. Operator Data Blocks   | Variable     | Sequence of Operator Blocks | [cite_start]A contiguous sequence of data blocks for all operators in this layer.          |

### 3.3. Operator Data Block

[cite_start]This block is produced by a derived `Operator::serializeToBytes()` method and represents a single operator.

| Field                    | Size (Bytes) | Data Type / Representation | Description                                                                                           |
| ------------------------ | ------------ | -------------------------- | ----------------------------------------------------------------------------------------------------- |
| 1. Operator Payload Size | 4            | `uint32_t` (Big Endian)    | [cite_start]Total byte size of the fields that follow this one (Fields 2 through N).                 |
| 2. Operator Type         | 2            | `uint16_t` (Big Endian)    | The specific type of the operator (e.g., `ADD`, `IN`, `OUT`).                             |
| 3. Operator ID           | 4            | `uint32_t` (Big Endian)    | [cite_start]The operator's unique ID.                                                                |
| 4. Connection Data       | Variable     | Connection Data            | [cite_start]The operator's outgoing connection data (see Section 3.4).                               |
| 5. Derived Class Data    | Variable     | Subclass-specific bytes    | [cite_start]Data specific to the derived operator type (e.g., `AddOperator` appends `weight`, `threshold`). |

### 3.4. Connection Data Format

[cite_start]This structure serializes the `outputConnections` data.

| Field                       | Size (Bytes) | Data Type / Representation | Description                                                |
| --------------------------- | ------------ | -------------------------- | ---------------------------------------------------------- |
| 1. Number of Buckets        | 2            | `uint16_t` (Big Endian)    | [cite_start]Total count of distinct distance buckets with connections. |
| 2. Bucket Data Blocks       | Variable     | Sequence of Bucket Blocks  | [cite_start]Data for each bucket.                         |
| **-- Bucket Block --** |              |                            |                                                            |
| 2a. Distance                | 2            | `uint16_t` (Big Endian)    | [cite_start]The distance (delay) for this bucket.         |
| 2b. Number of Connections   | 2            | `uint16_t` (Big Endian)    | [cite_start]The number of target operator IDs in this bucket. |
| 2c. Target Operator IDs     | Variable     | Sequence of `uint32_t`s    | [cite_start]The sequence of target operator IDs for this bucket. |

## 4. Section 2: Simulation State File Format (`TimeController`)

[cite_start]**Purpose**: Persists the dynamic simulation state (active `Payload`s, processing flags). [cite_start]`TimeController` is responsible for this file.

### 4.1. State File Header

| Field                       | Size (Bytes) | Data Type / Representation | Description                                    |
| --------------------------- | ------------ | -------------------------- | ---------------------------------------------- |
| 1. Current Payload Count    | 8            | `uint64_t` (Big Endian)    | Number of active `Payload`s in the current step. |
| 2. Next Step Payload Count  | 8            | `uint64_t` (Big Endian)    | [cite_start]Number of active `Payload`s for the next step.   |
| 3. OperatorsToProcess Count | 8            | `uint64_t` (Big Endian)    | Number of flagged `Operator` IDs.           |

### 4.2. State File Data Sections

[cite_start]Following the header, the file contains three contiguous data sections:
1.  [cite_start]**Current Payloads Data**: A sequence of `Payload Data Blocks`.
2.  [cite_start]**Next Step Payloads Data**: A sequence of `Payload Data Blocks`.
3.  [cite_start]**Operators To Process Data**: A sequence of `Operator ID Data Blocks`.

## 5. Section 3: Pending Updates File Format (`UpdateController`)

[cite_start]**Purpose**: Persists the queue of pending `UpdateEvent`s. [cite_start]`UpdateController` is responsible for this file.

**File Structure**: A contiguous sequence of **`UpdateEvent Data Blocks`**. [cite_start]There is no file header; the file is read until EOF.

## 6. Section 4: Shared Data Block Format Definitions

### 6.1. Payload Data Block

This reflects the format from `Payload::serializeToBytes`, which uses `Serializer::write` for its fields.

| Field                 | Size (Bytes) | Data Type / Representation | Description                                                                        |
| --------------------- | ------------ | -------------------------- | ---------------------------------------------------------------------------------- |
| 1. Payload Data Size  | 1            | `uint8_t`                  | [cite_start]Size N of fields 2-5 below (Max 255 bytes).                           |
| 2. Payload Type       | 2            | `uint16_t` (Big Endian)    | [cite_start]Type ID (e.g., 0x0000 = Default).                                     |
| 3. Current Op ID      | 4            | `uint32_t` (Big Endian)    | The `currentOperatorId` value.                           |
| 4. Message            | 1 + `sizeof(int)` | Size-Prefixed `int` (BE) | [cite_start]The `message` value, prefixed by its own size. |
| 5. Distance Traveled  | 2            | `uint16_t` (Big Endian)    | The `distanceTraveled` value.                                         |

### 6.2. Operator ID Data Block

[cite_start]This block is used in the `TimeController` state file for the `operatorsToProcess` set.

| Field        | Size (Bytes) | Data Type / Representation | Description                               |
| ------------ | ------------ | -------------------------- | ----------------------------------------- |
| 1. Op ID Size| 1            | `uint8_t`                  | [cite_start]Size S of the following ID value. |
| 2. Op ID Value | S            | `bytes` (Big Endian)       | [cite_start]The S bytes representing the ID.  |

### 6.3. UpdateEvent Data Block

[cite_start]This block is used in the `UpdateController` state file.

| Field                     | Size (Bytes) | Data Type / Representation   | Description                                                           |
| ------------------------- | ------------ | ---------------------------- | --------------------------------------------------------------------- |
| 1. Event Data Size        | 1            | `uint8_t`                    | [cite_start]Size N of fields 2-7 below (Max 255 bytes).              |
| 2. Update Type            | 1            | `uint8_t`                    | Numeric value of the `UpdateType`.                       |
| 3. Target Op ID           | 1 + `sizeof(int)` | Size-Prefixed `int` (BE) | [cite_start]The `targetOperatorId` value.                      |
| 4. Number of Parameters   | 1            | `uint8_t`                    | [cite_start]Count P of parameters that follow (Max 255).             |
| 5. Parameter Type Code    | 1            | `uint8_t`                    | [cite_start]Type code indicating data type of all parameters (e.g., 0=int). |
| 6. Parameter Values       | Variable     | Sequence of P size-prefixed `int`s (BE) | [cite_start]Sequence of parameter values, each encoded consistently. |

## 7. Section 5: Implementation Notes & Controller Roles

* **`MetaController`**: Manages the static network configuration file. It orchestrates `Layer` serialization/deserialization. [cite_start]When loading, it reads the "envelope" of each layer block, instantiates the correct `Layer` subclass, and delegates the parsing of the layer's internal payload to the subclass's constructor.
* **`TimeController`**: Manages the dynamic simulation state file, required to pause and resume a simulation. [cite_start]It saves/loads active payloads and the set of operators flagged for processing.
* [cite_start]**`UpdateController`**: Manages the pending updates file, saving and loading the queue of `UpdateEvent`s that have been submitted but not yet processed.