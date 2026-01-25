#include "limo/matrix/Fraction.hpp"

#include <gtest/gtest.h>

using limo::fraction::Fraction;

TEST(FractionNormalizeTests, ReducesAndKeepsPositiveDenominator) {
    Fraction value(2, 4);
    value.normalize();
    EXPECT_EQ(value.getNumerator(), 1);
    EXPECT_EQ(value.getDenominator(), 2);

    Fraction negativeDenom(1, -3);
    negativeDenom.normalize();
    EXPECT_EQ(negativeDenom.getNumerator(), -1);
    EXPECT_EQ(negativeDenom.getDenominator(), 3);
}

TEST(FractionArithmeticTests, SupportsAddSubtractMultiplyDivide) {
    Fraction left(1, 2);
    Fraction right(1, 3);

    Fraction sum = left + right;
    sum.normalize();
    EXPECT_EQ(sum.getNumerator(), 5);
    EXPECT_EQ(sum.getDenominator(), 6);

    Fraction diff = left - right;
    diff.normalize();
    EXPECT_EQ(diff.getNumerator(), 1);
    EXPECT_EQ(diff.getDenominator(), 6);

    Fraction product = left * right;
    product.normalize();
    EXPECT_EQ(product.getNumerator(), 1);
    EXPECT_EQ(product.getDenominator(), 6);

    Fraction quotient = left / right;
    quotient.normalize();
    EXPECT_EQ(quotient.getNumerator(), 3);
    EXPECT_EQ(quotient.getDenominator(), 2);
}

TEST(FractionComparisonTests, ComparesWithCrossMultiplication) {
    Fraction half(1, 2);
    Fraction equivalent(2, 4);
    Fraction third(1, 3);

    EXPECT_TRUE(half == equivalent);
    EXPECT_TRUE(half != third);
    EXPECT_TRUE(third < half);
    EXPECT_TRUE(third <= half);
    EXPECT_TRUE(half > third);
    EXPECT_TRUE(half >= third);
}

TEST(FractionValueTests, ConvertsToDouble) {
    Fraction value(1, 4);
    EXPECT_DOUBLE_EQ(value.toDouble(), 0.25);
}
