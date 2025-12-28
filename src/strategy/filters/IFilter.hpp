#pragma once

#include <BOT.hpp>

struct IFilter {
    int Period;

    explicit IFilter(int period = 0);
    virtual ~IFilter() = default;

    IFilter* Init(IStrategy *strat);
    virtual void init() {}

    virtual bool validate(int sym) = 0;
    virtual void step(const Bar &b, int sym) = 0;
    virtual void ResetAll() = 0;
    virtual void ResetSym() {};
    virtual void warmUp(std::vector<Bar> &data, int sym) = 0;

    virtual void setThresh(long double t, int sym) {}
    virtual long double getValue(int sym) { return 0; }
};
