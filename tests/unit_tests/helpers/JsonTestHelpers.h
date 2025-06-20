#pragma once

#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error
#include <algorithm> // For std::remove_if for whitespace stripping
#include <sstream>   // Required for std::stringstream
#include <fstream>
#include <cctype>    // Required for isdigit

namespace JsonTestHelpers {

// Basic helper to trim whitespace from both ends of a string
static inline std::string trim(const std::string& str) {
    const std::string whitespace = " \t\r\n\f\v"; // Standard C++ whitespace characters
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) { // No non-whitespace characters
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, (end - start + 1));
}


// Helper function to extract an array of integer values from a JSON string
// This is a simplified parser. A robust JSON library would be better for complex JSON.
static std::vector<int> getJsonArrayIntValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\""; // e.g., "accumulatedData"
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) {
        throw std::runtime_error("Key '" + key + "' not found in JSON: " + json);
    }

    size_t colon_pos = json.find(':', key_pos + search_key.length());
    if (colon_pos == std::string::npos) {
        throw std::runtime_error("Colon not found after key '" + key + "' in JSON: " + json);
    }

    size_t array_start_pos = json.find('[', colon_pos + 1);
    if (array_start_pos == std::string::npos) {
        throw std::runtime_error("Array start '[' not found for key '" + key + "' in JSON: " + json);
    }

    size_t array_end_pos = json.find(']', array_start_pos + 1);
    if (array_end_pos == std::string::npos) {
        throw std::runtime_error("Array end ']' not found for key '" + key + "' in JSON: " + json);
    }

    std::string array_content_str = json.substr(array_start_pos + 1, array_end_pos - (array_start_pos + 1));

    std::vector<int> values;
    if (trim(array_content_str).empty()) { // Handle empty array
        return values;
    }

    std::stringstream ss(array_content_str);
    std::string segment;

    while(std::getline(ss, segment, ',')) {
        try {
            // Trim whitespace from segment before converting
            std::string trimmed_segment = trim(segment);
            if (!trimmed_segment.empty()) { // Ensure not pushing empty strings if there are trailing commas etc.
                 values.push_back(std::stoi(trimmed_segment));
            }
        } catch (const std::invalid_argument& ia) {
            throw std::runtime_error("Invalid argument to stoi for segment '" + segment + "' in array for key '" + key + "' in JSON: " + ia.what() + " Original JSON: " + json );
        } catch (const std::out_of_range& oor) {
            throw std::runtime_error("Out of range for stoi for segment '" + segment + "' in array for key '" + key + "' in JSON: " + oor.what() + " Original JSON: " + json);
        }
    }
    return values;
}

// Overload for getting a single integer value (from AddOperatorTests.cpp, slightly modified)
static int getJsonIntValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) {
        throw std::runtime_error("Key '" + key + "' not found in JSON: " + json);
    }

    size_t colon_pos = json.find(':', key_pos + search_key.length());
    if (colon_pos == std::string::npos) {
        throw std::runtime_error("Colon not found after key '" + key + "' in JSON: " + json);
    }

    size_t value_start_pos = colon_pos + 1;
    // Skip leading whitespace for the value
    const std::string val_whitespace = " \t\r\n\f\v"; // Standard C++ whitespace characters
    value_start_pos = json.find_first_not_of(val_whitespace, value_start_pos);
    if (value_start_pos == std::string::npos) {
         throw std::runtime_error("Value start not found for key '" + key + "' in JSON: " + json);
    }


    size_t value_end_pos = value_start_pos;
    // Allow for negative sign
    if (json[value_start_pos] == '-') {
        value_end_pos++;
    }
    while (value_end_pos < json.length() && isdigit(json[value_end_pos])) {
        value_end_pos++;
    }

    std::string value_str = json.substr(value_start_pos, value_end_pos - value_start_pos);
    if (value_str.empty() || (value_str == "-" && value_str.length() == 1)) { // check for empty or just "-"
        throw std::runtime_error("Empty or invalid value for key '" + key + "' in JSON: " + json);
    }

    try {
        return std::stoi(value_str);
    } catch (const std::invalid_argument& ia) {
        throw std::runtime_error("Invalid argument to stoi for key '" + key + "' with value '" + value_str + "' in JSON: " + ia.what() + " Original JSON: " + json);
    } catch (const std::out_of_range& oor) {
        throw std::runtime_error("Out of range for stoi for key '" + key + "' with value '" + value_str + "' in JSON: " + oor.what() + " Original JSON: " + json);
    }
}


// Helper function to read a file into a string (can be put in TestHelpers.h)
/**
 *  @note Place files in the build/golden_files or where the test exe is located
 */  
static inline std::string ReadGoldenFile(const std::string& path) {
    std::ifstream file("golden_files/" + path);
    if (!file) {
        throw std::runtime_error("Failed to open golden file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace JsonTestHelpers
