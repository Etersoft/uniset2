CREATE TABLE IF NOT EXISTS main_history
(
    timestamp DateTime('UTC') DEFAULT now(),
    time_usec UInt64,
    sensor_id UInt64,
    value Float64,
    node  UInt64,
    confirm UInt64,
    tags Nested(
        name String, 
        value String
    )
) ENGINE = MergeTree
PARTITION BY toStartOfDay(timestamp)
PRIMARY KEY(timestamp,time_usec) 
ORDER BY (timestamp,time_usec,sensor_id);

-- INSERT INTO main_history(timestamp,time_usec,sensor_id,tags.name, tags.value) VALUES ('2017-08-17 15:00:00',5,100,['tag1','tag2'],['value1','value2']);
