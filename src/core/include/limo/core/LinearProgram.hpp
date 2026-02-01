#pragma once

#include <vector>

#include "limo/numerics/Fraction.hpp"
#include "limo/numerics/Matrix.hpp"

namespace limo::core {

/**
 * @brief Data container for a linear programming problem.
 *
 * This class stores the math data but does not solve the problem.
 * It is meant to be passed to a solver (for example, simplex).
 *
 * Example (maximization):
 *   Maximize: Z = 30*x1 + 20*x2
 *   Constraints:
 *     2*x1 + 1*x2 <= 100
 *     1*x1 + 1*x2 = 80
 *     1*x1 >= 20
 *   Note: x1 and x2 are column indices 0 and 1.
 *
 * How it maps to fields:
 *   - objectiveSense_ = ObjectiveSense::Maximize
 *   - objectiveCoefficients_ = { 30, 20 }
 *   - constraintMatrix_ (3x2):
 *       [ 2 1 ] - 2*x1 + 1*x2
 *       [ 1 1 ] - 1*x1 + 1*x2
 *       [ 1 0 ] - 1*x1
 *   - constraintSense_ = { LessEqual, Equal, GreaterEqual }
 *   - rightHandSide_ = { 100, 80, 20 }
 *
 * @author Volodymyr Shpyrka
 */
class LinearProgram {
public:
	using value_type = limo::numerics::fraction::Fraction;
	using Matrix = limo::numerics::Matrix<value_type>;

	enum class ObjectiveSense {
		Maximize,
		Minimize
	};

	enum class ConstraintSense {
		LessEqual,
		Equal,
		GreaterEqual
	};

	LinearProgram() = default;

	LinearProgram(Matrix constraintMatrix,
				  std::vector<value_type> rightHandSide,
				  std::vector<value_type> objectiveCoefficients,
				  ObjectiveSense objectiveSense,
				  std::vector<ConstraintSense> constraintSense);

	const Matrix& constraintMatrix() const { return constraintMatrix_; }
	const std::vector<value_type>& rightHandSide() const { return rightHandSide_; }
	const std::vector<value_type>& objectiveCoefficients() const { return objectiveCoefficients_; }

	ObjectiveSense objectiveSense() const { return objectiveSense_; }
	const std::vector<ConstraintSense>& constraintSense() const { return constraintSense_; }

	std::size_t rows() const { return constraintMatrix_.rows(); }
	std::size_t cols() const { return constraintMatrix_.cols(); }
	bool empty() const { return constraintMatrix_.empty(); }
	void validate() const;

private:
	Matrix constraintMatrix_{};
	std::vector<value_type> rightHandSide_{};
	std::vector<value_type> objectiveCoefficients_{};
	ObjectiveSense objectiveSense_{ObjectiveSense::Maximize};
	std::vector<ConstraintSense> constraintSense_{};
};

} // namespace limo::core
