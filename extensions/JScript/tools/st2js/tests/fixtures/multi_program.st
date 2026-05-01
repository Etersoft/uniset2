PROGRAM Alarms
VAR
    alarmActive : BOOL := FALSE;
    alarmTimer : TON;
END_VAR

alarmTimer(IN := alarmActive, PT := T#10s);
IF alarmTimer.Q THEN
    alarmActive := FALSE;
END_IF;

END_PROGRAM

PROGRAM HMI_Update
VAR
    counter : INT := 0;
END_VAR

counter := counter + 1;

END_PROGRAM

PROGRAM Main
VAR_INPUT
    Temperature : REAL;
    Enable : BOOL;
END_VAR
VAR_OUTPUT
    Heater : BOOL;
END_VAR
VAR
    state : INT := 0;
END_VAR

IF Enable THEN
    state := 1;
END_IF;

IF state = 1 THEN
    IF Temperature < 20.0 THEN
        Heater := TRUE;
    ELSE
        Heater := FALSE;
    END_IF;
END_IF;

Alarms();
HMI_Update();

END_PROGRAM
