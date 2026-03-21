#pragma once

#include <string>

#include "limo/core/LinearProgram.hpp"
#include "limo/numerics/Fraction.hpp"

namespace limo::io {

/**
 * @brief Solver configuration extracted from the input JSON.
 */
struct SolverConfig {
    std::string method = "simplex";
    std::string basisMethod = "big-m";
    limo::numerics::fraction::Fraction bigM{1000};
};

/**
 * @brief Result of parsing the input JSON.
 */
struct ParsedInput {
    limo::core::LinearProgram linearProgram;
    SolverConfig config;
};

/**
 * @brief Parses a JSON string into a LinearProgram and solver configuration.
 *
 * Expected input schema:
 * @code{.json}
 * {
 *   "objective": {
 *     "sense": "maximize",
 *     "coefficients": [{"num": 30, "den": 1}]
 *   },
 *   "constraints": [
 *     {
 *       "coefficients": [{"num": 2, "den": 1}],
 *       "sense": "<=",
 *       "rhs": {"num": 100, "den": 1}
 *     }
 *   ],
 *   "solver": {
 *     "method": "simplex",
 *     "basisMethod": "big-m",
 *     "bigM": {"num": 1000, "den": 1}
 *   }
 * }
 * @endcode
 *
 * @author Volodymyr Shpyrka
 */
class InputParser {
public:
    /**
     * @brief Parse JSON string into a LinearProgram and SolverConfig.
     * @param json Raw JSON string from stdin.
     * @return ParsedInput containing the LP and solver settings.
     * @throws std::invalid_argument on validation errors.
     * @throws nlohmann::json::parse_error on malformed JSON.
     */
    static ParsedInput parse(const std::string& json);
};

} // namespace limo::io
