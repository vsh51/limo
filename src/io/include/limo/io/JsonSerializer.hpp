#pragma once

#include <nlohmann/json.hpp>

#include "limo/numerics/Fraction.hpp"
#include "limo/numerics/Matrix.hpp"
#include "limo/core/LinearProgram.hpp"
#include "limo/core/Result.hpp"
#include "limo/core/Solution.hpp"
#include "limo/io/SimplexTableau.hpp"

/**
 * @file JsonSerializer.hpp
 * @brief nlohmann::adl_serializer specializations for all LIMO types.
 *
 * Uses explicit adl_serializer specializations rather than ADL free
 * functions so that container-like types (Matrix) are not mistakenly
 * treated as JSON arrays by nlohmann's type detection.
 *
 * @author Volodymyr Shpyrka
 */
namespace nlohmann {

template<>
struct adl_serializer<limo::numerics::fraction::Fraction> {
    static void to_json(json& j, const limo::numerics::fraction::Fraction& f);
    static void from_json(const json& j, limo::numerics::fraction::Fraction& f);
};

template<>
struct adl_serializer<limo::numerics::Matrix<limo::numerics::fraction::Fraction>> {
    static void to_json(json& j, const limo::numerics::Matrix<limo::numerics::fraction::Fraction>& m);
    static void from_json(const json& j, limo::numerics::Matrix<limo::numerics::fraction::Fraction>& m);
};

template<>
struct adl_serializer<limo::core::LinearProgram::ObjectiveSense> {
    static void to_json(json& j, const limo::core::LinearProgram::ObjectiveSense& s);
    static void from_json(const json& j, limo::core::LinearProgram::ObjectiveSense& s);
};

template<>
struct adl_serializer<limo::core::LinearProgram::ConstraintSense> {
    static void to_json(json& j, const limo::core::LinearProgram::ConstraintSense& s);
    static void from_json(const json& j, limo::core::LinearProgram::ConstraintSense& s);
};

template<>
struct adl_serializer<limo::core::LinearProgram> {
    static void to_json(json& j, const limo::core::LinearProgram& lp);
    static void from_json(const json& j, limo::core::LinearProgram& lp);
};

template<>
struct adl_serializer<limo::core::Result> {
    static void to_json(json& j, const limo::core::Result& r);
    static void from_json(const json& j, limo::core::Result& r);
};

template<>
struct adl_serializer<limo::io::SimplexTableau> {
    static void to_json(json& j, const limo::io::SimplexTableau& t);
    static void from_json(const json& j, limo::io::SimplexTableau& t);
};

template<>
struct adl_serializer<limo::core::Solution> {
    static void to_json(json& j, const limo::core::Solution& s);
    static void from_json(const json& j, limo::core::Solution& s);
};

} // namespace nlohmann
