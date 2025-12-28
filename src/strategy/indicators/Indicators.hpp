#pragma once
#include <IIndicator.hpp>
#include <PerSymbol.hpp>

namespace indi {
    struct ATR final : IIndicator {
        explicit ATR(int period = 0);

        void step(const Bar &b, int sym, bool charting) override;
        void warmUp(std::vector<Bar> &data, int sym) override;

        double getValue(int sym) override;

        void ResetAll() override;

        PerSymbol<double> prevCloses;
        PerSymbol<double> atrValues;
    };

    struct VWAP final : IIndicator {
        explicit VWAP(int period = 0);

        void step(const Bar &b, int sym, bool charting) override;

        double getValue(int sym) override;

        void ResetAll() override;
        void ResetSym(int sym) override;

        PerSymbol<double> cumuPriceVol;
        PerSymbol<double> cumuVol;
    };
}
