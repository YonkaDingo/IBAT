#pragma once
#include <TemporalData.hpp>

struct NormalizationStats final : DataStatsImpl {
    explicit NormalizationStats(const DataStatsImpl &ref) : DataStatsImpl(ref) {}
    NormalizationStats() = default;
    void operator()(const torch::Tensor &_x) {
        const torch::Tensor &x = _x.to(torch::kDouble).contiguous();
        const int64_t xN = x.numel();

        if (xN == 0) return;
        if (medianFixed) return;

        const auto* ptr = x.data_ptr<double>();
        for (int64_t i = 0; i < xN; i++) 
            kllMed.update(ptr[i]);

        const torch::Tensor xMean = x.mean().to(torch::kDouble);
        const torch::Tensor xMin  = x.min().to(torch::kDouble);
        const torch::Tensor xMax  = x.max().to(torch::kDouble);
        const torch::Tensor xM2 = (x - xMean).pow(2).sum().to(torch::kDouble);

        if (N.item<int64_t>() == 0) {
            torch::set_out(mean, mean, xMean);
            torch::set_out(M2, M2, xM2);
            torch::set_out(min_val, min_val, xMin);
            torch::set_out(max_val, max_val, xMax);
            torch::sub_out(range, max_val, min_val);
            torch::set_out(l1_norm, l1_norm, x.norm(1).to(torch::kDouble));
            torch::set_out(l2_norm, l2_norm, x.norm(2).to(torch::kDouble));
            torch::div_out(variance, M2, xN - 1);
            torch::sqrt_out(std_dev, variance);
            N += xN;
            return;
        }

        const auto Nd = static_cast<double>(N.item<int64_t>());
        const auto xNd = static_cast<double>(xN);
        const double total = Nd + xNd;

        const torch::Tensor delta = xMean - mean;
        mean += delta * (xNd / total);
        M2 += xM2 + delta.pow(2) * (Nd * xNd / total);
        l1_norm += x.norm(1);
        l2_norm += x.norm(2);
        N += xN;

        torch::minimum_out(min_val, min_val, xMin);
        torch::maximum_out(max_val, max_val, xMax);
        torch::sub_out(range, max_val, min_val);
        torch::div_out(variance, M2, xN - 1);
        torch::sqrt_out(std_dev, variance);
    }

    void finalizeMedian() {
        if (medianFixed) throw std::runtime_error("Median already finalized");
        const auto Q = torch::tensor(kllMed.get_quantile(0.5)).to(torch::kDouble);
        torch::set_out(median, median, Q);
        medianFixed = true;
    }

    void stepMAD(const torch::Tensor& _x) {
        if (!medianFixed) throw std::runtime_error("Median must be finalized before stepping MAD");
        const torch::Tensor &x = _x.to(torch::kDouble).contiguous();
        const auto* ptr = x.data_ptr<double>();
        const auto med = median.item<double>();
        for (int64_t i = 0; i < x.numel(); i++) {
            double dev = std::abs(ptr[i] - med);
            kllDev.update(dev);
        }
    }

    void finalizeMAD() {
        if (!medianFixed) throw std::runtime_error("Median must be computed before finalizing MAD");
        const auto Q = torch::tensor(kllDev.get_quantile(0.5)).to(torch::kDouble);
        torch::set_out(mad, mad, Q);
    }
};

struct INormalizer {
    virtual ~INormalizer() = default;
    virtual void Normalize(torch::Tensor e) = 0;
    virtual void DeNormalize(torch::Tensor e) = 0;
    NormalizationStats Stats = NormalizationStats();
};