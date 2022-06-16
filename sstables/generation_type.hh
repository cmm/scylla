/*
 * Copyright (C) 2022-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <atomic>
#include <compare>
#include <limits>
#include <fmt/core.h>
#include <seastar/core/sstring.hh>

namespace sstables {
class generation_type {
public:
    using value_type = uint64_t;
private:
    value_type _value;
public:
    generation_type() noexcept = default;
    explicit generation_type(value_type value) noexcept: _value(value) {}
    value_type value() const noexcept { return _value; }
    bool operator<(const generation_type& other) const noexcept { return _value < other._value; }
    bool operator>(const generation_type& other) const noexcept { return _value > other._value; }
    bool operator<=(const generation_type& other) const noexcept { return _value <= other._value; }
    bool operator>=(const generation_type& other) const noexcept { return _value >= other._value; }
    bool operator==(const generation_type& other) const noexcept { return _value == other._value; }
    bool operator!=(const generation_type& other) const noexcept { return _value != other._value; }
    std::strong_ordering operator<=>(const generation_type& other) const noexcept { return _value <=> other._value; }
    generation_type operator++(int) { return generation_type{_value++}; }
    generation_type& operator++() { ++_value; return *this; }
    generation_type operator+(generation_type other) const { return generation_type{_value + other._value}; }
    generation_type operator*(generation_type other) const { return generation_type{_value * other._value}; }
    generation_type operator/(generation_type other) const { return generation_type{_value / other._value}; }
    generation_type operator%(generation_type other) const { return generation_type{_value % other._value}; }
};
}

namespace seastar {
template <typename string_type = sstring>
string_type to_sstring(sstables::generation_type generation) {
    return to_sstring(generation.value());
}
}

namespace fmt {
template <> struct formatter<sstables::generation_type> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.end();
    }
    template <typename FormatContext>
    auto format(const sstables::generation_type& g, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", g.value());
    }
};
}

namespace std {
template <> struct numeric_limits<sstables::generation_type> : public numeric_limits<sstables::generation_type::value_type> {
private:
    using value_limits = numeric_limits<sstables::generation_type::value_type>;
public:
    static sstables::generation_type min() noexcept { return sstables::generation_type{value_limits::min()}; }
    static sstables::generation_type max() noexcept { return sstables::generation_type{value_limits::max()}; }
    static sstables::generation_type lowest() noexcept { return sstables::generation_type{value_limits::lowest()}; }
};

template <> struct hash<sstables::generation_type> {
    size_t operator()(const sstables::generation_type& generation) const noexcept {
        return hash<sstables::generation_type::value_type>{}(generation.value());
    }
};

static inline ostream& operator<<(ostream& s, sstables::generation_type generation) {
    return s << generation.value();
}
}
