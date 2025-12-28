#pragma once

/// Integrates a member as a PerSymbol<T> object into IntegrationList for locality and management.
/// @param name Name of the member that will be integrated.
/// @param type Type of the member that will be integrated.
/// @param ... Optional default object instance to be constructed.
#define INTEGRATE(name, type, ...) \
    TIntegrated<type> name = [&] { \
        TIntegrated<type> i(this __VA_OPT__(, __VA_ARGS__)); \
        i.nm = #name; \
        return i; \
    }();

#define DAILY(name, type, ...) \
        TDaily<type> name = [&] { \
        TDaily<type> d(this __VA_OPT__(, __VA_ARGS__)); \
        d.nm = #name; \
        return d; \
    }();

#define METRIC(name, type, ...) \
        TMetric<type> name = [&] { \
        TMetric<type> m(this __VA_OPT__(, __VA_ARGS__)); \
        m.nm = #name; \
        return m; \
    }();

/// Integrates a member as a PerSymbol<std::vector<float>> object to represent a sequence feature into training data processing.
/// @param name Name of the member that will be integrated.
/// @param norm INormalizer derived type to be used for per-feature normalization.
#define SEQUENCE_FEATURE(name, norm) \
    norm name##_norm{}; \
    PerSymbol<norm> name##_sym_norm{}; \
    PerSymbol<INormalizer*> name##_i_sym_norm{}; \
    std::vector<norm> name##_coll_norm{this->NumCollapsed}; \
    PerSymbol<std::vector<norm>> name##_sym_coll_norm{std::vector<norm>{this->NumCollapsed}}; \
    PerSymbol<std::vector<INormalizer*>> name##_i_sym_coll_norm{std::vector<INormalizer*>{this->NumCollapsed}}; \
    SequenceFeature name = [&]{ \
        std::vector<float> defVec; \
        defVec.reserve(this->maxSteps); \
        SequenceFeature f(this, defVec); \
        for(int s = 0; s < name##_sym_norm.size(); ++s) \
            name##_i_sym_norm[s] = &name##_sym_norm[s]; \
        f.NormalizerG = &name##_norm; \
        f.NormalizersSym = &name##_i_sym_norm; \
        for(int s = 0; s < name##_sym_coll_norm.size(); ++s) \
            for(int c = 0; c < this->NumCollapsed; ++c) \
                name##_i_sym_coll_norm[s][c] = &name##_sym_coll_norm[s][c]; \
        for (auto &cn : name##_coll_norm) \
            f.CollapsedNormalizersG.push_back(&cn); \
        f.CollapsedNormalizersSym = &name##_i_sym_coll_norm; \
        f.nm = #name; \
        return f; \
    }();

/// Integrates a member as a PerSymbol<T> object to represent a sequence label into training data processing.
/// @param name Name of the member that will be integrated.
/// @param type Primitive data type to be used in the label.
/// @param norm INormalizer derived type to be used for per-label normalization.
/// @param loss torch::nn::Module wrapped loss function to be used as the loss function in training.
/// @param head IHead derived module to be used as the associated output head.
/// @param ... Optional default value to be used.
#define LABEL(name, type, norm, loss, head,  ...) \
    norm name##_norm; \
    PerSymbol<norm> name##_sym_norm{}; \
    PerSymbol<INormalizer*> name##_i_sym_norm{}; \
    TLabel<type> name = [&]{ \
        std::shared_ptr<IHead> hd = head().ptr(); hd->out = 1; \
        TLabel<type> l(this __VA_OPT__(, __VA_ARGS__)); \
        l.mods = std::tuple(hd, loss().ptr()); \
        for(int s = 0; s < name##_sym_norm.size(); ++s) \
            name##_i_sym_norm[s] = &name##_sym_norm[s]; \
        l.NormalizerG = &name##_norm; \
        l.NormalizersSym = &name##_i_sym_norm; \
        l.nm = #name; \
        return l; \
    }();

/// Integrates a member as a PerSymbol<T> object to represent a sequence label associated with an output of classes into training data processing.
/// @param name Name of the member that will be integrated.
/// @param type Primitive data type to be used in the label.
/// @param loss torch::nn::Module wrapped loss function to be used as the loss function in training.
/// @param head IHead derived module to be used as the associated output head.
/// @param nCats Number of possible output classes.
/// @param ... Optional default value to be used.
#define CLASS_LABEL(name, type, loss, head, nCats, ...) \
    None name##_norm; \
    PerSymbol<None> name##_sym_norm{}; \
    PerSymbol<INormalizer*> name##_i_sym_norm{}; \
    TClassLabel<type> name = [&]{ \
        std::shared_ptr<IHead> hd = head().ptr(); hd->out = nCats; \
        TClassLabel<type> l(this __VA_OPT__(, __VA_ARGS__)); \
        l.mods = std::tuple(hd, loss().ptr()); \
        for(int s = 0; s < name##_sym_norm.size(); ++s) \
            name##_i_sym_norm[s] = &name##_sym_norm[s]; \
        l.NormalizerG = &name##_norm; \
        l.NormalizersSym = &name##_i_sym_norm; \
        l.nm = #name; \
        return l; \
    }();

#define EMBEDDED(name, ...) \
    Embedded name = [&]{ \
        Embedded e(this __VA_OPT__(, __VA_ARGS__)); \
        e.nm = #name; \
        return e; \
    }();

/// Integrates an IFilter derived member into strategy filter processing.
/// @param name Name of the IFilter* member that will be integrated.
/// @param filter IFilter derived type to be integrated.
/// @param ... Optional arguments to be passed to the derived filter's constructor.
#define FILTER(name, filter, ...) \
    std::unique_ptr<filter> name##_obj = std::make_unique<filter>(__VA_ARGS__); \
    IFilter* name = name##_obj->Init(this);

/// Integrates an IIndicator derived member into strategy indicator processing.
/// @param name Name of the IIndicator* member that will be integrated.
/// @param indicator IIndicator derived type to be integrated.
/// @param chart Boolean value whether to display the indicator on the chart.
/// @param ... Optional arguments to be passed to the derived indicator's constructor.
#define INDICATOR(name, indicator, ...) \
    std::unique_ptr<indicator> name##_obj = std::make_unique<indicator>(__VA_ARGS__); \
    IIndicator* name = name##_obj->Init(this, #name);