#include <iostream>
#include <iterator>
#include <string>

#include <nlohmann/json.hpp>

#include "limo/io/InputParser.hpp"
#include "limo/io/OutputBuilder.hpp"
#include "limo/io/IterationCollector.hpp"
#include "limo/basis/BigMBasisFinder.hpp"
#include "limo/basis/ArtificialBasisFinder.hpp"
#include "limo/simplex/SimplexSolver.hpp"

using Fraction    = limo::numerics::fraction::Fraction;
using LP          = limo::core::LinearProgram;
using Solution    = limo::core::Solution;
using SolveStatus = limo::simplex::SimplexSolver::SolveStatus;

static Solution makeSolution(
    const limo::simplex::SimplexSolver::SolveResult& sr,
    std::size_t originalVarCount)
{
    Solution sol;
    switch (sr.status) {
        case SolveStatus::Optimal:
            sol.status = Solution::Status::Optimal;
            sol.objectiveValue = sr.objectiveValue;
            sol.variableValues.assign(
                sr.values.begin(),
                sr.values.begin() + static_cast<std::ptrdiff_t>(originalVarCount));
            break;
        case SolveStatus::Infeasible:
            sol.status = Solution::Status::Infeasible;
            break;
        case SolveStatus::Unbounded:
            sol.status = Solution::Status::Unbounded;
            break;
    }
    return sol;
}

static Solution solveTwoPhase(
    const LP& originalLP,
    const limo::core::Result& basisResult,
    limo::io::IterationCollector& collector)
{
    limo::simplex::SimplexSolver solver;

    auto phase1 = solver.solve(
        basisResult.augmented,
        basisResult.basisColumns,
        &collector, 1);

    if (phase1.status == SolveStatus::Unbounded || phase1.objectiveValue > Fraction{0}) {
        return {Solution::Status::Infeasible, {}, {}};
    }

    // Phase-2 LP: drop artificial columns (they are the last nArt columns by
    // construction of both basis finders).
    const std::size_t nAug    = basisResult.augmented.cols();
    const std::size_t nPhase2 = nAug - basisResult.artificialColumns.size();
    const std::size_t m       = basisResult.augmented.rows();

    LP::Matrix phase2Mat(m, nPhase2, Fraction{0});
    for (std::size_t r = 0; r < m; ++r) {
        for (std::size_t c = 0; c < nPhase2; ++c) {
            phase2Mat(r, c) = basisResult.augmented.constraintMatrix()(r, c);
        }
    }

    auto phase2Obj = originalLP.objectiveCoefficients();
    phase2Obj.resize(nPhase2, Fraction{0});

    auto phase2Basis = phase1.finalBasis;
    for (std::size_t col : phase2Basis) {
        if (col >= nPhase2) {
            return {Solution::Status::Infeasible, {}, {}};
        }
    }

    LP phase2LP(
        std::move(phase2Mat),
        basisResult.augmented.rightHandSide(),
        std::move(phase2Obj),
        originalLP.objectiveSense(),
        basisResult.augmented.constraintSense());

    auto phase2 = solver.solve(phase2LP, phase2Basis, &collector, 2);
    collector.onSolveComplete();

    return makeSolution(phase2, basisResult.originalVariableCount);
}

static Solution solveBigM(
    const limo::core::Result& basisResult,
    limo::io::IterationCollector& collector)
{
    limo::simplex::SimplexSolver solver;

    auto sr = solver.solve(
        basisResult.augmented,
        basisResult.basisColumns,
        &collector, 1);

    collector.onSolveComplete();

    if (sr.status == SolveStatus::Optimal) {
        for (std::size_t artCol : basisResult.artificialColumns) {
            if (sr.values[artCol] > Fraction{0}) {
                return {Solution::Status::Infeasible, {}, {}};
            }
        }
    }

    return makeSolution(sr, basisResult.originalVariableCount);
}

int main() {
    try {
        std::string input(std::istreambuf_iterator<char>(std::cin), {});
        auto [lp, config] = limo::io::InputParser::parse(input);

        limo::core::Result basisResult;
        if (config.basisMethod == "artificial") {
            limo::basis::ArtificialBasisFinder finder;
            basisResult = finder.build(lp);
        } else {
            limo::basis::BigMBasisFinder finder;
            basisResult = finder.build(lp, config.bigM);
        }

        limo::io::IterationCollector collector;
        Solution solution = (config.basisMethod == "artificial")
            ? solveTwoPhase(lp, basisResult, collector)
            : solveBigM(basisResult, collector);

        auto output = limo::io::OutputBuilder::build(
            lp, basisResult, config.basisMethod, collector.iterations(), solution);
        std::cout << output.dump() << '\n';
        return 0;

    } catch (const std::exception& e) {
        auto output = limo::io::OutputBuilder::buildError(e.what());
        std::cout << output.dump() << '\n';
        return 1;
    }
}
