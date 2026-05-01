"""Tests for debug instrumentation in the st2js code generator (codegen.py).

Tests cover:
- debug=False (default) produces no debug stubs or meta
- debug=True emits debug server init (load uniset2-debug.js)
- debug=True does NOT emit _dbg_if/_dbg_case trace wrapping (removed)
- debug=True emits globalThis._program_meta with correct structure
- _program_meta includes inputs, outputs, locals, fb_instances
- _program_meta includes scale factors from mapping
"""
from __future__ import annotations

import json
import re

import pytest

from st2js.ir import (
    IECType,
    IRAssignment,
    IRBinaryOp,
    IRCase,
    IRFBInstance,
    IRIfElse,
    IRLiteral,
    IRProgram,
    IRVarRef,
    IRVariable,
)
from st2js.mapping import SensorEntry, SensorMapping, MappingOptions


def _make_program(
    name: str = "Main",
    inputs: list | None = None,
    outputs: list | None = None,
    locals_: list | None = None,
    body: list | None = None,
    fb_instances: list | None = None,
) -> IRProgram:
    """Helper to create an IRProgram with defaults."""
    return IRProgram(
        name=name,
        inputs=inputs or [],
        outputs=outputs or [],
        locals=locals_ or [],
        fb_instances=fb_instances or [],
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


class TestDebugStubs:
    """Test debug=True emits server init, no trace instrumentation."""

    def test_no_debug_code_by_default(self):
        from st2js.codegen import generate

        program = _make_program()
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "_dbg_if" not in js
        assert "_dbg_case" not in js
        assert "uniset2-debug.js" not in js

    def test_debug_server_init_emitted(self):
        from st2js.codegen import generate

        program = _make_program()
        mapping = _make_mapping()
        js = generate(program, mapping, debug=True)

        assert 'load("uniset2-debug.js")' in js
        assert "uniset_debug_start(8088)" in js

    def test_no_trace_wrapping_in_debug_mode(self):
        """Trace instrumentation (_dbg_if/_dbg_case) was removed."""
        from st2js.codegen import generate

        program = _make_program(
            locals_=[IRVariable(name="x", iec_type=IECType.INT, is_local=True)],
            body=[
                IRIfElse(
                    condition=IRVarRef("x"),
                    then_body=[],
                    elsif_branches=[],
                    else_body=None,
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping, debug=True)

        assert "_dbg_if" not in js
        assert "_dbg_case" not in js
        assert "_dbg_begin_cycle" not in js
        assert "_dbg_end_cycle" not in js
        # IF should still emit as plain if (condition)
        assert "if (x)" in js


class TestProgramMeta:
    """Test that debug=True emits globalThis._program_meta."""

    def test_no_meta_without_debug(self):
        from st2js.codegen import generate

        program = _make_program()
        mapping = _make_mapping()
        js = generate(program, mapping)

        assert "_program_meta" not in js

    def test_meta_emitted_with_debug(self):
        from st2js.codegen import generate

        program = _make_program(name="Thermostat")
        mapping = _make_mapping()
        js = generate(program, mapping, debug=True)

        assert "globalThis._program_meta" in js

    def test_meta_has_program_name(self):
        from st2js.codegen import generate

        program = _make_program(name="Thermostat")
        mapping = _make_mapping()
        js = generate(program, mapping, debug=True)

        assert '"Thermostat"' in js

    def test_meta_has_inputs(self):
        from st2js.codegen import generate

        program = _make_program(
            inputs=[
                IRVariable(name="Temperature", iec_type=IECType.REAL, is_input=True),
            ],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(st_name="Temperature", sensor_name="AI_Temp_S", iec_type=IECType.REAL)],
        )
        js = generate(program, mapping, debug=True)

        assert '"Temperature"' in js
        assert '"AI_Temp_S"' in js
        assert '"REAL"' in js

    def test_meta_has_outputs(self):
        from st2js.codegen import generate

        program = _make_program(
            outputs=[
                IRVariable(name="HeaterOn", iec_type=IECType.BOOL, is_output=True),
            ],
        )
        mapping = _make_mapping(
            outputs=[SensorEntry(st_name="HeaterOn", sensor_name="DO_Heater_C", iec_type=IECType.BOOL)],
        )
        js = generate(program, mapping, debug=True)

        assert '"HeaterOn"' in js
        assert '"DO_Heater_C"' in js

    def test_meta_has_locals(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="state", iec_type=IECType.INT, is_local=True),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping, debug=True)

        assert '"state"' in js
        assert '"INT"' in js

    def test_meta_has_fb_instances(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(name="myTimer", fb_type="TON", constructor_args={}),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping, debug=True)

        assert '"myTimer"' in js
        assert '"TON"' in js

    def test_meta_includes_scale(self):
        from st2js.codegen import generate

        program = _make_program(
            inputs=[
                IRVariable(name="Temperature", iec_type=IECType.REAL, is_input=True),
            ],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(st_name="Temperature", sensor_name="AI_Temp_S",
                                iec_type=IECType.REAL, scale=100)],
        )
        js = generate(program, mapping, debug=True)

        assert "scale" in js
        assert "100" in js

    def test_meta_appears_before_load(self):
        from st2js.codegen import generate

        program = _make_program(name="TestProg")
        mapping = _make_mapping()
        js = generate(program, mapping, debug=True)

        meta_pos = js.index("_program_meta")
        load_pos = js.index('load("uniset2-iec61131.js")')
        assert meta_pos < load_pos


class TestDebugCLIFlag:
    """Test that --debug flag is accepted by CLI."""

    def test_debug_flag_accepted(self):
        """Test that --debug is a valid argument."""
        from st2js.cli import _build_parser

        parser = _build_parser()
        parsed = parser.parse_args(["test.st", "--debug"])
        assert parsed.debug is True

    def test_debug_flag_default_false(self):
        """Test that --debug defaults to False."""
        from st2js.cli import _build_parser

        parser = _build_parser()
        parsed = parser.parse_args(["test.st"])
        assert parsed.debug is False
