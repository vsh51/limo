#pragma once

#include "limo/core/LinearProgram.hpp"
#include <vector>
#include <cstddef>

namespace limo::core {

struct Result {
        std::size_t originalVariableCount;
        std::vector<std::size_t> basisColumns;
        std::vector<std::size_t> slackColumns;
        std::vector<std::size_t> surplusColumns;
        std::vector<std::size_t> artificialColumns;
        limo::core::LinearProgram augmented;
        limo::core::LinearProgram::value_type bigM{};
    };

} // namespace limo::core
