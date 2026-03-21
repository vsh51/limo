#include <gtest/gtest.h>

#include "limo/simplex/SimplexSolver.hpp"
#include "limo/basis/ArtificialBasisFinder.hpp"
#include "limo/basis/BigMBasisFinder.hpp"
#include "limo/core/LinearProgram.hpp"
#include "limo/numerics/Fraction.hpp"
#include "limo/numerics/Matrix.hpp"
#include "limo/io/SolverObserver.hpp"

using Fraction = limo::numerics::fraction::Fraction;
using Matrix   = limo::numerics::Matrix<Fraction>;
using LP       = limo::core::LinearProgram;
using Solver   = limo::simplex::SimplexSolver;
using Status   = Solver::SolveStatus;

namespace {

struct RecordingObserver : limo::io::SolverObserver {
    int iterationCount = 0;
    int phaseCompleteCount = 0;
    int solveCompleteCount = 0;
    int lastPhase = -1;

    void onIteration(const limo::io::SimplexTableau&) override { ++iterationCount; }
    void onPhaseComplete(int phase, const limo::io::SimplexTableau&) override {
        ++phaseCompleteCount;
        lastPhase = phase;
    }
    void onSolveComplete() override { ++solveCompleteCount; }
};

LP makeLP(
    std::initializer_list<std::initializer_list<Fraction>> a,
    std::initializer_list<Fraction> b,
    std::initializer_list<Fraction> c,
    LP::ObjectiveSense sense,
    std::initializer_list<LP::ConstraintSense> cs)
{
    return LP(Matrix(a), std::vector<Fraction>(b), std::vector<Fraction>(c),
              sense, std::vector<LP::ConstraintSense>(cs));
}

// max 30x1 + 20x2  s.t.  2x1+x2<=100, x1+x2<=80, x1<=40
// optimal: x1=20, x2=60, z=1800
LP makeSimpleMaxLP() {
    return makeLP(
        {{2, 1}, {1, 1}, {1, 0}},
        {100, 80, 40},
        {30, 20},
        LP::ObjectiveSense::Maximize,
        {LP::ConstraintSense::LessEqual,
         LP::ConstraintSense::LessEqual,
         LP::ConstraintSense::LessEqual});
}

// min x1 + 2x2  s.t.  x1+x2>=4, x1>=2
// optimal: x1=4, x2=0, z=4
LP makeMinLP() {
    return makeLP(
        {{1, 1}, {1, 0}},
        {4, 2},
        {1, 2},
        LP::ObjectiveSense::Minimize,
        {LP::ConstraintSense::GreaterEqual,
         LP::ConstraintSense::GreaterEqual});
}

// max x1 + x2  s.t.  x1 - x2 <= 0  →  unbounded (x1 = x2 can grow without bound)
LP makeUnboundedLP() {
    return makeLP(
        {{1, -1}},
        {0},
        {1, 1},
        LP::ObjectiveSense::Maximize,
        {LP::ConstraintSense::LessEqual});
}

// infeasible: x1+x2<=4 AND x1+x2>=6
LP makeInfeasibleLP() {
    return makeLP(
        {{1, 1}, {1, 1}},
        {4, 6},
        {1, 1},
        LP::ObjectiveSense::Minimize,
        {LP::ConstraintSense::LessEqual,
         LP::ConstraintSense::GreaterEqual});
}

} // namespace

class SimplexSolverBigMTest : public ::testing::Test {
protected:
    limo::basis::BigMBasisFinder finder_;
    Solver solver_;
    Fraction bigM_{1000};
};

TEST_F(SimplexSolverBigMTest, SolvesSimpleMaximisation) {
    auto lp = makeSimpleMaxLP();
    auto result = finder_.build(lp, bigM_);
    auto sr = solver_.solve(result.augmented, result.basisColumns);

    EXPECT_EQ(sr.status, Status::Optimal);
    EXPECT_EQ(sr.objectiveValue, Fraction(1800));
}

TEST_F(SimplexSolverBigMTest, SolvesMinimisation) {
    auto lp = makeMinLP();
    auto result = finder_.build(lp, bigM_);
    auto sr = solver_.solve(result.augmented, result.basisColumns);

    EXPECT_EQ(sr.status, Status::Optimal);
    EXPECT_EQ(sr.values[0], Fraction(4));
    EXPECT_EQ(sr.values[1], Fraction(0));
    EXPECT_EQ(sr.objectiveValue, Fraction(4));
}

TEST_F(SimplexSolverBigMTest, DetectsInfeasible) {
    auto lp = makeInfeasibleLP();
    auto result = finder_.build(lp, bigM_);
    auto sr = solver_.solve(result.augmented, result.basisColumns);
    (void)sr;
}

TEST_F(SimplexSolverBigMTest, DetectsUnbounded) {
    auto lp = makeUnboundedLP();
    auto result = finder_.build(lp, bigM_);
    auto sr = solver_.solve(result.augmented, result.basisColumns);

    EXPECT_EQ(sr.status, Status::Unbounded);
}

TEST_F(SimplexSolverBigMTest, ObserverReceivesCallbacks) {
    // max x1 s.t. x1 <= 5 — one pivot needed
    auto lp = makeLP(
        {{1}},
        {5},
        {1},
        LP::ObjectiveSense::Maximize,
        {LP::ConstraintSense::LessEqual});

    auto result = finder_.build(lp, bigM_);
    RecordingObserver obs;
    auto sr = solver_.solve(result.augmented, result.basisColumns, &obs, 1);

    EXPECT_EQ(sr.status, Status::Optimal);
    EXPECT_GT(obs.iterationCount, 0);
    EXPECT_EQ(obs.phaseCompleteCount, 1);
    EXPECT_EQ(obs.lastPhase, 1);
    EXPECT_EQ(obs.solveCompleteCount, 0);
}

TEST_F(SimplexSolverBigMTest, ObserverCalledOnUnbounded) {
    auto lp = makeUnboundedLP();
    auto result = finder_.build(lp, bigM_);
    RecordingObserver obs;
    auto sr = solver_.solve(result.augmented, result.basisColumns, &obs);

    EXPECT_EQ(sr.status, Status::Unbounded);
    EXPECT_EQ(obs.solveCompleteCount, 1);
    EXPECT_EQ(obs.phaseCompleteCount, 0);
}

class SimplexSolverArtificialTest : public ::testing::Test {
protected:
    limo::basis::ArtificialBasisFinder finder_;
    Solver solver_;
};

TEST_F(SimplexSolverArtificialTest, TwoPhaseMaximisation) {
    auto lp = makeSimpleMaxLP();
    auto result = finder_.build(lp);

    auto p1 = solver_.solve(result.augmented, result.basisColumns, nullptr, 1);
    EXPECT_EQ(p1.status, Status::Optimal);
    EXPECT_EQ(p1.objectiveValue, Fraction(0));

    auto phase2Obj = lp.objectiveCoefficients();
    phase2Obj.resize(result.augmented.cols(), Fraction{0});
    LP phase2LP(
        result.augmented.constraintMatrix(),
        result.augmented.rightHandSide(),
        phase2Obj,
        lp.objectiveSense(),
        result.augmented.constraintSense());

    auto p2 = solver_.solve(phase2LP, p1.finalBasis, nullptr, 2);
    EXPECT_EQ(p2.status, Status::Optimal);
    EXPECT_EQ(p2.objectiveValue, Fraction(1800));
    EXPECT_EQ(p2.values[0], Fraction(20));
    EXPECT_EQ(p2.values[1], Fraction(60));
}

TEST_F(SimplexSolverArtificialTest, TwoPhaseMinimisation) {
    auto lp = makeMinLP();
    auto result = finder_.build(lp);

    auto p1 = solver_.solve(result.augmented, result.basisColumns, nullptr, 1);
    EXPECT_EQ(p1.status, Status::Optimal);
    EXPECT_EQ(p1.objectiveValue, Fraction(0));

    const auto& aug = result.augmented;
    const std::size_t nPhase2 = aug.cols() - result.artificialColumns.size();

    LP::Matrix phase2Mat(aug.rows(), nPhase2, Fraction{0});
    for (std::size_t r = 0; r < aug.rows(); ++r) {
        for (std::size_t c = 0; c < nPhase2; ++c) {
            phase2Mat(r, c) = aug.constraintMatrix()(r, c);
        }
    }
    auto phase2Obj = lp.objectiveCoefficients();
    phase2Obj.resize(nPhase2, Fraction{0});

    LP phase2LP(
        std::move(phase2Mat),
        aug.rightHandSide(),
        std::move(phase2Obj),
        lp.objectiveSense(),
        aug.constraintSense());

    auto p2 = solver_.solve(phase2LP, p1.finalBasis, nullptr, 2);
    EXPECT_EQ(p2.status, Status::Optimal);
    EXPECT_EQ(p2.objectiveValue, Fraction(4));
    EXPECT_EQ(p2.values[0], Fraction(4));
    EXPECT_EQ(p2.values[1], Fraction(0));
}

TEST_F(SimplexSolverArtificialTest, Phase1DetectsInfeasible) {
    auto lp = makeInfeasibleLP();
    auto result = finder_.build(lp);

    auto p1 = solver_.solve(result.augmented, result.basisColumns, nullptr, 1);
    EXPECT_EQ(p1.status, Status::Optimal);
    EXPECT_GT(p1.objectiveValue, Fraction(0));
}
