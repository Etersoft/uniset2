PROGRAM ControlFlowTest
VAR
    x : INT := 0;
    i : INT;
    mystate : INT := 1;
    running : BOOL := TRUE;
END_VAR

CASE mystate OF
    1: x := 10;
    2: x := 20;
    3: x := 30;
ELSE
    x := 0;
END_CASE;

FOR i := 1 TO 10 BY 1 DO
    x := x + i;
END_FOR;

WHILE running DO
    x := x + 1;
END_WHILE;

REPEAT
    x := x - 1;
UNTIL x <= 0
END_REPEAT;

END_PROGRAM
