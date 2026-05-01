"""Tests for Phase 6: Advanced Types (STRUCT, ARRAY, TIME, STRING).

Tests cover:
- Task 6.1: STRUCT type declaration -> JS object literal; field access -> .field
- Task 6.2: ARRAY declaration -> new Array(size).fill(default); index -> arr[i - 1]
- Task 6.3: TIME literals -> millisecond integers
- Task 6.4: STRING type -> empty string default; string literals -> JS quotes
"""
from __future__ import annotations

import os

import pytest

from st2js.ir import (
    IECType,
    IRArrayAccess,
    IRAssignment,
    IRFieldAccess,
    IRLiteral,
    IRProgram,
    IRStructField,
    IRVarRef,
    IRVariable,
)
from st2js.mapping import MappingOptions, SensorEntry, SensorMapping
from st2js.parser import parse_st
from st2js.transformer import transform


def _transform_source(source: str) -> IRProgram:
    """Helper: parse ST source and transform to IR."""
    result = parse_st(source)
    return transform(result)


def _load_fixture(name: str) -> str:
    """Load a test fixture file."""
    fixture_dir = os.path.join(os.path.dirname(__file__), "fixtures")
    with open(os.path.join(fixture_dir, name)) as f:
        return f.read()


def _make_program(
    name: str = "Main",
    inputs: list | None = None,
    outputs: list | None = None,
    locals_: list | None = None,
    body: list | None = None,
) -> IRProgram:
    """Helper to create an IRProgram with defaults."""
    return IRProgram(
        name=name,
        inputs=inputs or [],
        outputs=outputs or [],
        locals=locals_ or [],
        fb_instances=[],
        body=body or [],
        function_blocks=[],
    )


def _make_mapping(
    inputs: list[SensorEntry] | None = None,
    outputs: list[SensorEntry] | None = None,
) -> SensorMapping:
    """Helper to create a SensorMapping with defaults."""
    return SensorMapping(
        inputs=inputs or [],
        outputs=outputs or [],
        options=MappingOptions(),
    )


# ===== Task 6.1: STRUCT Support =====


class TestStructTransformer:
    """Test STRUCT type declaration parsing and transformation."""

    def test_struct_type_parsed_as_struct_variable(self):
        source = """
TYPE MyStruct :
STRUCT
    field1 : INT;
    field2 : REAL := 3.14;
    active : BOOL;
END_STRUCT
END_TYPE

PROGRAM Test
VAR
    data : MyStruct;
END_VAR
data.field1 := 42;
END_PROGRAM
"""
        program = _transform_source(source)
        assert len(program.locals) == 1
        var = program.locals[0]
        assert var.name == "data"
        assert var.iec_type == IECType.STRUCT
        assert var.struct_type_name == "MyStruct"

    def test_struct_fields_populated(self):
        source = """
TYPE MyStruct :
STRUCT
    field1 : INT;
    field2 : REAL := 3.14;
    active : BOOL;
END_STRUCT
END_TYPE

PROGRAM Test
VAR
    data : MyStruct;
END_VAR
data.field1 := 42;
END_PROGRAM
"""
        program = _transform_source(source)
        var = program.locals[0]
        assert var.struct_fields is not None
        assert len(var.struct_fields) == 3

        f1 = var.struct_fields[0]
        assert f1.name == "field1"
        assert f1.iec_type == IECType.INT
        assert f1.initial_value is None

        f2 = var.struct_fields[1]
        assert f2.name == "field2"
        assert f2.iec_type == IECType.REAL
        assert f2.initial_value == 3.14

        f3 = var.struct_fields[2]
        assert f3.name == "active"
        assert f3.iec_type == IECType.BOOL
        assert f3.initial_value is None

    def test_struct_field_access_produces_ir_field_access(self):
        source = """
TYPE MyStruct :
STRUCT
    field1 : INT;
END_STRUCT
END_TYPE

PROGRAM Test
VAR
    data : MyStruct;
END_VAR
data.field1 := 42;
END_PROGRAM
"""
        program = _transform_source(source)
        assert len(program.body) == 1
        assign = program.body[0]
        assert isinstance(assign, IRAssignment)
        target = assign.target
        assert isinstance(target, IRFieldAccess)
        assert isinstance(target.obj, IRVarRef)
        assert target.obj.name == "data"
        assert target.field == "field1"

    def test_struct_field_read_produces_ir_field_access(self):
        source = """
TYPE MyStruct :
STRUCT
    value : INT;
END_STRUCT
END_TYPE

PROGRAM Test
VAR
    data : MyStruct;
    x : INT;
END_VAR
x := data.value;
END_PROGRAM
"""
        program = _transform_source(source)
        assign = program.body[0]
        assert isinstance(assign, IRAssignment)
        value = assign.value
        assert isinstance(value, IRFieldAccess)
        assert isinstance(value.obj, IRVarRef)
        assert value.obj.name == "data"
        assert value.field == "value"


class TestStructCodegen:
    """Test STRUCT code generation."""

    def test_struct_variable_emits_object_literal(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(
                    name="data",
                    iec_type=IECType.STRUCT,
                    struct_type_name="MyStruct",
                    struct_fields=[
                        IRStructField(name="field1", iec_type=IECType.INT),
                        IRStructField(name="field2", iec_type=IECType.REAL, initial_value=3.14),
                        IRStructField(name="active", iec_type=IECType.BOOL),
                    ],
                    is_local=True,
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "let data = { field1: 0, field2: 3.14, active: false };" in js

    def test_struct_field_access_emits_dot_notation(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="data", iec_type=IECType.STRUCT, struct_type_name="X",
                           struct_fields=[IRStructField(name="f", iec_type=IECType.INT)],
                           is_local=True),
                IRVariable(name="x", iec_type=IECType.INT, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRFieldAccess(obj=IRVarRef(name="data"), field="f"),
                    value=IRLiteral(value=42, iec_type=IECType.INT),
                ),
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRFieldAccess(obj=IRVarRef(name="data"), field="f"),
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "data.f = 42;" in js
        assert "x = data.f;" in js

    def test_struct_with_all_default_fields(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(
                    name="s",
                    iec_type=IECType.STRUCT,
                    struct_type_name="S",
                    struct_fields=[
                        IRStructField(name="a", iec_type=IECType.INT),
                        IRStructField(name="b", iec_type=IECType.BOOL),
                        IRStructField(name="c", iec_type=IECType.REAL),
                    ],
                    is_local=True,
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "let s = { a: 0, b: false, c: 0.0 };" in js


# ===== Task 6.2: ARRAY Support =====


class TestArrayTransformer:
    """Test ARRAY declaration parsing and transformation."""

    def test_array_variable_parsed_correctly(self):
        source = """
PROGRAM Test
VAR
    values : ARRAY[1..10] OF INT;
END_VAR
values[1] := 42;
END_PROGRAM
"""
        program = _transform_source(source)
        assert len(program.locals) == 1
        var = program.locals[0]
        assert var.name == "values"
        assert var.iec_type == IECType.ARRAY
        assert var.array_element_type == IECType.INT
        assert var.array_size == 10
        assert var.array_lower_bound == 1

    def test_array_of_real(self):
        source = """
PROGRAM Test
VAR
    matrix : ARRAY[1..3] OF REAL;
END_VAR
matrix[1] := 1.0;
END_PROGRAM
"""
        program = _transform_source(source)
        var = program.locals[0]
        assert var.array_element_type == IECType.REAL
        assert var.array_size == 3

    def test_array_index_access_produces_ir_array_access(self):
        source = """
PROGRAM Test
VAR
    values : ARRAY[1..10] OF INT;
END_VAR
values[1] := 42;
END_PROGRAM
"""
        program = _transform_source(source)
        assign = program.body[0]
        assert isinstance(assign, IRAssignment)
        target = assign.target
        assert isinstance(target, IRArrayAccess)
        assert isinstance(target.array, IRVarRef)
        assert target.array.name == "values"
        assert isinstance(target.index, IRLiteral)
        assert target.index.value == 1

    def test_array_index_read(self):
        source = """
PROGRAM Test
VAR
    values : ARRAY[1..10] OF INT;
    x : INT;
END_VAR
x := values[5];
END_PROGRAM
"""
        program = _transform_source(source)
        assign = program.body[0]
        assert isinstance(assign, IRAssignment)
        value = assign.value
        assert isinstance(value, IRArrayAccess)
        assert isinstance(value.index, IRLiteral)
        assert value.index.value == 5


class TestArrayCodegen:
    """Test ARRAY code generation."""

    def test_array_of_int_emits_new_array_fill(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(
                    name="values",
                    iec_type=IECType.ARRAY,
                    array_element_type=IECType.INT,
                    array_size=10,
                    array_lower_bound=1,
                    is_local=True,
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "let values = [" in js
        assert js.count("0, ") >= 9  # 10 zeros

    def test_array_of_real_emits_fill_with_zero_point_zero(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(
                    name="matrix",
                    iec_type=IECType.ARRAY,
                    array_element_type=IECType.REAL,
                    array_size=3,
                    array_lower_bound=1,
                    is_local=True,
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "let matrix = [0.0, 0.0, 0.0];" in js

    def test_array_of_bool_emits_fill_false(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(
                    name="flags",
                    iec_type=IECType.ARRAY,
                    array_element_type=IECType.BOOL,
                    array_size=4,
                    array_lower_bound=1,
                    is_local=True,
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "let flags = [false, false, false, false];" in js

    def test_array_index_access_emits_minus_lower_bound(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(
                    name="values",
                    iec_type=IECType.ARRAY,
                    array_element_type=IECType.INT,
                    array_size=10,
                    array_lower_bound=1,
                    is_local=True,
                ),
            ],
            body=[
                IRAssignment(
                    target=IRArrayAccess(
                        array=IRVarRef(name="values"),
                        index=IRLiteral(value=1, iec_type=IECType.INT),
                    ),
                    value=IRLiteral(value=42, iec_type=IECType.INT),
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "values[1 - 1] = 42;" in js

    def test_array_variable_index_access(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(
                    name="values",
                    iec_type=IECType.ARRAY,
                    array_element_type=IECType.INT,
                    array_size=10,
                    array_lower_bound=1,
                    is_local=True,
                ),
                IRVariable(name="x", iec_type=IECType.INT, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRArrayAccess(
                        array=IRVarRef(name="values"),
                        index=IRLiteral(value=5, iec_type=IECType.INT),
                    ),
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "x = values[5 - 1];" in js


# ===== Task 6.3: TIME Literal Support =====


class TestTimeLiteralTransformer:
    """Test TIME literal parsing and transformation."""

    def test_time_seconds(self):
        source = """
PROGRAM Test
VAR
    t : TIME := T#3s;
END_VAR
t := T#3s;
END_PROGRAM
"""
        program = _transform_source(source)
        var = program.locals[0]
        assert var.iec_type == IECType.TIME
        assert var.initial_value == 3000

    def test_time_milliseconds(self):
        source = """
PROGRAM Test
VAR
    t : TIME := T#500ms;
END_VAR
t := T#500ms;
END_PROGRAM
"""
        program = _transform_source(source)
        var = program.locals[0]
        assert var.initial_value == 500

    def test_time_minutes(self):
        source = """
PROGRAM Test
VAR
    t : TIME := T#2m;
END_VAR
t := T#2m;
END_PROGRAM
"""
        program = _transform_source(source)
        var = program.locals[0]
        assert var.initial_value == 120000

    def test_time_literal_in_expression(self):
        source = """
PROGRAM Test
VAR
    t : TIME;
END_VAR
t := T#3s;
END_PROGRAM
"""
        program = _transform_source(source)
        assign = program.body[0]
        assert isinstance(assign, IRAssignment)
        value = assign.value
        assert isinstance(value, IRLiteral)
        assert value.iec_type == IECType.TIME
        assert value.value == 3000

    def test_time_500ms_literal(self):
        source = """
PROGRAM Test
VAR
    t : TIME;
END_VAR
t := T#500ms;
END_PROGRAM
"""
        program = _transform_source(source)
        assign = program.body[0]
        value = assign.value
        assert isinstance(value, IRLiteral)
        assert value.value == 500

    def test_time_2m_literal(self):
        source = """
PROGRAM Test
VAR
    t : TIME;
END_VAR
t := T#2m;
END_PROGRAM
"""
        program = _transform_source(source)
        assign = program.body[0]
        value = assign.value
        assert isinstance(value, IRLiteral)
        assert value.value == 120000


class TestTimeLiteralCodegen:
    """Test TIME literal code generation."""

    def test_time_literal_emits_integer(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="t", iec_type=IECType.TIME, initial_value=3000, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="t"),
                    value=IRLiteral(value=5000, iec_type=IECType.TIME),
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "let t = 3000;" in js
        assert "t = 5000;" in js

    def test_time_default_is_zero(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="t", iec_type=IECType.TIME, is_local=True),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "let t = 0;" in js


# ===== Task 6.4: STRING Type Support =====


class TestStringTransformer:
    """Test STRING type parsing and transformation."""

    def test_string_variable_with_initial_value(self):
        source = """
PROGRAM Test
VAR
    name : STRING := 'hello';
END_VAR
name := 'world';
END_PROGRAM
"""
        program = _transform_source(source)
        var = program.locals[0]
        assert var.name == "name"
        assert var.iec_type == IECType.STRING
        assert var.initial_value == "hello"

    def test_string_variable_without_initial_value(self):
        source = """
PROGRAM Test
VAR
    label : STRING;
END_VAR
label := 'test';
END_PROGRAM
"""
        program = _transform_source(source)
        var = program.locals[0]
        assert var.iec_type == IECType.STRING
        assert var.initial_value is None

    def test_string_literal_in_expression(self):
        source = """
PROGRAM Test
VAR
    name : STRING;
END_VAR
name := 'world';
END_PROGRAM
"""
        program = _transform_source(source)
        assign = program.body[0]
        assert isinstance(assign, IRAssignment)
        value = assign.value
        assert isinstance(value, IRLiteral)
        assert value.iec_type == IECType.STRING
        assert value.value == "world"


class TestStringCodegen:
    """Test STRING code generation."""

    def test_string_variable_default_empty(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="label", iec_type=IECType.STRING, is_local=True),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert 'let label = "";' in js

    def test_string_variable_with_initial_value(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="name", iec_type=IECType.STRING, initial_value="hello", is_local=True),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert 'let name = "hello";' in js

    def test_string_literal_emits_js_quotes(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="s", iec_type=IECType.STRING, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="s"),
                    value=IRLiteral(value="world", iec_type=IECType.STRING),
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert 's = "world";' in js


# ===== E2E: struct_array.st fixture =====


class TestStructArrayFixtureE2E:
    """End-to-end test with the struct_array.st fixture."""

    @pytest.fixture
    def program(self) -> IRProgram:
        source = _load_fixture("struct_array.st")
        return _transform_source(source)

    def test_fixture_parses_successfully(self, program: IRProgram):
        assert program.name == "StructArrayTest"

    def test_fixture_has_struct_variable(self, program: IRProgram):
        sensor = program.locals[0]
        assert sensor.name == "sensor"
        assert sensor.iec_type == IECType.STRUCT
        assert sensor.struct_type_name == "SensorData"
        assert len(sensor.struct_fields) == 3

    def test_fixture_has_array_variable(self, program: IRProgram):
        readings = program.locals[1]
        assert readings.name == "readings"
        assert readings.iec_type == IECType.ARRAY
        assert readings.array_element_type == IECType.INT
        assert readings.array_size == 5

    def test_fixture_has_string_variable(self, program: IRProgram):
        name = program.locals[2]
        assert name.name == "name"
        assert name.iec_type == IECType.STRING
        assert name.initial_value == "test"

    def test_fixture_has_time_variable(self, program: IRProgram):
        timeout = program.locals[3]
        assert timeout.name == "timeout"
        assert timeout.iec_type == IECType.TIME
        assert timeout.initial_value == 5000

    def test_fixture_body_has_assignments(self, program: IRProgram):
        assert len(program.body) == 4

    def test_fixture_generates_js(self, program: IRProgram):
        from st2js.codegen import generate

        mapping = _make_mapping()
        js = generate(program, mapping)

        # Struct variable
        assert "let sensor = { value: 0, scale: 1.0, valid: false };" in js
        # Array variable
        assert "let readings = [0, 0, 0, 0, 0];" in js
        # String variable
        assert 'let name = "test";' in js
        # Time variable
        assert "let timeout = 5000;" in js
        # Struct field assignment
        assert "sensor.value = 42;" in js
        assert "sensor.valid = true;" in js
        # Array index access with offset
        assert "readings[1 - 1] = sensor.value;" in js
        assert "readings[3 - 1] = 100;" in js
