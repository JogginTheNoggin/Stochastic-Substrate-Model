#pragma once

/**
 * @enum NodeType
 * @brief Defines the fundamental types of processing nodes in the simulation.
 * @details Currently only includes OPERATOR, but kept extensible for potential
 * future additions like MetaNeurons or specialized node types.
 */
enum class NodeType {
	OPERATOR,   // Standard processing unit in the current model.
	// META_NEURON, // Example for future extension
	// SENSOR,  	// Example for future extension
	// ACTUATOR,	// Example for future extension
	UNKNOWN  	// Default or error state
};
