TYPE TempConfig :
STRUCT
    setpoint : REAL := 20.0;
    hysteresis : REAL := 2.0;
    maxTemp : REAL := 95.0;
END_STRUCT
END_TYPE

PROGRAM Thermostat
VAR_INPUT
    CurrentTemp : REAL;
    Enable : BOOL;
    Reset : BOOL;
END_VAR
VAR_OUTPUT
    HeaterOn : BOOL;
    AlarmHigh : BOOL;
    CycleCount : INT;
END_VAR
VAR
    config : TempConfig;
    onDelay : TON;
    offDelay : TOF;
    startEdge : R_TRIG;
    cycleCounter : CTU;
    state : INT := 0;
    i : INT;
END_VAR

startEdge(CLK := Enable);
IF startEdge.Q THEN
    state := 1;
END_IF;

CASE state OF
    0: (* Idle *)
        HeaterOn := FALSE;
    1: (* Running *)
        IF CurrentTemp < config.setpoint - config.hysteresis THEN
            HeaterOn := TRUE;
        ELSIF CurrentTemp > config.setpoint + config.hysteresis THEN
            HeaterOn := FALSE;
        END_IF;

        onDelay(IN := HeaterOn, PT := T#5s);
        cycleCounter(CU := onDelay.Q, RESET := Reset, PV := 100);
        CycleCount := cycleCounter.CV;
    2: (* Alarm *)
        HeaterOn := FALSE;
END_CASE;

IF CurrentTemp > config.maxTemp THEN
    AlarmHigh := TRUE;
    state := 2;
END_IF;

IF Reset THEN
    state := 0;
    AlarmHigh := FALSE;
END_IF;

END_PROGRAM
