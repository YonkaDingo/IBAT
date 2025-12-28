#pragma once

#include <IFilter.hpp>
#include <PerSymbol.hpp>
#include <Calc.hpp>

namespace filt {
    struct RollingVolume final : IFilter {
        explicit RollingVolume(int period);
        void init() override;
        void setThresh(long double t, int sym) override;
        void step(const Bar &b, int sym) override;
        bool validate(int sym) override;
        void warmUp(std::vector<Bar> &data, int sym) override;
        long double getValue(int sym) override;

        void ResetAll() override;

        PerSymbol<long double> thresh;
        PerSymbol<Calc::RollingSum> rollingVols{Calc::RollingSum(5)};
    };

    struct NR7 final : IFilter {
        explicit NR7();
        bool validate(int sym) override;
        void step(const Bar &b, int sym) override;
        void warmUp(std::vector<Bar> &data, int sym) override;

        void ResetAll() override;

        PerSymbol<std::deque<double>> ranges;
        PerSymbol<Date> currentDates{Date(0)};
        PerSymbol<std::pair<double, double>> highLows;
    };
}