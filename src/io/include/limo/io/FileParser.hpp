#pragma once

#include <string>

#include "limo/io/InputParser.hpp"

namespace limo::io {

/**
 * @brief Parses LP problems from plain-text file formats (CSV and TXT).
 *
 * Both parsers produce the same @c ParsedInput returned by InputParser,
 * so the rest of the solver pipeline is format-agnostic.
 *
 * CSV format
 * ----------
 * Row 1:   sense, c1, c2, ..., cn [, big-m | artificial]
 * Rows 2+: c1, c2, ..., cn, sense, rhs
 *
 * Example (maximize 30x1 + 20x2):
 * @code
 * maximize,30,20
 * 2,1,<=,100
 * 1,1,<=,80
 * @endcode
 *
 * TXT format
 * ----------
 * Line 1:    maximize | minimize
 * Line 2:    c1 c2 ... cn         (space-separated objective coefficients)
 * Line 3:    big-m | artificial   (optional; default: big-m)
 * Lines 3+:  c1 c2 ... cn <= rhs  (relational operator: <=, =, >=)
 *
 * Lines whose first non-whitespace character is '#' are treated as comments.
 *
 * Coefficients in both formats are decimal integers or rational fractions
 * written as "num/den" (e.g. "1/2", "-3/4").
 *
 * @author Volodymyr Shpyrka
 */
class FileParser {
public:
    /**
     * @brief Parse an LP problem from CSV content.
     * @throws std::invalid_argument on structural or value errors.
     */
    static ParsedInput parseCSV(const std::string& content);

    /**
     * @brief Parse an LP problem from TXT content.
     * @throws std::invalid_argument on structural or value errors.
     */
    static ParsedInput parseTXT(const std::string& content);
};

} // namespace limo::io
