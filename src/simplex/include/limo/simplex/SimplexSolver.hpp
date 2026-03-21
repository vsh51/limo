#pragma once

#include <cstddef>
#include <vector>

#include "limo/core/LinearProgram.hpp"
#include "limo/core/Solution.hpp"
#include "limo/io/SolverObserver.hpp"

namespace limo::simplex {

/**
 * @brief Primal simplex solver for standard-form LPs.
 *
 * Accepts an LP in equality form together with an initial basic feasible
 * solution and drives it to optimality.  Call once per phase; two-phase
 * logic lives in the caller.
 *
 * @author Volodymyr Shpyrka
 */
class SimplexSolver {
public:
    using value_type = limo::core::LinearProgram::value_type;

    enum class SolveStatus { Optimal, Infeasible, Unbounded };

    struct SolveResult {
        SolveStatus status;
        std::vector<value_type> values;
        value_type objectiveValue;
        std::vector<std::size_t> finalBasis;
    };

    /**
     * @brief Run the primal simplex from a given initial basis.
     *
     * @param lp           LP in equality form with non-negative RHS.
     * @param initialBasis One basis column per constraint row.
     * @param observer     Optional observer for iteration snapshots.
     * @param phaseNumber  Passed to SolverObserver::onPhaseComplete.
     */
    SolveResult solve(
        const limo::core::LinearProgram& lp,
        std::vector<std::size_t> initialBasis,
        limo::io::SolverObserver* observer = nullptr,
        int phaseNumber = 1
    );
};

} // namespace limo::simplex
