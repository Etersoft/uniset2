"""Tests for st2js IR dataclasses."""
from __future__ import annotations

from st2js.ir import (
    IECType,
    IRVariable,
    IRFBInstance,
    IRLiteral,
    IRVarRef,
    IRBinaryOp,
    IRUnaryOp,
    IRFieldAccess,
    IRArrayAccess,
    IRTypeCoercion,
    IRFBCall,
    IRAssignment,
    IRIfElse,
    IRCase,
    IRForLoop,
    IRWhileLoop,
    IRRepeatLoop,
    IRFunctionBlock,
    IRProgram,
)


class TestIECType:
    """Tests for the IECType enum."""

    def test_all_types_defined(self):
        expected = {
            "BOOL",
            "SINT", "INT", "DINT", "LINT",
            "USINT", "UINT", "UDINT", "ULINT",
            "BYTE", "WORD", "DWORD", "LWORD",
            "REAL", "LREAL",
            "TIME", "STRING", "WSTRING",
            "STRUCT", "ARRAY", "FB_INSTANCE", "VOID",
        }
        actual = {t.name for t in IECType}
        assert actual == expected

    def test_enum_values_are_strings(self):
        assert IECType.BOOL.value == "BOOL"
        assert IECType.INT.value == "INT"
        assert IECType.VOID.value == "VOID"


class TestIRVariable:
    """Tests for IRVariable dataclass."""

    def test_minimal_construction(self):
        var = IRVariable(name="x", iec_type=IECType.INT)
        assert var.name == "x"
        assert var.iec_type == IECType.INT
        assert var.initial_value is None
        assert var.is_input is False
        assert var.is_output is False
        assert var.is_local is False
        assert var.struct_type_name is None
        assert var.array_element_type is None
        assert var.array_size is None

    def test_full_construction(self):
        var = IRVariable(
            name="temp",
            iec_type=IECType.REAL,
            initial_value=0.0,
            is_input=True,
        )
        assert var.name == "temp"
        assert var.iec_type == IECType.REAL
        assert var.initial_value == 0.0
        assert var.is_input is True

    def test_array_variable(self):
        var = IRVariable(
            name="data",
            iec_type=IECType.ARRAY,
            array_element_type=IECType.INT,
            array_size=10,
        )
        assert var.iec_type == IECType.ARRAY
        assert var.array_element_type == IECType.INT
        assert var.array_size == 10

    def test_struct_variable(self):
        var = IRVariable(
            name="s",
            iec_type=IECType.STRUCT,
            struct_type_name="MotorData",
        )
        assert var.struct_type_name == "MotorData"


class TestIRFBInstance:
    """Tests for IRFBInstance dataclass."""

    def test_construction_no_args(self):
        inst = IRFBInstance(name="rs1", fb_type="RS", constructor_args={})
        assert inst.name == "rs1"
        assert inst.fb_type == "RS"
        assert inst.constructor_args == {}

    def test_construction_with_args(self):
        pt_literal = IRLiteral(value=3000, iec_type=IECType.TIME)
        inst = IRFBInstance(
            name="ton1",
            fb_type="TON",
            constructor_args={"PT": pt_literal},
        )
        assert inst.constructor_args["PT"].value == 3000


class TestExpressionNodes:
    """Tests for expression IR nodes."""

    def test_literal_int(self):
        lit = IRLiteral(value=42, iec_type=IECType.INT)
        assert lit.value == 42
        assert lit.iec_type == IECType.INT

    def test_literal_bool(self):
        lit = IRLiteral(value=True, iec_type=IECType.BOOL)
        assert lit.value is True

    def test_literal_real(self):
        lit = IRLiteral(value=3.14, iec_type=IECType.REAL)
        assert lit.value == 3.14

    def test_literal_string(self):
        lit = IRLiteral(value="hello", iec_type=IECType.STRING)
        assert lit.value == "hello"

    def test_var_ref(self):
        ref = IRVarRef(name="temperature")
        assert ref.name == "temperature"

    def test_binary_op_arithmetic(self):
        left = IRLiteral(value=1, iec_type=IECType.INT)
        right = IRLiteral(value=2, iec_type=IECType.INT)
        op = IRBinaryOp(op="+", left=left, right=right)
        assert op.op == "+"
        assert op.left is left
        assert op.right is right

    def test_binary_op_comparison(self):
        left = IRVarRef(name="x")
        right = IRLiteral(value=10, iec_type=IECType.INT)
        op = IRBinaryOp(op=">", left=left, right=right)
        assert op.op == ">"

    def test_binary_op_logical(self):
        left = IRVarRef(name="a")
        right = IRVarRef(name="b")
        op = IRBinaryOp(op="AND", left=left, right=right)
        assert op.op == "AND"

    def test_unary_op_not(self):
        operand = IRVarRef(name="flag")
        op = IRUnaryOp(op="NOT", operand=operand)
        assert op.op == "NOT"
        assert op.operand is operand

    def test_unary_op_negate(self):
        operand = IRLiteral(value=5, iec_type=IECType.INT)
        op = IRUnaryOp(op="-", operand=operand)
        assert op.op == "-"

    def test_field_access(self):
        obj = IRVarRef(name="motor")
        fa = IRFieldAccess(obj=obj, field="speed")
        assert fa.obj is obj
        assert fa.field == "speed"

    def test_array_access(self):
        arr = IRVarRef(name="data")
        idx = IRLiteral(value=3, iec_type=IECType.INT)
        aa = IRArrayAccess(array=arr, index=idx)
        assert aa.array is arr
        assert aa.index is idx

    def test_type_coercion(self):
        expr = IRVarRef(name="flag")
        tc = IRTypeCoercion(expr=expr, from_type=IECType.BOOL, to_type=IECType.INT)
        assert tc.from_type == IECType.BOOL
        assert tc.to_type == IECType.INT
        assert tc.expr is expr

    def test_fb_call(self):
        call = IRFBCall(
            instance="ton1",
            arguments={"IN": IRVarRef(name="start"), "PT": IRLiteral(value=5000, iec_type=IECType.TIME)},
        )
        assert call.instance == "ton1"
        assert len(call.arguments) == 2
        assert call.arguments["IN"].name == "start"


class TestStatementNodes:
    """Tests for statement IR nodes."""

    def test_assignment(self):
        target = IRVarRef(name="x")
        value = IRLiteral(value=42, iec_type=IECType.INT)
        assign = IRAssignment(target=target, value=value)
        assert assign.target is target
        assert assign.value is value

    def test_if_else_minimal(self):
        cond = IRVarRef(name="flag")
        body = [IRAssignment(target=IRVarRef(name="x"), value=IRLiteral(value=1, iec_type=IECType.INT))]
        stmt = IRIfElse(
            condition=cond,
            then_body=body,
            elsif_branches=[],
            else_body=None,
        )
        assert stmt.condition is cond
        assert len(stmt.then_body) == 1
        assert stmt.elsif_branches == []
        assert stmt.else_body is None

    def test_if_else_with_elsif_and_else(self):
        cond1 = IRBinaryOp(op=">", left=IRVarRef(name="x"), right=IRLiteral(value=10, iec_type=IECType.INT))
        cond2 = IRBinaryOp(op=">", left=IRVarRef(name="x"), right=IRLiteral(value=5, iec_type=IECType.INT))
        then_body = [IRAssignment(target=IRVarRef(name="y"), value=IRLiteral(value=1, iec_type=IECType.INT))]
        elsif_body = [IRAssignment(target=IRVarRef(name="y"), value=IRLiteral(value=2, iec_type=IECType.INT))]
        else_body = [IRAssignment(target=IRVarRef(name="y"), value=IRLiteral(value=3, iec_type=IECType.INT))]
        stmt = IRIfElse(
            condition=cond1,
            then_body=then_body,
            elsif_branches=[(cond2, elsif_body)],
            else_body=else_body,
        )
        assert len(stmt.elsif_branches) == 1
        assert stmt.else_body is not None

    def test_case(self):
        selector = IRVarRef(name="state")
        branch1_vals = [IRLiteral(value=1, iec_type=IECType.INT)]
        branch1_body = [IRAssignment(target=IRVarRef(name="x"), value=IRLiteral(value=10, iec_type=IECType.INT))]
        branch2_vals = [IRLiteral(value=2, iec_type=IECType.INT), IRLiteral(value=3, iec_type=IECType.INT)]
        branch2_body = [IRAssignment(target=IRVarRef(name="x"), value=IRLiteral(value=20, iec_type=IECType.INT))]
        else_body = [IRAssignment(target=IRVarRef(name="x"), value=IRLiteral(value=0, iec_type=IECType.INT))]
        stmt = IRCase(
            selector=selector,
            branches=[(branch1_vals, branch1_body), (branch2_vals, branch2_body)],
            else_body=else_body,
        )
        assert len(stmt.branches) == 2
        assert len(stmt.branches[1][0]) == 2  # branch 2 has two case values
        assert stmt.else_body is not None

    def test_for_loop(self):
        stmt = IRForLoop(
            var="i",
            start=IRLiteral(value=1, iec_type=IECType.INT),
            end=IRLiteral(value=10, iec_type=IECType.INT),
            step=IRLiteral(value=2, iec_type=IECType.INT),
            body=[IRAssignment(target=IRVarRef(name="sum"), value=IRBinaryOp(op="+", left=IRVarRef(name="sum"), right=IRVarRef(name="i")))],
        )
        assert stmt.var == "i"
        assert stmt.step.value == 2
        assert len(stmt.body) == 1

    def test_for_loop_no_step(self):
        stmt = IRForLoop(
            var="j",
            start=IRLiteral(value=0, iec_type=IECType.INT),
            end=IRLiteral(value=5, iec_type=IECType.INT),
            step=None,
            body=[],
        )
        assert stmt.step is None

    def test_while_loop(self):
        stmt = IRWhileLoop(
            condition=IRBinaryOp(op="<", left=IRVarRef(name="x"), right=IRLiteral(value=100, iec_type=IECType.INT)),
            body=[IRAssignment(target=IRVarRef(name="x"), value=IRBinaryOp(op="+", left=IRVarRef(name="x"), right=IRLiteral(value=1, iec_type=IECType.INT)))],
        )
        assert len(stmt.body) == 1

    def test_repeat_loop(self):
        stmt = IRRepeatLoop(
            condition=IRBinaryOp(op=">=", left=IRVarRef(name="x"), right=IRLiteral(value=10, iec_type=IECType.INT)),
            body=[IRAssignment(target=IRVarRef(name="x"), value=IRBinaryOp(op="+", left=IRVarRef(name="x"), right=IRLiteral(value=1, iec_type=IECType.INT)))],
        )
        assert len(stmt.body) == 1


class TestIRFunctionBlock:
    """Tests for IRFunctionBlock dataclass."""

    def test_construction(self):
        fb = IRFunctionBlock(
            name="MyFB",
            inputs=[IRVariable(name="in1", iec_type=IECType.BOOL)],
            outputs=[IRVariable(name="out1", iec_type=IECType.BOOL)],
            locals=[IRVariable(name="state", iec_type=IECType.INT, initial_value=0)],
            fb_instances=[],
            body=[
                IRAssignment(
                    target=IRVarRef(name="out1"),
                    value=IRVarRef(name="in1"),
                ),
            ],
        )
        assert fb.name == "MyFB"
        assert len(fb.inputs) == 1
        assert len(fb.outputs) == 1
        assert len(fb.locals) == 1
        assert len(fb.body) == 1


class TestIRProgram:
    """Tests for IRProgram dataclass."""

    def test_minimal_construction(self):
        prog = IRProgram(
            name="Main",
            inputs=[],
            outputs=[],
            locals=[],
            fb_instances=[],
            body=[],
            function_blocks=[],
        )
        assert prog.name == "Main"
        assert prog.function_blocks == []

    def test_full_construction(self):
        """Construct a program with variables, FB instances, body, and function blocks."""
        fb = IRFunctionBlock(
            name="ToggleFB",
            inputs=[IRVariable(name="trigger", iec_type=IECType.BOOL)],
            outputs=[IRVariable(name="state", iec_type=IECType.BOOL)],
            locals=[],
            fb_instances=[],
            body=[
                IRIfElse(
                    condition=IRVarRef(name="trigger"),
                    then_body=[IRAssignment(target=IRVarRef(name="state"), value=IRUnaryOp(op="NOT", operand=IRVarRef(name="state")))],
                    elsif_branches=[],
                    else_body=None,
                ),
            ],
        )

        prog = IRProgram(
            name="Thermostat",
            inputs=[
                IRVariable(name="Temperature", iec_type=IECType.REAL, is_input=True),
                IRVariable(name="Setpoint", iec_type=IECType.REAL, is_input=True),
            ],
            outputs=[
                IRVariable(name="HeaterOn", iec_type=IECType.BOOL, is_output=True),
            ],
            locals=[
                IRVariable(name="hysteresis", iec_type=IECType.REAL, initial_value=2.0),
            ],
            fb_instances=[
                IRFBInstance(name="delay", fb_type="TON", constructor_args={"PT": IRLiteral(value=5000, iec_type=IECType.TIME)}),
            ],
            body=[
                IRIfElse(
                    condition=IRBinaryOp(op="<", left=IRVarRef(name="Temperature"), right=IRBinaryOp(op="-", left=IRVarRef(name="Setpoint"), right=IRVarRef(name="hysteresis"))),
                    then_body=[IRAssignment(target=IRVarRef(name="HeaterOn"), value=IRLiteral(value=True, iec_type=IECType.BOOL))],
                    elsif_branches=[],
                    else_body=[IRAssignment(target=IRVarRef(name="HeaterOn"), value=IRLiteral(value=False, iec_type=IECType.BOOL))],
                ),
            ],
            function_blocks=[fb],
        )
        assert prog.name == "Thermostat"
        assert len(prog.inputs) == 2
        assert len(prog.outputs) == 1
        assert len(prog.locals) == 1
        assert len(prog.fb_instances) == 1
        assert len(prog.body) == 1
        assert len(prog.function_blocks) == 1
        assert prog.function_blocks[0].name == "ToggleFB"


class TestNestedExpressions:
    """Tests for nested/composed expression trees."""

    def test_nested_binary_ops(self):
        """(a + b) * c"""
        expr = IRBinaryOp(
            op="*",
            left=IRBinaryOp(
                op="+",
                left=IRVarRef(name="a"),
                right=IRVarRef(name="b"),
            ),
            right=IRVarRef(name="c"),
        )
        assert expr.op == "*"
        assert expr.left.op == "+"

    def test_field_access_on_array_element(self):
        """data[i].value"""
        expr = IRFieldAccess(
            obj=IRArrayAccess(
                array=IRVarRef(name="data"),
                index=IRVarRef(name="i"),
            ),
            field="value",
        )
        assert expr.field == "value"
        assert expr.obj.array.name == "data"

    def test_type_coercion_in_binary_op(self):
        """BOOL flag used in arithmetic: (flag ? 1 : 0) + count"""
        expr = IRBinaryOp(
            op="+",
            left=IRTypeCoercion(
                expr=IRVarRef(name="flag"),
                from_type=IECType.BOOL,
                to_type=IECType.INT,
            ),
            right=IRVarRef(name="count"),
        )
        assert expr.left.from_type == IECType.BOOL
        assert expr.left.to_type == IECType.INT
