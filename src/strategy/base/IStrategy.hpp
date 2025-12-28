#pragma once

#include <barrier>

#include <BOT.hpp>
#include <Calc.hpp>
#include <PerSymbol.hpp>

#include <IFilter.hpp>
#include <IIndicator.hpp>

#include <Integrators.hpp>
#include <Interfaces.hpp>

#include <TensorForge.hpp>
#include <Temporal.hpp>

using enum PositionStatus;

class IStrategy {
public:
    IStrategy();
    virtual ~IStrategy() = default;

    bool backtest = false;
    bool Synchronize = false;
    PerSymbol<bool> NoSeq{false};

    bool simSlippage = false;
    double slippage = 0.05;
    bool simCommission = false;
    double commission = 0.35;

    bool symNorm = false;
    bool rollNorm = false;
    int windowLength = 10;
    bool GenerateCollapsed = false;
    static constexpr size_t NumCollapsed = 4;

    Date SyncDate = Date(0);

    //& Default PerSymbol members
    PerSymbol<Date> CurrentDate{Date(0)};
    PerSymbol<Position> Positions;
    PerSymbol<int64_t> FirstTime;
    PerSymbol<Bar const*> PrevBar;
    PerSymbol<std::vector<Bar const*>> PrevBars;
    PerSymbol<bool> Warming;
    PerSymbol<int64_t> StartTime;

private:
    PerSymbol<DataStatus> TensorStatus{DataStatus::Empty};

    bool liveInference = false;
    bool buildDataset = false;

    [[nodiscard]] bool FiltValidate(int sym) const;

    void FiltStep(const Bar &b, int sym) const;
    void FiltWarmUp(std::vector<Bar> &data, int sym) const;
    void IndiStep(const Bar &b, int sym) const;
    void IndiWarmUp(std::vector<Bar> &data, int sym) const;

    void UpdateDate(const Date &d, int sym);

public:
/*  *** STRAT PUBLIC */
    void(*callback)() noexcept = []() noexcept -> void {BOT->DayEnd();};
    Portfolio* Port = nullptr;
    std::shared_ptr<std::barrier<void(*)() noexcept>> SyncPoint = nullptr;

//& Core methods
    void CLEAR() const;
    void Reset();
    void SetVar(std::string var, std::string val) const;
    void PrintVarDefs() const;
    void Save(const std::string &file) const;
    void Load(const std::string &file) const;
    void WarmUp(int sym, int64_t startTime = 0, duckdb::Connection *con = nullptr, bool Eval = false);
    void ProcessBar(const Bar &Price, int sym);
    void ProcessCharting(const Bar &Price, int sym);
    void ClearCharting();
    void ReadData(int threadIndex) const;
    static void StartUpdateReader();

//& Charting hooks
    virtual void pricePlotFirst(double t0, double tN, std::vector<Bar> &data, int sym) {}
    virtual void pricePlot(double t0, double tN, std::vector<Bar> &data, int sym) {}
    virtual void volumePlot(double t0, double tN, std::vector<Bar> &data, int sym) {}

//& Getters
    [[nodiscard]] double GetCostBasis(const int sym) {
        return Positions[sym].enterPrice;
    }
    [[nodiscard]] double GetPNL(const int sym) {
        return Positions[sym].unrealizedPNL;
    }
    [[nodiscard]] bool ShouldExit(const int sym, const Bar &Price) {
        return Positions[sym].CheckExit(Price);
    }
    [[nodiscard]] bool IsFree(const int sym) const {
        return Positions[sym].GetStatus() == FREE;
    }
    [[nodiscard]] bool IsHeld(const int sym) const {
        return Positions[sym].GetStatus() == LONG_HELD || Positions[sym].GetStatus() == SHORT_HELD;
    }
    [[nodiscard]] bool IsBlocked(const int sym) const {
        return Positions[sym].GetStatus() == BLOCKED;
    }
    [[nodiscard]] int GetPeriod() const {
        return Period;
    }
    [[nodiscard]] int64_t GetStartTime(const int sym) const {
        return StartTime[sym];
    }

    std::vector<IIndicator*> &Indicators() {return IndicatorList;}
    std::vector<IFilter*> &Filters() {return FilterList;}

protected:
/*  *** STRAT PROTECTED */
//& Virtual methods with base-wrapped hooks
    virtual void start(int sym) {}
    virtual void newDay(const Bar &Price, int sym) {}
    virtual void processBar(const Bar &Price, int Sym) = 0;

//& Position
    void LongSignal(int sym, double mag);
    void ShortSignal(int sym, double mag);
    void Exit(int sym, const Bar &Price, bool timeStop = false);
    void consumePNL(int sym, double &pnl);

    void Block(int sym);

    void Free(int sym);

    //& Lists
    std::vector<IPerSymbol*> MasterList{};
    std::vector<IPerSymbol*> DailyList{};
    std::vector<IPerSymbol*> IntegrationList{};
    std::vector<IPerSymbol*> MetricList{};

    std::vector<IIndicator*> IndicatorList{};
    std::vector<IFilter*> FilterList{};

//& Default period for filter/indicator calculations in bars
    int Period = 14;

public:
    /*  *** ML PUBLIC */
    //& Model
    Temporal Mind = nullptr;

    //& Inference/dataset sequence management
    void Running();
    void Building();
    void InitHandlers();

    [[nodiscard]] bool IsRunning() const {return liveInference;}
    [[nodiscard]] bool IsBuilding() const {return buildDataset;}
    [[nodiscard]] bool IsWarming(const int sym) const {return Warming[sym];}

//? Symbol forges are used during building and running, static and rolling norm.
    PerSymbol<TensorForge> Sym_SeqForges{TensorForge(this)};

//? Global forge is only used for building global static norm stats.
    TensorForge G_SeqForge{this};

    //& Getters
    [[nodiscard]] DataStatus SampleStatus(const int sym) const {
        return TensorStatus[sym];
    }
    [[nodiscard]] int MaxSteps() const {
        return maxSteps;
    }
    [[nodiscard]] int MaxSeqs() const {
        return maxSeqPerDay;
    }

    void PrintStats(int sym) const;

    std::vector<ISequenceFeature*> &Features() {return FeatureList;}
    std::vector<ILabel*> &Labels() {return LabelList;}
    std::vector<PerSymbol<int64_t>*> &Embeddings() {return EmbeddingList;}

    //& Model output modules
    void SaveMods(const std::string &file) const;

protected:
    /*  *** ML PROTECTED */
    //& Sequence Building
    DataStatus TensorsReady(int sym);
    void PushSampleStep(int sym);
    void TrimSequence(int sym, int64_t length, bool fromBegin = true);
    void ClearSequence(int sym);
    void GetLiveSeq(int sym);
    void GetDatasetSeq(int sym);
    virtual DataStatus validateSequence(int sym) {return DataStatus::Finished;}

    //& Default maximums for tensor allocations
    int maxSteps = 390;
    int maxSeqPerDay = 32;

    //& Lists
    std::vector<ISequenceFeature*> FeatureList{};
    std::vector<ILabel*> LabelList{};
    std::vector<PerSymbol<int64_t>*> EmbeddingList{};

//& Internally registered member types
    template<typename T>
    struct MemberBase : PerSymbol<T> {
        explicit MemberBase(IStrategy *strat, const T &def = T())
            : PerSymbol<T>(def) {strat->MasterList.push_back(this);}
    };

    template<typename T>
    struct TIntegrated final : MemberBase<T> {
        explicit TIntegrated(IStrategy *strat, const T &def = T())
            : MemberBase<T>(strat, def) {strat->IntegrationList.push_back(this);}
    };

    template<typename T>
    struct TDaily final : MemberBase<T> {
        explicit TDaily(IStrategy *strat, const T &def = T())
            : MemberBase<T>(strat, def) {strat->DailyList.push_back(this);}
    };

    template<typename T>
    struct TMetric final : MemberBase<T> {
        explicit TMetric(IStrategy *strat, const T &def = T())
            : MemberBase<T>(strat, def) {strat->MetricList.push_back(this);}
    };

    struct SequenceFeature final : ISequenceFeature, MemberBase<std::vector<float>> {
        explicit SequenceFeature(IStrategy *strat, const std::vector<float> &def = std::vector<float>())
            : MemberBase(strat, def) {strat->FeatureList.push_back(this);}
        std::vector<float> &Get(const int sym) override {return (*this)[sym];}
        void pushStep(const int sym) override {
            (*this)[sym].push_back(step[sym]);
            step.ResetSym(sym);
        }
        PerSymbol<float> step;
    };

    template<typename T>
    struct TLabel final : ILabel, MemberBase<T> {
        explicit TLabel(IStrategy *strat, const T &def = T())
            : ILabel(typeid(T)), MemberBase<T>(strat, def) {strat->LabelList.push_back(this);}
        void *Get(const int sym) override {return &(*this)[sym];}
    };

    template<typename T>
    struct TClassLabel final : ILabel, MemberBase<T> {
        explicit TClassLabel(IStrategy *strat, const T &def = T())
            : ILabel(typeid(T)), MemberBase<T>(strat, def) {strat->LabelList.push_back(this);}
        void *Get(const int sym) override {return &(*this)[sym];}
    };

    struct Embedded final : MemberBase<int64_t> {
        explicit Embedded(IStrategy *strat, const int64_t def = 0)
            : MemberBase(strat, def) {strat->EmbeddingList.push_back(this);}
    };

public:
//? Performance metrics
    METRIC(DaysRun, int)
    METRIC(Trades, int)
    METRIC(Wins, int)
    METRIC(Losses, int)
    METRIC(TimeWins, int)
    METRIC(TimeLosses, int)
    METRIC(TimeStops, int)
    METRIC(Profit, double)

//? Charting Data
    bool Charting = false;
    const Bar *CurrentBar = nullptr;
    std::vector<double> SeqTimes;
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
};

