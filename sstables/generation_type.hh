/*
 * Copyright (C) 2022-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <concepts>
#include <compare>
#include <limits>
#include <iostream>
#include <seastar/core/sstring.hh>

namespace sstables {

template <typename V>
requires std::copyable<V> && std::equality_comparable<V> && std::three_way_comparable<V>
struct generation_wrapper {
    using value_type = V;
private:
    V _value;
public:
    generation_wrapper() = delete;

    explicit constexpr generation_wrapper(const V& value) noexcept: _value(value) {}
    constexpr const V& value() const noexcept { return _value; }

    constexpr bool operator==(const generation_wrapper& other) const noexcept { return _value == other._value; }
    constexpr std::strong_ordering operator<=>(const generation_wrapper& other) const noexcept { return _value <=> other._value; }
};

using generation_type = generation_wrapper<int64_t>;

constexpr generation_type generation_from_value(generation_type::value_type value) {
    return generation_type{value};
}
template <typename V>
constexpr V generation_value(generation_wrapper<V> generation) {
    return generation.value();
}

} //namespace sstables

namespace seastar {
template <typename V, typename string_type = sstring>
string_type to_sstring(sstables::generation_wrapper<V> generation) {
    return to_sstring(sstables::generation_value(generation));
}
} //namespace seastar

namespace std {
template <typename V>
struct hash<sstables::generation_wrapper<V>> {
    constexpr size_t operator()(const sstables::generation_wrapper<V>& generation) const noexcept {
        return hash<V>{}(generation.value());
    }
};

// for min_max_tracker
template <typename V>
struct numeric_limits<sstables::generation_wrapper<V>> : public numeric_limits<V> {
    static constexpr sstables::generation_wrapper<V> min() noexcept {
        return sstables::generation_wrapper<V>{numeric_limits<V>::min()};
    }
    static constexpr sstables::generation_wrapper<V> max() noexcept {
        return sstables::generation_wrapper<V>{numeric_limits<V>::max()};
    }
};

template <typename V>
ostream& operator<<(ostream& s, const sstables::generation_wrapper<V>& generation) {
    return s << generation.value();
}
} //namespace std
