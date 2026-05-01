PROGRAM ScaleTest
VAR_INPUT
    Temperature : REAL;
    Pressure : REAL;
    StartButton : BOOL;
END_VAR
VAR_OUTPUT
    ValvePos : REAL;
    Alarm : BOOL;
END_VAR
VAR
    setpoint : REAL := 50.0;
END_VAR

IF Temperature > setpoint THEN
    ValvePos := Temperature - setpoint;
    Alarm := TRUE;
END_IF;

END_PROGRAM
