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

struct type {
    value_type value;
    type() noexcept = default;
    explicit constexpr type(value_type value) noexcept: value(value) {}
    constexpr bool operator==(const type& other) const noexcept { return value == other.value; }
    constexpr std::strong_ordering operator<=>(const type& other) const noexcept { return value <=> other.value; }
    type operator++(int) noexcept { return type{value++}; }
    type& operator++() noexcept { ++value; return *this; }
};

constexpr type from_value(value_type value) {
    return type{value};
}
constexpr value_type value(type generation) {
    return generation.value;
}
}; // namespace generation
}; // namespace sstables

namespace seastar {
template <typename string_type = sstring>
string_type to_sstring(sstables::generation::type generation) {
    return to_sstring(sstables::generation::value(generation));
}
}; // namespace seastar

// XXX should somehow consume and forward any format specs valid for value_type
template <> struct fmt::formatter<sstables::generation::type> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.end();
    }
    template <typename FormatContext>
    auto format(const sstables::generation::type& generation, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", sstables::generation::value(generation));
    }
};

template <> class std::numeric_limits<sstables::generation::type> : public numeric_limits<sstables::generation::value_type> {
    using value_limits = numeric_limits<sstables::generation::value_type>;
public:
    static constexpr sstables::generation::type min() noexcept { return sstables::generation::from_value(value_limits::min()); }
    static constexpr sstables::generation::type max() noexcept { return sstables::generation::from_value(value_limits::max()); }
    static constexpr sstables::generation::type lowest() noexcept { return sstables::generation::from_value(value_limits::lowest()); }
};

template <> struct std::hash<sstables::generation::type> {
    constexpr size_t operator()(const sstables::generation::type& generation) const noexcept {
        return hash<sstables::generation::value_type>{}(sstables::generation::value(generation));
    }
};

static inline std::ostream& operator<<(std::ostream& s, sstables::generation::type generation) {
    return s << sstables::generation::value(generation);
}
