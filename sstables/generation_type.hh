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

using generation_value_type = int64_t;

struct generation_type {
    generation_value_type value;
    generation_type() noexcept = default;
    explicit constexpr generation_type(generation_value_type value) noexcept: value(value) {}
    constexpr bool operator==(const generation_type& other) const noexcept { return value == other.value; }
    constexpr std::strong_ordering operator<=>(const generation_type& other) const noexcept { return value <=> other.value; }
    generation_type operator++(int) noexcept { return generation_type{value++}; }
    generation_type& operator++() noexcept { ++value; return *this; }
};

constexpr generation_type generation_from_value(generation_value_type value) {
    return generation_type{value};
}
constexpr generation_value_type generation_value(generation_type generation) {
    return generation.value;
}

}; // namespace sstables

namespace seastar {
template <typename string_type = sstring>
string_type to_sstring(sstables::generation_type generation) {
    return to_sstring(sstables::generation_value(generation));
}
}; // namespace seastar

// XXX should somehow consume and forward any format specs valid for value_type
template <> struct fmt::formatter<sstables::generation_type> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.end();
    }
    template <typename FormatContext>
    auto format(const sstables::generation_type& generation, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", sstables::generation_value(generation));
    }
};

template <> class std::numeric_limits<sstables::generation_type> : public numeric_limits<sstables::generation_value_type> {
    using value_limits = numeric_limits<sstables::generation_value_type>;
public:
    static constexpr sstables::generation_type min() noexcept { return sstables::generation_from_value(value_limits::min()); }
    static constexpr sstables::generation_type max() noexcept { return sstables::generation_from_value(value_limits::max()); }
    static constexpr sstables::generation_type lowest() noexcept { return sstables::generation_from_value(value_limits::lowest()); }
};

template <> struct std::hash<sstables::generation_type> {
    constexpr size_t operator()(const sstables::generation_type& generation) const noexcept {
        return hash<sstables::generation_value_type>{}(sstables::generation_value(generation));
    }
};

static inline std::ostream& operator<<(std::ostream& s, sstables::generation_type generation) {
    return s << sstables::generation_value(generation);
}
