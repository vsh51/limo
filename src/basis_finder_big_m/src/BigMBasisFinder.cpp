#include "limo/basis/BigMBasisFinder.hpp"

namespace limo::basis {

BigMBasisFinder::Result BigMBasisFinder::build(
    const LinearProgram& linearProgram,
    const value_type& bigM) const {
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
        value_type rhs = originalRhs[r];
        auto sense = originalSense[r];

        // If RHS is negative, multiply the equation by -1, which flips the inequality sense
        if (rhs < value_type{0}) {
            if (sense == ConstraintSense::LessEqual) {
                sense = ConstraintSense::GreaterEqual;
            } else if (sense == ConstraintSense::GreaterEqual) {
                sense = ConstraintSense::LessEqual;
            }
            // Equality (=) remains Equality (=)
        }

        switch (sense) {
            case ConstraintSense::LessEqual:
                numSlack++;
                break;
            case ConstraintSense::GreaterEqual:
                numSurplus++;
                numArtificial++;
                break;
            case ConstraintSense::Equal:
                numArtificial++;
                break;
        }
    }

    // Prepare results structure
    Result result;
    result.originalVariableCount = originalCols;
    result.bigM = bigM;

    const auto totalCols = originalCols + numSlack + numSurplus + numArtificial;

    result.basisColumns.resize(rows);
    result.slackColumns.reserve(numSlack);
    result.surplusColumns.reserve(numSurplus);
    result.artificialColumns.reserve(numArtificial);

    // Initialize augmented LP components
    Matrix augmentedMatrix(rows, totalCols, value_type{0});
    std::vector<value_type> augmentedRhs(rows);
    std::vector<value_type> augmentedObj(totalCols, value_type{0});
    std::vector<ConstraintSense> augmentedSense(rows, ConstraintSense::Equal);

    // Copy original objective coefficients
    const auto& originalObj = linearProgram.objectiveCoefficients();
    for (std::size_t c = 0; c < originalCols; ++c) {
        if (c < originalObj.size()) {
            augmentedObj[c] = originalObj[c];
        }
    }

    // Determine penalty for artificial variables based on goal
    // Maximize: -M
    // Minimize: +M
    value_type artificialPenalty =
        (linearProgram.objectiveSense() == ObjectiveSense::Maximize)
            ? value_type{0} - bigM
            : bigM;

    std::size_t currentSlackCol = originalCols;
    std::size_t currentSurplusCol = currentSlackCol + numSlack;
    std::size_t currentArtificialCol = currentSurplusCol + numSurplus;

    // Fill the matrix and vectors
    for (std::size_t r = 0; r < rows; ++r) {
        value_type rhs = originalRhs[r];
        auto sense = originalSense[r];
        value_type multiplier = value_type{1};

        // Normalize RHS to be non-negative
        if (rhs < value_type{0}) {
            rhs = value_type{0} - rhs;
            multiplier = value_type{-1};

            // Flip sense logic
            if (sense == ConstraintSense::LessEqual) {
                sense = ConstraintSense::GreaterEqual;
            } else if (sense == ConstraintSense::GreaterEqual) {
                sense = ConstraintSense::LessEqual;
            }
        }

        augmentedRhs[r] = rhs;

        // Copy original coefficients with multiplier
        for (std::size_t c = 0; c < originalCols; ++c) {
            augmentedMatrix(r, c) = originalMatrix(r, c) * multiplier;
        }

        // Add slack/surplus/artificial variables
        switch (sense) {
            case ConstraintSense::LessEqual: {
                // Add Slack variable with coefficient +1
                // ... + 1*s = RHS
                const auto slackIdx = currentSlackCol++;
                augmentedMatrix(r, slackIdx) = value_type{1};
                result.slackColumns.push_back(slackIdx);

                // Slack variable is the initial basis for this row
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
                augmentedObj[artificialIdx] = artificialPenalty;

                // Artificial variable is the initial basis for this row
                result.basisColumns[r] = artificialIdx;
                break;
            }
            case ConstraintSense::Equal: {
                // Add Artificial variable with coefficient +1
                // ... + 1*a = RHS
                const auto artificialIdx = currentArtificialCol++;
                augmentedMatrix(r, artificialIdx) = value_type{1};
                result.artificialColumns.push_back(artificialIdx);
                augmentedObj[artificialIdx] = artificialPenalty;

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
        linearProgram.objectiveSense(),
        std::move(augmentedSense)
    );

    return result;
}

} // namespace limo::basis
