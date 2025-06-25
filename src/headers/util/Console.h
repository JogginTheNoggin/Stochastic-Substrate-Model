#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

/**
 * @class ConsoleWriter
 * @brief An RAII class to ensure thread-safe printing to std::cout.
 * @details On construction, this class locks a static, global mutex. It buffers all
 * streamed output. On destruction (when it goes out of scope), it prints the
 * entire buffered string to std::cout at once and unlocks the mutex.
 * This prevents output from different threads from interleaving.
 * * @usage ConsoleWriter() << "This is a " << "thread-safe message." << std::endl;
 */
class ConsoleWriter {
public:
    /**
     * @brief Constructor that acquires a lock on the global printing mutex.
     */
    ConsoleWriter() : lock(get_print_mutex()) {}

    /**
     * @brief Destructor that prints the buffered content and releases the lock.
     */
    ~ConsoleWriter() {
        // Print the complete, buffered string to the actual console in one operation.
        std::cout << buffer.str();
    }

    /**
     * @brief Overload of the stream insertion operator to buffer content.
     * @tparam T The type of the data to be streamed.
     * @param val The value to be streamed into the buffer.
     * @return A reference to this ConsoleWriter instance for chaining.
     */
    template<typename T>
    ConsoleWriter& operator<<(const T& val) {
        buffer << val;
        return *this;
    }

    /**
     * @brief Overload to handle stream manipulators like std::endl.
     * @param manip A function pointer to a stream manipulator.
     * @return A reference to this ConsoleWriter instance for chaining.
     */
    ConsoleWriter& operator<<(std::ostream& (*manip)(std::ostream&)) {
        manip(buffer);
        return *this;
    }

private:
    /**
     * @brief Gets a reference to the static mutex for console printing.
     * @return A static std::mutex reference.
     * @details The 'static' keyword ensures there is only one instance of this mutex for the entire application.
     */
    static std::mutex& get_print_mutex() {
        static std::mutex print_mutex;
        return print_mutex;
    }

    std::unique_lock<std::mutex> lock;
    std::stringstream buffer;
};