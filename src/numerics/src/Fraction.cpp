#include "limo/numerics/Fraction.hpp"

#include <cstdlib>

namespace limo::numerics::fraction {

void Fraction::normalize() {
    if (denom < 0) {
        num = -num;
        denom = -denom;
    }
    int64_t a = std::abs(num);
    int64_t b = std::abs(denom);
    while (b != 0) {
        int64_t temp = b;
        b = a % b;
        a = temp;
    }
    num /= a;
    denom /= a;
}


double Fraction::toDouble() const {
    return static_cast<double>(num) / static_cast<double>(denom);
}

Fraction Fraction::operator+(const Fraction& other) const {
    return Fraction(num * other.denom + other.num * denom, denom * other.denom);
}

Fraction Fraction::operator-(const Fraction& other) const {
    return Fraction(num * other.denom - other.num * denom, denom * other.denom);
}

Fraction Fraction::operator*(const Fraction& other) const {
    return Fraction(num * other.num, denom * other.denom);
}

Fraction Fraction::operator/(const Fraction& other) const {
    return Fraction(num * other.denom, denom * other.num);
}

bool Fraction::operator==(const Fraction& other) const {
    return num * other.denom == other.num * denom;
}

bool Fraction::operator!=(const Fraction& other) const {
    return !(*this == other);
}

bool Fraction::operator<(const Fraction& other) const {
    return num * other.denom < other.num * denom;
}

bool Fraction::operator<=(const Fraction& other) const {
    return *this < other || *this == other;
}

bool Fraction::operator>(const Fraction& other) const {
    return !(*this <= other);
}

bool Fraction::operator>=(const Fraction& other) const {
    return !(*this < other);
}

} // namespace limo::numerics::fraction
