#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "limo/numerics/Fraction.hpp"
#include "limo/numerics/Matrix.hpp"

namespace limo::io {

/**
 * @brief Snapshot of one simplex iteration.
 *
 * Captures the full tableau state after a pivot so that the solver's
 * progress can be replayed or serialized for a web frontend.
 *
 * @author Volodymyr Shpyrka
 */
struct SimplexTableau {
    std::size_t iterationNumber;
    limo::numerics::Matrix<limo::numerics::fraction::Fraction> tableau;
    std::vector<std::size_t> basisVariables;
    std::optional<std::size_t> pivotRow;
    std::optional<std::size_t> pivotColumn;
    limo::numerics::fraction::Fraction objectiveValue;
    std::vector<std::optional<limo::numerics::fraction::Fraction>> ratios;
};

} // namespace limo::io
