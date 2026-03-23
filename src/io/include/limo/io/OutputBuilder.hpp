#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "limo/core/LinearProgram.hpp"
#include "limo/core/Result.hpp"
#include "limo/core/Solution.hpp"
#include "limo/io/InputParser.hpp"
#include "limo/io/SimplexTableau.hpp"

namespace limo::io {

/**
 * @brief Builds the complete JSON response for the CLI.
 *
 * Assembles input summary, basis-finding result, iteration snapshots,
 * and solution data into a single JSON object suitable for the web frontend.
 *
 * @author Volodymyr Shpyrka
 */
class OutputBuilder {
public:
    /**
     * @brief Build a full JSON response.
     * @param input     The original LinearProgram.
     * @param result    Basis-finding result.
     * @param basisMethod Name of the method used ("big-m" or "artificial").
     * @param iterations Simplex iteration snapshots (empty if solver not yet run).
     * @return Complete JSON response object.
     */
    static nlohmann::json build(
        const limo::core::LinearProgram& input,
        const limo::core::Result& result,
        const std::string& basisMethod,
        const std::vector<SimplexTableau>& iterations,
        const limo::core::Solution& solution
    );

    /**
     * @brief Build a JSON response containing only the parsed LP problem.
     *
     * Used by the --parse-only CLI mode so that the web server can convert
     * a CSV or TXT file into the web frontend's input format without running
     * the solver.
     *
     * @param lp     The parsed linear program.
     * @param config Solver configuration (only basisMethod is included).
     * @return JSON with status "ok", objective, constraints, and basisMethod.
     */
    static nlohmann::json buildParsedInput(
        const limo::core::LinearProgram& lp,
        const SolverConfig& config
    );

    /**
     * @brief Build an error JSON response.
     * @param message Error description.
     * @return JSON object with status "error".
     */
    static nlohmann::json buildError(const std::string& message);
};

} // namespace limo::io
