#ifndef PSIM_UTILS_H
#define PSIM_UTILS_H

#include "geometry.h"

#include <array>
#include <random>

enum class SimulationType { SteadyState, Periodic, Transient };
inline constexpr double GEOEPS = std::numeric_limits<double>::epsilon() * 1E9;
inline constexpr double PI = 3.1415926535897932384626433832795028841971693993751058209749445923;

namespace Utils {

// Generates a random number from a uniform distribution over [0,1].
inline double urand() noexcept {
    static std::random_device rd;// NOLINT
    thread_local std::mt19937 generator(rd());
    std::uniform_real_distribution dist(0., std::nextafter(1., 2.));// NOLINT
    return dist(generator);
}

// Allows usage of enum classes as integral values similar to unscoped enums. Enum classes seems to
// require a static_cast<std::size_t> so this save having to type that everytime.
// Can use eType(enum) instead of std::static_cast<std::size_t>(enum).
// Must be constexpr to work with std::get when using variants
template<typename E> constexpr auto toInteger(E enumerator) noexcept {
    return static_cast<std::underlying_type_t<E>>(enumerator);
}

// Custom to function to emulate python zip functionality
// General implementation
template<typename In1, typename In2, typename Out>
void zip(In1 begin1, In2 end1, In2 begin2, In2 end2, Out result) {// NOLINT
    auto it1{ begin1 };
    auto it2{ begin2 };
    while (it1 != end1 && it2 != end2) { result++ = { *it1++, *it2++ }; }
}

// Specifically for vectors
template<typename T, typename U>
std::vector<std::pair<T, U>> zip(const std::vector<T>& r1, const std::vector<U> r2) {// NOLINT
    std::vector<std::pair<T, U>> result;
    zip(std::cbegin(r1), std::cend(r1), std::cbegin(r2), std::cend(r2), std::back_inserter(result));
    return result;
}

// Specifically for arrays
template<typename T, typename U, std::size_t V>
std::array<std::pair<T, U>, V> zip(const std::array<T, V>& r1, const std::array<U, V>& r2) {// NOLINT
    std::array<std::pair<T, U>, V> result;
    std::size_t index = 0;
    auto it1{ std::cbegin(r1) };
    auto it2{ std::cbegin(r2) };
    while (it1 != std::cend(r1) && it2 != std::cend(r2)) { result[index++] = { *it1++, *it2++ }; }
    return result;
}

/**
 * @brief Check if two floating-point values are approximately equal within a specified tolerance.
 *
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @param epsilon The tolerance used for the comparison (default: EPSILON).
 * @return true if the values are approximately equal within the given tolerance, false otherwise.
 */
[[nodiscard]] inline bool approxEqual(double a, double b, double epsilon = GEOEPS) noexcept {// NOLINT
    return std::abs(a - b) < epsilon;
}

}// namespace Utils

#endif// PSIM_UTILS_H
