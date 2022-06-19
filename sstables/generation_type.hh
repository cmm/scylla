/*
 * Copyright (C) 2022-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <concepts>
#include <fmt/core.h>
#include <seastar/core/sstring.hh>

namespace sstables {

template <typename V>
concept GenerationValue = std::copyable<V> && std::totally_ordered<V>;

template <typename V>
requires GenerationValue<V>
struct generation_wrapper {
    using value_type = V;
private:
    V _value;
public:
    generation_wrapper() noexcept = default;
    explicit constexpr generation_wrapper(V value) noexcept: _value(value) {}
    constexpr V value() const noexcept { return _value; }
    constexpr bool operator==(const generation_wrapper& other) const noexcept { return _value == other._value; }
    constexpr bool operator!=(const generation_wrapper& other) const noexcept { return !(*this == other); }
    constexpr bool operator<(const generation_wrapper& other) const noexcept { return _value < other._value; }
    constexpr bool operator>(const generation_wrapper& other) const noexcept { return other < *this; }
    constexpr bool operator<=(const generation_wrapper& other) const noexcept { return !(other < *this); }
    constexpr bool operator>=(const generation_wrapper& other) const noexcept { return !(*this < other); }
};

using generation_type = generation_wrapper<int64_t>;
using generation_value_type = generation_type::value_type;

constexpr generation_type generation_from_value(generation_value_type value) {
    return generation_type{value};
}
template <typename V>
constexpr V generation_value(generation_wrapper<V> generation) {
    return generation.value();
}

}; // namespace sstables

namespace seastar {
template <typename V, typename string_type = sstring>
string_type to_sstring(sstables::generation_wrapper<V> generation) {
    return to_sstring(sstables::generation_value(generation));
}
}; // namespace seastar

template <typename V>
struct fmt::formatter<sstables::generation_wrapper<V>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.end();
    }
    template <typename FormatContext>
    auto format(const sstables::generation_type& generation, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", sstables::generation_value(generation));
    }
};

template <typename V>
struct std::hash<sstables::generation_wrapper<V>> {
    constexpr size_t operator()(sstables::generation_wrapper<V> generation) const noexcept {
        return hash<V>{}(sstables::generation_value(generation));
    }
};

template <typename V>
std::ostream& operator<<(std::ostream& s, sstables::generation_wrapper<V> generation) {
    return s << sstables::generation_value(generation);
}
