FUNCTION_BLOCK MyController
VAR_INPUT
    SetPoint : REAL;
    Enable : BOOL;
END_VAR
VAR_OUTPUT
    Active : BOOL;
END_VAR
VAR
    counter : INT := 0;
END_VAR

counter := counter + 1;
IF SetPoint > 50.0 THEN
    Active := TRUE;
ELSE
    Active := FALSE;
END_IF;

END_FUNCTION_BLOCK

PROGRAM Main
VAR_INPUT
    Temperature : REAL;
END_VAR
VAR_OUTPUT
    Heater : BOOL;
END_VAR

Heater := TRUE;

END_PROGRAM
