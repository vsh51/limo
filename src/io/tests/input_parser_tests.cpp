#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>

#include "limo/io/InputParser.hpp"
#include "limo/numerics/Fraction.hpp"
#include "limo/core/LinearProgram.hpp"

using limo::io::InputParser;
using limo::io::ParsedInput;
using limo::numerics::fraction::Fraction;
using limo::core::LinearProgram;

namespace {

const std::string validJson = R"({
    "objective": {
        "sense": "maximize",
        "coefficients": [{"num": 30, "den": 1}, {"num": 20, "den": 1}]
    },
    "constraints": [
        {
            "coefficients": [{"num": 2, "den": 1}, {"num": 1, "den": 1}],
            "sense": "<=",
            "rhs": {"num": 100, "den": 1}
        },
        {
            "coefficients": [{"num": 1, "den": 1}, {"num": 1, "den": 1}],
            "sense": "=",
            "rhs": {"num": 80, "den": 1}
        }
    ],
    "solver": {
        "method": "simplex",
        "basisMethod": "big-m",
        "bigM": {"num": 1000, "den": 1}
    }
})";

} // anonymous namespace

TEST(InputParser, ValidParse) {
    auto [lp, config] = InputParser::parse(validJson);

    EXPECT_EQ(lp.rows(), 2);
    EXPECT_EQ(lp.cols(), 2);
    EXPECT_EQ(lp.objectiveSense(), LinearProgram::ObjectiveSense::Maximize);

    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(30));
    EXPECT_EQ(lp.objectiveCoefficients()[1], Fraction(20));

    EXPECT_EQ(lp.constraintMatrix()(0, 0), Fraction(2));
    EXPECT_EQ(lp.constraintMatrix()(0, 1), Fraction(1));
    EXPECT_EQ(lp.constraintMatrix()(1, 0), Fraction(1));
    EXPECT_EQ(lp.constraintMatrix()(1, 1), Fraction(1));

    EXPECT_EQ(lp.rightHandSide()[0], Fraction(100));
    EXPECT_EQ(lp.rightHandSide()[1], Fraction(80));

    EXPECT_EQ(lp.constraintSense()[0], LinearProgram::ConstraintSense::LessEqual);
    EXPECT_EQ(lp.constraintSense()[1], LinearProgram::ConstraintSense::Equal);
}

TEST(InputParser, SolverConfig) {
    auto [lp, config] = InputParser::parse(validJson);

    EXPECT_EQ(config.method, "simplex");
    EXPECT_EQ(config.basisMethod, "big-m");
    EXPECT_EQ(config.bigM, Fraction(1000));
}

TEST(InputParser, DefaultSolverConfig) {
    std::string json = R"({
        "objective": {
            "sense": "minimize",
            "coefficients": [{"num": 1, "den": 1}]
        },
        "constraints": [
            {
                "coefficients": [{"num": 1, "den": 1}],
                "sense": ">=",
                "rhs": {"num": 5, "den": 1}
            }
        ]
    })";

    auto [lp, config] = InputParser::parse(json);

    EXPECT_EQ(lp.objectiveSense(), LinearProgram::ObjectiveSense::Minimize);
    EXPECT_EQ(config.method, "simplex");
    EXPECT_EQ(config.basisMethod, "big-m");
    EXPECT_EQ(config.bigM, Fraction(1000));
}

TEST(InputParser, FractionalCoefficients) {
    std::string json = R"({
        "objective": {
            "sense": "maximize",
            "coefficients": [{"num": 1, "den": 3}]
        },
        "constraints": [
            {
                "coefficients": [{"num": 2, "den": 5}],
                "sense": "<=",
                "rhs": {"num": 7, "den": 2}
            }
        ]
    })";

    auto [lp, config] = InputParser::parse(json);
    EXPECT_EQ(lp.objectiveCoefficients()[0], Fraction(1, 3));
    EXPECT_EQ(lp.constraintMatrix()(0, 0), Fraction(2, 5));
    EXPECT_EQ(lp.rightHandSide()[0], Fraction(7, 2));
}

TEST(InputParser, MalformedJsonThrows) {
    EXPECT_THROW(InputParser::parse("not json"), nlohmann::json::parse_error);
}

TEST(InputParser, MissingObjectiveThrows) {
    std::string json = R"({
        "constraints": [
            {
                "coefficients": [{"num": 1, "den": 1}],
                "sense": "<=",
                "rhs": {"num": 10, "den": 1}
            }
        ]
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}

TEST(InputParser, MissingConstraintsThrows) {
    std::string json = R"({
        "objective": {
            "sense": "maximize",
            "coefficients": [{"num": 1, "den": 1}]
        }
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}

TEST(InputParser, EmptyConstraintsThrows) {
    std::string json = R"({
        "objective": {
            "sense": "maximize",
            "coefficients": [{"num": 1, "den": 1}]
        },
        "constraints": []
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}

TEST(InputParser, DimensionMismatchThrows) {
    std::string json = R"({
        "objective": {
            "sense": "maximize",
            "coefficients": [{"num": 1, "den": 1}, {"num": 2, "den": 1}]
        },
        "constraints": [
            {
                "coefficients": [{"num": 1, "den": 1}],
                "sense": "<=",
                "rhs": {"num": 10, "den": 1}
            }
        ]
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}

TEST(InputParser, ZeroDenominatorThrows) {
    std::string json = R"({
        "objective": {
            "sense": "maximize",
            "coefficients": [{"num": 1, "den": 0}]
        },
        "constraints": [
            {
                "coefficients": [{"num": 1, "den": 1}],
                "sense": "<=",
                "rhs": {"num": 10, "den": 1}
            }
        ]
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}

TEST(InputParser, InvalidSenseThrows) {
    std::string json = R"({
        "objective": {
            "sense": "optimze",
            "coefficients": [{"num": 1, "den": 1}]
        },
        "constraints": [
            {
                "coefficients": [{"num": 1, "den": 1}],
                "sense": "<=",
                "rhs": {"num": 10, "den": 1}
            }
        ]
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}

TEST(InputParser, InvalidConstraintSenseThrows) {
    std::string json = R"({
        "objective": {
            "sense": "maximize",
            "coefficients": [{"num": 1, "den": 1}]
        },
        "constraints": [
            {
                "coefficients": [{"num": 1, "den": 1}],
                "sense": "!=",
                "rhs": {"num": 10, "den": 1}
            }
        ]
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}

TEST(InputParser, EmptyObjectThrows) {
    EXPECT_THROW(InputParser::parse("{}"), std::invalid_argument);
}

TEST(InputParser, EmptyObjectiveCoefficientsThrows) {
    std::string json = R"({
        "objective": {
            "sense": "maximize",
            "coefficients": []
        },
        "constraints": [
            {
                "coefficients": [],
                "sense": "<=",
                "rhs": {"num": 10, "den": 1}
            }
        ]
    })";
    EXPECT_THROW(InputParser::parse(json), std::invalid_argument);
}
