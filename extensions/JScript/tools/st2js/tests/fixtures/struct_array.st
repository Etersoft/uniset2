TYPE SensorData :
STRUCT
    value : INT;
    scale : REAL := 1.0;
    valid : BOOL;
END_STRUCT
END_TYPE

PROGRAM StructArrayTest
VAR
    sensor : SensorData;
    readings : ARRAY[1..5] OF INT;
    name : STRING := 'test';
    timeout : TIME := T#5s;
END_VAR

sensor.value := 42;
sensor.valid := TRUE;
readings[1] := sensor.value;
readings[3] := 100;

END_PROGRAM
