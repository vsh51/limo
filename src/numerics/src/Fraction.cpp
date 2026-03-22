#include "limo/numerics/Fraction.hpp"

#include <cstdlib>

namespace limo::numerics::fraction {

// ── Internal GCD (always returns non-negative) ────────────────────────────────

static int64_t gcd64(int64_t a, int64_t b) {
    a = a < 0 ? -a : a;
    b = b < 0 ? -b : b;
    while (b) { int64_t t = b; b = a % b; a = t; }
    return a;
}

static __int128 gcd128(__int128 a, __int128 b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { __int128 t = b; b = a % b; a = t; }
    return a;
}

// ── Normalize ─────────────────────────────────────────────────────────────────

void Fraction::normalize() {
    if (denom == 0) {
        // Invalid state — guard against SIGFPE; leave as-is
        return;
    }
    if (denom < 0) {
        num   = -num;
        denom = -denom;
    }
    int64_t g = gcd64(num, denom);
    if (g > 0) {
        num   /= g;
        denom /= g;
    }
}

// ── toDouble ──────────────────────────────────────────────────────────────────

double Fraction::toDouble() const {
    return static_cast<double>(num) / static_cast<double>(denom);
}

// ── Arithmetic ────────────────────────────────────────────────────────────────
//
// All four operations use __int128 for intermediate products so that
// int64_t overflow cannot occur and collapse a denominator to zero.
// A GCD reduction on the 128-bit result keeps the final int64_t values small.

Fraction Fraction::operator+(const Fraction& other) const {
    // a/b + c/d  with cross-reduction:  let g = gcd(b,d)
    int64_t g = gcd64(denom, other.denom);
    int64_t b_div_g = denom       / g;
    int64_t d_div_g = other.denom / g;

    using i128 = __int128;
    i128 n = (i128)num * d_div_g + (i128)other.num * b_div_g;
    i128 d = (i128)b_div_g * other.denom;

    i128 r = gcd128(n, d);
    if (r > 0) { n /= r; d /= r; }

    return Fraction(static_cast<int64_t>(n), static_cast<int64_t>(d));
}

Fraction Fraction::operator-(const Fraction& other) const {
    int64_t g = gcd64(denom, other.denom);
    int64_t b_div_g = denom       / g;
    int64_t d_div_g = other.denom / g;

    using i128 = __int128;
    i128 n = (i128)num * d_div_g - (i128)other.num * b_div_g;
    i128 d = (i128)b_div_g * other.denom;

    i128 r = gcd128(n, d);
    if (r > 0) { n /= r; d /= r; }

    return Fraction(static_cast<int64_t>(n), static_cast<int64_t>(d));
}

Fraction Fraction::operator*(const Fraction& other) const {
    // Cross-reduce before multiplying: (a/b)*(c/d)
    // g1 = gcd(|a|,|d|),  g2 = gcd(|c|,|b|)
    int64_t g1 = gcd64(num,       other.denom);
    int64_t g2 = gcd64(other.num, denom);

    using i128 = __int128;
    i128 n = (i128)(num       / g1) * (other.num  / g2);
    i128 d = (i128)(denom     / g2) * (other.denom / g1);

    i128 r = gcd128(n, d);
    if (r > 0) { n /= r; d /= r; }

    return Fraction(static_cast<int64_t>(n), static_cast<int64_t>(d));
}

Fraction Fraction::operator/(const Fraction& other) const {
    return *this * Fraction(other.denom, other.num);
}

// ── Comparisons ───────────────────────────────────────────────────────────────
//
// Use __int128 for cross-products to avoid overflow changing the sign.

bool Fraction::operator==(const Fraction& other) const {
    using i128 = __int128;
    return (i128)num * other.denom == (i128)other.num * denom;
}

bool Fraction::operator!=(const Fraction& other) const {
    return !(*this == other);
}

bool Fraction::operator<(const Fraction& other) const {
    using i128 = __int128;
    return (i128)num * other.denom < (i128)other.num * denom;
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
