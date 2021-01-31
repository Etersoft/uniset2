CREATE TABLE main_history (
  id BIGSERIAL PRIMARY KEY NOT NULL,
  date date NOT NULL,
  time time NOT NULL,
  time_usec int NOT NULL CHECK (time_usec >= 0),
  sensor_id int NOT NULL,
  value double precision NOT NULL,
  node int NOT NULL,
  confirm int DEFAULT NULL
);
