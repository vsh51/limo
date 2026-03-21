#include "limo/io/JsonSerializer.hpp"

#include <stdexcept>
#include <string>

using Fraction = limo::numerics::fraction::Fraction;
using Matrix = limo::numerics::Matrix<Fraction>;
using LP = limo::core::LinearProgram;
using Result = limo::core::Result;
using Solution = limo::core::Solution;
using Tableau = limo::io::SimplexTableau;

namespace nlohmann {

// --- Fraction ---

void adl_serializer<Fraction>::to_json(json& j, const Fraction& f) {
    j = json{{"num", f.getNumerator()}, {"den", f.getDenominator()}};
}

void adl_serializer<Fraction>::from_json(const json& j, Fraction& f) {
    int64_t num = j.at("num").get<int64_t>();
    int64_t den = j.at("den").get<int64_t>();
    if (den == 0) {
        throw std::invalid_argument("Fraction denominator cannot be zero");
    }
    f = Fraction(num, den);
}

// --- Matrix<Fraction> ---

void adl_serializer<Matrix>::to_json(json& j, const Matrix& m) {
    json data = json::array();
    for (std::size_t r = 0; r < m.rows(); ++r) {
        json row = json::array();
        for (std::size_t c = 0; c < m.cols(); ++c) {
            row.push_back(m(r, c));
        }
        data.push_back(std::move(row));
    }
    j = json{{"rows", m.rows()}, {"cols", m.cols()}, {"data", std::move(data)}};
}

void adl_serializer<Matrix>::from_json(const json& j, Matrix& m) {
    auto rows = j.at("rows").get<std::size_t>();
    auto cols = j.at("cols").get<std::size_t>();
    m = Matrix(rows, cols);
    const auto& data = j.at("data");
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t c = 0; c < cols; ++c) {
            m(r, c) = data.at(r).at(c).get<Fraction>();
        }
    }
}

// --- ObjectiveSense ---

void adl_serializer<LP::ObjectiveSense>::to_json(json& j, const LP::ObjectiveSense& s) {
    switch (s) {
        case LP::ObjectiveSense::Maximize: j = "maximize"; break;
        case LP::ObjectiveSense::Minimize: j = "minimize"; break;
    }
}

void adl_serializer<LP::ObjectiveSense>::from_json(const json& j, LP::ObjectiveSense& s) {
    auto str = j.get<std::string>();
    if (str == "maximize") {
        s = LP::ObjectiveSense::Maximize;
    } else if (str == "minimize") {
        s = LP::ObjectiveSense::Minimize;
    } else {
        throw std::invalid_argument("Invalid objective sense: " + str);
    }
}

// --- ConstraintSense ---

void adl_serializer<LP::ConstraintSense>::to_json(json& j, const LP::ConstraintSense& s) {
    switch (s) {
        case LP::ConstraintSense::LessEqual:    j = "<="; break;
        case LP::ConstraintSense::Equal:        j = "=";  break;
        case LP::ConstraintSense::GreaterEqual: j = ">="; break;
    }
}

void adl_serializer<LP::ConstraintSense>::from_json(const json& j, LP::ConstraintSense& s) {
    auto str = j.get<std::string>();
    if (str == "<=") {
        s = LP::ConstraintSense::LessEqual;
    } else if (str == "=") {
        s = LP::ConstraintSense::Equal;
    } else if (str == ">=") {
        s = LP::ConstraintSense::GreaterEqual;
    } else {
        throw std::invalid_argument("Invalid constraint sense: " + str);
    }
}

// --- LinearProgram ---

void adl_serializer<LP>::to_json(json& j, const LP& lp) {
    j["constraintMatrix"] = lp.constraintMatrix();
    j["rightHandSide"] = lp.rightHandSide();
    j["objectiveCoefficients"] = lp.objectiveCoefficients();
    j["objectiveSense"] = lp.objectiveSense();
    j["constraintSense"] = lp.constraintSense();
}

void adl_serializer<LP>::from_json(const json& j, LP& lp) {
    auto matrix  = j.at("constraintMatrix").get<LP::Matrix>();
    auto rhs     = j.at("rightHandSide").get<std::vector<LP::value_type>>();
    auto obj     = j.at("objectiveCoefficients").get<std::vector<LP::value_type>>();
    auto sense   = j.at("objectiveSense").get<LP::ObjectiveSense>();
    auto csenses = j.at("constraintSense").get<std::vector<LP::ConstraintSense>>();
    lp = LP(std::move(matrix), std::move(rhs), std::move(obj), sense, std::move(csenses));
}

// --- Result ---

void adl_serializer<Result>::to_json(json& j, const Result& r) {
    j = json{
        {"originalVariableCount", r.originalVariableCount},
        {"basisColumns",          r.basisColumns},
        {"slackColumns",          r.slackColumns},
        {"surplusColumns",        r.surplusColumns},
        {"artificialColumns",     r.artificialColumns},
        {"augmented",             r.augmented},
        {"bigM",                  r.bigM}
    };
}

void adl_serializer<Result>::from_json(const json& j, Result& r) {
    r.originalVariableCount = j.at("originalVariableCount").get<std::size_t>();
    r.basisColumns          = j.at("basisColumns").get<std::vector<std::size_t>>();
    r.slackColumns          = j.at("slackColumns").get<std::vector<std::size_t>>();
    r.surplusColumns        = j.at("surplusColumns").get<std::vector<std::size_t>>();
    r.artificialColumns     = j.at("artificialColumns").get<std::vector<std::size_t>>();
    r.augmented             = j.at("augmented").get<LP>();
    r.bigM                  = j.at("bigM").get<LP::value_type>();
}

// --- SimplexTableau ---

void adl_serializer<Tableau>::to_json(json& j, const Tableau& t) {
    j["iterationNumber"] = t.iterationNumber;
    j["phase"] = t.phase;
    j["tableau"] = t.tableau;
    j["basisVariables"] = t.basisVariables;
    j["pivotRow"] = t.pivotRow.has_value() ? json(t.pivotRow.value()) : json(nullptr);
    j["pivotColumn"] = t.pivotColumn.has_value() ? json(t.pivotColumn.value()) : json(nullptr);
    j["objectiveValue"] = t.objectiveValue;

    json ratios = json::array();
    for (const auto& r : t.ratios) {
        ratios.push_back(r.has_value() ? json(r.value()) : json(nullptr));
    }
    j["ratios"] = std::move(ratios);
}

void adl_serializer<Tableau>::from_json(const json& j, Tableau& t) {
    t.iterationNumber = j.at("iterationNumber").get<std::size_t>();
    t.phase           = j.value("phase", 0);
    t.tableau         = j.at("tableau").get<Matrix>();
    t.basisVariables  = j.at("basisVariables").get<std::vector<std::size_t>>();

    const auto& pr = j.at("pivotRow");
    t.pivotRow = pr.is_null() ? std::nullopt : std::optional<std::size_t>(pr.get<std::size_t>());

    const auto& pc = j.at("pivotColumn");
    t.pivotColumn = pc.is_null() ? std::nullopt : std::optional<std::size_t>(pc.get<std::size_t>());

    t.objectiveValue = j.at("objectiveValue").get<Fraction>();

    t.ratios.clear();
    for (const auto& elem : j.at("ratios")) {
        if (elem.is_null()) {
            t.ratios.push_back(std::nullopt);
        } else {
            t.ratios.push_back(elem.get<Fraction>());
        }
    }
}

// --- Solution ---

void adl_serializer<Solution>::to_json(json& j, const Solution& s) {
    std::string status;
    switch (s.status) {
        case Solution::Status::Optimal:     status = "optimal";     break;
        case Solution::Status::Infeasible:  status = "infeasible";  break;
        case Solution::Status::Unbounded:   status = "unbounded";   break;
    }
    j["status"] = status;
    j["objectiveValue"] = s.objectiveValue;
    j["variableValues"] = s.variableValues;
}

void adl_serializer<Solution>::from_json(const json& j, Solution& s) {
    auto str = j.at("status").get<std::string>();
    if (str == "optimal") {
        s.status = Solution::Status::Optimal;
    } else if (str == "infeasible") {
        s.status = Solution::Status::Infeasible;
    } else if (str == "unbounded") {
        s.status = Solution::Status::Unbounded;
    } else {
        throw std::invalid_argument("Invalid solution status: " + str);
    }
    s.objectiveValue = j.at("objectiveValue").get<Fraction>();
    s.variableValues = j.at("variableValues").get<std::vector<Fraction>>();
}

} // namespace nlohmann
