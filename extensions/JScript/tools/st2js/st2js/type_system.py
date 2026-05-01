#!/usr/bin/env python3
"""Type system for the st2js converter.

Provides type resolution and type checking for IEC 61131-3 types.
Walks the IR tree and inserts IRTypeCoercion nodes where implicit
type conversions are needed for correct JavaScript code generation.

Supported coercions:
  - BOOL in arithmetic context -> IRTypeCoercion(BOOL, INT)
  - REAL assigned to INT/DINT variable -> IRTypeCoercion(REAL, INT/DINT)
  - INT assigned to REAL -> no coercion (JS handles automatically)

Type errors are raised for incompatible types (e.g., STRING in arithmetic).
"""
from __future__ import annotations

import copy
import warnings
from dataclasses import replace

from st2js.errors import STTypeError

# Module-level flag: when True, undefined variables produce warnings instead of errors
_ignore_undefined = False
from st2js.fb_registry import get_fb_info
from st2js.ir import (
    IECType,
    IRAssignment,
    IRBinaryOp,
    IRCase,
    IRExpression,
    IRArrayAccess,
    IRFBCall,
    IRFBInstance,
    IRFieldAccess,
    IRForLoop,
    IRFunctionCall,
    IRIfElse,
    IRLiteral,
    IRProgram,
    IRContinueStatement,
    IRExitStatement,
    IRProgramCall,
    IRRepeatLoop,
    IRReturnStatement,
    IRStatement,
    IRTypeCoercion,
    IRUnaryOp,
    IRVariable,
    IRVarRef,
    IRWhileLoop,
)


# Operators grouped by category
_ARITHMETIC_OPS = {"+", "-", "*", "/", "MOD"}
_COMPARISON_OPS = {"<", ">", "<=", ">=", "=", "<>"}
_LOGICAL_OPS = {"AND", "OR", "XOR"}

# Types that can participate in arithmetic
_NUMERIC_TYPES = {
    IECType.BOOL,
    IECType.SINT, IECType.INT, IECType.DINT, IECType.LINT,
    IECType.USINT, IECType.UINT, IECType.UDINT, IECType.ULINT,
    IECType.BYTE, IECType.WORD, IECType.DWORD, IECType.LWORD,
    IECType.REAL, IECType.LREAL,
}

# Integer types (for REAL->INT coercion detection)
_INTEGER_TYPES = {
    IECType.SINT, IECType.INT, IECType.DINT, IECType.LINT,
    IECType.USINT, IECType.UINT, IECType.UDINT, IECType.ULINT,
    IECType.BYTE, IECType.WORD, IECType.DWORD, IECType.LWORD,
}


def resolve_type(
    expr: IRExpression,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IECType:
    """Determine the IEC type of an expression.

    Args:
        expr: The IR expression node to resolve.
        variables: A name->IRVariable dict for variable type lookup.
        fb_instances: Optional list of FB instances for field access resolution.

    Returns:
        The resolved IECType for the expression.

    Raises:
        STTypeError: If the expression contains incompatible types.
    """
    if isinstance(expr, IRLiteral):
        return expr.iec_type

    if isinstance(expr, IRVarRef):
        var = variables.get(expr.name)
        if var is None:
            if _ignore_undefined:
                warnings.warn(f"undefined variable '{expr.name}'; assuming INT", stacklevel=2)
                return IECType.INT
            raise STTypeError(f"undefined variable '{expr.name}'")
        return var.iec_type

    if isinstance(expr, IRTypeCoercion):
        return expr.to_type

    if isinstance(expr, IRUnaryOp):
        operand_type = resolve_type(expr.operand, variables, fb_instances)
        if expr.op == "NOT":
            return IECType.BOOL
        # Negation preserves type
        return operand_type

    if isinstance(expr, IRBinaryOp):
        left_type = resolve_type(expr.left, variables, fb_instances)
        right_type = resolve_type(expr.right, variables, fb_instances)

        if expr.op in _COMPARISON_OPS:
            return IECType.BOOL

        if expr.op in _LOGICAL_OPS:
            return IECType.BOOL

        if expr.op in _ARITHMETIC_OPS:
            return _resolve_arithmetic_type(left_type, right_type, expr.op)

    if isinstance(expr, IRFieldAccess):
        return _resolve_field_access_type(expr, fb_instances, variables)

    if isinstance(expr, IRFBCall):
        # FB call type depends on which output is accessed;
        # standalone FB call is a statement, not typed
        return IECType.VOID

    if isinstance(expr, IRFunctionCall):
        # Without a full function registry we cannot determine the return
        # type precisely.  Default to INT; the generated JS will still work
        # since JS is dynamically typed.
        return IECType.INT

    if isinstance(expr, IRArrayAccess):
        # Resolve the base array type; for now return INT as fallback
        return IECType.INT

    raise STTypeError(f"cannot resolve type for expression: {type(expr).__name__}")


# Type name string to IECType mapping for FB output resolution
_FB_TYPE_MAP: dict[str, IECType] = {
    "BOOL": IECType.BOOL,
    "SINT": IECType.SINT,
    "INT": IECType.INT,
    "DINT": IECType.DINT,
    "LINT": IECType.LINT,
    "USINT": IECType.USINT,
    "UINT": IECType.UINT,
    "UDINT": IECType.UDINT,
    "ULINT": IECType.ULINT,
    "BYTE": IECType.BYTE,
    "WORD": IECType.WORD,
    "DWORD": IECType.DWORD,
    "LWORD": IECType.LWORD,
    "REAL": IECType.REAL,
    "LREAL": IECType.LREAL,
    "TIME": IECType.TIME,
    "STRING": IECType.STRING,
    "WSTRING": IECType.WSTRING,
}


def _resolve_field_access_type(
    expr: IRFieldAccess,
    fb_instances: list[IRFBInstance] | None = None,
    variables: dict[str, IRVariable] | None = None,
) -> IECType:
    """Resolve the type of a field access expression.

    For FB instances, looks up the output type from the fb_registry.
    For STRUCT variables, looks up the field type from struct_fields.
    Returns VOID if the instance/variable or field is not found.
    """
    if isinstance(expr.obj, IRVarRef):
        obj_name = expr.obj.name

        # Check FB instances first
        if fb_instances is not None:
            for fb_inst in fb_instances:
                if fb_inst.name == obj_name:
                    fb_info = get_fb_info(fb_inst.fb_type)
                    if fb_info is not None:
                        # Look up the output field type
                        for output_name, output_type_str in fb_info.outputs:
                            if output_name == expr.field:
                                return _FB_TYPE_MAP.get(output_type_str, IECType.VOID)
                    return IECType.VOID

        # Check STRUCT variables
        if variables is not None:
            var = variables.get(obj_name)
            if var is not None and var.iec_type == IECType.STRUCT and var.struct_fields:
                for field in var.struct_fields:
                    if field.name == expr.field:
                        return field.iec_type

    return IECType.VOID


def _resolve_arithmetic_type(
    left: IECType, right: IECType, op: str
) -> IECType:
    """Resolve the result type of an arithmetic operation.

    Rules:
      - Any operand REAL -> result is REAL
      - DINT + INT -> DINT
      - INT + INT -> INT
      - BOOL is treated as INT in arithmetic (coercion handled separately)

    Raises:
        STTypeError: If a non-numeric type is used in arithmetic.
    """
    # Treat VOID (unknown type from unresolved FB fields etc.) as INT
    if left == IECType.VOID:
        left = IECType.INT
    if right == IECType.VOID:
        right = IECType.INT

    if left not in _NUMERIC_TYPES:
        raise STTypeError(
            f"cannot use {left.value} in arithmetic expression (operator '{op}')"
        )
    if right not in _NUMERIC_TYPES:
        raise STTypeError(
            f"cannot use {right.value} in arithmetic expression (operator '{op}')"
        )

    _FLOAT_TYPES = {IECType.REAL, IECType.LREAL}

    # BOOL in arithmetic is treated as INT for result type purposes
    effective_left = IECType.INT if left == IECType.BOOL else left
    effective_right = IECType.INT if right == IECType.BOOL else right

    # Float promotion: any float operand -> LREAL if either is LREAL, else REAL
    if effective_left in _FLOAT_TYPES or effective_right in _FLOAT_TYPES:
        if effective_left == IECType.LREAL or effective_right == IECType.LREAL:
            return IECType.LREAL
        return IECType.REAL

    # Integer promotion: pick the widest type.  For JS they are all just
    # numbers, so use a simple size ranking: DINT > INT > everything else.
    _INT_RANK = {
        IECType.LINT: 4, IECType.ULINT: 4, IECType.LWORD: 4,
        IECType.DINT: 3, IECType.UDINT: 3, IECType.DWORD: 3,
        IECType.INT: 2, IECType.UINT: 2, IECType.WORD: 2,
        IECType.SINT: 1, IECType.USINT: 1, IECType.BYTE: 1,
    }
    rank_l = _INT_RANK.get(effective_left, 2)
    rank_r = _INT_RANK.get(effective_right, 2)
    if rank_l >= rank_r:
        return effective_left
    return effective_right


def check_types(program: IRProgram, *, ignore_undefined: bool = False) -> IRProgram:
    """Walk the IR tree, inserting type coercion nodes where needed.

    Modifies a copy of the program; the original is not mutated.

    Args:
        program: The IRProgram to type-check.
        ignore_undefined: If True, undefined variables produce warnings
            instead of errors (assumes INT type).

    Returns:
        A new IRProgram with IRTypeCoercion nodes inserted.

    Raises:
        STTypeError: If incompatible types are detected.
    """
    global _ignore_undefined
    _ignore_undefined = ignore_undefined
    variables = _build_variable_dict(program)
    fb_instances = program.fb_instances
    new_body = [_check_statement(stmt, variables, fb_instances) for stmt in program.body]

    # Type-check secondary programs (globals from main are visible)
    new_secondary = []
    for sec_prog in program.secondary_programs:
        sec_prog_with_ctx = replace(
            sec_prog,
            globals=program.globals,
            gvl_field_map=program.gvl_field_map,
            inputs=program.inputs + sec_prog.inputs,
            outputs=program.outputs + sec_prog.outputs,
        )
        sec_vars = _build_variable_dict(sec_prog_with_ctx)
        sec_body = [_check_statement(stmt, sec_vars, sec_prog.fb_instances) for stmt in sec_prog.body]
        new_secondary.append(replace(sec_prog, body=sec_body))

    return replace(program, body=new_body, secondary_programs=new_secondary)


def _build_variable_dict(program: IRProgram) -> dict[str, IRVariable]:
    """Build a name->IRVariable dict from all program variable sections.

    Also adds flat aliases for GVL fields so unqualified access
    (e.g., DI_GRU_QG1_IsOn instead of IO.DI_GRU_QG1_IsOn) resolves correctly.
    """
    result: dict[str, IRVariable] = {}
    for var in program.globals + program.inputs + program.outputs + program.locals:
        result[var.name] = var
    # Add GVL field aliases for unqualified access
    for field_name, (gvl_name, iec_type) in program.gvl_field_map.items():
        if field_name not in result:
            result[field_name] = IRVariable(name=field_name, iec_type=iec_type)
    return result


def _check_statement(
    stmt: IRStatement,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IRStatement:
    """Type-check a single statement, returning a potentially modified copy."""
    if isinstance(stmt, IRAssignment):
        return _check_assignment(stmt, variables, fb_instances)

    if isinstance(stmt, IRIfElse):
        return _check_if_else(stmt, variables, fb_instances)

    if isinstance(stmt, IRCase):
        return _check_case(stmt, variables, fb_instances)

    if isinstance(stmt, IRForLoop):
        return _check_for_loop(stmt, variables, fb_instances)

    if isinstance(stmt, IRWhileLoop):
        new_cond = _check_expression(stmt.condition, variables, fb_instances)
        new_body = [_check_statement(s, variables, fb_instances) for s in stmt.body]
        return replace(stmt, condition=new_cond, body=new_body)

    if isinstance(stmt, IRRepeatLoop):
        new_cond = _check_expression(stmt.condition, variables, fb_instances)
        new_body = [_check_statement(s, variables, fb_instances) for s in stmt.body]
        return replace(stmt, condition=new_cond, body=new_body)

    if isinstance(stmt, IRFBCall):
        new_args = {
            k: _check_expression(v, variables, fb_instances)
            for k, v in stmt.arguments.items()
        }
        return replace(stmt, arguments=new_args)

    if isinstance(stmt, IRProgramCall):
        return stmt

    if isinstance(stmt, (IRExitStatement, IRContinueStatement, IRReturnStatement)):
        return stmt

    return stmt


def _check_assignment(
    assign: IRAssignment,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IRAssignment:
    """Type-check an assignment, inserting coercions on the value if needed."""
    new_value = _check_expression(assign.value, variables, fb_instances)
    value_type = resolve_type(new_value, variables, fb_instances)

    # Determine target type
    target_type = _resolve_target_type(assign.target, variables)

    if target_type is not None:
        new_value = _coerce_assignment(new_value, value_type, target_type)

    return replace(assign, value=new_value)


def _resolve_target_type(
    target: IRExpression, variables: dict[str, IRVariable]
) -> IECType | None:
    """Resolve the type of an assignment target."""
    if isinstance(target, IRVarRef):
        var = variables.get(target.name)
        if var is not None:
            return var.iec_type
    return None


def _coerce_assignment(
    value: IRExpression,
    value_type: IECType,
    target_type: IECType,
) -> IRExpression:
    """Insert coercion node if value type doesn't match target type.

    Rules:
      - REAL -> INT/DINT: insert coercion (Math.trunc in codegen)
      - INT -> REAL: no coercion (JS handles)
      - STRING -> numeric: type error
      - Same type: no coercion
    """
    if value_type == target_type:
        return value

    # VOID (unresolved type) is compatible with anything -- pass through
    if value_type == IECType.VOID or target_type == IECType.VOID:
        return value

    # REAL assigned to integer type -> coerce
    if value_type == IECType.REAL and target_type in _INTEGER_TYPES:
        return IRTypeCoercion(expr=value, from_type=IECType.REAL, to_type=target_type)

    # Integer types assigned to REAL -> no coercion (JS auto-promotes)
    if value_type in (_INTEGER_TYPES | {IECType.BOOL}) and target_type == IECType.REAL:
        return value

    # BOOL <-> integer: coerce in both directions
    if value_type == IECType.BOOL and target_type in _INTEGER_TYPES:
        return IRTypeCoercion(expr=value, from_type=IECType.BOOL, to_type=target_type)
    if value_type in _INTEGER_TYPES and target_type == IECType.BOOL:
        return IRTypeCoercion(expr=value, from_type=value_type, to_type=IECType.BOOL)

    # INT <-> DINT: compatible, no coercion needed
    if value_type in _INTEGER_TYPES and target_type in _INTEGER_TYPES:
        return value

    # TIME <-> integer: TIME is stored as milliseconds (integer) in JS
    if value_type == IECType.TIME and target_type in _INTEGER_TYPES:
        return value
    if value_type in _INTEGER_TYPES and target_type == IECType.TIME:
        return value

    # Incompatible types
    raise STTypeError(
        f"cannot assign {value_type.value} to {target_type.value}"
    )


def _check_expression(
    expr: IRExpression,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IRExpression:
    """Type-check an expression, inserting BOOL->INT coercions in arithmetic."""
    if isinstance(expr, (IRLiteral, IRVarRef)):
        return expr

    if isinstance(expr, IRTypeCoercion):
        new_inner = _check_expression(expr.expr, variables, fb_instances)
        return replace(expr, expr=new_inner)

    if isinstance(expr, IRUnaryOp):
        new_operand = _check_expression(expr.operand, variables, fb_instances)
        return replace(expr, operand=new_operand)

    if isinstance(expr, IRBinaryOp):
        return _check_binary_op(expr, variables, fb_instances)

    if isinstance(expr, IRFBCall):
        new_args = {
            k: _check_expression(v, variables, fb_instances)
            for k, v in expr.arguments.items()
        }
        return replace(expr, arguments=new_args)

    # FieldAccess, ArrayAccess, etc. -- pass through for now
    return expr


def _check_binary_op(
    expr: IRBinaryOp,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IRBinaryOp:
    """Type-check a binary operation, inserting BOOL->INT coercions for arithmetic."""
    new_left = _check_expression(expr.left, variables, fb_instances)
    new_right = _check_expression(expr.right, variables, fb_instances)

    if expr.op in _ARITHMETIC_OPS:
        left_type = resolve_type(new_left, variables, fb_instances)
        right_type = resolve_type(new_right, variables, fb_instances)

        # Validate types are numeric (will raise STTypeError if not)
        _resolve_arithmetic_type(left_type, right_type, expr.op)

        # Insert BOOL->INT coercion for arithmetic context
        if left_type == IECType.BOOL:
            new_left = IRTypeCoercion(
                expr=new_left, from_type=IECType.BOOL, to_type=IECType.INT
            )
        if right_type == IECType.BOOL:
            new_right = IRTypeCoercion(
                expr=new_right, from_type=IECType.BOOL, to_type=IECType.INT
            )

    return replace(expr, left=new_left, right=new_right)


def _check_if_else(
    stmt: IRIfElse,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IRIfElse:
    """Type-check an IF/ELSIF/ELSE statement."""
    new_cond = _check_expression(stmt.condition, variables, fb_instances)
    new_then = [_check_statement(s, variables, fb_instances) for s in stmt.then_body]

    new_elsif = []
    for cond, body in stmt.elsif_branches:
        new_elsif.append((
            _check_expression(cond, variables, fb_instances),
            [_check_statement(s, variables, fb_instances) for s in body],
        ))

    new_else = None
    if stmt.else_body is not None:
        new_else = [_check_statement(s, variables, fb_instances) for s in stmt.else_body]

    return replace(
        stmt,
        condition=new_cond,
        then_body=new_then,
        elsif_branches=new_elsif,
        else_body=new_else,
    )


def _check_case(
    stmt: IRCase,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IRCase:
    """Type-check a CASE statement."""
    new_selector = _check_expression(stmt.selector, variables, fb_instances)

    new_branches = []
    for case_values, body in stmt.branches:
        new_values = [_check_expression(v, variables, fb_instances) for v in case_values]
        new_body = [_check_statement(s, variables, fb_instances) for s in body]
        new_branches.append((new_values, new_body))

    new_else = None
    if stmt.else_body is not None:
        new_else = [_check_statement(s, variables, fb_instances) for s in stmt.else_body]

    return replace(
        stmt,
        selector=new_selector,
        branches=new_branches,
        else_body=new_else,
    )


def _check_for_loop(
    stmt: IRForLoop,
    variables: dict[str, IRVariable],
    fb_instances: list[IRFBInstance] | None = None,
) -> IRForLoop:
    """Type-check a FOR loop."""
    new_start = _check_expression(stmt.start, variables, fb_instances)
    new_end = _check_expression(stmt.end, variables, fb_instances)
    new_step = _check_expression(stmt.step, variables, fb_instances) if stmt.step else None
    new_body = [_check_statement(s, variables, fb_instances) for s in stmt.body]

    return replace(
        stmt,
        start=new_start,
        end=new_end,
        step=new_step,
        body=new_body,
    )
