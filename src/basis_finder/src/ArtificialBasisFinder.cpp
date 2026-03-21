#include "limo/basis/ArtificialBasisFinder.hpp"
#include <limo/core/LinearProgram.hpp>

namespace limo::basis {

using limo::core::Result;

Result ArtificialBasisFinder::build(const limo::core::LinearProgram& linearProgram) const {
	if (linearProgram.empty()) {
		return Result{};
	}

	const auto rows = linearProgram.rows();
	const auto originalCols = linearProgram.cols();
	const auto& originalMatrix = linearProgram.constraintMatrix();
	const auto& originalRhs = linearProgram.rightHandSide();
	const auto& originalSense = linearProgram.constraintSense();

	std::size_t numSlack = 0;
	std::size_t numSurplus = 0;
	std::size_t numArtificial = 0;

	// Account for RHS signs: if RHS < 0, the constraint is effectively flipped
	for (std::size_t r = 0; r < rows; ++r) {
		limo::core::LinearProgram::value_type rhs = originalRhs[r];
		auto sense = originalSense[r];

		if (rhs < limo::core::LinearProgram::value_type{0}) {
			if (sense == limo::core::LinearProgram::ConstraintSense::LessEqual) {
				sense = limo::core::LinearProgram::ConstraintSense::GreaterEqual;
			} else if (sense == limo::core::LinearProgram::ConstraintSense::GreaterEqual) {
				sense = limo::core::LinearProgram::ConstraintSense::LessEqual;
			}
		}

		switch (sense) {
			case limo::core::LinearProgram::ConstraintSense::LessEqual:
				numSlack++;
				break;
			case limo::core::LinearProgram::ConstraintSense::GreaterEqual:
				numSurplus++;
				numArtificial++;
				break;
			case limo::core::LinearProgram::ConstraintSense::Equal:
				numArtificial++;
				break;
		}
	}

	// Prepare results structure
	Result result;
	result.originalVariableCount = originalCols;

	const auto totalCols = originalCols + numSlack + numSurplus + numArtificial;

	result.basisColumns.resize(rows);
	result.slackColumns.reserve(numSlack);
	result.surplusColumns.reserve(numSurplus);
	result.artificialColumns.reserve(numArtificial);

	// Initialize augmented LP components (phase I objective)
	Matrix augmentedMatrix(rows, totalCols, value_type{0});
	std::vector<value_type> augmentedRhs(rows);
	std::vector<value_type> augmentedObj(totalCols, value_type{0});
	std::vector<ConstraintSense> augmentedSense(rows, ConstraintSense::Equal);

	std::size_t currentSlackCol = originalCols;
	std::size_t currentSurplusCol = currentSlackCol + numSlack;
	std::size_t currentArtificialCol = currentSurplusCol + numSurplus;

	for (std::size_t r = 0; r < rows; ++r) {
		value_type rhs = originalRhs[r];
		auto sense = originalSense[r];
		value_type multiplier = value_type{1};

		if (rhs < value_type{0}) {
			rhs = value_type{0} - rhs;
			multiplier = value_type{-1};
			if (sense == ConstraintSense::LessEqual) {
				sense = ConstraintSense::GreaterEqual;
			} else if (sense == ConstraintSense::GreaterEqual) {
				sense = ConstraintSense::LessEqual;
			}
		}

		augmentedRhs[r] = rhs;

		for (std::size_t c = 0; c < originalCols; ++c) {
			augmentedMatrix(r, c) = originalMatrix(r, c) * multiplier;
		}

		switch (sense) {
			case ConstraintSense::LessEqual: {
				// Add Slack variable with coefficient +1
				// ... + 1*s = RHS
				const auto slackIdx = currentSlackCol++;
				augmentedMatrix(r, slackIdx) = value_type{1};
				result.slackColumns.push_back(slackIdx);
				result.basisColumns[r] = slackIdx;
				break;
			}
			case ConstraintSense::GreaterEqual: {
				// Add Surplus variable with coefficient -1
				// ... - 1*s + 1*a = RHS
				const auto surplusIdx = currentSurplusCol++;
				augmentedMatrix(r, surplusIdx) = value_type{-1};
				result.surplusColumns.push_back(surplusIdx);

				// Add Artificial variable with coefficient +1
				const auto artificialIdx = currentArtificialCol++;
				augmentedMatrix(r, artificialIdx) = value_type{1};
				result.artificialColumns.push_back(artificialIdx);
				augmentedObj[artificialIdx] = value_type{1};

				// Artificial variable is the initial basis for this row
				result.basisColumns[r] = artificialIdx;
				break;
			}
			case ConstraintSense::Equal: {
				// Add Artificial variable with coefficient +1
				const auto artificialIdx = currentArtificialCol++;
				augmentedMatrix(r, artificialIdx) = value_type{1};
				result.artificialColumns.push_back(artificialIdx);
				augmentedObj[artificialIdx] = value_type{1};

				// Artificial variable is the initial basis for this row
				result.basisColumns[r] = artificialIdx;
				break;
			}
		}
	}

	result.augmented = LinearProgram(
		std::move(augmentedMatrix),
		std::move(augmentedRhs),
		std::move(augmentedObj),
		ObjectiveSense::Minimize,
		std::move(augmentedSense)
	);

	return result;
}

} // namespace limo::basis
