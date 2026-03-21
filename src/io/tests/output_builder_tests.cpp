#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "limo/io/OutputBuilder.hpp"
#include "limo/io/IterationCollector.hpp"
#include "limo/io/JsonSerializer.hpp"
#include "limo/io/SimplexTableau.hpp"
#include "limo/numerics/Fraction.hpp"
#include "limo/core/LinearProgram.hpp"
#include "limo/core/Result.hpp"
#include "limo/core/Solution.hpp"

using limo::io::IterationCollector;
using limo::io::OutputBuilder;
using limo::io::SimplexTableau;
using limo::numerics::fraction::Fraction;
using limo::numerics::Matrix;
using limo::core::LinearProgram;
using limo::core::Result;
using limo::core::Solution;
using nlohmann::json;

namespace {

LinearProgram makeSimpleLP() {
    LinearProgram::Matrix matrix(1, 2);
    matrix(0, 0) = Fraction{2}; matrix(0, 1) = Fraction{1};

    std::vector<Fraction> rhs = {Fraction{100}};
    std::vector<Fraction> obj = {Fraction{30}, Fraction{20}};
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::LessEqual
    };

    return LinearProgram(matrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, senses);
}

Result makeSimpleResult(const LinearProgram& lp) {
    LinearProgram::Matrix augMatrix(1, 3);
    augMatrix(0, 0) = Fraction{2}; augMatrix(0, 1) = Fraction{1}; augMatrix(0, 2) = Fraction{1};

    std::vector<Fraction> rhs = {Fraction{100}};
    std::vector<Fraction> obj = {Fraction{30}, Fraction{20}, Fraction{0}};
    std::vector<LinearProgram::ConstraintSense> senses = {
        LinearProgram::ConstraintSense::LessEqual
    };

    LinearProgram augmented(augMatrix, rhs, obj, LinearProgram::ObjectiveSense::Maximize, senses);

    Result r;
    r.originalVariableCount = 2;
    r.basisColumns = {2};
    r.slackColumns = {2};
    r.surplusColumns = {};
    r.artificialColumns = {};
    r.augmented = augmented;
    r.bigM = Fraction{0};
    return r;
}

} // anonymous namespace

TEST(OutputBuilder, StructureWithEmptyIterations) {
    auto lp = makeSimpleLP();
    auto result = makeSimpleResult(lp);
    std::vector<SimplexTableau> iterations;
    Solution solution;
    solution.status = Solution::Status::Optimal;
    solution.objectiveValue = Fraction{1800};
    solution.variableValues = {Fraction{30}, Fraction{20}};

    auto output = OutputBuilder::build(lp, result, "big-m", iterations, solution);

    EXPECT_EQ(output["status"], "ok");
    EXPECT_TRUE(output["error"].is_null());

    ASSERT_TRUE(output.contains("input"));
    EXPECT_EQ(output["input"]["variableCount"], 2);
    EXPECT_EQ(output["input"]["constraintCount"], 1);
    EXPECT_EQ(output["input"]["objectiveSense"], "maximize");

    ASSERT_TRUE(output.contains("basisFinding"));
    EXPECT_EQ(output["basisFinding"]["method"], "big-m");
    ASSERT_TRUE(output["basisFinding"].contains("result"));

    EXPECT_TRUE(output.contains("iterations"));
    EXPECT_TRUE(output["iterations"].is_array());
    EXPECT_TRUE(output["iterations"].empty());

    ASSERT_TRUE(output.contains("solution"));
    EXPECT_EQ(output["solution"]["status"], "optimal");
}

TEST(OutputBuilder, ResultContainsCorrectData) {
    auto lp = makeSimpleLP();
    auto result = makeSimpleResult(lp);
    std::vector<SimplexTableau> iterations;
    Solution solution;

    auto output = OutputBuilder::build(lp, result, "big-m", iterations, solution);

    auto& bfResult = output["basisFinding"]["result"];
    EXPECT_EQ(bfResult["originalVariableCount"], 2);
    EXPECT_EQ(bfResult["basisColumns"], json::array({2}));
    EXPECT_EQ(bfResult["slackColumns"], json::array({2}));
    EXPECT_TRUE(bfResult["surplusColumns"].empty());
    EXPECT_TRUE(bfResult["artificialColumns"].empty());
}

TEST(OutputBuilder, WithIterations) {
    auto lp = makeSimpleLP();
    auto result = makeSimpleResult(lp);

    SimplexTableau t;
    t.iterationNumber = 1;
    t.tableau = Matrix<Fraction>(1, 1);
    t.tableau(0, 0) = Fraction{1};
    t.basisVariables = {0};
    t.pivotRow = 0;
    t.pivotColumn = 0;
    t.objectiveValue = Fraction{100};
    t.ratios = {Fraction{10}};

    std::vector<SimplexTableau> iterations = {t};

    auto output = OutputBuilder::build(lp, result, "artificial", iterations, Solution{});

    EXPECT_EQ(output["iterations"].size(), 1);
    EXPECT_EQ(output["iterations"][0]["iterationNumber"], 1);
    EXPECT_EQ(output["basisFinding"]["method"], "artificial");
}

TEST(OutputBuilder, MinimizeSense) {
    LinearProgram::Matrix matrix(1, 1);
    matrix(0, 0) = Fraction{1};

    LinearProgram lp(
        matrix,
        {Fraction{5}},
        {Fraction{1}},
        LinearProgram::ObjectiveSense::Minimize,
        {LinearProgram::ConstraintSense::GreaterEqual}
    );

    Result r;
    r.originalVariableCount = 1;
    r.basisColumns = {2};
    r.slackColumns = {};
    r.surplusColumns = {1};
    r.artificialColumns = {2};
    r.augmented = lp;
    r.bigM = Fraction{1000};

    auto output = OutputBuilder::build(lp, r, "big-m", {}, Solution{});
    EXPECT_EQ(output["input"]["objectiveSense"], "minimize");
}

TEST(OutputBuilder, BuildError) {
    auto output = OutputBuilder::buildError("something went wrong");

    EXPECT_EQ(output["status"], "error");
    EXPECT_EQ(output["error"], "something went wrong");
}

// --- IterationCollector ---

namespace {

SimplexTableau makeTableau(std::size_t iteration) {
    SimplexTableau t;
    t.iterationNumber = iteration;
    t.tableau = Matrix<Fraction>(1, 1);
    t.tableau(0, 0) = Fraction{1};
    t.basisVariables = {0};
    t.pivotRow = std::nullopt;
    t.pivotColumn = std::nullopt;
    t.objectiveValue = Fraction{0};
    t.ratios = {};
    return t;
}

} // anonymous namespace

TEST(IterationCollector, CollectsIterations) {
    auto collector = std::make_unique<IterationCollector>();

    collector->onIteration(makeTableau(1));
    collector->onIteration(makeTableau(2));

    EXPECT_EQ(collector->iterations().size(), 2);
    EXPECT_EQ(collector->iterations()[0].iterationNumber, 1);
    EXPECT_EQ(collector->iterations()[1].iterationNumber, 2);
}

TEST(IterationCollector, CollectsPhaseComplete) {
    IterationCollector collector;

    collector.onPhaseComplete(1, makeTableau(0));

    EXPECT_EQ(collector.iterations().size(), 1);
    EXPECT_EQ(collector.iterations()[0].iterationNumber, 0);
}

TEST(IterationCollector, SolveCompleteIsNoOp) {
    IterationCollector collector;

    collector.onSolveComplete();

    EXPECT_TRUE(collector.iterations().empty());
}

TEST(IterationCollector, DestructorViaBasePointer) {
    std::unique_ptr<limo::io::SolverObserver> observer =
        std::make_unique<IterationCollector>();
    observer->onIteration(makeTableau(1));
    observer.reset();
}
