#pragma once

#include <EWrapperAdapter.hpp>

#include <Shell.hpp>
#include <Portfolio.hpp>
#include <DBI.hpp>
#include <BTester.hpp>

class IStrategy;
enum class StrategyType;

class Executor final : public EWrapperAdapter {
public:
    explicit Executor();
    ~Executor() override;

    static void fitRun(int epochs = 10, int batchSize = 64, float lr = 1e-4);

    // Setup
    bool Setup();
    void setClient(EClientSocket* client);
    static std::optional<StrategyType> GetStrat();
    void SetStrat(StrategyType s);
    void RegisterCommands();

    // Disconnect
    void JoinClient();

    int NUM_READERS = 1;

    // Core Components
    DBI dbI;
    ImGuiTerminalShell terminal;
    std::shared_ptr<BTester> tester = nullptr;
    std::shared_ptr<IStrategy> strat = nullptr;
    Portfolio Port;
    double backtestBal = 1000;
    bool EOD = false;

    // TWS Components
    EClientSocket* m_client;
    std::unique_ptr<EReader> reader;
    std::unique_ptr<ReaderSignal> readerSignal;
    std::unique_ptr<EClientSocket> client;
    std::thread readerThread;
    std::error_code ec;
    OrderId m_orderId;

    int numDone = 0;

    // Threading and synchronization
    std::vector<std::thread> threads;
    std::queue<std::pair<int,Bar>> dataQueue;
    std::mutex enterTex;
    std::mutex m_mutex;
    std::mutex dataEndMutex;
    std::mutex logtex;
    std::condition_variable cv;
    std::condition_variable cv_dataEnd;
    std::condition_variable cv_dataWritten;

    bool done = false;
    bool historicalDataDone = false;
    bool historicalDataWritten = false;

    void Start(bool eval);
    void DayEnd() noexcept;

    void Evaluate(bool eval = false, bool noLog = false);
    void backTestViz(int sym, long startTime = 0, bool eval = false);

    void updateTensors();
    void generateTensors(bool eval = false);

    void syncDays(bool sync) const;
    void simSlip(bool sim) const;
    void simComm(bool sim) const;

    void printNorms() const;

    // * Market data *
    void reqBars(const std::string &end, const std::string &dur, int Sym) const;
    void ExtendHistoricalData(int RequestSizeYears, long long Cutoff = 0);
    void updateAllHistoricalData(bool sort = false, bool verify = false);
    void recHistoricalDataForUpdate();
    void updateParquets() const;
    void verifyDataIntegrity();

    // * Feature validation *
    void showCollCorr(int64_t Label = 0, bool eval = false, int batch = 0) const;
    void showAutoCorr(int64_t Feat = 0, bool eval = false, int batch = 0) const;
    void validateFeatsRidge(int64_t Label = 0) const;

private:
    // * EWrapper interface *
    void currentTime(long time) override;
    void nextValidId(OrderId orderId) override;
    void tickPrice(TickerId tickerId, TickType field, double price, 
                  const TickAttrib& attrib) override;
    void historicalData(TickerId reqId, const Bar& bar) override;
    void historicalDataEnd(int reqId, const std::string& startDateStr,
                          const std::string& endDateStr) override;
    void accountSummary(int reqId, const std::string& account,
                       const std::string& tag, const std::string& value,
                       const std::string& currency) override;
    void accountSummaryEnd(int reqId) override;
    void error(int id, time_t errorTime, int errorCode, 
               const std::string& errorString, 
               const std::string& advancedOrderRejectJson) override;
    void connectionClosed() override;
};
