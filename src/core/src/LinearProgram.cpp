#include "limo/core/LinearProgram.hpp"

#include <stdexcept>

namespace limo::core {

LinearProgram::LinearProgram(Matrix matrix,
							 std::vector<value_type> rhs,
							 std::vector<value_type> objective,
							 ObjectiveSense sense,
							 std::vector<ConstraintSense> constraintSense)
	: constraintMatrix_(std::move(matrix))
	, rightHandSide_(std::move(rhs))
	, objectiveCoefficients_(std::move(objective))
	, objectiveSense_(sense)
	, constraintSense_(std::move(constraintSense)) {
	validate();
}

void LinearProgram::validate() const {
    if (constraintMatrix_.rows() == 0 || constraintMatrix_.cols() == 0) {
        throw std::invalid_argument("LinearProgram constraint matrix must be non-empty");
    }
    if (rightHandSide_.size() != constraintMatrix_.rows()) {
        throw std::invalid_argument("LinearProgram right-hand side must match constraint rows");
    }
    if (objectiveCoefficients_.size() != constraintMatrix_.cols()) {
        throw std::invalid_argument("LinearProgram objective coefficients must match constraint cols");
    }
    if (!constraintSense_.empty() && constraintSense_.size() != constraintMatrix_.rows()) {
        throw std::invalid_argument("LinearProgram constraint senses must match A rows");
    }
}

} // namespace limo::core
