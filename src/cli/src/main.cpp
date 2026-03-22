#include <iostream>
#include <iterator>
#include <string>

#include <nlohmann/json.hpp>

#include "limo/io/FileParser.hpp"
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

// ── Argument parsing ──────────────────────────────────────────────────────────

struct CliArgs {
    std::string format    = "json";   // json | csv | txt
    bool        parseOnly = false;    // --parse-only: echo parsed LP, skip solver
};

static CliArgs parseArgs(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" && i + 1 < argc) {
            args.format = argv[++i];
        } else if (arg == "--parse-only") {
            args.parseOnly = true;
        }
    }
    if (args.format != "json" && args.format != "csv" && args.format != "txt") {
        throw std::invalid_argument(
            "Unknown format '" + args.format + "'. Use: json, csv, or txt");
    }
    return args;
}

// ── Input parsing ─────────────────────────────────────────────────────────────

static limo::io::ParsedInput parseInput(const std::string& content,
                                         const std::string& format) {
    if (format == "csv") return limo::io::FileParser::parseCSV(content);
    if (format == "txt") return limo::io::FileParser::parseTXT(content);
    return limo::io::InputParser::parse(content);
}

// ── Solver helpers ────────────────────────────────────────────────────────────

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

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    try {
        const CliArgs args = parseArgs(argc, argv);

        const std::string input(std::istreambuf_iterator<char>(std::cin), {});
        auto [lp, config] = parseInput(input, args.format);

        // --parse-only: output the parsed problem and exit without solving
        if (args.parseOnly) {
            auto output = limo::io::OutputBuilder::buildParsedInput(lp, config);
            std::cout << output.dump() << '\n';
            return 0;
        }

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
