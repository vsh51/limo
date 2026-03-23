#include <gtest/gtest.h>
#include <string>

#include "limo/io/FileParser.hpp"
#include "limo/numerics/Fraction.hpp"
#include "limo/core/LinearProgram.hpp"

using limo::io::FileParser;
using limo::io::ParsedInput;
using limo::numerics::fraction::Fraction;
using LP = limo::core::LinearProgram;

// ── CSV tests ────────────────────────────────────────────────────────────────

TEST(FileParserCSV, ParsesBasicProblem) {
    const std::string csv =
        "maximize,30,20\n"
        "2,1,<=,100\n"
        "1,1,<=,80\n";

    auto [lp, config] = FileParser::parseCSV(csv);

    EXPECT_EQ(lp.objectiveSense(), LP::ObjectiveSense::Maximize);
    EXPECT_EQ(lp.objectiveCoefficients().size(), 2);
    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(30));
    EXPECT_EQ(lp.objectiveCoefficients()[1], Fraction(20));
    EXPECT_EQ(lp.rightHandSide().size(), 2);
    EXPECT_EQ(lp.rightHandSide()[0], Fraction(100));
    EXPECT_EQ(lp.rightHandSide()[1], Fraction(80));
    EXPECT_EQ(config.basisMethod, "big-m");
}

TEST(FileParserCSV, ParsesMinimize) {
    const std::string csv =
        "minimize,2,3\n"
        "1,2,>=,4\n"
        "2,1,>=,6\n";

    auto [lp, config] = FileParser::parseCSV(csv);

    EXPECT_EQ(lp.objectiveSense(), LP::ObjectiveSense::Minimize);
    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(2));
    EXPECT_EQ(lp.objectiveCoefficients()[1], Fraction(3));
}

TEST(FileParserCSV, ParsesBasisMethod) {
    const std::string csv =
        "maximize,5,4,artificial\n"
        "6,4,<=,24\n";

    auto [lp, config] = FileParser::parseCSV(csv);

    EXPECT_EQ(config.basisMethod, "artificial");
    EXPECT_EQ(lp.objectiveCoefficients().size(), 2);
}

TEST(FileParserCSV, ParsesFractions) {
    const std::string csv =
        "maximize,1/2,3/4\n"
        "1,1/3,<=,5/2\n";

    auto [lp, config] = FileParser::parseCSV(csv);

    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(1, 2));
    EXPECT_EQ(lp.objectiveCoefficients()[1], Fraction(3, 4));
    EXPECT_EQ(lp.rightHandSide()[0], Fraction(5, 2));
}

TEST(FileParserCSV, ParsesEqualityConstraint) {
    const std::string csv =
        "minimize,1,1\n"
        "1,1,=,10\n";

    auto [lp, config] = FileParser::parseCSV(csv);

    EXPECT_EQ(lp.constraintSense()[0], LP::ConstraintSense::Equal);
}

TEST(FileParserCSV, ThrowsOnTooFewRows) {
    EXPECT_THROW(FileParser::parseCSV("maximize,1,2\n"), std::invalid_argument);
}

TEST(FileParserCSV, ThrowsOnEmptyObjectiveRow) {
    EXPECT_THROW(FileParser::parseCSV("\n1,1,<=,5\n"), std::invalid_argument);
}

TEST(FileParserCSV, ThrowsOnNoCoefficients) {
    EXPECT_THROW(FileParser::parseCSV("maximize\n1,<=,5\n"), std::invalid_argument);
}

TEST(FileParserCSV, ThrowsOnColumnCountMismatch) {
    const std::string csv =
        "maximize,1,2\n"
        "1,<=,5\n";

    EXPECT_THROW(FileParser::parseCSV(csv), std::invalid_argument);
}

TEST(FileParserCSV, ThrowsOnInvalidSense) {
    EXPECT_THROW(
        FileParser::parseCSV("foobar,1,2\n1,1,<=,5\n"),
        std::invalid_argument);
}

TEST(FileParserCSV, ThrowsOnZeroDenominator) {
    EXPECT_THROW(
        FileParser::parseCSV("maximize,1/0,2\n1,1,<=,5\n"),
        std::invalid_argument);
}

TEST(FileParserCSV, SkipsEmptyLines) {
    const std::string csv =
        "maximize,10,6\n"
        "\n"
        "1,1,<=,100\n"
        "\n";

    auto [lp, config] = FileParser::parseCSV(csv);

    EXPECT_EQ(lp.rightHandSide().size(), 1);
    EXPECT_EQ(lp.rightHandSide()[0], Fraction(100));
}

// ── TXT tests ────────────────────────────────────────────────────────────────

TEST(FileParserTXT, ParsesBasicProblem) {
    const std::string txt =
        "maximize\n"
        "30 20\n"
        "2 1 <= 100\n"
        "1 1 <= 80\n";

    auto [lp, config] = FileParser::parseTXT(txt);

    EXPECT_EQ(lp.objectiveSense(), LP::ObjectiveSense::Maximize);
    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(30));
    EXPECT_EQ(lp.objectiveCoefficients()[1], Fraction(20));
    EXPECT_EQ(lp.rightHandSide()[0], Fraction(100));
    EXPECT_EQ(config.basisMethod, "big-m");
}

TEST(FileParserTXT, ParsesBasisMethod) {
    const std::string txt =
        "minimize\n"
        "2 3\n"
        "artificial\n"
        "1 2 >= 4\n";

    auto [lp, config] = FileParser::parseTXT(txt);

    EXPECT_EQ(config.basisMethod, "artificial");
    EXPECT_EQ(lp.objectiveSense(), LP::ObjectiveSense::Minimize);
}

TEST(FileParserTXT, SkipsComments) {
    const std::string txt =
        "# This is a comment\n"
        "maximize\n"
        "5 4\n"
        "# Another comment\n"
        "6 4 <= 24\n";

    auto [lp, config] = FileParser::parseTXT(txt);

    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(5));
    EXPECT_EQ(lp.rightHandSide().size(), 1);
}

TEST(FileParserTXT, SkipsEmptyLines) {
    const std::string txt =
        "\n"
        "maximize\n"
        "\n"
        "10 6\n"
        "\n"
        "1 1 <= 100\n"
        "\n";

    auto [lp, config] = FileParser::parseTXT(txt);

    EXPECT_EQ(lp.rightHandSide().size(), 1);
}

TEST(FileParserTXT, ParsesFractions) {
    const std::string txt =
        "minimize\n"
        "1/2 3/4\n"
        "1 1/3 >= 5/2\n";

    auto [lp, config] = FileParser::parseTXT(txt);

    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(1, 2));
    EXPECT_EQ(lp.rightHandSide()[0], Fraction(5, 2));
}

TEST(FileParserTXT, ParsesAllConstraintSenses) {
    const std::string txt =
        "maximize\n"
        "1 1\n"
        "1 0 <= 10\n"
        "0 1 = 5\n"
        "1 1 >= 3\n";

    auto [lp, config] = FileParser::parseTXT(txt);

    EXPECT_EQ(lp.constraintSense()[0], LP::ConstraintSense::LessEqual);
    EXPECT_EQ(lp.constraintSense()[1], LP::ConstraintSense::Equal);
    EXPECT_EQ(lp.constraintSense()[2], LP::ConstraintSense::GreaterEqual);
}

TEST(FileParserTXT, ThrowsOnTooFewLines) {
    EXPECT_THROW(FileParser::parseTXT("maximize\n1 2\n"), std::invalid_argument);
}

TEST(FileParserTXT, ThrowsOnMissingSense) {
    const std::string txt =
        "maximize\n"
        "1 2\n"
        "1 2 5\n";

    EXPECT_THROW(FileParser::parseTXT(txt), std::invalid_argument);
}

TEST(FileParserTXT, ThrowsOnCoefficientCountMismatch) {
    const std::string txt =
        "maximize\n"
        "1 2 3\n"
        "1 2 <= 5\n";

    EXPECT_THROW(FileParser::parseTXT(txt), std::invalid_argument);
}

TEST(FileParserTXT, ThrowsOnMissingRHS) {
    const std::string txt =
        "maximize\n"
        "1 2\n"
        "1 2 <=\n";

    EXPECT_THROW(FileParser::parseTXT(txt), std::invalid_argument);
}

TEST(FileParserTXT, ThrowsOnNoConstraints) {
    const std::string txt =
        "maximize\n"
        "1 2\n"
        "big-m\n";

    EXPECT_THROW(FileParser::parseTXT(txt), std::invalid_argument);
}

TEST(FileParserTXT, ThrowsOnEmptyCoefficients) {
    const std::string txt =
        "maximize\n"
        "\n"
        "1 1 <= 5\n";

    EXPECT_THROW(FileParser::parseTXT(txt), std::invalid_argument);
}
