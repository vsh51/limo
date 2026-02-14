#pragma once

#include <cstddef>
#include <vector>

#include "limo/core/LinearProgram.hpp"

namespace limo::basis {

/**
 * @brief Builds an initial basis using the artificial (two-phase) method.
 *
 * The artificial (two-phase) method constructs an auxiliary problem to obtain
 * a feasible starting basis for the simplex algorithm.
 *
 * How it works:
 *
 * 1. **Add slack variables** for each "less than or equal" (<=) constraint.
 *    These variables represent unused resources and can serve as a basis.
 *
 * 2. **Add surplus variables** for each "greater than or equal" (>=) constraint.
 *    These variables represent the excess over the minimum requirement.
 *
 * 3. **Add artificial variables** when a basic variable is missing.
 *    This happens for:
 *    - Equality (=) constraints
 *    - Greater than or equal (>=) constraints (after adding surplus variables)
 *
 * 4. **Create phase I objective** by minimizing the sum of artificial variables.
 *    This drives artificial variables to zero if the original problem is feasible.
 *
 * 5. **Use the resulting basis** as a starting point for phase II (the real problem).
 *
 * If any artificial variable remains positive after phase I, the original
 * problem has no feasible solution.
 *
 * @author Yelyzaveta Chepurna
 */
class ArtificialBasisFinder {
public:
    using LinearProgram = limo::core::LinearProgram;
    using value_type = LinearProgram::value_type;
    using Matrix = LinearProgram::Matrix;
    using ConstraintSense = LinearProgram::ConstraintSense;
    using ObjectiveSense = LinearProgram::ObjectiveSense;

    struct Result {
        LinearProgram augmented;
        std::vector<std::size_t> basisColumns;
        std::vector<std::size_t> slackColumns;
        std::vector<std::size_t> surplusColumns;
        std::vector<std::size_t> artificialColumns;
        std::size_t originalVariableCount = 0;
    };

    ArtificialBasisFinder() = default;
    Result build(const LinearProgram& linearProgram) const;
};

} // namespace limo::basis
