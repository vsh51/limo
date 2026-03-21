#include <gtest/gtest.h>
#include <vector>

#include "limo/basis/BigMBasisFinder.hpp"
#include "limo/core/Result.hpp"
#include "limo/numerics/Fraction.hpp"
#include "limo/core/LinearProgram.hpp"

using limo::basis::BigMBasisFinder;
using limo::core::LinearProgram;
using limo::numerics::fraction::Fraction;

class BigMBasisFinderTest : public ::testing::Test {
protected:
    // Helper to check if a matrix entry is correct
    bool isValueAt(const LinearProgram::Matrix& mat, std::size_t r, std::size_t c, int num, int den = 1) {
        return mat(r, c) == Fraction(num, den);
    }

    // Helper to check object coefficient
    bool isObjCoeff(const LinearProgram& lp, std::size_t c, Fraction val) {
        if (c >= lp.objectiveCoefficients().size()) return false;
        return lp.objectiveCoefficients()[c] == val;
    }
};

TEST_F(BigMBasisFinderTest, BasicSlackVariableCase) {
    // Maximize Z = x1 + x2
    // subject to:
    // x1 + x2 <= 10
    
    LinearProgram::Matrix matrix(1, 2);
    matrix(0, 0) = Fraction{1}; matrix(0, 1) = Fraction{1};
    
    std::vector<Fraction> rhs = { Fraction{10} };
    std::vector<Fraction> obj = { Fraction{1}, Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::LessEqual };
    
    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);
    
    BigMBasisFinder finder;
    Fraction bigM{1000};
    auto result = finder.build(lp, bigM);

    // Should have 1 slack variable
    EXPECT_EQ(result.originalVariableCount, 2);
    EXPECT_EQ(result.slackColumns.size(), 1);
    EXPECT_TRUE(result.surplusColumns.empty());
    EXPECT_TRUE(result.artificialColumns.empty());
    EXPECT_EQ(result.basisColumns.size(), 1);
    
    // Slack column should be at index 2
    EXPECT_EQ(result.slackColumns[0], 2);
    // Basis for row 0 is the slack variable
    EXPECT_EQ(result.basisColumns[0], 2);

    // Check augmented matrix
    // Row 0: [1, 1, 1]
    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, 0, 1));
    EXPECT_TRUE(isValueAt(augMat, 0, 1, 1));
    EXPECT_TRUE(isValueAt(augMat, 0, 2, 1)); // Slack coefficient
    
    // Objective for slack should be 0
    EXPECT_TRUE(isObjCoeff(result.augmented, 2, Fraction{0}));
}

TEST_F(BigMBasisFinderTest, GreaterThanOrEqualConstraint) {
    // Minimize Z = x1
    // subject to:
    // x1 >= 5

    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};
    
    std::vector<Fraction> rhs = { Fraction{5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::GreaterEqual };
    
    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Minimize, sense);
    
    BigMBasisFinder finder;
    Fraction bigM{1000};
    auto result = finder.build(lp, bigM);

    // Should add 1 surplus and 1 artificial variable
    EXPECT_EQ(result.originalVariableCount, 1);
    EXPECT_EQ(result.surplusColumns.size(), 1);
    EXPECT_EQ(result.artificialColumns.size(), 1);
    
    auto surplusIdx = result.surplusColumns[0]; // Index 1
    auto artificialIdx = result.artificialColumns[0]; // Index 2
    
    EXPECT_EQ(surplusIdx, 1);
    EXPECT_EQ(artificialIdx, 2);
    
    // Basis should be the artificial variable
    EXPECT_EQ(result.basisColumns[0], artificialIdx);

    // Check augmented matrix
    // Row 0: [1, -1, 1]
    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, surplusIdx, -1));
    EXPECT_TRUE(isValueAt(augMat, 0, artificialIdx, 1));

    // Check penalty in objective function (Minimize -> +M)
    EXPECT_TRUE(isObjCoeff(result.augmented, artificialIdx, bigM));
}

TEST_F(BigMBasisFinderTest, EqualityConstraint) {
    // Maximize Z = x1
    // subject to:
    // x1 = 5

    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};
    
    std::vector<Fraction> rhs = { Fraction{5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::Equal };
    
    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);
    
    BigMBasisFinder finder;
    Fraction bigM{1000};
    auto result = finder.build(lp, bigM);

    // Should add 1 artificial variable, no slack/surplus
    EXPECT_EQ(result.artificialColumns.size(), 1);
    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_TRUE(result.surplusColumns.empty());
    
    auto artificialIdx = result.artificialColumns[0];
    EXPECT_EQ(artificialIdx, 1);
    
    // Basis is artificial
    EXPECT_EQ(result.basisColumns[0], artificialIdx);
    
    // Check penalty in objective function (Maximize -> -M)
    EXPECT_TRUE(isObjCoeff(result.augmented, artificialIdx, Fraction{0} - bigM));
}

TEST_F(BigMBasisFinderTest, NegativeRHSHandling) {
    // Maximize Z = x1
    // subject to:
    // x1 <= -5
    // Should be flipped to: -x1 >= 5 (requires surplus + artificial)

    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};
    
    std::vector<Fraction> rhs = { Fraction{-5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::LessEqual };
    
    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);
    
    BigMBasisFinder finder;
    Fraction bigM{1000};
    auto result = finder.build(lp, bigM);

    // Original "<=" became ">=" due to negative RHS
    // So we expect surplus + artificial, NOT slack
    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_EQ(result.surplusColumns.size(), 1);
    EXPECT_EQ(result.artificialColumns.size(), 1);
    
    // Augmented RHS should be positive
    EXPECT_EQ(result.augmented.rightHandSide()[0], Fraction{5});
    
    // Matrix coefficient should be flipped (1 -> -1)
    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, 0, -1));
}

TEST_F(BigMBasisFinderTest, ComplexMix) {
    // Maximize Z = 3x1 + 2x2
    // subject to:
    // 2x1 + x2 <= 100  (Slack)
    // x1 + x2 = 80     (Artificial)
    // x1 >= 20         (Surplus + Artificial)
    
    LinearProgram::Matrix matrix(3, 2);
    // Row 0
    matrix(0, 0) = Fraction{2}; matrix(0, 1) = Fraction{1};
    // Row 1
    matrix(1, 0) = Fraction{1}; matrix(1, 1) = Fraction{1};
    // Row 2
    matrix(2, 0) = Fraction{1}; matrix(2, 1) = Fraction{0};

    std::vector<Fraction> rhs = { Fraction{100}, Fraction{80}, Fraction{20} };
    std::vector<Fraction> obj = { Fraction{3}, Fraction{2} };
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::LessEqual,
        LinearProgram::ConstraintSense::Equal,
        LinearProgram::ConstraintSense::GreaterEqual
    };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, senses);
    
    BigMBasisFinder finder;
    Fraction bigM{9999};
    auto result = finder.build(lp, bigM);

    // Verification
    // Slack: 1 (Row 0)
    // Surplus: 1 (Row 2)
    // Artificial: 2 (Row 1, Row 2)
    
    EXPECT_EQ(result.slackColumns.size(), 1);
    EXPECT_EQ(result.surplusColumns.size(), 1);
    EXPECT_EQ(result.artificialColumns.size(), 2);
    
    EXPECT_EQ(result.slackColumns[0], 2);
    EXPECT_EQ(result.surplusColumns[0], 3);
    EXPECT_EQ(result.artificialColumns[0], 4);
    EXPECT_EQ(result.artificialColumns[1], 5);
    
    // Check Basis Identification
    EXPECT_EQ(result.basisColumns[0], 2); // s1
    EXPECT_EQ(result.basisColumns[1], 4); // a1
    EXPECT_EQ(result.basisColumns[2], 5); // a2
    
    // Check Penalties (Maximize -> -M)
    Fraction penalty = Fraction{0} - bigM;
    EXPECT_TRUE(isObjCoeff(result.augmented, 4, penalty));
    EXPECT_TRUE(isObjCoeff(result.augmented, 5, penalty));
    EXPECT_TRUE(isObjCoeff(result.augmented, 2, Fraction{0})); // Slack has 0 cost
    EXPECT_TRUE(isObjCoeff(result.augmented, 3, Fraction{0})); // Surplus has 0 cost
}

TEST_F(BigMBasisFinderTest, NoBigMParameterThrows) {
    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};
    std::vector<Fraction> rhs = { Fraction{5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::LessEqual };
    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Minimize, sense);

    BigMBasisFinder finder;
    EXPECT_THROW(finder.build(lp), std::logic_error);
}

TEST_F(BigMBasisFinderTest, EmptyLPReturnsEmptyResult) {
    LinearProgram lp;
    BigMBasisFinder finder;
    Fraction bigM{1000};
    auto result = finder.build(lp, bigM);

    EXPECT_EQ(result.originalVariableCount, 0u);
    EXPECT_TRUE(result.basisColumns.empty());
    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_TRUE(result.surplusColumns.empty());
    EXPECT_TRUE(result.artificialColumns.empty());
    EXPECT_TRUE(result.augmented.constraintMatrix().empty());
}

TEST_F(BigMBasisFinderTest, MockLP) {
    // Minimize Z = x1 - x2 + 3x3
    // subject to:
    // 2x1 - x2 + 3x3 <= 5      (Slack)
    // x1 + 2x3 = 8             (Artificial)
    // -x1 - 2x2 >= 1           (Surplus + Artificial)

    // LP taken from "Дослідження операцій. Частина 1. Лінійні моделі" by M. Ya. Bartish, I. M. Dudzyanyi (Task 1, page 99)

    // Construct the LP
    std::vector<Fraction> obj = { Fraction{1}, Fraction{-1}, Fraction{3} };

    // Constraints left-hand side matrix:
    LinearProgram::Matrix matrix(3, 3);
    // Row 0: 2x1 - x2 + 3x3
    matrix(0, 0) = Fraction{2}; matrix(0, 1) = Fraction{-1}; matrix(0, 2) = Fraction{3};
    // Row 1: x1 + 2x3
    matrix(1, 0) = Fraction{1}; matrix(1, 1) = Fraction{0}; matrix(1, 2) = Fraction{2};
    // Row 2: -x1 - 2x2
    matrix(2, 0) = Fraction{-1}; matrix(2, 1) = Fraction{-2}; matrix(2, 2) = Fraction{0};

    // Constraints right-hand side
    std::vector<Fraction> rhs = { Fraction{5}, Fraction{8}, Fraction{1} };

    // Constraint senses
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::LessEqual,
        LinearProgram::ConstraintSense::Equal,
        LinearProgram::ConstraintSense::GreaterEqual
    };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Minimize, senses);

    BigMBasisFinder finder;
    Fraction bigM{10000};
    auto result = finder.build(lp, bigM);

    // Expected: 1 slack, 1 surplus, 2 artificial variables
    EXPECT_EQ(result.originalVariableCount, 3);
    EXPECT_EQ(result.slackColumns.size(), 1);
    EXPECT_EQ(result.surplusColumns.size(), 1);
    EXPECT_EQ(result.artificialColumns.size(), 2);

    // Columns: 0-2 original, 3 slack, 4 surplus, 5-6 artificial
    EXPECT_EQ(result.slackColumns[0], 3);
    EXPECT_EQ(result.surplusColumns[0], 4);
    EXPECT_EQ(result.artificialColumns[0], 5);
    EXPECT_EQ(result.artificialColumns[1], 6);

    // Basis: slack, artificial, artificial
    EXPECT_EQ(result.basisColumns[0], 3);
    EXPECT_EQ(result.basisColumns[1], 5);
    EXPECT_EQ(result.basisColumns[2], 6);

    // Penalties for minimize: +M
    EXPECT_TRUE(isObjCoeff(result.augmented, 5, bigM));
    EXPECT_TRUE(isObjCoeff(result.augmented, 6, bigM));
}