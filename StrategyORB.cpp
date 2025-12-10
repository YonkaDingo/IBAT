#include <StrategyORB.hpp>
#include <implot.h>

void StrategyORB::price_plotFirst(double t0, double tN, std::vector<Bar> &data, const int sym) {
    const double range = openingHighs[sym] - openingLows[sym];
    const double s1l = openingHighs[sym] + (range * thresh);
    const double s1s = openingLows[sym] - (range * thresh);
    const std::vector thresholds1{s1l, s1s};
    const auto col = Colors::Teal;
    ImPlot::SetNextLineStyle(col, 2);
    ImPlot::PlotInfLines("Thresh1", thresholds1.data(), thresholds1.size(), ImPlotInfLinesFlags_Horizontal);
}

void StrategyORB::price_plotUniques(const double t0, const double tN, std::vector<Bar> &data, const int sym) {
    const double high = openingHighs[sym];
    ImPlot::PlotInfLines("OR High", &high, 1, ImPlotInfLinesFlags_Horizontal);

    const double low = openingLows[sym];
    ImPlot::PlotInfLines("OR Low", &low, 1, ImPlotInfLinesFlags_Horizontal);

    const double t = std::stod(data[OR[sym]-1].time);
    ImPlot::PlotInfLines("OR end", &t, 1);

    if (IsBlocked(sym)) {
        const double t2 = position[sym].enterTime + static_cast<long>(60 * 60 * exitAfter);
        const auto col = Colors::Red;
        ImPlot::SetNextLineStyle(col, 2);
        ImPlot::PlotInfLines("Time exit", &t2, 1);
    }

    const double t3 = std::stod(data[stopTrading*60].time);
    ImPlot::PlotInfLines("Trade stop", &t3, 1);
}

void StrategyORB::volume_plotUniques(const double t0, const double tN, std::vector<Bar> &data, const int sym) {
    ImPlot::PlotLine("OR Vol Thresh",
        std::vector{t0, tN}.data(),
        std::vector{avgORVol[sym]/5.0f*volMult, avgORVol[sym]/5.0f*volMult}.data(),
        2
    );
}

bool StrategyORB::validateTrainingData(const int sym) {
    const auto seqSize = FeatureList[0]->GetSize(sym);
    if (seqSize < OR[sym]) {
        return false;
    }

//* graph
    seqStart = 0;

    constexpr size_t max = 90;
    if (seqSize > max) {
        const auto diff = seqSize - max;
        seqStart = diff;
        for (auto* f : FeatureList) {
            auto* vec = static_cast<std::vector<float> *>(f->Get(sym));
            vec->erase(vec->begin(), vec->begin() + diff);
        }
    }

    const auto hit = excursion[sym]/(openingHighs[sym] - openingLows[sym]);

    Success1[sym] = hit >= thresh ? 1.f : 0.f;
    Symbol[sym] = sym;

    excursion.ResetSym(sym);
    trainingDataReady[sym] = false;
    return true;
}


void StrategyORB::newDay(const Bar &Price, const int sym) {
    firstTime[sym] = std::stol(Price.time);
    prevBar.ResetSym(sym);
    Free(sym);
    avgORVol.ResetSym(sym);
    openingHighs.ResetSym(sym);
    openingLows.ResetSym(sym);
    ClearSequence(sym);
}

void StrategyORB::infer(int sym, BTester* Tester) {

}

void StrategyORB::processData(const Bar Price, const int sym) {
    const long long time = std::stol(Price.time);

//* LSTM Features
    const auto prevClose = prevBar[sym].close > 0 ? prevBar[sym].close : Price.open;

    OpenRel.step[sym] = static_cast<float>(std::log(Price.open) - std::log(prevClose));
    CloseRel.step[sym] = static_cast<float>(std::log(Price.close) - std::log(prevClose));
    HighRel.step[sym] = static_cast<float>(std::log(Price.high) - std::log(prevClose));
    LowRel.step[sym] = static_cast<float>(std::log(Price.low) - std::log(prevClose));

    VWAPDist.step[sym] =
        static_cast<float>(((Price.high + Price.low + Price.close) / 3 - VWAP->getValue(sym)) / VWAP->getValue(sym));
    Volume.step[sym] = static_cast<float>(std::log(Price.volume + 1));
    Volatility.step[sym] = static_cast<float>(std::log(ATR->getValue(sym) / VWAP->getValue(sym)));

    auto pos = openingHighs[sym] > 0 && openingHighs[sym] != openingLows[sym] ? static_cast<float>(
        (Price.close - (openingHighs[sym] + openingLows[sym])/2) /
        ((openingHighs[sym] - openingLows[sym])/2)
    ) : 0.f;
    if (pos > 5)
        pos = 5;
    else if (pos < -5)
        pos = -5;
    PosInRange.step[sym] = pos;

    const auto step =
        static_cast<float>(2.0*M_PI * (static_cast<double>(time-firstTime[sym]) / (60*(60*stopTrading - 5))));
    SinTime.step[sym] = std::sin(step);
    CosTime.step[sym] = std::cos(step);
//*

    //?If within the opening time range, log the highest highs and lowest lows
    if (time < firstTime[sym] + 60*OR[sym]) {
        if (Price.high > openingHighs[sym]) openingHighs[sym] = Price.high;
        if (Price.low < openingLows[sym]) openingLows[sym] = Price.low;

        //?Average volume during opening range for checking against during breakout.
        avgORVol[sym] += static_cast<double>(Price.volume) / (OR[sym] / 5.0L);

        ORFinished.step[sym] = 0.f;
        pushSequenceSteps(sym);
    } else if (IsFree(sym) && time <= firstTime[sym] + static_cast<long long>(60 * 60 * stopTrading)) {
        //?If the opening range is defined and position is free, check for breaks out of the opening range
        const double low = openingLows[sym];
        const double high = openingHighs[sym];
        const double rangeMult = (high - low) * 0.2;

        const bool brokeLow = Price.close <= low - rangeMult;
        const bool brokeHigh = Price.close >= high + rangeMult;

        if (brokeLow)
          ShortSignal(sym, Price);
        else if (brokeHigh)
          LongSignal(sym, Price);

        ORFinished.step[sym] = 1.f;
        pushSequenceSteps(sym);
    } else if (IsHeld(sym)) {
        if (position[sym].GetStatus() == LONG_HELD) {
            if (Price.close - openingHighs[sym] > excursion[sym]) excursion[sym] = Price.close - openingHighs[sym];
        } else if (position[sym].GetStatus() == SHORT_HELD) {
            if (openingLows[sym] - Price.close > excursion[sym]) excursion[sym] = openingLows[sym] - Price.close;
        }
        if (time >= position[sym].enterTime + static_cast<long>(60 * 60 * exitAfter)) {
            trainingDataReady[sym] = true;
            Exit(sym, Price, true);
            Block(sym);
        }

        else if (ShouldExit(sym, Price)) {
            Exit(sym, Price);
            Block(sym);
        }

        if (IsBlocked(sym)) {
            double pnl;
            consumePNL(sym, pnl);
        }
    }

    absTargDist[sym] = ATR->getValue(sym) * atrMult * RR;
    absStopDist[sym] = ATR->getValue(sym) * atrMult;
}

void StrategyORB::warmUp(int sym, bool backtest, long long startTime) {

}
