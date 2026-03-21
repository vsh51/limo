#pragma once

#include "limo/io/SimplexTableau.hpp"

namespace limo::io {

/**
 * @brief Abstract observer interface for simplex solver events.
 *
 * Implementations receive callbacks after each pivot, at phase boundaries
 * (two-phase method), and when the solver finishes. This keeps observation
 * concerns decoupled from the solver itself.
 *
 * @author Volodymyr Shpyrka
 */
class SolverObserver {
public:
    virtual ~SolverObserver() = default;

    /** @brief Called after each pivot operation. */
    virtual void onIteration(const SimplexTableau& tableau) = 0;

    /** @brief Called when a phase completes (for two-phase method). */
    virtual void onPhaseComplete(int phase, const SimplexTableau& tableau) = 0;

    /** @brief Called when the solver finishes. */
    virtual void onSolveComplete() = 0;
};

} // namespace limo::io
