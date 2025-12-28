#pragma once
#include <INormalizer.hpp>

struct None final : INormalizer {
    void Normalize(torch::Tensor e) override {}
    void DeNormalize(torch::Tensor e) override {}
};

struct ZScore final : INormalizer {
    using INormalizer::Stats;

    void Normalize(torch::Tensor e) override {
        torch::sub_out(e, e, Stats.mean);
        torch::div_out(e, e, Stats.std_dev + 1e-7);
    }
    void DeNormalize(torch::Tensor e) override {
        torch::mul_out(e, e, Stats.std_dev + 1e-7);
        torch::add_out(e, e, Stats.mean);
    }
};

struct Robust final : INormalizer {
    using INormalizer::Stats;

    void Normalize(torch::Tensor e) override {
        torch::sub_out(e, e, Stats.median);
        torch::div_out(e, e, (Stats.mad + 1e-7) * 1.4826);
    }
    void DeNormalize(torch::Tensor e) override {
        torch::mul_out(e, e, (Stats.mad + 1e-7) * 1.4826);
        torch::add_out(e, e, Stats.median);
    }
};