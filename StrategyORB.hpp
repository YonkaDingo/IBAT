#pragma once
#include <IStrategy.hpp>
#include <INDICATORS.hpp>
#include <FILTERS.hpp>
#include <NORMALIZERS.hpp>

#include <LossFunctions.hpp>

class StrategyORB final : public IStrategy {
public:
    StrategyORB() {
        generateCollapsed = true;
    }

    void warmUp(int sym, bool backtest, long long startTime) override;
    void processData(Bar Price, int sym) override;
    void price_plotFirst(double t0, double tN, std::vector<Bar> &data, int sym) override;
    void price_plotUniques(double t0, double tN, std::vector<Bar> &data, int sym) override;
    void volume_plotUniques(double t0, double tN, std::vector<Bar> &data, int sym) override;
    bool validateTrainingData(int sym) override;
    void newDay(const Bar &Price, int sym) override;
    void infer(int sym, BTester* Tester) override;


    //STRATEGY ORB:
    INTEGRATE(OR, long, 32)
    double RR = 1.5;
    double atrMult = 4;
    double volMult = 1;
    double exitAfter = 1;
    double stopTrading = 2;

    INDICATOR(ATR, indi::ATR, false)
    INDICATOR(VWAP, indi::VWAP, true)
    FILTER(VOLUME5, filt::RollingVolume, 5)

    INTEGRATE(firstTime, long long)
    INTEGRATE(openingHighs, double, 0)
    INTEGRATE(openingLows, double, INFINITY)
    INTEGRATE(avgORVol, double, 1)
    INTEGRATE(excursion, double, 0)

//MIND:
    SEQUENCE_FEATURE(OpenRel, Robust)
    SEQUENCE_FEATURE(CloseRel, Robust)
    SEQUENCE_FEATURE(HighRel, Robust)
    SEQUENCE_FEATURE(LowRel, Robust)
    SEQUENCE_FEATURE(VWAPDist, Robust)
    SEQUENCE_FEATURE(Volume, Robust)
    SEQUENCE_FEATURE(Volatility, Robust)
    SEQUENCE_FEATURE(PosInRange, None)
    SEQUENCE_FEATURE(SinTime, None)
    SEQUENCE_FEATURE(CosTime, None)
    SEQUENCE_FEATURE(ORFinished, None)

    float thresh = 0.9;
    LABEL(Success1, float, None, BCELossFn, linearSigHd)

    EMBEDDED(Symbol)
};