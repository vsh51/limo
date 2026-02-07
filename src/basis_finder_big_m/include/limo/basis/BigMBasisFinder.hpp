#pragma once

#include <cstddef>
#include <vector>

#include "limo/core/LinearProgram.hpp"

namespace limo::basis {

/**
 * @brief Finds an initial basic solution using the Big M method.
 *
 * The Big M method is a technique for finding a starting basis for the simplex algorithm.
 * It works by creating an artificial problem that is easier to solve.
 *
 * How it works:
 *
 * 1. **Add slack variables** for each "less than or equal" (<=) constraint.
 *    These variables represent unused resources and can form part of the initial basis.
 *
 * 2. **Add surplus variables** for each "greater than or equal" (>=) constraint.
 *    These variables represent the excess over the minimum requirement.
 *
 * 3. **Add artificial variables** where we don't have an obvious basic variable.
 *    This happens for:
 *    - Equality (=) constraints
 *    - Greater than or equal (>=) constraints (after adding surplus variables)
 *
 * 4. **Modify the objective function** by adding a penalty term for each artificial variable.
 *    The penalty uses a very large coefficient called "Big M". This makes artificial
 *    variables very expensive, so the algorithm tries to remove them from the solution.
 *
 * 5. **Solve the modified problem** using the simplex method. The artificial variables
 *    give us an easy starting point because they form an identity matrix.
 *
 * 6. **Check the result**:
 *    - If all artificial variables are zero in the final solution, we found a valid
 *      basic solution for the original problem.
 *    - If any artificial variable is positive, the original problem has no solution.
 *
 * The method is called "Big M" because M must be large enough that any solution with
 * artificial variables is worse than any solution without them.
 * 
 * @author Volodymyr Shpyrka
 */
class BigMBasisFinder {
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
        value_type bigM{};
        std::size_t originalVariableCount = 0;
    };

    BigMBasisFinder() = default;
    Result build(const LinearProgram& linearProgram, const value_type& bigM) const;
};

} // namespace limo::basis
