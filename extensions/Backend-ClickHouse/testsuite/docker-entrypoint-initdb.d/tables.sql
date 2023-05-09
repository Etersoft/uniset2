CREATE DATABASE IF NOT EXISTS uniset;

CREATE TABLE IF NOT EXISTS uniset.main_history
(
    timestamp DateTime64(9,'UTC') DEFAULT now(),
    value Float64 Codec(DoubleDelta, LZ4),
    uniset_hash_id UInt32 Default murmurHash2_32(name),
    name_hash_id UInt64 Default cityHash64(name),
    node_hash_id UInt64 Default cityHash64(nodename),
    producer_hash_id UInt64 Default cityHash64(producer),
    nodename LowCardinality(String),
    name LowCardinality(String),
    producer LowCardinality(String),
    tags Nested(
        name LowCardinality(String),
        value LowCardinality(String)
    )
) ENGINE = MergeTree
PARTITION BY toStartOfDay(timestamp)
PRIMARY KEY(timestamp,name_hash_id)
ORDER BY (timestamp,name_hash_id)
TTL toStartOfDay(timestamp) + INTERVAL 180 DAY;
