"""Tests for the st2js type system module.

Tests type checking, type resolution, and coercion insertion for
IEC 61131-3 types (BOOL, INT, DINT, REAL).
"""
import pytest

from st2js.ir import (
    IECType,
    IRAssignment,
    IRBinaryOp,
    IRLiteral,
    IRProgram,
    IRTypeCoercion,
    IRUnaryOp,
    IRVariable,
    IRVarRef,
)
from st2js.errors import STTypeError
from st2js.type_system import check_types, resolve_type


# --- Helper to build a minimal IRProgram ---

def _make_program(
    body,
    inputs=None,
    outputs=None,
    locals_=None,
):
    """Build a minimal IRProgram for testing."""
    return IRProgram(
        name="TestProg",
        inputs=inputs or [],
        outputs=outputs or [],
        locals=locals_ or [],
        fb_instances=[],
        body=body,
        function_blocks=[],
    )


def _vars_dict(program):
    """Build a name->IRVariable dict from program declarations."""
    result = {}
    for v in program.inputs + program.outputs + program.locals:
        result[v.name] = v
    return result


# --- Tests for resolve_type ---

class TestResolveType:
    """Tests for resolve_type() function."""

    def test_bool_literal(self):
        lit = IRLiteral(value=True, iec_type=IECType.BOOL)
        assert resolve_type(lit, {}) == IECType.BOOL

    def test_int_literal(self):
        lit = IRLiteral(value=42, iec_type=IECType.INT)
        assert resolve_type(lit, {}) == IECType.INT

    def test_dint_literal(self):
        lit = IRLiteral(value=100000, iec_type=IECType.DINT)
        assert resolve_type(lit, {}) == IECType.DINT

    def test_real_literal(self):
        lit = IRLiteral(value=3.14, iec_type=IECType.REAL)
        assert resolve_type(lit, {}) == IECType.REAL

    def test_variable_reference(self):
        variables = {
            "x": IRVariable(name="x", iec_type=IECType.INT),
        }
        ref = IRVarRef(name="x")
        assert resolve_type(ref, variables) == IECType.INT

    def test_variable_reference_real(self):
        variables = {
            "temp": IRVariable(name="temp", iec_type=IECType.REAL),
        }
        ref = IRVarRef(name="temp")
        assert resolve_type(ref, variables) == IECType.REAL

    def test_arithmetic_int_int(self):
        """INT + INT -> INT."""
        variables = {}
        expr = IRBinaryOp(
            op="+",
            left=IRLiteral(value=1, iec_type=IECType.INT),
            right=IRLiteral(value=2, iec_type=IECType.INT),
        )
        assert resolve_type(expr, variables) == IECType.INT

    def test_arithmetic_int_real_promotes_to_real(self):
        """INT + REAL -> REAL."""
        variables = {}
        expr = IRBinaryOp(
            op="+",
            left=IRLiteral(value=1, iec_type=IECType.INT),
            right=IRLiteral(value=2.0, iec_type=IECType.REAL),
        )
        assert resolve_type(expr, variables) == IECType.REAL

    def test_arithmetic_dint_int(self):
        """DINT + INT -> DINT."""
        variables = {}
        expr = IRBinaryOp(
            op="+",
            left=IRLiteral(value=100000, iec_type=IECType.DINT),
            right=IRLiteral(value=1, iec_type=IECType.INT),
        )
        assert resolve_type(expr, variables) == IECType.DINT

    def test_comparison_returns_bool(self):
        """x > 10 -> BOOL."""
        variables = {}
        expr = IRBinaryOp(
            op=">",
            left=IRLiteral(value=5, iec_type=IECType.INT),
            right=IRLiteral(value=10, iec_type=IECType.INT),
        )
        assert resolve_type(expr, variables) == IECType.BOOL

    def test_logical_and_returns_bool(self):
        """BOOL AND BOOL -> BOOL."""
        variables = {}
        expr = IRBinaryOp(
            op="AND",
            left=IRLiteral(value=True, iec_type=IECType.BOOL),
            right=IRLiteral(value=False, iec_type=IECType.BOOL),
        )
        assert resolve_type(expr, variables) == IECType.BOOL

    def test_unary_not_returns_bool(self):
        """NOT bool_expr -> BOOL."""
        variables = {}
        expr = IRUnaryOp(
            op="NOT",
            operand=IRLiteral(value=True, iec_type=IECType.BOOL),
        )
        assert resolve_type(expr, variables) == IECType.BOOL

    def test_unary_negate_preserves_type(self):
        """-int_expr -> INT."""
        variables = {}
        expr = IRUnaryOp(
            op="-",
            operand=IRLiteral(value=5, iec_type=IECType.INT),
        )
        assert resolve_type(expr, variables) == IECType.INT

    def test_type_coercion_node_returns_to_type(self):
        """IRTypeCoercion reports its to_type."""
        coercion = IRTypeCoercion(
            expr=IRLiteral(value=True, iec_type=IECType.BOOL),
            from_type=IECType.BOOL,
            to_type=IECType.INT,
        )
        assert resolve_type(coercion, {}) == IECType.INT

    def test_modulo_int_int(self):
        """INT MOD INT -> INT."""
        expr = IRBinaryOp(
            op="MOD",
            left=IRLiteral(value=10, iec_type=IECType.INT),
            right=IRLiteral(value=3, iec_type=IECType.INT),
        )
        assert resolve_type(expr, {}) == IECType.INT


# --- Tests for check_types ---

class TestCheckTypesBoolCoercion:
    """Tests for BOOL-to-INT coercion in arithmetic contexts."""

    def test_bool_in_addition_gets_coerced(self):
        """x + boolVar should wrap boolVar in IRTypeCoercion(BOOL->INT)."""
        program = _make_program(
            inputs=[],
            outputs=[],
            locals_=[
                IRVariable(name="x", iec_type=IECType.INT, is_local=True),
                IRVariable(name="flag", iec_type=IECType.BOOL, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRBinaryOp(
                        op="+",
                        left=IRVarRef(name="x"),
                        right=IRVarRef(name="flag"),
                    ),
                ),
            ],
        )

        result = check_types(program)

        # The assignment value should now have a coercion on the right operand
        assign = result.body[0]
        assert isinstance(assign, IRAssignment)
        binop = assign.value
        assert isinstance(binop, IRBinaryOp)
        assert isinstance(binop.right, IRTypeCoercion)
        assert binop.right.from_type == IECType.BOOL
        assert binop.right.to_type == IECType.INT
        # Left operand should remain unchanged (INT + INT is fine)
        assert isinstance(binop.left, IRVarRef)

    def test_bool_literal_in_multiplication_gets_coerced(self):
        """5 * TRUE should wrap TRUE in IRTypeCoercion(BOOL->INT)."""
        program = _make_program(
            locals_=[
                IRVariable(name="y", iec_type=IECType.INT, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="y"),
                    value=IRBinaryOp(
                        op="*",
                        left=IRLiteral(value=5, iec_type=IECType.INT),
                        right=IRLiteral(value=True, iec_type=IECType.BOOL),
                    ),
                ),
            ],
        )

        result = check_types(program)
        binop = result.body[0].value
        assert isinstance(binop.right, IRTypeCoercion)
        assert binop.right.from_type == IECType.BOOL
        assert binop.right.to_type == IECType.INT


class TestCheckTypesRealToIntCoercion:
    """Tests for REAL-to-INT coercion on assignment."""

    def test_real_assigned_to_int_gets_coerced(self):
        """intVar := realExpr should wrap realExpr in IRTypeCoercion(REAL->INT)."""
        program = _make_program(
            locals_=[
                IRVariable(name="count", iec_type=IECType.INT, is_local=True),
                IRVariable(name="temp", iec_type=IECType.REAL, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="count"),
                    value=IRVarRef(name="temp"),
                ),
            ],
        )

        result = check_types(program)
        assign = result.body[0]
        assert isinstance(assign.value, IRTypeCoercion)
        assert assign.value.from_type == IECType.REAL
        assert assign.value.to_type == IECType.INT

    def test_real_expression_assigned_to_dint_gets_coerced(self):
        """dintVar := 1.5 + 2.5 should coerce the REAL result to DINT."""
        program = _make_program(
            locals_=[
                IRVariable(name="big", iec_type=IECType.DINT, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="big"),
                    value=IRBinaryOp(
                        op="+",
                        left=IRLiteral(value=1.5, iec_type=IECType.REAL),
                        right=IRLiteral(value=2.5, iec_type=IECType.REAL),
                    ),
                ),
            ],
        )

        result = check_types(program)
        assign = result.body[0]
        assert isinstance(assign.value, IRTypeCoercion)
        assert assign.value.from_type == IECType.REAL
        assert assign.value.to_type == IECType.DINT


class TestCheckTypesNoCoercion:
    """Tests for cases where no coercion should be inserted."""

    def test_int_assigned_to_real_no_coercion(self):
        """realVar := intExpr -- JS handles INT->REAL automatically."""
        program = _make_program(
            locals_=[
                IRVariable(name="result", iec_type=IECType.REAL, is_local=True),
                IRVariable(name="count", iec_type=IECType.INT, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="result"),
                    value=IRVarRef(name="count"),
                ),
            ],
        )

        result = check_types(program)
        assign = result.body[0]
        # Value should remain a plain IRVarRef, not wrapped in coercion
        assert isinstance(assign.value, IRVarRef)
        assert assign.value.name == "count"

    def test_int_assigned_to_int_no_coercion(self):
        """intVar := intExpr -- same type, no coercion."""
        program = _make_program(
            locals_=[
                IRVariable(name="a", iec_type=IECType.INT, is_local=True),
                IRVariable(name="b", iec_type=IECType.INT, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="a"),
                    value=IRVarRef(name="b"),
                ),
            ],
        )

        result = check_types(program)
        assign = result.body[0]
        assert isinstance(assign.value, IRVarRef)

    def test_bool_in_logical_context_no_coercion(self):
        """BOOL AND BOOL -- logical context, no coercion needed."""
        program = _make_program(
            locals_=[
                IRVariable(name="a", iec_type=IECType.BOOL, is_local=True),
                IRVariable(name="b", iec_type=IECType.BOOL, is_local=True),
                IRVariable(name="c", iec_type=IECType.BOOL, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="c"),
                    value=IRBinaryOp(
                        op="AND",
                        left=IRVarRef(name="a"),
                        right=IRVarRef(name="b"),
                    ),
                ),
            ],
        )

        result = check_types(program)
        assign = result.body[0]
        binop = assign.value
        assert isinstance(binop, IRBinaryOp)
        # No coercion on either operand
        assert isinstance(binop.left, IRVarRef)
        assert isinstance(binop.right, IRVarRef)


class TestCheckTypesTypeErrors:
    """Tests for type errors on incompatible types."""

    def test_string_in_arithmetic_raises_type_error(self):
        """STRING + INT should raise STTypeError."""
        program = _make_program(
            locals_=[
                IRVariable(name="s", iec_type=IECType.STRING, is_local=True),
                IRVariable(name="x", iec_type=IECType.INT, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRBinaryOp(
                        op="+",
                        left=IRVarRef(name="s"),
                        right=IRLiteral(value=1, iec_type=IECType.INT),
                    ),
                ),
            ],
        )

        with pytest.raises(STTypeError):
            check_types(program)

    def test_string_assigned_to_int_raises_type_error(self):
        """intVar := stringVar should raise STTypeError."""
        program = _make_program(
            locals_=[
                IRVariable(name="x", iec_type=IECType.INT, is_local=True),
                IRVariable(name="s", iec_type=IECType.STRING, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRVarRef(name="s"),
                ),
            ],
        )

        with pytest.raises(STTypeError):
            check_types(program)


class TestCheckTypesPreservesStructure:
    """Tests that check_types preserves the overall program structure."""

    def test_preserves_program_name_and_variables(self):
        """check_types should not modify program metadata."""
        program = _make_program(
            inputs=[IRVariable(name="inp", iec_type=IECType.INT, is_input=True)],
            outputs=[IRVariable(name="out", iec_type=IECType.INT, is_output=True)],
            locals_=[IRVariable(name="tmp", iec_type=IECType.INT, is_local=True)],
            body=[
                IRAssignment(
                    target=IRVarRef(name="out"),
                    value=IRVarRef(name="inp"),
                ),
            ],
        )

        result = check_types(program)
        assert result.name == "TestProg"
        assert len(result.inputs) == 1
        assert len(result.outputs) == 1
        assert len(result.locals) == 1
        assert result.inputs[0].name == "inp"
