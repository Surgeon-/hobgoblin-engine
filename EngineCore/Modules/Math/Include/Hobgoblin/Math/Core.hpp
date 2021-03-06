#ifndef UHOBGOBLIN_UTIL_MATH_HPP
#define UHOBGOBLIN_UTIL_MATH_HPP

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>

#include <algorithm>
#include <cmath>
#include <climits>
#include <type_traits>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace math {

using sf::Vector2;
using sf::Vector2i;
using sf::Vector2u;
using sf::Vector2f;
using Vector2d = Vector2<double>;

using sf::Vector3;
using sf::Vector3i;
using Vector3u = Vector3<unsigned>;
using sf::Vector3f;
using Vector3d = Vector3<double>;

template <class T>
T Clamp(const T& value, const T& low, const T& high) {
    return std::min(high, std::max(value, low));
}

template <class T>
T Sqr(T value) {
    return value * value;
}

template <class T>
T EuclideanDist(const Vector2<T>& p1, const Vector2<T>& p2) {
    return std::sqrt(Sqr(p2.x - p1.x) + Sqr(p2.y - p1.y));
}

template <class T>
constexpr typename std::enable_if_t<std::is_integral_v<T>, T> IntegralCeilDiv(T dividend, T divisor) {
    return dividend / divisor - ((-(dividend % divisor)) >> (sizeof(T) * CHAR_BIT - 1));
}

// Solves a quadratic equation
// aX0 will contain the lower solution, aX1 will contain the larger solution (if any)
// Returns false if can't be solved in real numbers
template <class taReal>
bool SolveQuadratic(taReal aA, taReal aB, taReal aC, taReal& aX0, taReal& aX1)
{
    using Real = taReal;

    const Real discriminant = (aB * aB) - (Real{4} * aA * aC);
    if (discriminant < Real{0}) {
        return false;
    }

    if (discriminant == Real{0}) {
        aX0 = aX1 = Real{-0.5} * aB / aA;
    }
    else {
        const Real q = (aB > Real{0}) ? -0.5 * (aB + std::sqrt(discriminant))
                                      : -0.5 * (aB - std::sqrt(discriminant));
        aX0 = q / aA;
        aX1 = aC / q;
    }

    if (aX0 > aX1) {
        std::swap(aX0, aX1);
    }

    return true;
}

} // namespace math
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_UTIL_MATH_HPP