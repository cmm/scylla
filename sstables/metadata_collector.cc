/*
 * Copyright (C) 2020-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "log.hh"
#include "metadata_collector.hh"
#include "range_tombstone.hh"
#include "sstables_manager.hh"

logging::logger mdclogger("metadata_collector");

namespace sstables {

void metadata_collector::convert(disk_array<uint32_t, disk_string<uint16_t>>& to, const std::optional<clustering_key_prefix>& from) {
    if (!from) {
        mdclogger.trace("{}: convert: empty", _name);
        return;
    }
    mdclogger.trace("{}: convert: {}", _name, clustering_key_prefix::with_schema_wrapper(_schema, *from));
    for (auto& value : from->components()) {
        to.elements.push_back(disk_string<uint16_t>{to_bytes(value)});
    }
}

void metadata_collector::update_min_max_components(const clustering_key_prefix& key) {
    if (!_min_clustering_key) {
        mdclogger.trace("{}: initializing min/max clustering keys={}", _name, clustering_key_prefix::with_schema_wrapper(_schema, key));
        _min_clustering_key.emplace(key);
        _max_clustering_key.emplace(key);
        return;
    }

    const bound_view::tri_compare cmp(_schema);

    auto res = cmp(bound_view(key, bound_kind::incl_start), bound_view(*_min_clustering_key, bound_kind::incl_start));
    if (res < 0) {
        mdclogger.trace("{}: setting min_clustering_key={}", _name, clustering_key_prefix::with_schema_wrapper(_schema, key));
        _min_clustering_key.emplace(key);
    }

    res = cmp(bound_view(key, bound_kind::incl_end), bound_view(*_max_clustering_key, bound_kind::incl_end));
    if (res > 0) {
        mdclogger.trace("{}: setting max_clustering_key={}", _name, clustering_key_prefix::with_schema_wrapper(_schema, key));
        _max_clustering_key.emplace(key);
    }
}

void metadata_collector::update_min_max_components(const range_tombstone& rt) {
    const bound_view::tri_compare cmp(_schema);

    if (!_min_clustering_key) {
        mdclogger.trace("{}: initializing min_clustering_key to rt.start={}", _name, clustering_key_prefix::with_schema_wrapper(_schema, rt.start));
        _min_clustering_key.emplace(rt.start);
    } else if (cmp(rt.start_bound(), bound_view(*_min_clustering_key, bound_kind::incl_start)) < 0) {
        mdclogger.trace("{}: updating min_clustering_key to rt.start={}", _name, clustering_key_prefix::with_schema_wrapper(_schema, rt.start));
        _min_clustering_key.emplace(rt.start);
    }

    if (!_max_clustering_key) {
        mdclogger.trace("{}: initializing max_clustering_key to rt.end={}", _name, clustering_key_prefix::with_schema_wrapper(_schema, rt.end));
        _max_clustering_key.emplace(rt.end);
    } else if (cmp(rt.end_bound(), bound_view(*_max_clustering_key, bound_kind::incl_end)) > 0) {
        mdclogger.trace("{}: updating max_clustering_key to rt.end={}", _name, clustering_key_prefix::with_schema_wrapper(_schema, rt.end));
        _max_clustering_key.emplace(rt.end);
    }
}

void metadata_collector::construct_stats(stats_metadata& m) {
    m.estimated_partition_size = std::move(_estimated_partition_size);
    m.estimated_cells_count = std::move(_estimated_cells_count);
    m.position = _replay_position;
    m.min_timestamp = _timestamp_tracker.min();
    m.max_timestamp = _timestamp_tracker.max();
    m.min_local_deletion_time = _local_deletion_time_tracker.min();
    m.max_local_deletion_time = _local_deletion_time_tracker.max();
    m.min_ttl = _ttl_tracker.min();
    m.max_ttl = _ttl_tracker.max();
    m.compression_ratio = _compression_ratio;
    m.estimated_tombstone_drop_time = std::move(_estimated_tombstone_drop_time);
    m.sstable_level = _sstable_level;
    m.repaired_at = _repaired_at;
    convert(m.min_column_names, _min_clustering_key);
    convert(m.max_column_names, _max_clustering_key);
    m.has_legacy_counter_shards = _has_legacy_counter_shards;
    m.columns_count = _columns_count;
    m.rows_count = _rows_count;
    m.originating_host_id = _manager.get_local_host_id();
}

} // namespace sstables
