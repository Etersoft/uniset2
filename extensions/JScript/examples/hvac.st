(* HVAC Controller — two-zone heating/cooling with alarms
 *
 * Zone 1: Office — heating only
 * Zone 2: Server room — cooling only
 * Common: outdoor temperature sensor, alarm system
 *)

TYPE ZoneConfig :
STRUCT
    setpoint : REAL := 22.0;
    hysteresis : REAL := 1.5;
    alarmHigh : REAL := 35.0;
    alarmLow : REAL := 5.0;
END_STRUCT
END_TYPE

PROGRAM HVAC
VAR_INPUT
    OutdoorTemp : REAL;
    Zone1Temp : REAL;
    Zone2Temp : REAL;
    Enable : BOOL;
    Reset : BOOL;
    ManualMode : BOOL;
END_VAR
VAR_OUTPUT
    Zone1Heater : BOOL;
    Zone2Cooler : BOOL;
    AlarmZone1 : BOOL;
    AlarmZone2 : BOOL;
    AlarmCommon : BOOL;
    RunHours : INT;
END_VAR
VAR
    cfgZone1 : ZoneConfig;
    cfgZone2 : ZoneConfig;
    enableEdge : R_TRIG;
    resetEdge : R_TRIG;

    (* Zone 1: heating *)
    z1_hyst : HYSTERESIS;
    z1_onDelay : TON;
    z1_offDelay : TOF;
    z1_alarmHigh : LIMITALARM;
    z1_cycles : CTU;

    (* Zone 2: cooling *)
    z2_hyst : HYSTERESIS;
    z2_onDelay : TON;
    z2_offDelay : TOF;
    z2_alarmHigh : LIMITALARM;
    z2_cycles : CTU;

    (* Common *)
    blinker : BLINK;
    runTimer : TON;
    runCounter : CTU;
    state : INT := 0;
END_VAR

(* Enable/Reset edge detection *)
enableEdge(CLK := Enable);
resetEdge(CLK := Reset);

IF enableEdge.Q THEN
    state := 1;
END_IF;

IF resetEdge.Q THEN
    state := 0;
    AlarmZone1 := FALSE;
    AlarmZone2 := FALSE;
    AlarmCommon := FALSE;
END_IF;

CASE state OF
    0: (* Standby *)
        Zone1Heater := FALSE;
        Zone2Cooler := FALSE;

    1: (* Running *)
        IF NOT ManualMode THEN
            (* Zone 1: Heating — turn on below setpoint-hyst, off above setpoint+hyst *)
            z1_hyst(IN := Zone1Temp, HIGH := cfgZone1.setpoint + cfgZone1.hysteresis,
                    LOW := cfgZone1.setpoint - cfgZone1.hysteresis);
            z1_onDelay(IN := z1_hyst.OUT, PT := T#3s);
            z1_offDelay(IN := z1_onDelay.Q, PT := T#10s);
            Zone1Heater := z1_offDelay.Q;

            (* Zone 2: Cooling — turn on above setpoint+hyst, off below setpoint-hyst *)
            z2_hyst(IN := Zone2Temp, HIGH := cfgZone2.setpoint + cfgZone2.hysteresis,
                    LOW := cfgZone2.setpoint - cfgZone2.hysteresis);
            z2_onDelay(IN := NOT z2_hyst.OUT, PT := T#5s);
            z2_offDelay(IN := z2_onDelay.Q, PT := T#15s);
            Zone2Cooler := z2_offDelay.Q;
        END_IF;

        (* Alarm monitoring *)
        z1_alarmHigh(IN := Zone1Temp, HIGH := cfgZone1.alarmHigh, LOW := cfgZone1.alarmLow);
        z2_alarmHigh(IN := Zone2Temp, HIGH := cfgZone2.alarmHigh, LOW := cfgZone2.alarmLow);

        AlarmZone1 := NOT z1_alarmHigh.IL;
        AlarmZone2 := NOT z2_alarmHigh.IL;

        IF AlarmZone1 OR AlarmZone2 THEN
            AlarmCommon := TRUE;
            blinker(ENABLE := TRUE, TIMEHIGH := T#500ms, TIMELOW := T#500ms);
        ELSE
            blinker(ENABLE := FALSE, TIMEHIGH := T#500ms, TIMELOW := T#500ms);
        END_IF;

        (* Run hours counter *)
        runTimer(IN := TRUE, PT := T#60s);
        z1_cycles(CU := z1_onDelay.Q, RESET := Reset, PV := 1000);
        z2_cycles(CU := z2_onDelay.Q, RESET := Reset, PV := 1000);
        runCounter(CU := runTimer.Q, RESET := Reset, PV := 10000);
        RunHours := runCounter.CV;

    2: (* Fault *)
        Zone1Heater := FALSE;
        Zone2Cooler := FALSE;
END_CASE;

END_PROGRAM
