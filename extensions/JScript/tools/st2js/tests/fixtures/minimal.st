PROGRAM Main
VAR_INPUT
    Temperature : REAL;
    StartButton : BOOL;
END_VAR
VAR_OUTPUT
    Heater : BOOL;
    Alarm : BOOL;
END_VAR
VAR
    state : INT := 0;
END_VAR

IF StartButton THEN
    Heater := TRUE;
END_IF;

IF Temperature > 80.0 THEN
    Alarm := TRUE;
END_IF;

END_PROGRAM
