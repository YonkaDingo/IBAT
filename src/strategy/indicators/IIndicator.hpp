#pragma once

#include <BOT.hpp>
#include <Calc.hpp>

template<typename T>
struct PerSymbol;

struct IIndicator {
    const bool Daily;
    const bool Chart;
    int Period;
    std::vector<double> chartVals;
    std::string name;

    explicit IIndicator(bool daily, bool chart, int period = 0);
    virtual ~IIndicator() = default;

    IIndicator* Init(IStrategy *strat, const std::string &name);

    virtual void step(const Bar &b, int sym, bool charting = false) = 0;
    virtual void ResetAll() = 0;
    virtual void ResetSym(int sym) {}
    virtual void warmUp(std::vector<Bar> &data, int sym) {}

    virtual double getValue(int sym) { return 0; }
};
