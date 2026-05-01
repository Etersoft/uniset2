#!/usr/bin/env python3
"""Internal Representation (IR) dataclasses for the st2js converter.

These dataclasses represent the intermediate form between the parsed ST AST
(from blark) and the generated JavaScript output. The IR is designed to map
cleanly to UniSet2 JScript semantics.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Union


class IECType(Enum):
    """IEC 61131-3 data types supported by the converter."""

    BOOL = "BOOL"
    # Signed integers
    SINT = "SINT"       # 8-bit signed
    INT = "INT"         # 16-bit signed
    DINT = "DINT"       # 32-bit signed
    LINT = "LINT"       # 64-bit signed
    # Unsigned integers
    USINT = "USINT"     # 8-bit unsigned
    UINT = "UINT"       # 16-bit unsigned
    UDINT = "UDINT"     # 32-bit unsigned
    ULINT = "ULINT"     # 64-bit unsigned
    # Bit-string types (treated as unsigned integers in JS)
    BYTE = "BYTE"       # 8-bit
    WORD = "WORD"       # 16-bit
    DWORD = "DWORD"     # 32-bit
    LWORD = "LWORD"     # 64-bit
    # Floating point
    REAL = "REAL"       # 32-bit float
    LREAL = "LREAL"     # 64-bit float
    # Other
    TIME = "TIME"
    STRING = "STRING"
    WSTRING = "WSTRING"
    STRUCT = "STRUCT"
    ARRAY = "ARRAY"
    FB_INSTANCE = "FB_INSTANCE"
    VOID = "VOID"


# --- Expression Nodes ---

@dataclass
class IRLiteral:
    """A literal value (integer, real, bool, string, time)."""

    value: Any
    iec_type: IECType


@dataclass
class IRVarRef:
    """A reference to a variable by name."""

    name: str


@dataclass
class IRBinaryOp:
    """A binary operation: arithmetic, comparison, or logical.

    Supported operators: +, -, *, /, MOD, AND, OR, XOR, <, >, <=, >=, =, <>
    """

    op: str
    left: IRExpression
    right: IRExpression


@dataclass
class IRUnaryOp:
    """A unary operation: NOT or negation.

    Supported operators: NOT, -
    """

    op: str
    operand: IRExpression


@dataclass
class IRFieldAccess:
    """Access to a struct field: obj.field."""

    obj: IRExpression
    field: str


@dataclass
class IRArrayAccess:
    """Access to an array element: array[index]."""

    array: IRExpression
    index: IRExpression


@dataclass
class IRTypeCoercion:
    """Explicit type coercion inserted by the type checker."""

    expr: IRExpression
    from_type: IECType
    to_type: IECType


@dataclass
class IRFunctionCall:
    """A FUNCTION call used as an expression (e.g. ``result := myFunc(a, b)``).

    Unlike FB calls, function calls are stateless and return a value directly.
    Parameters may be positional or named.
    """

    name: str
    arguments: list[tuple[str | None, IRExpression]]  # (param_name_or_None, value)


@dataclass
class IRFBCall:
    """A function block call with named arguments.

    Used both as an expression (when reading FB outputs) and as a statement
    (when calling the FB's update method).
    """

    instance: str
    arguments: dict[str, IRExpression]


# Type alias for all expression nodes
IRExpression = Union[
    IRLiteral,
    IRVarRef,
    IRBinaryOp,
    IRUnaryOp,
    IRFieldAccess,
    IRArrayAccess,
    IRTypeCoercion,
    IRFunctionCall,
    IRFBCall,
]


# --- Statement Nodes ---

@dataclass
class IRAssignment:
    """Assignment statement: target := value."""

    target: IRExpression
    value: IRExpression


@dataclass
class IRIfElse:
    """IF/ELSIF/ELSE statement.

    elsif_branches is a list of (condition, body) tuples.
    else_body is None if there is no ELSE clause.
    """

    condition: IRExpression
    then_body: list[IRStatement]
    elsif_branches: list[tuple[IRExpression, list[IRStatement]]]
    else_body: list[IRStatement] | None


@dataclass
class IRCase:
    """CASE..OF statement.

    Each branch is a tuple of (case_values, body) where case_values
    is a list of expressions that match the selector.
    """

    selector: IRExpression
    branches: list[tuple[list[IRExpression], list[IRStatement]]]
    else_body: list[IRStatement] | None


@dataclass
class IRForLoop:
    """FOR loop: FOR var := start TO end [BY step] DO body END_FOR."""

    var: str
    start: IRExpression
    end: IRExpression
    step: IRExpression | None
    body: list[IRStatement]


@dataclass
class IRWhileLoop:
    """WHILE loop: WHILE condition DO body END_WHILE."""

    condition: IRExpression
    body: list[IRStatement]


@dataclass
class IRRepeatLoop:
    """REPEAT..UNTIL loop: REPEAT body UNTIL condition END_REPEAT.

    Note: the condition is the exit condition (loop continues while NOT condition).
    """

    condition: IRExpression
    body: list[IRStatement]


@dataclass
class IRExitStatement:
    """EXIT statement — break out of the innermost loop."""
    pass


@dataclass
class IRContinueStatement:
    """CONTINUE statement — skip to next iteration of the innermost loop."""
    pass


@dataclass
class IRReturnStatement:
    """RETURN statement — return from the current POU."""
    pass


@dataclass
class IRProgramCall:
    """A call to another PROGRAM (emitted as a function call in JS)."""

    program_name: str


# Type alias for all statement nodes
IRStatement = Union[
    IRAssignment,
    IRIfElse,
    IRCase,
    IRForLoop,
    IRWhileLoop,
    IRRepeatLoop,
    IRFBCall,
    IRProgramCall,
    IRExitStatement,
    IRContinueStatement,
    IRReturnStatement,
]


# --- Variable and Instance Declarations ---

@dataclass
class IRStructField:
    """A field within a STRUCT type definition."""

    name: str
    iec_type: IECType
    initial_value: Any | None = None


@dataclass
class IRVariable:
    """A variable declaration with type information.

    The is_input/is_output/is_local flags are convenience markers;
    IRProgram and IRFunctionBlock also separate variables into
    distinct lists by section.
    """

    name: str
    iec_type: IECType
    initial_value: Any | None = None
    is_input: bool = False
    is_output: bool = False
    is_local: bool = False
    struct_type_name: str | None = None
    struct_fields: list[IRStructField] | None = None
    array_element_type: IECType | None = None
    array_size: int | None = None
    array_lower_bound: int = 1


@dataclass
class IRFBInstance:
    """A function block instance declaration.

    constructor_args maps parameter names to their initial value expressions.
    """

    name: str
    fb_type: str
    constructor_args: dict[str, IRExpression] = field(default_factory=dict)


# --- Top-Level Structures ---

@dataclass
class IRFunctionBlock:
    """A FUNCTION_BLOCK declaration.

    Represents a user-defined function block with its own variables and body.
    """

    name: str
    inputs: list[IRVariable]
    outputs: list[IRVariable]
    locals: list[IRVariable]
    fb_instances: list[IRFBInstance]
    body: list[IRStatement]


@dataclass
class IRProgram:
    """A PROGRAM declaration -- the top-level compilation unit.

    Contains the program's variables, FB instances, body statements,
    and any FUNCTION_BLOCK definitions found in the same source file.

    The main program (entry point for uniset_on_step) stores additional
    programs in `secondary_programs`. Each secondary PROGRAM is converted
    to a standalone JS function that can be called from the main program.
    """

    name: str
    inputs: list[IRVariable]
    outputs: list[IRVariable]
    locals: list[IRVariable]
    fb_instances: list[IRFBInstance]
    body: list[IRStatement]
    function_blocks: list[IRFunctionBlock]
    globals: list[IRVariable] = field(default_factory=list)
    # Maps flat variable name -> (gvl_object_name, field_iec_type) for GVL field access
    gvl_field_map: dict[str, tuple[str, 'IECType']] = field(default_factory=dict)
    secondary_programs: list[IRProgram] = field(default_factory=list)
