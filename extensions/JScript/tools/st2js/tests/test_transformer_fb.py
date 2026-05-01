"""Tests for function block support in transformer.

Tests cover:
- FB instance detection in VAR declarations (TON, CTU, R_TRIG, RS, etc.)
- FB call statement transformation (FunctionCallStatement -> IRFBCall)
- FB output field access (myTimer.Q -> IRFieldAccess)
- FUNCTION_BLOCK declaration -> IRFunctionBlock
- All 10 standard FB types recognized
"""
import os

import pytest

from st2js.ir import (
    IECType,
    IRAssignment,
    IRBinaryOp,
    IRFBCall,
    IRFBInstance,
    IRFieldAccess,
    IRFunctionBlock,
    IRIfElse,
    IRLiteral,
    IRProgram,
    IRVarRef,
    IRVariable,
)
from st2js.errors import UnsupportedError


FIXTURES_DIR = os.path.join(os.path.dirname(__file__), "fixtures")


def _transform_source(source: str) -> IRProgram:
    """Helper: parse ST source and transform to IR."""
    from st2js.parser import parse_st
    from st2js.transformer import transform

    result = parse_st(source)
    return transform(result)


def _load_fixture(name: str) -> str:
    """Load a test fixture file."""
    with open(os.path.join(FIXTURES_DIR, name)) as f:
        return f.read()


class TestFBInstanceDetection:
    """Test that VAR declarations with known FB types become IRFBInstance."""

    def test_ton_instance(self):
        prog = _transform_source("""PROGRAM T
VAR
    myTimer : TON;
END_VAR
END_PROGRAM""")
        assert len(prog.fb_instances) == 1
        assert prog.fb_instances[0].name == "myTimer"
        assert prog.fb_instances[0].fb_type == "TON"

    def test_ctu_instance(self):
        prog = _transform_source("""PROGRAM T
VAR
    myCounter : CTU;
END_VAR
END_PROGRAM""")
        assert len(prog.fb_instances) == 1
        assert prog.fb_instances[0].name == "myCounter"
        assert prog.fb_instances[0].fb_type == "CTU"

    def test_r_trig_instance(self):
        prog = _transform_source("""PROGRAM T
VAR
    myTrigger : R_TRIG;
END_VAR
END_PROGRAM""")
        assert len(prog.fb_instances) == 1
        assert prog.fb_instances[0].name == "myTrigger"
        assert prog.fb_instances[0].fb_type == "R_TRIG"

    def test_rs_instance(self):
        prog = _transform_source("""PROGRAM T
VAR
    myLatch : RS;
END_VAR
END_PROGRAM""")
        assert len(prog.fb_instances) == 1
        assert prog.fb_instances[0].name == "myLatch"
        assert prog.fb_instances[0].fb_type == "RS"

    def test_fb_instance_not_in_locals(self):
        """FB instances should NOT appear in the locals list."""
        prog = _transform_source("""PROGRAM T
VAR
    myTimer : TON;
    state : INT := 0;
END_VAR
END_PROGRAM""")
        assert len(prog.fb_instances) == 1
        assert len(prog.locals) == 1
        assert prog.locals[0].name == "state"

    def test_multiple_fb_instances(self):
        prog = _transform_source("""PROGRAM T
VAR
    t1 : TON;
    t2 : TOF;
    c1 : CTU;
END_VAR
END_PROGRAM""")
        assert len(prog.fb_instances) == 3
        assert prog.fb_instances[0].fb_type == "TON"
        assert prog.fb_instances[1].fb_type == "TOF"
        assert prog.fb_instances[2].fb_type == "CTU"

    def test_all_10_standard_fb_types(self):
        """All 10 standard FBs are recognized as FB instances."""
        fb_types = ["TON", "TOF", "TP", "CTU", "CTD", "CTUD", "RS", "SR", "R_TRIG", "F_TRIG"]
        for fb_type in fb_types:
            prog = _transform_source(f"""PROGRAM T
VAR
    inst : {fb_type};
END_VAR
END_PROGRAM""")
            assert len(prog.fb_instances) == 1, f"Failed for {fb_type}"
            assert prog.fb_instances[0].fb_type == fb_type, f"Wrong type for {fb_type}"


class TestFBCallTransformation:
    """Test FB call statements are transformed to IRFBCall."""

    def test_ton_call_with_named_params(self):
        prog = _transform_source("""PROGRAM T
VAR_INPUT
    Enable : BOOL;
END_VAR
VAR
    myTimer : TON;
END_VAR
myTimer(IN := Enable, PT := T#3000ms);
END_PROGRAM""")
        assert len(prog.body) == 1
        call = prog.body[0]
        assert isinstance(call, IRFBCall)
        assert call.instance == "myTimer"
        # IN is an update param, should stay in call arguments
        assert "IN" in call.arguments
        assert isinstance(call.arguments["IN"], IRVarRef)
        assert call.arguments["IN"].name == "Enable"
        # PT is a constructor arg, should be extracted to FB instance
        assert "PT" not in call.arguments
        fb = prog.fb_instances[0]
        assert "PT" in fb.constructor_args
        assert isinstance(fb.constructor_args["PT"], IRLiteral)
        assert fb.constructor_args["PT"].value == 3000

    def test_ctu_call(self):
        prog = _transform_source("""PROGRAM T
VAR_INPUT
    Pulse : BOOL;
    ResetCmd : BOOL;
END_VAR
VAR
    myCounter : CTU;
END_VAR
myCounter(CU := Pulse, RESET := ResetCmd, PV := 10);
END_PROGRAM""")
        call = prog.body[0]
        assert isinstance(call, IRFBCall)
        assert call.instance == "myCounter"
        # CU and RESET are update params
        assert "CU" in call.arguments
        assert "RESET" in call.arguments
        # PV is a constructor arg, should be extracted
        assert "PV" not in call.arguments
        fb = prog.fb_instances[0]
        assert "PV" in fb.constructor_args

    def test_r_trig_call(self):
        prog = _transform_source("""PROGRAM T
VAR_INPUT
    Enable : BOOL;
END_VAR
VAR
    myTrigger : R_TRIG;
END_VAR
myTrigger(CLK := Enable);
END_PROGRAM""")
        call = prog.body[0]
        assert isinstance(call, IRFBCall)
        assert call.instance == "myTrigger"
        assert "CLK" in call.arguments

    def test_pt_param_is_time_literal(self):
        """PT parameter should be extracted as a TIME literal in constructor_args."""
        prog = _transform_source("""PROGRAM T
VAR_INPUT
    Enable : BOOL;
END_VAR
VAR
    myTimer : TON;
END_VAR
myTimer(IN := Enable, PT := T#3000ms);
END_PROGRAM""")
        fb = prog.fb_instances[0]
        pt_arg = fb.constructor_args["PT"]
        assert isinstance(pt_arg, IRLiteral)
        assert pt_arg.iec_type == IECType.TIME
        assert pt_arg.value == 3000


class TestFBFieldAccess:
    """Test FB output access (myTimer.Q) -> IRFieldAccess."""

    def test_timer_q_access(self):
        prog = _transform_source("""PROGRAM T
VAR_OUTPUT
    Output : BOOL;
END_VAR
VAR
    myTimer : TON;
END_VAR
Output := myTimer.Q;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt, IRAssignment)
        assert isinstance(stmt.value, IRFieldAccess)
        assert isinstance(stmt.value.obj, IRVarRef)
        assert stmt.value.obj.name == "myTimer"
        assert stmt.value.field == "Q"

    def test_counter_cv_access(self):
        prog = _transform_source("""PROGRAM T
VAR_OUTPUT
    Count : INT;
END_VAR
VAR
    myCounter : CTU;
END_VAR
Count := myCounter.CV;
END_PROGRAM""")
        stmt = prog.body[0]
        assert isinstance(stmt.value, IRFieldAccess)
        assert stmt.value.obj.name == "myCounter"
        assert stmt.value.field == "CV"

    def test_fb_field_in_fb_call_param(self):
        """myTrigger.Q used as parameter value in another FB call."""
        prog = _transform_source("""PROGRAM T
VAR_INPUT
    ResetCmd : BOOL;
END_VAR
VAR
    myTrigger : R_TRIG;
    myLatch : RS;
END_VAR
myLatch(SET := myTrigger.Q, RESET1 := ResetCmd);
END_PROGRAM""")
        call = prog.body[0]
        assert isinstance(call, IRFBCall)
        set_arg = call.arguments["SET"]
        assert isinstance(set_arg, IRFieldAccess)
        assert set_arg.obj.name == "myTrigger"
        assert set_arg.field == "Q"


class TestFBCallsFixture:
    """Test transformation of the fb_calls.st fixture."""

    @pytest.fixture
    def program(self) -> IRProgram:
        source = _load_fixture("fb_calls.st")
        return _transform_source(source)

    def test_has_four_fb_instances(self, program: IRProgram):
        assert len(program.fb_instances) == 4

    def test_fb_instance_types(self, program: IRProgram):
        types = {fb.name: fb.fb_type for fb in program.fb_instances}
        assert types["myTimer"] == "TON"
        assert types["myCounter"] == "CTU"
        assert types["myTrigger"] == "R_TRIG"
        assert types["myLatch"] == "RS"

    def test_has_no_extra_locals(self, program: IRProgram):
        """FB instances should not be in locals list."""
        assert len(program.locals) == 0

    def test_body_has_six_statements(self, program: IRProgram):
        """4 FB calls + 2 assignments = 6 statements."""
        assert len(program.body) == 6

    def test_first_statement_is_fb_call(self, program: IRProgram):
        assert isinstance(program.body[0], IRFBCall)
        assert program.body[0].instance == "myTimer"

    def test_second_statement_is_assignment_with_field_access(self, program: IRProgram):
        stmt = program.body[1]
        assert isinstance(stmt, IRAssignment)
        assert isinstance(stmt.value, IRFieldAccess)
        assert stmt.value.obj.name == "myTimer"
        assert stmt.value.field == "Q"


class TestFunctionBlockDeclaration:
    """Test FUNCTION_BLOCK declaration -> IRFunctionBlock (Task 7.1)."""

    def test_fb_declaration_produces_ir_function_block(self):
        prog = _transform_source("""
FUNCTION_BLOCK MyFB
VAR_INPUT
    SetPoint : REAL;
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
    Enable : BOOL;
END_VAR
END_PROGRAM
""")
        assert len(prog.function_blocks) == 1
        fb = prog.function_blocks[0]
        assert isinstance(fb, IRFunctionBlock)
        assert fb.name == "MyFB"

    def test_fb_has_inputs(self):
        prog = _transform_source("""
FUNCTION_BLOCK MyFB
VAR_INPUT
    SetPoint : REAL;
    Enable : BOOL;
END_VAR
counter := 0;
END_FUNCTION_BLOCK

PROGRAM Main
END_PROGRAM
""")
        fb = prog.function_blocks[0]
        assert len(fb.inputs) == 2
        assert fb.inputs[0].name == "SetPoint"
        assert fb.inputs[0].iec_type == IECType.REAL
        assert fb.inputs[1].name == "Enable"
        assert fb.inputs[1].iec_type == IECType.BOOL

    def test_fb_has_outputs(self):
        prog = _transform_source("""
FUNCTION_BLOCK MyFB
VAR_OUTPUT
    Active : BOOL;
END_VAR
Active := TRUE;
END_FUNCTION_BLOCK

PROGRAM Main
END_PROGRAM
""")
        fb = prog.function_blocks[0]
        assert len(fb.outputs) == 1
        assert fb.outputs[0].name == "Active"

    def test_fb_has_locals(self):
        prog = _transform_source("""
FUNCTION_BLOCK MyFB
VAR
    counter : INT := 0;
END_VAR
counter := counter + 1;
END_FUNCTION_BLOCK

PROGRAM Main
END_PROGRAM
""")
        fb = prog.function_blocks[0]
        assert len(fb.locals) == 1
        assert fb.locals[0].name == "counter"
        assert fb.locals[0].initial_value == 0

    def test_fb_has_body(self):
        prog = _transform_source("""
FUNCTION_BLOCK MyFB
VAR
    counter : INT := 0;
END_VAR
counter := counter + 1;
END_FUNCTION_BLOCK

PROGRAM Main
END_PROGRAM
""")
        fb = prog.function_blocks[0]
        assert len(fb.body) == 1
        assert isinstance(fb.body[0], IRAssignment)

    def test_fb_declaration_fixture(self):
        source = _load_fixture("fb_declaration.st")
        prog = _transform_source(source)
        assert len(prog.function_blocks) == 1
        fb = prog.function_blocks[0]
        assert fb.name == "MyController"
        assert len(fb.inputs) == 2
        assert len(fb.outputs) == 1
        assert len(fb.locals) == 1
        assert len(fb.body) == 2  # assignment + if/else
