#pragma once
#include <Abstract.hpp>

struct ILoss : Module {
    std::string _type;
    ILoss() = default;
    virtual std::string loss_name() const { return _type; }
    virtual torch::Tensor forward(const torch::Tensor &x, const torch::Tensor &y) = 0;
};

struct CELossImpl final : ILoss {
    CrossEntropyLoss ce{};
    torch::Tensor forward(const torch::Tensor& x, const torch::Tensor& y) override {
        return ce->forward(x, torch::squeeze(y, 1));
    }
};
TORCH_MODULE(CELoss);

LOSS(MSELoss)
LOSS(SmoothL1Loss)
LOSS(L1Loss)
LOSS(BCELoss)
LOSS(CELoss)
LOSS(HuberLoss)

