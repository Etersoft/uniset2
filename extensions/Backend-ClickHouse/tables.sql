CREATE TABLE IF NOT EXISTS main_history
(
    timestamp DateTime('UTC') DEFAULT now(),
    time_usec UInt64 Codec(DoubleDelta, LZ4),
    sensor_id UInt64 Codec(DoubleDelta, LZ4),
    value Float64 Codec(DoubleDelta, LZ4),
    node  UInt64 Codec(DoubleDelta, LZ4),
    tags Nested(
        name LowCardinality(String),
        value LowCardinality(String)
    )
) ENGINE = MergeTree
PARTITION BY toStartOfDay(timestamp)
PRIMARY KEY(timestamp,time_usec) 
ORDER BY (timestamp,time_usec,sensor_id);
-- TTL timestamp + INTERVAL 90 DAY;

-- INSERT INTO main_history(timestamp,time_usec,sensor_id,tags.name, tags.value) VALUES ('2017-08-17 15:00:00',5,100,['tag1','tag2'],['value1','value2']);
