#include "limo/io/InputParser.hpp"
#include "limo/io/JsonSerializer.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace limo::io {

using Fraction = limo::numerics::fraction::Fraction;
using LP = limo::core::LinearProgram;

namespace {

LP::ObjectiveSense parseObjectiveSense(const std::string& s) {
    if (s == "maximize") return LP::ObjectiveSense::Maximize;
    if (s == "minimize") return LP::ObjectiveSense::Minimize;
    throw std::invalid_argument("Invalid objective sense: '" + s + "'. Expected 'maximize' or 'minimize'");
}

LP::ConstraintSense parseConstraintSense(const std::string& s) {
    if (s == "<=") return LP::ConstraintSense::LessEqual;
    if (s == "=")  return LP::ConstraintSense::Equal;
    if (s == ">=") return LP::ConstraintSense::GreaterEqual;
    throw std::invalid_argument("Invalid constraint sense: '" + s + "'. Expected '<=', '=', or '>='");
}

} // anonymous namespace

ParsedInput InputParser::parse(const std::string& json) {
    auto j = nlohmann::json::parse(json);

    if (!j.contains("objective")) {
        throw std::invalid_argument("Missing required field: 'objective'");
    }
    if (!j.contains("constraints")) {
        throw std::invalid_argument("Missing required field: 'constraints'");
    }

    const auto& objective = j.at("objective");
    auto sense = parseObjectiveSense(objective.at("sense").get<std::string>());
    auto objCoeffs = objective.at("coefficients").get<std::vector<Fraction>>();

    if (objCoeffs.empty()) {
        throw std::invalid_argument("Objective coefficients cannot be empty");
    }
    std::size_t numVars = objCoeffs.size();

    const auto& constraints = j.at("constraints");
    if (constraints.empty()) {
        throw std::invalid_argument("Constraints cannot be empty");
    }
    std::size_t numConstraints = constraints.size();

    LP::Matrix matrix(numConstraints, numVars);
    std::vector<Fraction> rhs;
    std::vector<LP::ConstraintSense> constraintSenses;

    rhs.reserve(numConstraints);
    constraintSenses.reserve(numConstraints);

    for (std::size_t i = 0; i < numConstraints; ++i) {
        const auto& c = constraints.at(i);
        auto coeffs = c.at("coefficients").get<std::vector<Fraction>>();

        if (coeffs.size() != numVars) {
            throw std::invalid_argument(
                "Constraint " + std::to_string(i) + " has " +
                std::to_string(coeffs.size()) + " coefficients, expected " +
                std::to_string(numVars)
            );
        }

        for (std::size_t col = 0; col < numVars; ++col) {
            matrix(i, col) = coeffs[col];
        }

        rhs.push_back(c.at("rhs").get<Fraction>());
        constraintSenses.push_back(
            parseConstraintSense(c.at("sense").get<std::string>())
        );
    }

    LP lp(std::move(matrix), std::move(rhs), std::move(objCoeffs), sense, std::move(constraintSenses));

    SolverConfig config;
    if (j.contains("solver")) {
        const auto& solver = j.at("solver");
        if (solver.contains("method")) {
            config.method = solver.at("method").get<std::string>();
        }
        if (solver.contains("basisMethod")) {
            config.basisMethod = solver.at("basisMethod").get<std::string>();
        }
        if (solver.contains("bigM")) {
            config.bigM = solver.at("bigM").get<Fraction>();
        }
    }

    return {std::move(lp), std::move(config)};
}

} // namespace limo::io
