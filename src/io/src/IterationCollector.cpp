#include "limo/io/IterationCollector.hpp"

namespace limo::io {

void IterationCollector::onIteration(const SimplexTableau& tableau) {
    iterations_.push_back(tableau);
}

void IterationCollector::onPhaseComplete(int /*phase*/, const SimplexTableau& tableau) {
    iterations_.push_back(tableau);
}

void IterationCollector::onSolveComplete() {
    // No action needed; iterations are already stored.
}

const std::vector<SimplexTableau>& IterationCollector::iterations() const {
    return iterations_;
}

} // namespace limo::io
