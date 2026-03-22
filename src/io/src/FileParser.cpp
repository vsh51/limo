#include "limo/io/FileParser.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace limo::io {

using Fraction = limo::numerics::fraction::Fraction;
using LP       = limo::core::LinearProgram;

namespace {

// ── String utilities ─────────────────────────────────────────────────────────

std::string trim(std::string s) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

bool isComment(const std::string& line) {
    auto it = std::find_if(line.begin(), line.end(),
                           [](unsigned char c) { return !std::isspace(c); });
    return it != line.end() && *it == '#';
}

// ── Fraction parsing ──────────────────────────────────────────────────────────

/**
 * Parse a coefficient string of the form "N" or "N/D"
 * where N and D are (possibly signed) integers.
 */
Fraction parseCoeff(const std::string& raw) {
    const std::string s = trim(raw);
    if (s.empty()) {
        throw std::invalid_argument("Empty coefficient");
    }
    const auto slash = s.find('/');
    if (slash == std::string::npos) {
        return Fraction(std::stoll(s));
    }
    const int64_t num = std::stoll(s.substr(0, slash));
    const int64_t den = std::stoll(s.substr(slash + 1));
    if (den == 0) {
        throw std::invalid_argument("Fraction denominator cannot be zero: " + s);
    }
    return Fraction(num, den);
}

// ── Sense helpers ─────────────────────────────────────────────────────────────

LP::ObjectiveSense parseObjectiveSense(const std::string& raw) {
    const std::string s = trim(raw);
    if (s == "maximize" || s == "max") return LP::ObjectiveSense::Maximize;
    if (s == "minimize" || s == "min") return LP::ObjectiveSense::Minimize;
    throw std::invalid_argument(
        "Expected 'maximize'/'minimize' (or 'max'/'min'), got: '" + s + "'");
}

LP::ConstraintSense parseConstraintSense(const std::string& raw) {
    const std::string s = trim(raw);
    if (s == "<=") return LP::ConstraintSense::LessEqual;
    if (s == "=")  return LP::ConstraintSense::Equal;
    if (s == ">=") return LP::ConstraintSense::GreaterEqual;
    throw std::invalid_argument(
        "Expected '<=', '=', or '>=', got: '" + s + "'");
}

std::string parseBasisMethod(const std::string& raw) {
    const std::string s = trim(raw);
    if (s == "big-m" || s == "artificial") return s;
    throw std::invalid_argument(
        "Expected 'big-m' or 'artificial', got: '" + s + "'");
}

// ── LP assembly ───────────────────────────────────────────────────────────────

ParsedInput buildParsedInput(
    LP::ObjectiveSense objSense,
    const std::vector<Fraction>& objCoeffs,
    const std::vector<std::vector<Fraction>>& coeffMatrix,
    const std::vector<LP::ConstraintSense>& conSenses,
    const std::vector<Fraction>& rhs,
    const std::string& basisMethod)
{
    const std::size_t numVars = objCoeffs.size();
    const std::size_t numCons = coeffMatrix.size();

    LP::Matrix matrix(numCons, numVars);
    for (std::size_t i = 0; i < numCons; ++i) {
        for (std::size_t j = 0; j < numVars; ++j) {
            matrix(i, j) = coeffMatrix[i][j];
        }
    }

    LP lp(matrix,
          std::vector<Fraction>(rhs),
          std::vector<Fraction>(objCoeffs),
          objSense,
          std::vector<LP::ConstraintSense>(conSenses));

    SolverConfig config;
    config.basisMethod = basisMethod;
    return {std::move(lp), std::move(config)};
}

} // anonymous namespace

// ── CSV parser ────────────────────────────────────────────────────────────────

ParsedInput FileParser::parseCSV(const std::string& content) {
    std::vector<std::vector<std::string>> rows;

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) continue;

        std::vector<std::string> cells;
        std::istringstream row(line);
        std::string cell;
        while (std::getline(row, cell, ',')) {
            cells.push_back(trim(cell));
        }
        rows.push_back(std::move(cells));
    }

    if (rows.size() < 2) {
        throw std::invalid_argument(
            "CSV must have at least 2 rows (objective row + 1 constraint)");
    }

    // ── Row 1: objective ──
    const auto& objRow = rows[0];
    if (objRow.empty()) {
        throw std::invalid_argument("CSV objective row is empty");
    }

    auto objSense = parseObjectiveSense(objRow[0]);

    // Optional last token is the basis method
    std::size_t coeffEnd = objRow.size();
    std::string basisMethod = "big-m";
    if (coeffEnd > 1) {
        const std::string& last = objRow.back();
        if (last == "big-m" || last == "artificial") {
            basisMethod = last;
            --coeffEnd;
        }
    }

    if (coeffEnd <= 1) {
        throw std::invalid_argument("CSV objective row has no coefficients");
    }

    std::vector<Fraction> objCoeffs;
    objCoeffs.reserve(coeffEnd - 1);
    for (std::size_t i = 1; i < coeffEnd; ++i) {
        objCoeffs.push_back(parseCoeff(objRow[i]));
    }
    const std::size_t numVars = objCoeffs.size();

    // ── Rows 2+: constraints ──
    std::vector<std::vector<Fraction>> coeffMatrix;
    std::vector<LP::ConstraintSense>   conSenses;
    std::vector<Fraction>               rhs;

    for (std::size_t ri = 1; ri < rows.size(); ++ri) {
        const auto& row = rows[ri];
        // Expected columns: numVars coefficients + sense + rhs = numVars + 2
        if (row.size() != numVars + 2) {
            throw std::invalid_argument(
                "CSV row " + std::to_string(ri + 1) + ": expected " +
                std::to_string(numVars + 2) + " columns (" +
                std::to_string(numVars) + " coefficients + sense + rhs), got " +
                std::to_string(row.size()));
        }

        std::vector<Fraction> rowCoeffs;
        rowCoeffs.reserve(numVars);
        for (std::size_t ci = 0; ci < numVars; ++ci) {
            rowCoeffs.push_back(parseCoeff(row[ci]));
        }

        coeffMatrix.push_back(std::move(rowCoeffs));
        conSenses.push_back(parseConstraintSense(row[numVars]));
        rhs.push_back(parseCoeff(row[numVars + 1]));
    }

    return buildParsedInput(objSense, objCoeffs, coeffMatrix, conSenses, rhs, basisMethod);
}

// ── TXT parser ────────────────────────────────────────────────────────────────

ParsedInput FileParser::parseTXT(const std::string& content) {
    // Collect non-empty, non-comment lines
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string raw;
    while (std::getline(stream, raw)) {
        const std::string t = trim(raw);
        if (t.empty() || isComment(t)) continue;
        lines.push_back(t);
    }

    if (lines.size() < 3) {
        throw std::invalid_argument(
            "TXT file must have at least 3 non-comment lines: "
            "objective sense, objective coefficients, and at least one constraint");
    }

    std::size_t li = 0;

    // Line 1: objective sense
    auto objSense = parseObjectiveSense(lines[li++]);

    // Line 2: objective coefficients
    std::vector<Fraction> objCoeffs;
    {
        std::istringstream row(lines[li++]);
        std::string tok;
        while (row >> tok) {
            objCoeffs.push_back(parseCoeff(tok));
        }
        if (objCoeffs.empty()) {
            throw std::invalid_argument("TXT line 2: no objective coefficients found");
        }
    }
    const std::size_t numVars = objCoeffs.size();

    // Line 3 (optional): basis method
    std::string basisMethod = "big-m";
    if (li < lines.size()) {
        const std::string& candidate = lines[li];
        if (candidate == "big-m" || candidate == "artificial") {
            basisMethod = candidate;
            ++li;
        }
    }

    // Remaining lines: constraints
    std::vector<std::vector<Fraction>> coeffMatrix;
    std::vector<LP::ConstraintSense>   conSenses;
    std::vector<Fraction>               rhs;

    for (; li < lines.size(); ++li) {
        std::istringstream row(lines[li]);
        std::vector<std::string> tokens;
        std::string tok;
        while (row >> tok) tokens.push_back(tok);

        // Find the relational operator
        std::size_t senseIdx = std::string::npos;
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == "<=" || tokens[i] == ">=" || tokens[i] == "=") {
                senseIdx = i;
                break;
            }
        }
        if (senseIdx == std::string::npos) {
            throw std::invalid_argument(
                "TXT constraint missing relational operator (<=, =, >=): '" +
                lines[li] + "'");
        }
        if (senseIdx + 1 >= tokens.size()) {
            throw std::invalid_argument(
                "TXT constraint missing right-hand side: '" + lines[li] + "'");
        }
        if (senseIdx != numVars) {
            throw std::invalid_argument(
                "TXT constraint has " + std::to_string(senseIdx) +
                " coefficient(s), expected " + std::to_string(numVars) +
                ": '" + lines[li] + "'");
        }

        std::vector<Fraction> rowCoeffs;
        rowCoeffs.reserve(numVars);
        for (std::size_t i = 0; i < numVars; ++i) {
            rowCoeffs.push_back(parseCoeff(tokens[i]));
        }

        coeffMatrix.push_back(std::move(rowCoeffs));
        conSenses.push_back(parseConstraintSense(tokens[senseIdx]));
        rhs.push_back(parseCoeff(tokens[senseIdx + 1]));
    }

    if (coeffMatrix.empty()) {
        throw std::invalid_argument("TXT file has no constraints");
    }

    return buildParsedInput(objSense, objCoeffs, coeffMatrix, conSenses, rhs, basisMethod);
}

} // namespace limo::io
