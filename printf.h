#pragma once
#include <concepts>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace sjtu {

using sv_t = std::string_view;

struct format_error : std::exception {
public:
    format_error(const char *msg = "invalid format") : msg(msg) {}
    auto what() const noexcept -> const char * override {
        return msg;
    }

private:
    const char *msg;
};

template <typename Tp>
struct formatter;

// Formatter for string-like types
template <typename StrLike>
    requires(
        std::same_as<StrLike, std::string> ||
        std::same_as<StrLike, std::string_view> ||
        std::same_as<StrLike, char *> ||
        std::same_as<StrLike, const char *>
    )
struct formatter<StrLike> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("s") || fmt.starts_with("_")) {
            return fmt.starts_with("s") ? 1 : 0;
        }
        return 0;
    }
    static auto format_to(std::ostream &os, const StrLike &val) -> void {
        os << static_cast<sv_t>(val);
    }
};

// Formatter for signed integer types
template <typename Int>
    requires std::is_integral_v<Int> && std::is_signed_v<Int>
struct formatter<Int> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("d") || fmt.starts_with("u") || fmt.starts_with("_")) {
            return fmt.starts_with("_") ? 0 : 1;
        }
        return 0;
    }
    static auto format_to(std::ostream &os, const Int &val, char spec = 'd') -> void {
        if (spec == 'd' || spec == '_') {
            os << static_cast<int64_t>(val);
        } else if (spec == 'u') {
            os << static_cast<uint64_t>(val);
        }
    }
};

// Formatter for unsigned integer types
template <typename Int>
    requires std::is_integral_v<Int> && std::is_unsigned_v<Int>
struct formatter<Int> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("d") || fmt.starts_with("u") || fmt.starts_with("_")) {
            return fmt.starts_with("_") ? 0 : 1;
        }
        return 0;
    }
    static auto format_to(std::ostream &os, const Int &val, char spec = 'u') -> void {
        if (spec == 'd') {
            os << static_cast<int64_t>(val);
        } else if (spec == 'u' || spec == '_') {
            os << static_cast<uint64_t>(val);
        }
    }
};

// Formatter for vector types
template <typename T>
struct formatter<std::vector<T>> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("_")) {
            return 0;
        }
        return 0;
    }
    static auto format_to(std::ostream &os, const std::vector<T> &val) -> void {
        os << "[";
        bool first = true;
        for (const auto &elem : val) {
            if (!first) {
                os << ",";
            }
            first = false;
            if constexpr (std::is_signed_v<T> && std::is_integral_v<T>) {
                formatter<T>::format_to(os, elem, '_');
            } else if constexpr (std::is_unsigned_v<T> && std::is_integral_v<T>) {
                formatter<T>::format_to(os, elem, '_');
            } else {
                formatter<T>::format_to(os, elem);
            }
        }
        os << "]";
    }
};

// Simple format string wrapper with compile-time validation
template <typename... Args>
struct format_string {
    consteval format_string(const char *fmt) : fmt_str(fmt) {
        // Basic validation: count format specifiers
        std::size_t spec_count = 0;
        for (std::size_t i = 0; fmt[i] != '\0'; ++i) {
            if (fmt[i] == '%') {
                if (fmt[i + 1] == '%') {
                    i++; // Skip %%
                } else if (fmt[i + 1] != '\0') {
                    spec_count++;
                }
            }
        }
        if (spec_count != sizeof...(Args)) {
            throw format_error{"specifier count mismatch"};
        }
    }
    
    constexpr auto get() const -> const char * {
        return fmt_str;
    }
    
private:
    const char *fmt_str;
};

template <typename... Args>
using format_string_t = format_string<std::decay_t<Args>...>;

// Helper to process one argument
template <typename T>
inline void process_arg(std::ostream &os, const T &arg, char spec) {
    using ArgType = std::decay_t<T>;
    if constexpr (std::is_same_v<ArgType, std::string> ||
                  std::is_same_v<ArgType, std::string_view> ||
                  std::is_same_v<ArgType, char *> ||
                  std::is_same_v<ArgType, const char *>) {
        // Cast to const char* for string literals
        if constexpr (std::is_array_v<T>) {
            formatter<const char *>::format_to(os, static_cast<const char *>(arg));
        } else {
            formatter<ArgType>::format_to(os, arg);
        }
    } else if constexpr (std::is_integral_v<ArgType>) {
        formatter<ArgType>::format_to(os, arg, spec);
    } else if constexpr (requires { typename ArgType::value_type; }) {
        // Vector-like type
        formatter<ArgType>::format_to(os, arg);
    }
}

template <typename... Args>
inline auto printf(format_string_t<Args...> fmt, const Args &...args) -> void {
    const char *format_str = fmt.get();
    std::size_t arg_idx = 0;
    std::size_t i = 0;
    
    auto format_one = [&](const auto &arg) {
        // Print up to next specifier
        while (format_str[i] != '\0') {
            if (format_str[i] == '%') {
                if (format_str[i + 1] == '%') {
                    std::cout << '%';
                    i += 2;
                } else {
                    // Found specifier
                    i++; // Skip '%'
                    char spec = format_str[i];
                    if (spec != '\0') {
                        i++; // Skip specifier char
                    }
                    process_arg(std::cout, arg, spec);
                    arg_idx++;
                    return;
                }
            } else {
                std::cout << format_str[i];
                i++;
            }
        }
    };
    
    (format_one(args), ...);
    
    // Print remaining string
    while (format_str[i] != '\0') {
        if (format_str[i] == '%' && format_str[i + 1] == '%') {
            std::cout << '%';
            i += 2;
        } else {
            std::cout << format_str[i];
            i++;
        }
    }
}

} // namespace sjtu
