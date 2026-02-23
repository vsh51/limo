#include <limo/core/Result.hpp>
#include <limo/core/LinearProgram.hpp>
#include <vector>
#include <cstddef>
#pragma once

namespace limo {
namespace basis {

using LinearProgram = limo::core::LinearProgram;
using Matrix = limo::core::LinearProgram::Matrix;
using value_type = limo::core::LinearProgram::value_type;
using ConstraintSense = limo::core::LinearProgram::ConstraintSense;
using ObjectiveSense = limo::core::LinearProgram::ObjectiveSense;

class BasisFinder {
public:
    virtual ~BasisFinder() = default;
    virtual limo::core::Result build(const limo::core::LinearProgram& linearProgram) const = 0;
};

} // namespace basis
} // namespace limo


