#pragma once

#include <vector>

#include "limo/io/SolverObserver.hpp"

namespace limo::io {

/**
 * @brief Concrete observer that stores all iteration snapshots.
 *
 * Collects every SimplexTableau emitted by a solver so the full
 * history can later be serialized into the JSON response.
 *
 * @author Volodymyr Shpyrka
 */
class IterationCollector : public SolverObserver {
public:
    void onIteration(const SimplexTableau& tableau) override;
    void onPhaseComplete(int phase, const SimplexTableau& tableau) override;
    void onSolveComplete() override;

    const std::vector<SimplexTableau>& iterations() const;

private:
    std::vector<SimplexTableau> iterations_;
};

} // namespace limo::io
