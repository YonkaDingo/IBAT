#pragma once

#include <BOT.hpp>
#include <PerSymbol.hpp>
#include <Position.hpp>

#include <IFilter.hpp>
#include <IIndicator.hpp>

#include <Integrators.hpp>
#include <Interfaces.hpp>

using enum PositionStatus;

class IStrategy {
public:
    IStrategy() = default;
    virtual ~IStrategy() = default;

protected:
    template<typename T>
    struct TSequenceFeature final : ISequenceFeature, PerSymbol<std::vector<T>> {
        explicit TSequenceFeature(IStrategy* strat, const std::vector<T> &instance = std::vector<T>())
        : ISequenceFeature(typeid(T)), PerSymbol<std::vector<T>>(instance)
            {strat->FeatureList.push_back(this);}
        PerSymbol<T> step;

        void pushStep(int sym) override {
            (*this)[sym].push_back(step[sym]);
            step.ResetSym(sym);
        }

        void* Get(int sym, size_t i) override
            {return &(*this)[sym][i];}
        void* Get(int sym) override
            {return &(*this)[sym];}
        size_t GetSize(int sym) override
            {return (*this)[sym].size();}

        void ResetSym(int sym) override
            {PerSymbol<std::vector<T>>::ResetSym(sym);};
        void ResetAll() override
            {PerSymbol<std::vector<T>>::ResetAll();};
    };

    template<typename T>
    struct TLabel final : ILabel, PerSymbol<T> {
        explicit TLabel(IStrategy* strat, const T instance = T())
        : ILabel(typeid(T)), PerSymbol<T>(instance)
            {strat->LabelList.push_back(this);}
        void* Get(int sym) override {return &(*this)[sym];}
        void ResetSym(int sym) override {PerSymbol<T>::ResetSym(sym);};
        void ResetAll() override {PerSymbol<T>::ResetAll();};
    };

    template<typename T>
    struct TClassLabel final : ILabel, PerSymbol<T> {
        explicit TClassLabel(IStrategy* strat, const T& instance = T())
        : ILabel(typeid(T)), PerSymbol<T>(instance)
            {strat->LabelList.push_back(this);}
        void* Get(int sym) override {return &(*this)[sym];}
        void ResetSym(int sym) override {PerSymbol<T>::ResetSym(sym);};
        void ResetAll() override {PerSymbol<T>::ResetAll();};
    };

    struct Embedded final : PerSymbol<int64_t> {
        explicit Embedded(IStrategy* strat, const int64_t instance = 0)
        : PerSymbol(instance)
            {strat->EmbeddingList.push_back(this);}
        std::string eName{};
    };

    template<typename T>
    struct TIntegrated final : PerSymbol<T>, IIntegrated {
        explicit TIntegrated(IStrategy* strat, const T& instance = T())
        : PerSymbol<T>(instance)
            {strat->IntegrationList.push_back(this);}
        void ResetSym(int sym) override {PerSymbol<T>::ResetSym(sym);};
        void ResetAll() override {PerSymbol<T>::ResetAll();};
    };


    Temporal Mind = nullptr;

    // Core Lists
    std::vector<IIntegrated*> IntegrationList{};
    std::vector<IFilter*> FilterList{};
    std::vector<IIndicator*> IndicatorList{};
    // ML Lists
    std::vector<ISequenceFeature*> FeatureList{};
    std::vector<ILabel*> LabelList{};
    std::vector<Embedded*> EmbeddingList{};

    // Default Integrated Members
    INTEGRATE(currentDate, Date, Date(0))
    INTEGRATE(position, Position)
    INTEGRATE(prevBar, Bar)
    INTEGRATE(absTargDist, double, 0)
    INTEGRATE(absStopDist, double, 0)
    INTEGRATE(trainingDataReady, bool)
    INTEGRATE(prevBars, std::vector<Bar>, []{
        std::vector<Bar> v;
        v.reserve(390);
        return v;
    }())

    // Default period for filter/indicator calculations
    int period = 10;

    // Default maximum number of steps in a training data sequence.
    int maxSteps = 390;

public:
    // Flag to toggle generating correlation features
    bool generateCollapsed = false;
    static constexpr size_t numCollapsed = 4;

    // Plumbing
    std::vector<ISequenceFeature*> &Features() {return FeatureList;}
    std::vector<ILabel*> &Labels() {return LabelList;}
    std::vector<Embedded*> &Embeddings() {return EmbeddingList;}

    std::vector<IIndicator*> &Indicators() {return IndicatorList;}
    std::vector<IFilter*> &Filters() {return FilterList;}

    void ResetIntegrations(int sym) const;
    void WarmUp(int sym, bool backtest, long long startTime = 0, duckdb::Connection *con = nullptr, bool Eval = false);
    static void StartForUpdate();
    void ReadData(int threadIndex) const;
    void LongSignal(int sym, const Bar &Price);
    void ShortSignal(int sym, const Bar &Price);
    void Exit(int sym, const Bar &Price, bool timeStop = false);

    // Getters
    void consumePNL(const int sym, double &pnl) {position[sym].consumePNL(pnl);}
    [[nodiscard]] double getCostBasis(const int sym) {return position[sym].costBasis;}
    [[nodiscard]] double getPNL(const int sym) {return position[sym].unrealizedPNL;}
    [[nodiscard]] bool ShouldExit(const int sym, const Bar &Price) {return position[sym].CheckExit(Price);}
    [[nodiscard]] bool IsFree(const int sym) const
        {return position[sym].GetStatus() == FREE;}
    [[nodiscard]] bool IsHeld(const int sym) const
        {return position[sym].GetStatus() == LONG_HELD || position[sym].GetStatus() == SHORT_HELD;}
    [[nodiscard]] bool IsBlocked(const int sym) const
        {return position[sym].GetStatus() == BLOCKED;}
    [[nodiscard]] bool TrainingDataReady(const int sym) const
        {return trainingDataReady[sym];}
    [[nodiscard]] int MaxSteps() const
        {return maxSteps;}
    [[nodiscard]] int Period() const
        {return period;}

    // Setters
    void Block(const int sym)
        {position[sym].Block();}
    bool Free(const int sym)
        {return position[sym].Free();}

    // Virtual Methods
    virtual void newDay(const Bar &Price, int sym) = 0;
    virtual void warmUp(int sym, bool backtest, long long startTime) = 0;
    virtual void processData(Bar Price, int Sym) = 0;
    virtual void price_plotFirst(double t0, double tN, std::vector<Bar> &data, int sym) = 0;
    virtual void price_plotUniques(double t0, double tN, std::vector<Bar> &data, int sym) = 0;
    virtual void volume_plotUniques(double t0, double tN, std::vector<Bar> &data, int sym) = 0;
    virtual bool validateTrainingData(int sym) = 0;
    virtual void infer(int sym, BTester* tester) = 0;

    // Core Methods
    void ProcessBar(const Bar &Price, int sym);
    void processCharting(const Bar &Price, int sym);
    void GetTrainingData(int sym, BTester &tester);
    void ClearSequence(int sym);
    void UpdateDate(const Date &d, int sym);
    void ClearCharting();
    [[nodiscard]] bool filtValidate(int sym) const;

    // Stepping
    void filtStep(const Bar &b, int sym) const;
    void filtWarmUp(std::vector<Bar> &data, int sym) const;
    void indiStep(const Bar &b, int sym) const;
    void indiWarmUp(std::vector<Bar> &data, int sym) const;
    void pushSequenceSteps(int sym) const;

    // Performance Metrics
    INTEGRATE(daysRun, int)
    INTEGRATE(trades, int)
    INTEGRATE(wins, int)
    INTEGRATE(timeStps, int)
    INTEGRATE(profit, double)
    INTEGRATE(currentProfit, double)

    // Charting Data
    int64_t seqStart = 0;
    std::vector<std::string> TradeDirections;
    std::vector<double> timesInPosition;
    std::vector<double> EntryTimes;
    std::vector<double> ExitTimes;
    std::vector<double> EntryPrices;
    std::vector<double> ExitPrices;
    std::vector<double> stops;
    std::vector<double> targets;
    Calc::RollingSum RollVol5 = Calc::RollingSum(5);
    std::vector<double> RollingVols;
    std::vector<double> VolumeBars;
    double highestVol = 0;
    bool Charting = false;
};

