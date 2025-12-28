#pragma once

enum class StrategyType {
    ORB
};

#include <StrategyORB.hpp>

struct StrategyFactory {
    static std::shared_ptr<IStrategy> create(const StrategyType type) {
        switch (type) {
        case StrategyType::ORB:
            return std::make_shared<StrategyORB>();
        default:
            throw std::runtime_error("StrategyFactory: Unknown StrategyType");
        }
    }
};