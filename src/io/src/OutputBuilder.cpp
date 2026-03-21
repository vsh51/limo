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

nlohmann::json OutputBuilder::buildError(const std::string& message) {
    return {
        {"status", "error"},
        {"error",  message}
    };
}

} // namespace limo::io
