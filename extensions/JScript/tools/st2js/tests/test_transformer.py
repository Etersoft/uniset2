"""Tests for st2js.transformer module.

Tests the transformation of blark AST nodes into IR dataclasses.
"""
import pytest

from st2js.parser import parse_st
from st2js.ir import (
    IECType,
    IRAssignment,
    IRBinaryOp,
    IRCase,
    IRForLoop,
    IRIfElse,
    IRLiteral,
    IRProgram,
    IRRepeatLoop,
    IRUnaryOp,
    IRVarRef,
    IRVariable,
    IRWhileLoop,
)
from st2js.errors import UnsupportedError


def _transform_source(source: str) -> IRProgram:
    """Helper: parse ST source and transform to IR."""
    from st2js.transformer import transform
    result = parse_st(source)
    return transform(result)


def _load_fixture(name: str) -> str:
    """Load a test fixture file."""
    import os
    fixture_dir = os.path.join(os.path.dirname(__file__), "fixtures")
    with open(os.path.join(fixture_dir, name)) as f:
        return f.read()


class TestMinimalProgram:
    """Test transformation of the minimal.st fixture."""

    @pytest.fixture
    def program(self) -> IRProgram:
        source = _load_fixture("minimal.st")
        return _transform_source(source)

    def test_program_name(self, program: IRProgram):
        assert program.name == "Main"

    def test_input_variables(self, program: IRProgram):
        assert len(program.inputs) == 2

        temp = program.inputs[0]
        assert temp.name == "Temperature"
        assert temp.iec_type == IECType.REAL
        assert temp.is_input is True
        assert temp.initial_value is None

        start = program.inputs[1]
        assert start.name == "StartButton"
        assert start.iec_type == IECType.BOOL
        assert start.is_input is True

    def test_output_variables(self, program: IRProgram):
        assert len(program.outputs) == 2

        heater = program.outputs[0]
        assert heater.name == "Heater"
        assert heater.iec_type == IECType.BOOL
        assert heater.is_output is True

        alarm = program.outputs[1]
        assert alarm.name == "Alarm"
        assert alarm.iec_type == IECType.BOOL
        assert alarm.is_output is True

    def test_local_variables(self, program: IRProgram):
        assert len(program.locals) == 1

        state = program.locals[0]
        assert state.name == "state"
        assert state.iec_type == IECType.INT
        assert state.is_local is True
        assert state.initial_value == 0

    def test_body_has_two_statements(self, program: IRProgram):
        assert len(program.body) == 2

    def test_first_if_statement(self, program: IRProgram):
        stmt = program.body[0]
        assert isinstance(stmt, IRIfElse)
        # IF StartButton THEN
        assert isinstance(stmt.condition, IRVarRef)
        assert stmt.condition.name == "StartButton"
        # Heater := TRUE
        assert len(stmt.then_body) == 1
        assign = stmt.then_body[0]
        assert isinstance(assign, IRAssignment)
        assert isinstance(assign.target, IRVarRef)
        assert assign.target.name == "Heater"
        assert isinstance(assign.value, IRLiteral)
        assert assign.value.value is True
        assert assign.value.iec_type == IECType.BOOL
        # No ELSIF or ELSE
        assert stmt.elsif_branches == []
        assert stmt.else_body is None

    def test_second_if_with_comparison(self, program: IRProgram):
        stmt = program.body[1]
        assert isinstance(stmt, IRIfElse)
        # IF Temperature > 80.0
        assert isinstance(stmt.condition, IRBinaryOp)
        assert stmt.condition.op == ">"
        assert isinstance(stmt.condition.left, IRVarRef)
        assert stmt.condition.left.name == "Temperature"
        assert isinstance(stmt.condition.right, IRLiteral)
        assert stmt.condition.right.value == 80.0
        assert stmt.condition.right.iec_type == IECType.REAL

    def test_function_blocks_empty(self, program: IRProgram):
        assert program.function_blocks == []

    def test_fb_instances_empty(self, program: IRProgram):
        assert program.fb_instances == []


class TestVariableTypes:
    """Test that ST types map correctly to IECType enum."""

    def test_bool_type(self):
        prog = _transform_source("""PROGRAM T
VAR x : BOOL; END_VAR
END_PROGRAM""")
        assert prog.locals[0].iec_type == IECType.BOOL

    def test_int_type(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT; END_VAR
END_PROGRAM""")
        assert prog.locals[0].iec_type == IECType.INT

    def test_dint_type(self):
        prog = _transform_source("""PROGRAM T
VAR x : DINT; END_VAR
END_PROGRAM""")
        assert prog.locals[0].iec_type == IECType.DINT

    def test_real_type(self):
        prog = _transform_source("""PROGRAM T
VAR x : REAL; END_VAR
END_PROGRAM""")
        assert prog.locals[0].iec_type == IECType.REAL


class TestInitialValues:
    """Test initial values in variable declarations."""

    def test_integer_initial_value(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 42; END_VAR
END_PROGRAM""")
        assert prog.locals[0].initial_value == 42

    def test_real_initial_value(self):
        prog = _transform_source("""PROGRAM T
VAR x : REAL := 3.14; END_VAR
END_PROGRAM""")
        assert prog.locals[0].initial_value == pytest.approx(3.14)

    def test_bool_true_initial_value(self):
        prog = _transform_source("""PROGRAM T
VAR x : BOOL := TRUE; END_VAR
END_PROGRAM""")
        assert prog.locals[0].initial_value is True

    def test_bool_false_initial_value(self):
        prog = _transform_source("""PROGRAM T
VAR x : BOOL := FALSE; END_VAR
END_PROGRAM""")
        assert prog.locals[0].initial_value is False

    def test_no_initial_value(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT; END_VAR
END_PROGRAM""")
        assert prog.locals[0].initial_value is None


class TestIfStatement:
    """Test IF/ELSIF/ELSE transformation."""

    def test_simple_if(self):
        prog = _transform_source("""PROGRAM T
VAR x : BOOL; y : INT := 0; END_VAR
IF x THEN y := 1; END_IF;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRIfElse)
        assert isinstance(stmt.condition, IRVarRef)
        assert len(stmt.then_body) == 1
        assert stmt.elsif_branches == []
        assert stmt.else_body is None

    def test_if_elsif_else(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
IF x > 10 THEN
    y := 1;
ELSIF x > 5 THEN
    y := 2;
ELSE
    y := 0;
END_IF;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRIfElse)
        # Main condition
        assert isinstance(stmt.condition, IRBinaryOp)
        assert stmt.condition.op == ">"
        # ELSIF branch
        assert len(stmt.elsif_branches) == 1
        elsif_cond, elsif_body = stmt.elsif_branches[0]
        assert isinstance(elsif_cond, IRBinaryOp)
        assert elsif_cond.op == ">"
        assert len(elsif_body) == 1
        # ELSE branch
        assert stmt.else_body is not None
        assert len(stmt.else_body) == 1


class TestAssignment:
    """Test assignment statement transformation."""

    def test_simple_assignment(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; END_VAR
x := 42;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRAssignment)
        assert isinstance(stmt.target, IRVarRef)
        assert stmt.target.name == "x"
        assert isinstance(stmt.value, IRLiteral)
        assert stmt.value.value == 42

    def test_variable_to_variable_assignment(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
x := y;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRAssignment)
        assert isinstance(stmt.value, IRVarRef)
        assert stmt.value.name == "y"


class TestBinaryExpressions:
    """Test binary expression transformation."""

    def test_arithmetic_add(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
x := x + y;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt.value, IRBinaryOp)
        assert stmt.value.op == "+"

    def test_arithmetic_subtract(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
x := x - y;
END_PROGRAM""")
        assert prog.body[0].value.op == "-"

    def test_arithmetic_multiply(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
x := x * y;
END_PROGRAM""")
        assert prog.body[0].value.op == "*"

    def test_arithmetic_divide(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
x := x / y;
END_PROGRAM""")
        assert prog.body[0].value.op == "/"

    def test_comparison_equal(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : BOOL; END_VAR
IF x = 0 THEN y := TRUE; END_IF;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRIfElse)
        assert isinstance(stmt.condition, IRBinaryOp)
        assert stmt.condition.op == "="

    def test_comparison_not_equal(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : BOOL; END_VAR
IF x <> 0 THEN y := TRUE; END_IF;
END_PROGRAM""")
        assert prog.body[0].condition.op == "<>"

    def test_comparison_greater_equal(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : BOOL; END_VAR
IF x >= 0 THEN y := TRUE; END_IF;
END_PROGRAM""")
        assert prog.body[0].condition.op == ">="

    def test_comparison_less_equal(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : BOOL; END_VAR
IF x <= 0 THEN y := TRUE; END_IF;
END_PROGRAM""")
        assert prog.body[0].condition.op == "<="

    def test_logical_and(self):
        prog = _transform_source("""PROGRAM T
VAR a : BOOL; b : BOOL; END_VAR
IF a AND b THEN a := TRUE; END_IF;
END_PROGRAM""")
        assert prog.body[0].condition.op == "AND"

    def test_logical_or(self):
        prog = _transform_source("""PROGRAM T
VAR a : BOOL; b : BOOL; END_VAR
IF a OR b THEN a := TRUE; END_IF;
END_PROGRAM""")
        assert prog.body[0].condition.op == "OR"

    def test_logical_xor(self):
        prog = _transform_source("""PROGRAM T
VAR a : BOOL; b : BOOL; END_VAR
IF a XOR b THEN a := TRUE; END_IF;
END_PROGRAM""")
        assert prog.body[0].condition.op == "XOR"


class TestUnaryExpressions:
    """Test unary expression transformation (NOT, negation)."""

    def test_not_expression(self):
        prog = _transform_source("""PROGRAM T
VAR x : BOOL; y : BOOL; END_VAR
y := NOT x;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRAssignment)
        assert isinstance(stmt.value, IRUnaryOp)
        assert stmt.value.op == "NOT"
        assert isinstance(stmt.value.operand, IRVarRef)
        assert stmt.value.operand.name == "x"

    def test_negation_expression(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; END_VAR
x := -5;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRAssignment)
        assert isinstance(stmt.value, IRUnaryOp)
        assert stmt.value.op == "-"
        assert isinstance(stmt.value.operand, IRLiteral)
        assert stmt.value.operand.value == 5


class TestCaseStatement:
    """Test CASE..OF statement transformation."""

    def test_simple_case(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
CASE x OF
    1: y := 10;
    2: y := 20;
END_CASE;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRCase)
        assert isinstance(stmt.selector, IRVarRef)
        assert stmt.selector.name == "x"
        assert len(stmt.branches) == 2
        assert stmt.else_body is None

    def test_case_branch_values(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
CASE x OF
    1: y := 10;
    2: y := 20;
END_CASE;
END_PROGRAM""")
        stmt = prog.body[0]
        # First branch: value 1
        values_0, body_0 = stmt.branches[0]
        assert len(values_0) == 1
        assert isinstance(values_0[0], IRLiteral)
        assert values_0[0].value == 1
        assert len(body_0) == 1
        assert isinstance(body_0[0], IRAssignment)

        # Second branch: value 2
        values_1, body_1 = stmt.branches[1]
        assert len(values_1) == 1
        assert isinstance(values_1[0], IRLiteral)
        assert values_1[0].value == 2

    def test_case_multiple_match_values(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
CASE x OF
    1, 2: y := 10;
    3: y := 20;
END_CASE;
END_PROGRAM""")
        stmt = prog.body[0]
        values_0, _ = stmt.branches[0]
        assert len(values_0) == 2
        assert values_0[0].value == 1
        assert values_0[1].value == 2

    def test_case_with_else(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
CASE x OF
    1: y := 10;
ELSE
    y := 0;
END_CASE;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRCase)
        assert stmt.else_body is not None
        assert len(stmt.else_body) == 1
        assert isinstance(stmt.else_body[0], IRAssignment)

    def test_case_body_assignments(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; y : INT := 0; END_VAR
CASE x OF
    1: y := 10;
    2: y := 20;
    3: y := 30;
ELSE
    y := 0;
END_CASE;
END_PROGRAM""")
        stmt = prog.body[0]
        assert len(stmt.branches) == 3
        # Verify each branch body
        _, body_0 = stmt.branches[0]
        assert body_0[0].value.value == 10
        _, body_1 = stmt.branches[1]
        assert body_1[0].value.value == 20
        _, body_2 = stmt.branches[2]
        assert body_2[0].value.value == 30
        # Else body
        assert stmt.else_body[0].value.value == 0


class TestForLoop:
    """Test FOR loop transformation."""

    def test_simple_for(self):
        prog = _transform_source("""PROGRAM T
VAR i : INT; x : INT := 0; END_VAR
FOR i := 1 TO 10 BY 2 DO
    x := x + i;
END_FOR;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRForLoop)
        assert stmt.var == "i"

    def test_for_start_value(self):
        prog = _transform_source("""PROGRAM T
VAR i : INT; x : INT := 0; END_VAR
FOR i := 1 TO 10 BY 2 DO
    x := x + i;
END_FOR;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt.start, IRLiteral)
        assert stmt.start.value == 1

    def test_for_end_value(self):
        prog = _transform_source("""PROGRAM T
VAR i : INT; x : INT := 0; END_VAR
FOR i := 1 TO 10 BY 2 DO
    x := x + i;
END_FOR;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt.end, IRLiteral)
        assert stmt.end.value == 10

    def test_for_step_value(self):
        prog = _transform_source("""PROGRAM T
VAR i : INT; x : INT := 0; END_VAR
FOR i := 1 TO 10 BY 2 DO
    x := x + i;
END_FOR;
END_PROGRAM""")
        stmt = prog.body[0]
        assert stmt.step is not None
        assert isinstance(stmt.step, IRLiteral)
        assert stmt.step.value == 2

    def test_for_without_step(self):
        prog = _transform_source("""PROGRAM T
VAR i : INT; x : INT := 0; END_VAR
FOR i := 1 TO 10 DO
    x := x + i;
END_FOR;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRForLoop)
        assert stmt.step is None

    def test_for_body(self):
        prog = _transform_source("""PROGRAM T
VAR i : INT; x : INT := 0; END_VAR
FOR i := 1 TO 10 BY 2 DO
    x := x + i;
END_FOR;
END_PROGRAM""")
        stmt = prog.body[0]
        assert len(stmt.body) == 1
        assert isinstance(stmt.body[0], IRAssignment)
        assert isinstance(stmt.body[0].value, IRBinaryOp)
        assert stmt.body[0].value.op == "+"


class TestWhileLoop:
    """Test WHILE loop transformation."""

    def test_simple_while(self):
        prog = _transform_source("""PROGRAM T
VAR x : BOOL := TRUE; y : INT := 0; END_VAR
WHILE x DO
    y := y + 1;
END_WHILE;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRWhileLoop)
        assert isinstance(stmt.condition, IRVarRef)
        assert stmt.condition.name == "x"

    def test_while_body(self):
        prog = _transform_source("""PROGRAM T
VAR x : BOOL := TRUE; y : INT := 0; END_VAR
WHILE x DO
    y := y + 1;
END_WHILE;
END_PROGRAM""")
        stmt = prog.body[0]
        assert len(stmt.body) == 1
        assert isinstance(stmt.body[0], IRAssignment)

    def test_while_with_comparison_condition(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 10; END_VAR
WHILE x > 0 DO
    x := x - 1;
END_WHILE;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRWhileLoop)
        assert isinstance(stmt.condition, IRBinaryOp)
        assert stmt.condition.op == ">"


class TestRepeatLoop:
    """Test REPEAT..UNTIL loop transformation."""

    def test_simple_repeat(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 10; END_VAR
REPEAT
    x := x - 1;
UNTIL x <= 0
END_REPEAT;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRRepeatLoop)

    def test_repeat_condition(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 10; END_VAR
REPEAT
    x := x - 1;
UNTIL x <= 0
END_REPEAT;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt.condition, IRBinaryOp)
        assert stmt.condition.op == "<="

    def test_repeat_body(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 10; END_VAR
REPEAT
    x := x - 1;
UNTIL x <= 0
END_REPEAT;
END_PROGRAM""")
        stmt = prog.body[0]
        assert len(stmt.body) == 1
        assert isinstance(stmt.body[0], IRAssignment)

    def test_repeat_with_variable_condition(self):
        prog = _transform_source("""PROGRAM T
VAR x : INT := 0; done : BOOL := FALSE; END_VAR
REPEAT
    x := x + 1;
UNTIL done
END_REPEAT;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRRepeatLoop)
        assert isinstance(stmt.condition, IRVarRef)
        assert stmt.condition.name == "done"


class TestControlFlowFixture:
    """Test transformation of the control_flow.st fixture with all constructs."""

    @pytest.fixture
    def program(self) -> IRProgram:
        source = _load_fixture("control_flow.st")
        return _transform_source(source)

    def test_program_name(self, program: IRProgram):
        assert program.name == "ControlFlowTest"

    def test_has_four_statements(self, program: IRProgram):
        assert len(program.body) == 4

    def test_first_is_case(self, program: IRProgram):
        assert isinstance(program.body[0], IRCase)

    def test_second_is_for(self, program: IRProgram):
        assert isinstance(program.body[1], IRForLoop)

    def test_third_is_while(self, program: IRProgram):
        assert isinstance(program.body[2], IRWhileLoop)

    def test_fourth_is_repeat(self, program: IRProgram):
        assert isinstance(program.body[3], IRRepeatLoop)
