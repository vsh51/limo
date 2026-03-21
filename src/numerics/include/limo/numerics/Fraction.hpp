#pragma once

#include <cstdint>

namespace limo::numerics::fraction {

/**
 * @brief Represents a rational number as a numerator/denominator pair.
 *
 * Provides normalization, arithmetic operators, and comparisons without
 * losing exact fractional precision. Aims to keep fractions in reduced form and
 * reduce fault accumulation during simplex operations.
 *
 * @author Volodymyr Shpyrka
 */
class Fraction {
public:
    Fraction(int64_t numerator = 0, int64_t denominator = 1) : num(numerator), denom(denominator) { normalize(); }

    void normalize();
    double toDouble() const;

    int64_t getNumerator() const { return num; }
    int64_t getDenominator() const { return denom; }

    Fraction operator+(const Fraction& other) const;
    Fraction operator-(const Fraction& other) const;
    Fraction operator*(const Fraction& other) const;
    Fraction operator/(const Fraction& other) const;
    bool operator==(const Fraction& other) const;
    bool operator!=(const Fraction& other) const;
    bool operator<(const Fraction& other) const;
    bool operator<=(const Fraction& other) const;
    bool operator>(const Fraction& other) const;
    bool operator>=(const Fraction& other) const;

private:
    int64_t num;
    int64_t denom;
};

} // namespace limo::numerics::fraction
