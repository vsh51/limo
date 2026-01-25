#pragma once

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
    Fraction(int numerator = 0, int denominator = 1) : num(numerator), denom(denominator) {}

    void normalize();
    double toDouble() const;

    int getNumerator() const { return num; }
    int getDenominator() const { return denom; }

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
    int num;
    int denom;
};

} // namespace limo::numerics::fraction
