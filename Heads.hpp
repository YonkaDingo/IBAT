#pragma once
#include <Abstract.hpp>

struct IHead : Module {
    std::string _type;
    IHead() = default;
    long long out = -1;
    virtual std::string head_name() const { return _type; }
    virtual void init(long long in, long long out) = 0;
    virtual torch::Tensor forward(const torch::Tensor &x) = 0;
};

struct linearImpl final : IHead {
    Linear linear{nullptr};
    void init(long long in, long long out) override {
        linear = register_module("linear", Linear(in, out));
    }
    torch::Tensor forward(const torch::Tensor &x) override {
        return linear->forward(x);
    }
};
TORCH_MODULE(linear);

struct linearSigImpl final : IHead {
    Linear linear{nullptr};
    void init(long long in, long long out) override {
        linear = register_module("linearSig", Linear(in, out));
    }
    torch::Tensor forward(const torch::Tensor &x) override {
        return torch::sigmoid(linear->forward(x));
    }
};
TORCH_MODULE(linearSig);

HEAD(linear)
HEAD(linearSig)