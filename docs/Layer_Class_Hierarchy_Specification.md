# Layer Class Hierarchy Specification

## 1. Purpose and Role of the Layer Class Hierarchy

[cite_start]The `Layer` class hierarchy provides a structured way to organize, manage, and interact with groups of `Operator` objects. [cite_start]The system moves away from a single, flat pool of operators in `MetaController` to a more robust architecture where `MetaController` orchestrates a collection of `Layer` objects.

* [cite_start]**Abstract Base Class `Layer`**: Defines a common interface and shared functionality for all layer types. [cite_start]It is an abstract class responsible for owning `Operator`s, managing their lifecycles, and handling serialization and ID validation.
* [cite_start]**Concrete Subclasses** (`InputLayer`, `OutputLayer`, `InternalLayer`): These classes inherit from `Layer` and implement specialized behaviors, particularly for programmatic random initialization (`randomInit`), representing the distinct functional segments of the network.

## 2. The `Layer` Abstract Base Class

### Core State and Properties

* [cite_start]`type` (LayerType): An enum that identifies the functional role of the concrete subclass (e.g., `INPUT_LAYER`, `INTERNAL_LAYER`).
* [cite_start]`isRangeFinal` (bool): A critical flag that determines if the layer's ID range is static (`true`) or dynamic (`false`). [cite_start]Dynamic layers can create new operators beyond their current maximum ID, extending their range forward.
* [cite_start]`reservedRange` (IdRange*): A pointer to an `IdRange` object defining the ID space allocated to this layer. [cite_start]The `Layer` class is responsible for managing the memory for this object.
* [cite_start]`currentMinId` & `currentMaxId` (uint32_t): Track the actual minimum and maximum IDs of the `Operator` objects currently present in the layer.
* `operators` (`std::unordered_map<uint32_t, Operator*>`): The collection of `Operator` pointers owned by the layer. [cite_start]The `Layer` destructor is responsible for deleting all contained `Operator` objects.

### Key Functionality and Responsibilities

* [cite_start]**ID Management**: The `Layer` is autonomous in managing IDs within its given range. [cite_start]It can generate the next valid ID (`generateNextId`) and validates any new operator's ID against its range and uniqueness (`isValidId`, `isIdAvailable`).
* [cite_start]**Runtime Action Delegation**: The `Layer` provides a public interface for `MetaController` to delegate runtime actions, enforcing encapsulation.
    * [cite_start]`messageOperator(uint32_t operatorId, int message)`: Finds the operator and calls its `message()` method.
    * [cite_start]`processOperatorData(uint32_t operatorId)`: Finds the operator and calls its `processData()` method.
    * [cite_start]`traverseOperatorPayload(Payload* payload)`: Finds the operator and calls its `traverse()` method.
* **Serialization**: `serializeToBytes()` produces a complete layer block for the configuration file. [cite_start]The block format is `[Type][isRangeFinal][NumBytesOfPayload][Payload]`, where the payload contains the reserved ID range and all operator data, sorted by ID for deterministic output.
* **Deserialization**: The `deserialize` method is called by subclass constructors. [cite_start]It is given the payload segment from the file, reads the layer's `reservedRange`, and then deserializes the sequence of `Operator` blocks.
* [cite_start]**Abstract Method**: The class is abstract due to the pure virtual method `virtual void randomInit(...) = 0;`, which forces subclasses to implement their own logic for programmatic generation.

## 3. Concrete Layer Subclasses

### `InputLayer`

* [cite_start]**Role**: Serves as the interface for external data to enter the simulation. [cite_start]It contains a fixed number of "channels," each being an `InOperator`.
* [cite_start]**Specialization**: Its `randomInit` implementation establishes outgoing connections from its channels to other layers. [cite_start]It validates that it only contains `InOperator` instances. [cite_start]It also provides methods like `inputText` to feed data into a channel.

### `OutputLayer`

* [cite_start]**Role**: Serves as the interface to read results from the simulation. [cite_start]It contains `OutOperator` "channels" that collect signals but do not propagate them further.
* [cite_start]**Specialization**: Its `randomInit` implementation is empty, as `OutOperator`s have no outgoing connections. [cite_start]It validates that it only contains `OutOperator` instances. [cite_start]It provides methods like `getTextOutput` to retrieve data.

### `InternalLayer`

* [cite_start]**Role**: The primary processing core of the network, containing operators like `AddOperator`.
* [cite_start]**Specialization**: This is the layer typically designated as dynamic (`isRangeFinal = false`), allowing the network to grow at runtime. [cite_start]Its `randomInit` implementation populates the layer with processing operators and establishes their connections to other internal or output operators.

## 4. System-Level Rules (Enforced by `MetaController`)

When loading or creating a network, `MetaController` enforces these global rules:
* [cite_start]**Single Dynamic Layer**: There must be exactly one layer with `isRangeFinal` set to `false`.
* [cite_start]**Dynamic Layer ID Supremacy**: The dynamic layer's minimum ID must be greater than the maximum ID of all other static layers, giving it a clear space to expand into.
* [cite_start]**No Inter-Layer ID Overlap**: It performs checks to ensure that no two layers have overlapping `reservedRange`s.