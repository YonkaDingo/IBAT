#pragma once

enum class DataStatus {
    Empty,
    WIP,
    Ready,
    Aborted,
    Finished
};

struct INormalizer;

struct ITrainingData : virtual IPerSymbol {
    INormalizer* NormalizerG{};
    PerSymbol<INormalizer*>* NormalizersSym{};
};

struct ISequenceFeature : ITrainingData {
    virtual void pushStep(int sym) = 0;
    virtual std::vector<float> &Get(int sym) = 0;
    std::vector<INormalizer*> CollapsedNormalizersG{};
    PerSymbol<std::vector<INormalizer*>>* CollapsedNormalizersSym{};
};

struct ILabel : ITrainingData {
    explicit ILabel(const std::type_index &t) : Type(t) {}
    virtual void* Get(int sym) = 0;

    std::type_index Type = typeid(void);
    std::tuple<std::shared_ptr<IHead>, std::shared_ptr<ILoss>> mods;
    int LabelSize = 1;
};