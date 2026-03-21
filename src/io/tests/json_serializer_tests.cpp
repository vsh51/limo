#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "limo/io/JsonSerializer.hpp"
#include "limo/io/SimplexTableau.hpp"
#include "limo/numerics/Fraction.hpp"
#include "limo/numerics/Matrix.hpp"
#include "limo/core/LinearProgram.hpp"
#include "limo/core/Result.hpp"
#include "limo/core/Solution.hpp"

using limo::numerics::fraction::Fraction;
using limo::numerics::Matrix;
using limo::core::LinearProgram;
using limo::core::Result;
using limo::core::Solution;
using limo::io::SimplexTableau;
using nlohmann::json;

// --- Fraction ---

TEST(FractionSerializer, ToJson) {
    Fraction f(3, 4);
    json j = f;
    EXPECT_EQ(j["num"], 3);
    EXPECT_EQ(j["den"], 4);
}

TEST(FractionSerializer, FromJson) {
    json j = {{"num", 7}, {"den", 2}};
    auto f = j.get<Fraction>();
    EXPECT_EQ(f.getNumerator(), 7);
    EXPECT_EQ(f.getDenominator(), 2);
}

TEST(FractionSerializer, RoundTrip) {
    Fraction original(5, 3);
    json j = original;
    auto restored = j.get<Fraction>();
    EXPECT_EQ(original, restored);
}

TEST(FractionSerializer, ZeroDenominatorThrows) {
    json j = {{"num", 1}, {"den", 0}};
    EXPECT_THROW(j.get<Fraction>(), std::invalid_argument);
}

TEST(FractionSerializer, NegativeFraction) {
    Fraction f(-3, 4);
    json j = f;
    auto restored = j.get<Fraction>();
    EXPECT_EQ(f, restored);
}

// --- Matrix ---

TEST(MatrixSerializer, ToJson) {
    Matrix<Fraction> m(2, 3);
    m(0, 0) = Fraction{1}; m(0, 1) = Fraction{2}; m(0, 2) = Fraction{3};
    m(1, 0) = Fraction{4}; m(1, 1) = Fraction{5}; m(1, 2) = Fraction{6};

    json j = m;
    EXPECT_EQ(j["rows"], 2);
    EXPECT_EQ(j["cols"], 3);
    EXPECT_EQ(j["data"].size(), 2);
    EXPECT_EQ(j["data"][0].size(), 3);
}

TEST(MatrixSerializer, RoundTrip) {
    Matrix<Fraction> original(2, 2);
    original(0, 0) = Fraction{1, 2};
    original(0, 1) = Fraction{3, 4};
    original(1, 0) = Fraction{5, 6};
    original(1, 1) = Fraction{7, 8};

    json j = original;
    auto restored = j.get<Matrix<Fraction>>();

    EXPECT_EQ(restored.rows(), 2);
    EXPECT_EQ(restored.cols(), 2);
    EXPECT_EQ(restored(0, 0), Fraction(1, 2));
    EXPECT_EQ(restored(0, 1), Fraction(3, 4));
    EXPECT_EQ(restored(1, 0), Fraction(5, 6));
    EXPECT_EQ(restored(1, 1), Fraction(7, 8));
}

TEST(MatrixSerializer, EmptyMatrix) {
    Matrix<Fraction> m;
    json j = m;
    EXPECT_EQ(j["rows"], 0);
    EXPECT_EQ(j["cols"], 0);

    auto restored = j.get<Matrix<Fraction>>();
    EXPECT_EQ(restored.rows(), 0);
    EXPECT_EQ(restored.cols(), 0);
}

// --- ObjectiveSense ---

TEST(ObjectiveSenseSerializer, RoundTrip) {
    auto max = LinearProgram::ObjectiveSense::Maximize;
    auto min = LinearProgram::ObjectiveSense::Minimize;

    json jMax = max;
    json jMin = min;

    EXPECT_EQ(jMax, "maximize");
    EXPECT_EQ(jMin, "minimize");

    EXPECT_EQ(jMax.get<LinearProgram::ObjectiveSense>(), max);
    EXPECT_EQ(jMin.get<LinearProgram::ObjectiveSense>(), min);
}

TEST(ObjectiveSenseSerializer, InvalidThrows) {
    json j = "unknown";
    EXPECT_THROW(j.get<LinearProgram::ObjectiveSense>(), std::invalid_argument);
}

// --- ConstraintSense ---

TEST(ConstraintSenseSerializer, RoundTrip) {
    auto le = LinearProgram::ConstraintSense::LessEqual;
    auto eq = LinearProgram::ConstraintSense::Equal;
    auto ge = LinearProgram::ConstraintSense::GreaterEqual;

    json jLe = le; json jEq = eq; json jGe = ge;

    EXPECT_EQ(jLe, "<=");
    EXPECT_EQ(jEq, "=");
    EXPECT_EQ(jGe, ">=");

    EXPECT_EQ(jLe.get<LinearProgram::ConstraintSense>(), le);
    EXPECT_EQ(jEq.get<LinearProgram::ConstraintSense>(), eq);
    EXPECT_EQ(jGe.get<LinearProgram::ConstraintSense>(), ge);
}

TEST(ConstraintSenseSerializer, InvalidThrows) {
    json j = "!=";
    EXPECT_THROW(j.get<LinearProgram::ConstraintSense>(), std::invalid_argument);
}

// --- LinearProgram ---

TEST(LinearProgramSerializer, RoundTrip) {
    LinearProgram::Matrix matrix(2, 2);
    matrix(0, 0) = Fraction{2}; matrix(0, 1) = Fraction{1};
    matrix(1, 0) = Fraction{1}; matrix(1, 1) = Fraction{1};

    std::vector<Fraction> rhs = {Fraction{100}, Fraction{80}};
    std::vector<Fraction> obj = {Fraction{30}, Fraction{20}};
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::LessEqual,
        LinearProgram::ConstraintSense::Equal
    };

    LinearProgram original(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, senses);

    json j = original;
    auto restored = j.get<LinearProgram>();

    EXPECT_EQ(restored.rows(), 2);
    EXPECT_EQ(restored.cols(), 2);
    EXPECT_EQ(restored.objectiveSense(), LinearProgram::ObjectiveSense::Maximize);
    EXPECT_EQ(restored.rightHandSide()[0], Fraction(100));
    EXPECT_EQ(restored.rightHandSide()[1], Fraction(80));
    EXPECT_EQ(restored.objectiveCoefficients()[0], Fraction(30));
    EXPECT_EQ(restored.objectiveCoefficients()[1], Fraction(20));
    EXPECT_EQ(restored.constraintSense()[0], LinearProgram::ConstraintSense::LessEqual);
    EXPECT_EQ(restored.constraintSense()[1], LinearProgram::ConstraintSense::Equal);
    EXPECT_EQ(restored.constraintMatrix()(0, 0), Fraction(2));
    EXPECT_EQ(restored.constraintMatrix()(1, 1), Fraction(1));
}

// --- Result ---

TEST(ResultSerializer, RoundTrip) {
    LinearProgram::Matrix matrix(1, 3);
    matrix(0, 0) = Fraction{1}; matrix(0, 1) = Fraction{1}; matrix(0, 2) = Fraction{1};

    std::vector<Fraction> rhs = {Fraction{10}};
    std::vector<Fraction> obj = {Fraction{1}, Fraction{0}, Fraction{0}};
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::LessEqual
    };

    LinearProgram augmented(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, senses);

    Result original;
    original.originalVariableCount = 1;
    original.basisColumns = {2};
    original.slackColumns = {1};
    original.surplusColumns = {};
    original.artificialColumns = {2};
    original.augmented = augmented;
    original.bigM = Fraction{1000};

    json j = original;
    auto restored = j.get<Result>();

    EXPECT_EQ(restored.originalVariableCount, 1);
    EXPECT_EQ(restored.basisColumns, original.basisColumns);
    EXPECT_EQ(restored.slackColumns, original.slackColumns);
    EXPECT_EQ(restored.surplusColumns, original.surplusColumns);
    EXPECT_EQ(restored.artificialColumns, original.artificialColumns);
    EXPECT_EQ(restored.bigM, Fraction(1000));
    EXPECT_EQ(restored.augmented.rows(), 1);
    EXPECT_EQ(restored.augmented.cols(), 3);
}

// --- SimplexTableau ---

TEST(SimplexTableauSerializer, RoundTrip) {
    SimplexTableau original;
    original.iterationNumber = 2;
    original.tableau = Matrix<Fraction>(1, 2);
    original.tableau(0, 0) = Fraction{1};
    original.tableau(0, 1) = Fraction{2};
    original.basisVariables = {0};
    original.pivotRow = 0;
    original.pivotColumn = 1;
    original.objectiveValue = Fraction{42};
    original.ratios = {Fraction{5}, std::nullopt};

    json j = original;
    auto restored = j.get<SimplexTableau>();

    EXPECT_EQ(restored.iterationNumber, 2);
    EXPECT_EQ(restored.tableau(0, 0), Fraction(1));
    EXPECT_EQ(restored.tableau(0, 1), Fraction(2));
    EXPECT_EQ(restored.basisVariables, original.basisVariables);
    EXPECT_EQ(restored.pivotRow, std::optional<std::size_t>(0));
    EXPECT_EQ(restored.pivotColumn, std::optional<std::size_t>(1));
    EXPECT_EQ(restored.objectiveValue, Fraction(42));
    ASSERT_EQ(restored.ratios.size(), 2);
    EXPECT_EQ(restored.ratios[0], Fraction(5));
    EXPECT_EQ(restored.ratios[1], std::nullopt);
}

TEST(SimplexTableauSerializer, NullOptionals) {
    SimplexTableau original;
    original.iterationNumber = 0;
    original.tableau = Matrix<Fraction>(1, 1);
    original.tableau(0, 0) = Fraction{1};
    original.basisVariables = {0};
    original.pivotRow = std::nullopt;
    original.pivotColumn = std::nullopt;
    original.objectiveValue = Fraction{0};
    original.ratios = {};

    json j = original;
    EXPECT_TRUE(j["pivotRow"].is_null());
    EXPECT_TRUE(j["pivotColumn"].is_null());

    auto restored = j.get<SimplexTableau>();
    EXPECT_EQ(restored.pivotRow, std::nullopt);
    EXPECT_EQ(restored.pivotColumn, std::nullopt);
}

// --- Solution ---

TEST(SolutionSerializer, ToJsonOptimal) {
    Solution s;
    s.status = Solution::Status::Optimal;
    s.objectiveValue = Fraction{42};
    s.variableValues = {Fraction{3}, Fraction{7}};

    json j = s;
    EXPECT_EQ(j["status"], "optimal");
    EXPECT_EQ(j["objectiveValue"]["num"], 42);
    EXPECT_EQ(j["variableValues"].size(), 2u);
}

TEST(SolutionSerializer, ToJsonInfeasible) {
    Solution s;
    s.status = Solution::Status::Infeasible;
    json j = s;
    EXPECT_EQ(j["status"], "infeasible");
}

TEST(SolutionSerializer, ToJsonUnbounded) {
    Solution s;
    s.status = Solution::Status::Unbounded;
    json j = s;
    EXPECT_EQ(j["status"], "unbounded");
}

TEST(SolutionSerializer, RoundTripOptimal) {
    Solution original;
    original.status = Solution::Status::Optimal;
    original.objectiveValue = Fraction{10, 3};
    original.variableValues = {Fraction{1}, Fraction{2}};

    json j = original;
    auto restored = j.get<Solution>();

    EXPECT_EQ(restored.status, Solution::Status::Optimal);
    EXPECT_EQ(restored.objectiveValue, Fraction(10, 3));
    ASSERT_EQ(restored.variableValues.size(), 2u);
    EXPECT_EQ(restored.variableValues[0], Fraction(1));
    EXPECT_EQ(restored.variableValues[1], Fraction(2));
}

TEST(SolutionSerializer, RoundTripInfeasible) {
    Solution original;
    original.status = Solution::Status::Infeasible;
    json j = original;
    auto restored = j.get<Solution>();
    EXPECT_EQ(restored.status, Solution::Status::Infeasible);
}

TEST(SolutionSerializer, RoundTripUnbounded) {
    Solution original;
    original.status = Solution::Status::Unbounded;
    json j = original;
    auto restored = j.get<Solution>();
    EXPECT_EQ(restored.status, Solution::Status::Unbounded);
}

TEST(SolutionSerializer, InvalidStatusThrows) {
    json j = {{"status", "broken"}, {"objectiveValue", {{"num", 0}, {"den", 1}}}, {"variableValues", json::array()}};
    EXPECT_THROW(j.get<Solution>(), std::invalid_argument);
}
