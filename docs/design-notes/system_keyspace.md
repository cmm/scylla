# System keyspace layout

This section describes layouts and usage of system.* tables.

## system.large\_data

Scylla performs better if partitions, rows, and individual cells are
not too large. To help diagnose cases where these grow too large,
scylla keeps a table that records large partitions, rows, cells, and
partition row counts.

Each entry means that there is a particular sstable with a large
partition, row, cell, or a partition with too many rows.  In
particular, this implies that:

* There is no entry until compaction aggregates enough data in a
  single sstable.
* The entry stays around until the sstable is deleted.

In addition, the entries also have a TTL of 30 days.

Schema:
~~~
CREATE TABLE system.large_data (
    keyspace_name text,
    table_name text,
    sstable_name text,
    ratio float,
    metric text,
    partition_key text,
    clustering_key text,
    column_name text,
    value bigint,
    compaction_time timestamp,
    PRIMARY KEY ((keyspace_name, table_name, sstable_name), ratio, partition_key, clustering_key, column_name)
) WITH CLUSTERING ORDER BY (ratio DESC, partition_key ASC, clustering_key ASC, column_name ASC);
~~~

The ratio (think "badness") column is there to provide a way to
meaningfully sort heterogeneous data -- no useful ordering can be
defined between, say, "12mb row" and "2mb cell" as absolute values,
but (if the thresholds are 10mb and 1mb respectively) 2x is clearly
worse than 1.2x, so the cell will be shown before the row.

### Example usage

(Assume `ALLOW FILTERING` throughout).

#### Extracting large data info
~~~
SELECT * FROM system.large_data;
~~~

#### Extracting large data info for a single table
~~~
SELECT * FROM system.large_data WHERE keyspace_name = 'ks1' and table_name = 'standard1';
~~~

#### Extracting large data info for a single table and metric
~~~
SELECT * FROM system.large_data WHERE keyspace_name = 'ks1' and table_name = 'standard1' and metric = 'row_size';
~~~

### Implemented large data metrics

#### partition_size

Records partitions larger than
`compaction_large_partition_warning_threshold_mb`.

#### row_size

Records clustering and static rows larger than
`compaction_large_row_warning_threshold_mb`.

#### cell_size

Records cells larger than `compaction_large_cell_warning_threshold_mb`.

Note that a collection is just one cell. There is no information about
the size of each collection element.

#### row_count

Records partitions with row count larger than `compaction_rows_count_warning_threshold`.

## system.truncated

Holds truncation replay positions per table and shard

Schema:
~~~
CREATE TABLE system.truncated (
    table_uuid uuid,    # id of truncated table
    shard int,          # shard
    position int,       # replay position
    segment_id bigint,  # replay segment
    truncated_at timestamp static,  # truncation time
    PRIMARY KEY (table_uuid, shard)
) WITH CLUSTERING ORDER BY (shard ASC)
~~~

When a table is truncated, sstables are removed and the current replay position for each 
shard (last mutation to be committed to either sstable or memtable) is collected.
These are then inserted into the above table, using shard as clustering.

When doing commitlog replay (in case of a crash), the data is read from the above 
table and mutations are filtered based on the replay positions to ensure 
truncated data is not resurrected.
 
Note that until the above table was added, truncation records where kept in the 
`truncated_at` map column in the `system.local` table. When booting up, scylla will
merge the data in the legacy store with data the `truncated` table. Until the whole
cluster agrees on the feature `TRUNCATION_TABLE` truncation will write both new and 
legacy records. When the feature is agreed upon the legacy map is removed.

## TODO: the rest
