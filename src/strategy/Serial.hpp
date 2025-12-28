#pragma once

#include <BOT.hpp>

template<typename T>
struct is_std_vector : std::false_type{};

template<typename T, typename A>
struct is_std_vector<std::vector<T,A>> : std::true_type{};

struct BinWriter {
    std::ofstream out;
    explicit BinWriter(const std::string& path)
        : out(path, std::ios::binary) {}

    template<typename T>
    void val(const T& v) {
        out.write(reinterpret_cast<const char*>(&v), sizeof(T));
    }

    void bytes(const void* p, const size_t n) {
        out.write(static_cast<const char*>(p), n);
    }

    void str(const std::string& s) {
        const uint16_t len = static_cast<uint16_t>(s.size());
        val(len);
        bytes(s.data(), len);
    }

    template<typename T>
    void writeValue(const T& v) {
        if constexpr (is_std_vector<T>::value) {
            const auto n = static_cast<uint32_t>(v.size());
            val(n);
            bytes(v.data(), n * sizeof(typename T::value_type));
        } else {
            val(v);
        }
    }
};

struct BinReader {
    std::ifstream in;
    explicit BinReader(const std::string& path)
        : in(path, std::ios::binary) {}

    template<typename T>
    void val(T& v) {
        in.read(reinterpret_cast<char*>(&v), sizeof(T));
    }

    void bytes(void* p, const size_t n) {
        in.read(static_cast<char*>(p), n);
    }

    std::string str() {
        uint16_t len;
        val(len);
        std::string s(len, '\0');
        in.read(s.data(), len);
        return s;
    }

    template<typename T>
    void readValue(T& v) {
        if constexpr (is_std_vector<T>::value) {
            uint32_t n;
            val(n);
            v.resize(n);
            bytes(v.data(), n * sizeof(typename T::value_type));
        } else {
            val(v);
        }
    }
};