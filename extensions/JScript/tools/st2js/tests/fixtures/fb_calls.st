PROGRAM Main
VAR_INPUT
    Enable : BOOL;
    Pulse : BOOL;
    ResetCmd : BOOL;
END_VAR
VAR_OUTPUT
    Output : BOOL;
    Count : INT;
END_VAR
VAR
    myTimer : TON;
    myCounter : CTU;
    myTrigger : R_TRIG;
    myLatch : RS;
END_VAR

myTimer(IN := Enable, PT := T#3000ms);
Output := myTimer.Q;

myCounter(CU := Pulse, RESET := ResetCmd, PV := 10);
Count := myCounter.CV;

myTrigger(CLK := Enable);

myLatch(SET := myTrigger.Q, RESET1 := ResetCmd);

END_PROGRAM
