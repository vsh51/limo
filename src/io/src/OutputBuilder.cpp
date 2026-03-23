#include "limo/io/OutputBuilder.hpp"
#include "limo/io/JsonSerializer.hpp"

namespace limo::io {

nlohmann::json OutputBuilder::build(
    const limo::core::LinearProgram& input,
    const limo::core::Result& result,
    const std::string& basisMethod,
    const std::vector<SimplexTableau>& iterations,
    const limo::core::Solution& solution
) {
    nlohmann::json output;
    output["status"] = "ok";
    output["error"] = nullptr;

    nlohmann::json inputInfo;
    inputInfo["variableCount"] = input.cols();
    inputInfo["constraintCount"] = input.rows();
    inputInfo["objectiveSense"] = input.objectiveSense();
    output["input"] = std::move(inputInfo);

    nlohmann::json basisFinding;
    basisFinding["method"] = basisMethod;
    basisFinding["result"] = result;
    output["basisFinding"] = std::move(basisFinding);

    output["iterations"] = iterations;
    output["solution"] = solution;

    return output;
}

nlohmann::json OutputBuilder::buildParsedInput(
    const limo::core::LinearProgram& lp,
    const SolverConfig& config)
{
    nlohmann::json output;
    output["status"] = "ok";

    // Objective
    nlohmann::json objective;
    objective["sense"]        = lp.objectiveSense();
    objective["coefficients"] = lp.objectiveCoefficients();
    output["objective"] = std::move(objective);

    // Constraints — rebuild row-by-row from the matrix
    nlohmann::json constraints = nlohmann::json::array();
    const auto& senses = lp.constraintSense();
    const auto& rhs    = lp.rightHandSide();
    for (std::size_t i = 0; i < lp.rows(); ++i) {
        nlohmann::json con;
        nlohmann::json coeffs = nlohmann::json::array();
        for (std::size_t j = 0; j < lp.cols(); ++j) {
            coeffs.push_back(lp.constraintMatrix()(i, j));
        }
        con["coefficients"] = std::move(coeffs);
        con["sense"]        = senses[i];
        con["rhs"]          = rhs[i];
        constraints.push_back(std::move(con));
    }
    output["constraints"] = std::move(constraints);
    output["basisMethod"] = config.basisMethod;

    return output;
}

nlohmann::json OutputBuilder::buildError(const std::string& message) {
    return {
        {"status", "error"},
        {"error",  message}
    };
}

} // namespace limo::io
