(* Thermostat controller example
 *
 * Demonstrates:
 * - STRUCT type for configuration
 * - State machine (CASE)
 * - Hysteresis control
 * - Standard FBs: R_TRIG, TON, TOF, CTU
 * - Alarm monitoring
 *
 * Convert to JS:
 *   python -m st2js thermostat.st -m thermostat_mapping.yaml --debug -o main-plc.js
 *
 * Run with debug:
 *   uniset2-jscript --js-file main-plc.js --confile configure.xml --js-name JSProxy1
 *   Open http://host:8088/debug/ui in browser
 *)

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
    state : INT := 0;    (* 0=Idle, 1=Running, 2=Alarm *)
END_VAR

(* Detect enable button rising edge *)
startEdge(CLK := Enable);
IF startEdge.Q THEN
    state := 1;
END_IF;

(* State machine *)
CASE state OF
    0: (* Idle — heater off *)
        HeaterOn := FALSE;

    1: (* Running — hysteresis control *)
        IF CurrentTemp < config.setpoint - config.hysteresis THEN
            HeaterOn := TRUE;
        ELSIF CurrentTemp > config.setpoint + config.hysteresis THEN
            HeaterOn := FALSE;
        END_IF;

        (* Count heater-on cycles via on-delay *)
        onDelay(IN := HeaterOn, PT := T#5s);
        cycleCounter(CU := onDelay.Q, RESET := Reset, PV := 100);
        CycleCount := cycleCounter.CV;

    2: (* Alarm — emergency stop *)
        HeaterOn := FALSE;
END_CASE;

(* Overtemperature alarm *)
IF CurrentTemp > config.maxTemp THEN
    AlarmHigh := TRUE;
    state := 2;
END_IF;

(* Reset from alarm *)
IF Reset THEN
    state := 0;
    AlarmHigh := FALSE;
END_IF;

END_PROGRAM
