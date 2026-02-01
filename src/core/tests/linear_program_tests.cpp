#include "limo/core/LinearProgram.hpp"

#include <gtest/gtest.h>

using limo::core::LinearProgram;
using limo::numerics::fraction::Fraction;

namespace {

LinearProgram::Matrix makeMatrix() {
    return {{Fraction(2), Fraction(1)},
            {Fraction(1), Fraction(1)},
            {Fraction(1), Fraction(0)}};
}

std::vector<Fraction> makeRhs() {
    return {Fraction(100), Fraction(80), Fraction(20)};
}

std::vector<Fraction> makeObjective() {
    return {Fraction(30), Fraction(20)};
}

std::vector<LinearProgram::ConstraintSense> makeSenses() {
    return {LinearProgram::ConstraintSense::LessEqual,
            LinearProgram::ConstraintSense::Equal,
            LinearProgram::ConstraintSense::GreaterEqual};
}

} // namespace

TEST(LinearProgramConstructionTests, StoresComponents) {
    LinearProgram program(
        makeMatrix(),
        makeRhs(),
        makeObjective(),
        LinearProgram::ObjectiveSense::Maximize,
        makeSenses());

    EXPECT_EQ(program.rows(), 3u);
    EXPECT_EQ(program.cols(), 2u);
    EXPECT_FALSE(program.empty());
    EXPECT_EQ(program.objectiveSense(), LinearProgram::ObjectiveSense::Maximize);

    const auto& matrix = program.constraintMatrix();
    EXPECT_TRUE(matrix(0, 0) == Fraction(2));
    EXPECT_TRUE(matrix(1, 1) == Fraction(1));
    EXPECT_TRUE(matrix(2, 1) == Fraction(0));

    const auto& rhs = program.rightHandSide();
    ASSERT_EQ(rhs.size(), 3u);
    EXPECT_TRUE(rhs[0] == Fraction(100));
    EXPECT_TRUE(rhs[2] == Fraction(20));

    const auto& objective = program.objectiveCoefficients();
    ASSERT_EQ(objective.size(), 2u);
    EXPECT_TRUE(objective[0] == Fraction(30));
    EXPECT_TRUE(objective[1] == Fraction(20));

    const auto& senses = program.constraintSense();
    ASSERT_EQ(senses.size(), 3u);
    EXPECT_EQ(senses[0], LinearProgram::ConstraintSense::LessEqual);
    EXPECT_EQ(senses[1], LinearProgram::ConstraintSense::Equal);
    EXPECT_EQ(senses[2], LinearProgram::ConstraintSense::GreaterEqual);
}

TEST(LinearProgramValidationTests, AllowsEmptyConstraintSense) {
    std::vector<LinearProgram::ConstraintSense> empty;
    EXPECT_NO_THROW(LinearProgram(
        makeMatrix(),
        makeRhs(),
        makeObjective(),
        LinearProgram::ObjectiveSense::Minimize,
        empty));
}

TEST(LinearProgramValidationTests, RejectsEmptyMatrix) {
    LinearProgram program;
    EXPECT_THROW(program.validate(), std::invalid_argument);
}

TEST(LinearProgramValidationTests, RejectsMismatchedVectorSizes) {
    EXPECT_THROW(
        LinearProgram(LinearProgram::Matrix(2, 2, Fraction(1)),
                      {Fraction(1)},
                      makeObjective(),
                      LinearProgram::ObjectiveSense::Maximize,
                      makeSenses()),
        std::invalid_argument);

    EXPECT_THROW(
        LinearProgram(LinearProgram::Matrix(2, 2, Fraction(1)),
                      {Fraction(1), Fraction(2)},
                      {Fraction(3)},
                      LinearProgram::ObjectiveSense::Maximize,
                      makeSenses()),
        std::invalid_argument);

    EXPECT_THROW(
        LinearProgram(LinearProgram::Matrix(2, 2, Fraction(1)),
                      {Fraction(1), Fraction(2)},
                      {Fraction(3), Fraction(4)},
                      LinearProgram::ObjectiveSense::Maximize,
                      {LinearProgram::ConstraintSense::Equal}),
        std::invalid_argument);
}
