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

namespace generation {

using value_type = int64_t;

};

struct generation_type {
    using value_type = generation::value_type;
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

namespace generation {

using type = generation_type;

static inline type from_value(value_type value) {
    return type(value);
}

static inline value_type value(type generation) {
    return generation.value();
}
};

};

namespace seastar {
template <typename string_type = sstring>
string_type to_sstring(sstables::generation::type generation) {
    return to_sstring(sstables::generation::value(generation));
}
}

namespace fmt {
template <> struct formatter<sstables::generation::type> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.end();
    }
    template <typename FormatContext>
    auto format(const sstables::generation::type& generation, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", sstables::generation::value(generation));
    }
};
}

namespace std {
template <> struct numeric_limits<sstables::generation::type> : public numeric_limits<sstables::generation::value_type> {
private:
    using value_limits = numeric_limits<sstables::generation::value_type>;
public:
    static sstables::generation::type min() noexcept { return sstables::generation::from_value(value_limits::min()); }
    static sstables::generation::type max() noexcept { return sstables::generation::from_value(value_limits::max()); }
    static sstables::generation::type lowest() noexcept { return sstables::generation::from_value(value_limits::lowest()); }
};

template <> struct hash<sstables::generation::type> {
    size_t operator()(const sstables::generation::type& generation) const noexcept {
        return hash<sstables::generation::value_type>{}(sstables::generation::value(generation));
    }
};

static inline ostream& operator<<(ostream& s, sstables::generation::type generation) {
    return s << sstables::generation::value(generation);
}
}
