#pragma once

#include <Serial.hpp>

template <typename T>
constexpr bool is_bool = std::is_same_v<T,bool>;

template <typename T>
constexpr bool is_bool_std_vector = std::is_same_v<T,std::vector<bool>>;

template<typename T>
struct parseable : std::false_type {};

template<>
struct parseable<double> : std::true_type {};
template<>
struct parseable<long> : std::true_type {};

struct IPerSymbol {
    virtual ~IPerSymbol() = default;
    virtual [[nodiscard]] size_t size() const = 0;
    virtual [[nodiscard]] size_t size(int sym) const = 0;
    virtual void Set(const std::string &val) = 0;
    virtual void ResetSym(int sym) = 0;
    virtual void ResetAll() = 0;
    virtual void PrintDef() = 0;
    virtual void Save(BinWriter& w) const {}
    virtual void Load(BinReader& r) {}
    std::string Name() const {return nm;}
    std::string nm;
};

template<typename T>
struct PerSymbol : virtual IPerSymbol {
    typedef T Type;

    explicit PerSymbol(const T &defInstance = T())
        : def(defInstance) {vals.resize(SYMBOLS.size(), defInstance);}

//! decltype(auto) is required here for compatability with std::vector<bool> proxy references.
    decltype(auto) operator[](size_t i) { return vals[i]; }
    decltype(auto) operator[](size_t i) const { return vals[i]; }

    typename std::vector<T>::iterator begin() {return vals.begin();}
    typename std::vector<T>::iterator end() {return vals.end();}

    [[nodiscard]] size_t size() const override { return vals.size(); }
    [[nodiscard]] size_t size(int sym) const override {
        if constexpr (is_std_vector<T>::value) return vals[sym].size();
        return 1;
    }

    void ResetSym(int sym) override {vals[sym] = def;}
    void ResetAll() override {std::fill(vals.begin(), vals.end(), def);}

    void Set(const std::string &val) override {
        if constexpr (parseable<T>::value) {
            T arg = parseArg<T>(val);
            SetDef(arg);
        }
    }

    void PrintDef() override {
        auto out = ibat::sout;
        out <<PURPLE<< nm;
        if constexpr (parseable<T>::value) {
            out << " DEF: " << def;
        }
        out << RES << std::endl;
    }

    void SetDef(const T& defInstance) {def = defInstance; ResetAll();}
    T& GetDef() {return def;}

private:
    std::vector<T> vals;
    T def;

public:
    void Save(BinWriter &w) const {
        w.val<uint32_t>(static_cast<uint32_t>(vals.size()));
        w.writeValue(def);
        for (const T& v : vals) {
            w.writeValue(v);
        }
    }

    void Load(BinReader &r) {
        uint32_t count;
        r.val(count);
        r.readValue(def);
        vals.resize(count, def);
        for (auto&& v : vals) {
            if constexpr (is_bool<T> || is_bool_std_vector<T>) {
                T tmp;
                r.readValue(tmp);
                v = tmp;
            } else {
                r.readValue(v);
            }
        }
    }
};