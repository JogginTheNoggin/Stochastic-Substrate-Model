#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace Constants {

// Example string literals
inline constexpr const char* Version = "1.0.0";
// maybe add cli strings


inline constexpr const int NETWORK_SIZE = 8; //careful not to go too high, it is power of 2  

// If you need std::string (rarely for constants, but sometimes)



} // namespace Constants

#endif // CONSTANTS_H