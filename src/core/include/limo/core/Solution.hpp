#pragma once

#include <vector>

#include "limo/numerics/Fraction.hpp"

namespace limo::core {

/**
 * @brief Result of solving a linear programming problem.
 * @author Volodymyr Shpyrka
 */
struct Solution {
    enum class Status { Optimal, Infeasible, Unbounded };

    Status status{Status::Infeasible};
    std::vector<limo::numerics::fraction::Fraction> variableValues;
    limo::numerics::fraction::Fraction objectiveValue{};
};

} // namespace limo::core
