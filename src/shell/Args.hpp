#pragma once

#include <string>

template<typename T>
T parseArg(const std::string& s);

template<>
inline long long parseArg<long long>(const std::string& s) {return std::stoll(s);}

template<>
inline long parseArg<long>(const std::string& s) {return std::stol(s);}

template<>
inline int parseArg<int>(const std::string& s) {return std::stoi(s);}

template<>
inline bool parseArg<bool>(const std::string& s) {return s.find("true") != std::string::npos;}

template<>
inline double parseArg<double>(const std::string& s) {return std::stod(s);}

template<>
inline float parseArg<float>(const std::string& s) {return std::stof(s);}

template<>
inline std::string parseArg<std::string>(const std::string& s) {return s;}

template<typename T>
struct ArgName {static constexpr auto value = static_cast<const char*>("unknown");};

template<>
struct ArgName<long long> {static constexpr auto *value = "long long";};

template<>
struct ArgName<long> {static constexpr auto *value = "long";};

template<>
struct ArgName<int> {static constexpr auto *value = "int";};

template<>
struct ArgName<bool> {static constexpr auto *value = "bool";};

template<>
struct ArgName<double> {static constexpr auto *value = "double";};

template<>
struct ArgName<std::string> {static constexpr auto *value = "string";};

template<>
struct ArgName<const std::string &> {static constexpr auto *value = "string";};

template<typename... Args, std::size_t... I>
    static auto MakeArgs(std::index_sequence<I...>, const std::vector<std::string> &args) {
    return std::tuple<Args...>{(I < args.size() ? parseArg<Args>(args[I]) : Args{})...};
}

template<typename... Args>
std::string buildArgList() {
    std::string s;
    ((s += ArgName<Args>::value, s += ", "), ...);
    if (!s.empty()) s.erase(s.size() - 2);
    return s;
}