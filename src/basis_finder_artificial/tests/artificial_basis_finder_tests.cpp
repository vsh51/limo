#include <gtest/gtest.h>

#include <vector>

#include "limo/basis/ArtificialBasisFinder.hpp"
#include "limo/core/LinearProgram.hpp"
#include "limo/numerics/Fraction.hpp"

using limo::basis::ArtificialBasisFinder;
using limo::core::LinearProgram;
using limo::numerics::fraction::Fraction;

class ArtificialBasisFinderTest : public ::testing::Test {
protected:
    bool isValueAt(const LinearProgram::Matrix& mat, std::size_t r, std::size_t c, int num, int den = 1) {
        return mat(r, c) == Fraction(num, den);
    }

    bool isObjCoeff(const LinearProgram& lp, std::size_t c, Fraction val) {
        if (c >= lp.objectiveCoefficients().size()) return false;
        return lp.objectiveCoefficients()[c] == val;
    }
};

TEST_F(ArtificialBasisFinderTest, BasicSlackVariableCase) {
    // Maximize Z = x1 + x2
    // subject to:
    // x1 + x2 <= 10 (Slack)
    LinearProgram::Matrix matrix(1, 2);
    matrix(0, 0) = Fraction{1};
    matrix(0, 1) = Fraction{1};

    std::vector<Fraction> rhs = { Fraction{10} };
    std::vector<Fraction> obj = { Fraction{1}, Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::LessEqual };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_EQ(result.originalVariableCount, 2);
    EXPECT_EQ(result.slackColumns.size(), 1);
    EXPECT_TRUE(result.surplusColumns.empty());
    EXPECT_TRUE(result.artificialColumns.empty());
    EXPECT_EQ(result.basisColumns.size(), 1);

    EXPECT_EQ(result.slackColumns[0], 2);
    EXPECT_EQ(result.basisColumns[0], 2);

    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, 0, 1));
    EXPECT_TRUE(isValueAt(augMat, 0, 1, 1));
    EXPECT_TRUE(isValueAt(augMat, 0, 2, 1));

    EXPECT_TRUE(isObjCoeff(result.augmented, 0, Fraction{0}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 1, Fraction{0}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 2, Fraction{0}));
}

TEST_F(ArtificialBasisFinderTest, EmptyMatrixReturnsEmptyResult) {
    // Empty LP should return an empty result without crashes.
    LinearProgram lp;

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_EQ(result.originalVariableCount, 0u);
    EXPECT_TRUE(result.basisColumns.empty());
    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_TRUE(result.surplusColumns.empty());
    EXPECT_TRUE(result.artificialColumns.empty());

    EXPECT_TRUE(result.augmented.constraintMatrix().empty());
    EXPECT_TRUE(result.augmented.rightHandSide().empty());
    EXPECT_TRUE(result.augmented.objectiveCoefficients().empty());
}

TEST_F(ArtificialBasisFinderTest, GreaterThanOrEqualConstraint) {
    // Minimize Z = x1
    // subject to:
    // x1 >= 5 (Surplus + Artificial)
    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};

    std::vector<Fraction> rhs = { Fraction{5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::GreaterEqual };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Minimize, sense);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_EQ(result.originalVariableCount, 1);
    EXPECT_EQ(result.surplusColumns.size(), 1);
    EXPECT_EQ(result.artificialColumns.size(), 1);

    auto surplusIdx = result.surplusColumns[0];
    auto artificialIdx = result.artificialColumns[0];

    EXPECT_EQ(surplusIdx, 1);
    EXPECT_EQ(artificialIdx, 2);
    EXPECT_EQ(result.basisColumns[0], artificialIdx);

    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, surplusIdx, -1));
    EXPECT_TRUE(isValueAt(augMat, 0, artificialIdx, 1));

    EXPECT_TRUE(isObjCoeff(result.augmented, artificialIdx, Fraction{1}));
}

TEST_F(ArtificialBasisFinderTest, EqualityConstraint) {
    // Maximize Z = x1
    // subject to:
    // x1 = 5 (Artificial)
    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};

    std::vector<Fraction> rhs = { Fraction{5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::Equal };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_EQ(result.artificialColumns.size(), 1);
    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_TRUE(result.surplusColumns.empty());

    auto artificialIdx = result.artificialColumns[0];
    EXPECT_EQ(artificialIdx, 1);
    EXPECT_EQ(result.basisColumns[0], artificialIdx);

    EXPECT_TRUE(isObjCoeff(result.augmented, artificialIdx, Fraction{1}));
}

TEST_F(ArtificialBasisFinderTest, NegativeRHSHandling) {
    // Maximize Z = x1
    // subject to:
    // x1 <= -5
    // Should be flipped to: -x1 >= 5 (Surplus + Artificial)
    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};

    std::vector<Fraction> rhs = { Fraction{-5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::LessEqual };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_EQ(result.surplusColumns.size(), 1);
    EXPECT_EQ(result.artificialColumns.size(), 1);

    EXPECT_EQ(result.augmented.rightHandSide()[0], Fraction{5});

    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, 0, -1));
}

TEST_F(ArtificialBasisFinderTest, GreaterEqualWithNegativeRHSBecomesSlack) {
    // Maximize Z = x1
    // subject to:
    // x1 >= -5
    // Should be flipped to: -x1 <= 5 (Slack)
    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};

    std::vector<Fraction> rhs = { Fraction{-5} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::GreaterEqual };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_EQ(result.slackColumns.size(), 1);
    EXPECT_TRUE(result.surplusColumns.empty());
    EXPECT_TRUE(result.artificialColumns.empty());

    const auto slackIdx = result.slackColumns[0];
    EXPECT_EQ(slackIdx, 1u);
    EXPECT_EQ(result.basisColumns[0], slackIdx);

    EXPECT_EQ(result.augmented.rightHandSide()[0], Fraction{5});

    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, 0, -1));
    EXPECT_TRUE(isValueAt(augMat, 0, slackIdx, 1));
    EXPECT_TRUE(isObjCoeff(result.augmented, slackIdx, Fraction{0}));
}

TEST_F(ArtificialBasisFinderTest, EqualityWithNegativeRHSUsesArtificial) {
    // Maximize Z = x1
    // subject to:
    // 2x1 = -6
    // Should be flipped to: -2x1 = 6 (Artificial)
    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{2};

    std::vector<Fraction> rhs = { Fraction{-6} };
    std::vector<Fraction> obj = { Fraction{1} };
    std::vector<LinearProgram::ConstraintSense> sense = { LinearProgram::ConstraintSense::Equal };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, sense);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_TRUE(result.surplusColumns.empty());
    EXPECT_EQ(result.artificialColumns.size(), 1);

    const auto artificialIdx = result.artificialColumns[0];
    EXPECT_EQ(artificialIdx, 1u);
    EXPECT_EQ(result.basisColumns[0], artificialIdx);

    EXPECT_EQ(result.augmented.rightHandSide()[0], Fraction{6});

    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, 0, -2));
    EXPECT_TRUE(isValueAt(augMat, 0, artificialIdx, 1));
    EXPECT_TRUE(isObjCoeff(result.augmented, artificialIdx, Fraction{1}));
}

TEST_F(ArtificialBasisFinderTest, ComplexMix) {
    // Maximize Z = 3x1 + 2x2
    // subject to:
    // 2x1 + x2 <= 100 (Slack)
    // x1 + x2 = 80    (Artificial)
    // x1 >= 20        (Surplus + Artificial)
    LinearProgram::Matrix matrix(3, 2);
    matrix(0, 0) = Fraction{2};
    matrix(0, 1) = Fraction{1};
    matrix(1, 0) = Fraction{1};
    matrix(1, 1) = Fraction{1};
    matrix(2, 0) = Fraction{1};
    matrix(2, 1) = Fraction{0};

    std::vector<Fraction> rhs = { Fraction{100}, Fraction{80}, Fraction{20} };
    std::vector<Fraction> obj = { Fraction{3}, Fraction{2} };
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::LessEqual,
        LinearProgram::ConstraintSense::Equal,
        LinearProgram::ConstraintSense::GreaterEqual
    };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, senses);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_EQ(result.slackColumns.size(), 1);
    EXPECT_EQ(result.surplusColumns.size(), 1);
    EXPECT_EQ(result.artificialColumns.size(), 2);

    EXPECT_EQ(result.slackColumns[0], 2);
    EXPECT_EQ(result.surplusColumns[0], 3);
    EXPECT_EQ(result.artificialColumns[0], 4);
    EXPECT_EQ(result.artificialColumns[1], 5);

    EXPECT_EQ(result.basisColumns[0], 2);
    EXPECT_EQ(result.basisColumns[1], 4);
    EXPECT_EQ(result.basisColumns[2], 5);

    EXPECT_TRUE(isObjCoeff(result.augmented, 4, Fraction{1}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 5, Fraction{1}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 2, Fraction{0}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 3, Fraction{0}));
}

TEST_F(ArtificialBasisFinderTest, Example2_5FromBook) {
    // Example 2.5 from the textbook (equality-only constraints).
    // Minimize f(x) = 5x1 + 3x2 + x3 - 4x4
    // subject to:
    // x1 + x2 + 2x3 + x4 = 5
    // x1 - 2x2 + 3x3     = 5
    // 3x1 + 7x3 + 2x4    = 13
    // xi >= 0

    std::vector<Fraction> obj = { Fraction{5}, Fraction{3}, Fraction{1}, Fraction{-4} };

    LinearProgram::Matrix matrix(3, 4);
    matrix(0, 0) = Fraction{1}; matrix(0, 1) = Fraction{1};  matrix(0, 2) = Fraction{2}; matrix(0, 3) = Fraction{1};
    matrix(1, 0) = Fraction{1}; matrix(1, 1) = Fraction{-2}; matrix(1, 2) = Fraction{3}; matrix(1, 3) = Fraction{0};
    matrix(2, 0) = Fraction{3}; matrix(2, 1) = Fraction{0};  matrix(2, 2) = Fraction{7}; matrix(2, 3) = Fraction{2};

    std::vector<Fraction> rhs = { Fraction{5}, Fraction{5}, Fraction{13} };
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::Equal,
        LinearProgram::ConstraintSense::Equal,
        LinearProgram::ConstraintSense::Equal
    };

    LinearProgram lp(matrix, rhs, obj, LinearProgram::ObjectiveSense::Minimize, senses);

    ArtificialBasisFinder finder;
    auto result = finder.build(lp);

    EXPECT_EQ(result.originalVariableCount, 4);
    EXPECT_TRUE(result.slackColumns.empty());
    EXPECT_TRUE(result.surplusColumns.empty());
    EXPECT_EQ(result.artificialColumns.size(), 3);

    EXPECT_EQ(result.artificialColumns[0], 4);
    EXPECT_EQ(result.artificialColumns[1], 5);
    EXPECT_EQ(result.artificialColumns[2], 6);

    EXPECT_EQ(result.basisColumns[0], 4);
    EXPECT_EQ(result.basisColumns[1], 5);
    EXPECT_EQ(result.basisColumns[2], 6);

    const auto& augMat = result.augmented.constraintMatrix();
    EXPECT_TRUE(isValueAt(augMat, 0, 0, 1));
    EXPECT_TRUE(isValueAt(augMat, 0, 1, 1));
    EXPECT_TRUE(isValueAt(augMat, 0, 2, 2));
    EXPECT_TRUE(isValueAt(augMat, 0, 3, 1));
    EXPECT_TRUE(isValueAt(augMat, 0, 4, 1));

    EXPECT_TRUE(isValueAt(augMat, 1, 0, 1));
    EXPECT_TRUE(isValueAt(augMat, 1, 1, -2));
    EXPECT_TRUE(isValueAt(augMat, 1, 2, 3));
    EXPECT_TRUE(isValueAt(augMat, 1, 3, 0));
    EXPECT_TRUE(isValueAt(augMat, 1, 5, 1));

    EXPECT_TRUE(isValueAt(augMat, 2, 0, 3));
    EXPECT_TRUE(isValueAt(augMat, 2, 1, 0));
    EXPECT_TRUE(isValueAt(augMat, 2, 2, 7));
    EXPECT_TRUE(isValueAt(augMat, 2, 3, 2));
    EXPECT_TRUE(isValueAt(augMat, 2, 6, 1));

    EXPECT_EQ(result.augmented.rightHandSide()[0], Fraction{5});
    EXPECT_EQ(result.augmented.rightHandSide()[1], Fraction{5});
    EXPECT_EQ(result.augmented.rightHandSide()[2], Fraction{13});

    EXPECT_TRUE(isObjCoeff(result.augmented, 0, Fraction{0}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 1, Fraction{0}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 2, Fraction{0}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 3, Fraction{0}));

    EXPECT_TRUE(isObjCoeff(result.augmented, 4, Fraction{1}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 5, Fraction{1}));
    EXPECT_TRUE(isObjCoeff(result.augmented, 6, Fraction{1}));
}
