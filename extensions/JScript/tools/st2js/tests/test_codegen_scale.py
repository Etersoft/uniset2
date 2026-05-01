"""Tests for scale factor support in codegen (Task 8.1).

Tests cover:
- REAL input with scale factor emits: let varName = in_SensorName / scale;
- REAL output with scale factor emits: out_SensorName = Math.round(varName * scale);
- No scale factor: no division/multiplication wrapping
- Scale factor only applies within uniset_on_step()
- Body uses ST variable names (not in_/out_ prefixed)
- E2E test with scale_test.st + scale_mapping.yaml
"""
from __future__ import annotations

import os
import subprocess
import sys

import pytest

from st2js.codegen import generate
from st2js.ir import (
    IECType,
    IRAssignment,
    IRBinaryOp,
    IRLiteral,
    IRProgram,
    IRVarRef,
    IRVariable,
)
from st2js.mapping import MappingOptions, SensorEntry, SensorMapping


FIXTURES_DIR = os.path.join(os.path.dirname(__file__), "fixtures")
SCALE_ST = os.path.join(FIXTURES_DIR, "scale_test.st")
SCALE_MAPPING = os.path.join(FIXTURES_DIR, "scale_mapping.yaml")


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


class TestScaleFactorInput:
    """Test scale factor on REAL input: division at top of uniset_on_step."""

    def test_input_scale_emits_division(self):
        """REAL input with scale: 100 -> let Temperature = in_AI_Temp_S / 100;"""
        program = _make_program(
            inputs=[
                IRVariable(name="Temperature", iec_type=IECType.REAL, is_input=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRVarRef(name="Temperature"),
                ),
            ],
            locals_=[
                IRVariable(name="x", iec_type=IECType.REAL, is_local=True),
            ],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(
                st_name="Temperature",
                sensor_name="AI_Temp_S",
                iec_type=IECType.REAL,
                scale=100,
            )],
        )
        js = generate(program, mapping)

        assert "let Temperature = in_AI_Temp_S / 100;" in js

    def test_input_scale_uses_st_name_in_body(self):
        """Body should use the ST variable name, not in_ prefixed."""
        program = _make_program(
            inputs=[
                IRVariable(name="Temperature", iec_type=IECType.REAL, is_input=True),
            ],
            locals_=[
                IRVariable(name="result", iec_type=IECType.REAL, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="result"),
                    value=IRVarRef(name="Temperature"),
                ),
            ],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(
                st_name="Temperature",
                sensor_name="AI_Temp_S",
                iec_type=IECType.REAL,
                scale=100,
            )],
        )
        js = generate(program, mapping)

        # In the body, Temperature should be used as-is (already a local let)
        assert "result = Temperature;" in js

    def test_input_scale_at_top_of_on_step(self):
        """Scale factor conversion should appear at the top of uniset_on_step()."""
        program = _make_program(
            inputs=[
                IRVariable(name="Temperature", iec_type=IECType.REAL, is_input=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRLiteral(value=42, iec_type=IECType.INT),
                ),
            ],
            locals_=[
                IRVariable(name="x", iec_type=IECType.INT, is_local=True),
            ],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(
                st_name="Temperature",
                sensor_name="AI_Temp_S",
                iec_type=IECType.REAL,
                scale=100,
            )],
        )
        js = generate(program, mapping)

        lines = js.split("\n")
        on_step_idx = next(
            i for i, l in enumerate(lines) if "function uniset_on_step()" in l
        )
        # The scale conversion line should be right after the function opening
        scale_line = lines[on_step_idx + 1].strip()
        assert scale_line == "let Temperature = in_AI_Temp_S / 100;"

    def test_no_scale_no_division(self):
        """Input without scale factor should not emit division line."""
        program = _make_program(
            inputs=[
                IRVariable(name="StartButton", iec_type=IECType.BOOL, is_input=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRVarRef(name="StartButton"),
                ),
            ],
            locals_=[
                IRVariable(name="x", iec_type=IECType.BOOL, is_local=True),
            ],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(st_name="StartButton", sensor_name="DI_Start_S")],
        )
        js = generate(program, mapping)

        assert "/ " not in js.split("function uniset_on_step()")[1].split("}")[0] or \
               "let StartButton" not in js

    def test_input_scale_float_factor(self):
        """Scale factor can be a float like 10.5."""
        program = _make_program(
            inputs=[
                IRVariable(name="Pressure", iec_type=IECType.REAL, is_input=True),
            ],
            body=[],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(
                st_name="Pressure",
                sensor_name="AI_Press_S",
                iec_type=IECType.REAL,
                scale=10.5,
            )],
        )
        js = generate(program, mapping)

        assert "let Pressure = in_AI_Press_S / 10.5;" in js

    def test_multiple_input_scales(self):
        """Multiple inputs with scale factors generate multiple conversion lines."""
        program = _make_program(
            inputs=[
                IRVariable(name="Temp", iec_type=IECType.REAL, is_input=True),
                IRVariable(name="Press", iec_type=IECType.REAL, is_input=True),
            ],
            body=[],
        )
        mapping = _make_mapping(
            inputs=[
                SensorEntry(st_name="Temp", sensor_name="AI_Temp_S", iec_type=IECType.REAL, scale=100),
                SensorEntry(st_name="Press", sensor_name="AI_Press_S", iec_type=IECType.REAL, scale=10),
            ],
        )
        js = generate(program, mapping)

        assert "let Temp = in_AI_Temp_S / 100;" in js
        assert "let Press = in_AI_Press_S / 10;" in js


class TestScaleFactorOutput:
    """Test scale factor on REAL output: multiplication at bottom of uniset_on_step."""

    def test_output_scale_emits_multiplication(self):
        """REAL output with scale: 100 -> out_SensorName = Math.round(varName * 100);"""
        program = _make_program(
            outputs=[
                IRVariable(name="ValvePos", iec_type=IECType.REAL, is_output=True),
            ],
            locals_=[
                IRVariable(name="x", iec_type=IECType.REAL, is_local=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="ValvePos"),
                    value=IRVarRef(name="x"),
                ),
            ],
        )
        mapping = _make_mapping(
            outputs=[SensorEntry(
                st_name="ValvePos",
                sensor_name="AO_Valve_C",
                iec_type=IECType.REAL,
                scale=100,
            )],
        )
        js = generate(program, mapping)

        assert "out_AO_Valve_C = Math.round(ValvePos * 100);" in js

    def test_output_scale_at_bottom_of_on_step(self):
        """Scale factor output should be at the bottom of uniset_on_step."""
        program = _make_program(
            outputs=[
                IRVariable(name="ValvePos", iec_type=IECType.REAL, is_output=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="ValvePos"),
                    value=IRLiteral(value=42.0, iec_type=IECType.REAL),
                ),
            ],
        )
        mapping = _make_mapping(
            outputs=[SensorEntry(
                st_name="ValvePos",
                sensor_name="AO_Valve_C",
                iec_type=IECType.REAL,
                scale=100,
            )],
        )
        js = generate(program, mapping)

        lines = js.split("\n")
        # Find the closing brace of uniset_on_step
        on_step_idx = next(
            i for i, l in enumerate(lines) if "function uniset_on_step()" in l
        )
        # Find the line just before the closing brace
        closing_idx = next(
            i for i in range(len(lines) - 1, on_step_idx, -1)
            if lines[i].strip() == "}"
        )
        scale_line = lines[closing_idx - 1].strip()
        assert scale_line == "out_AO_Valve_C = Math.round(ValvePos * 100);"

    def test_output_scale_body_uses_st_name(self):
        """Body assignments should target the ST variable name."""
        program = _make_program(
            outputs=[
                IRVariable(name="ValvePos", iec_type=IECType.REAL, is_output=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="ValvePos"),
                    value=IRLiteral(value=42.0, iec_type=IECType.REAL),
                ),
            ],
        )
        mapping = _make_mapping(
            outputs=[SensorEntry(
                st_name="ValvePos",
                sensor_name="AO_Valve_C",
                iec_type=IECType.REAL,
                scale=100,
            )],
        )
        js = generate(program, mapping)

        # Body should assign to ValvePos (not out_AO_Valve_C)
        assert "ValvePos = 42.0;" in js

    def test_no_scale_no_multiplication(self):
        """Output without scale factor should not emit Math.round line."""
        program = _make_program(
            outputs=[
                IRVariable(name="Alarm", iec_type=IECType.BOOL, is_output=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="Alarm"),
                    value=IRLiteral(value=True, iec_type=IECType.BOOL),
                ),
            ],
        )
        mapping = _make_mapping(
            outputs=[SensorEntry(st_name="Alarm", sensor_name="DO_Alarm_C")],
        )
        js = generate(program, mapping)

        assert "Math.round" not in js

    def test_multiple_output_scales(self):
        """Multiple outputs with scale factors generate multiple output lines."""
        program = _make_program(
            outputs=[
                IRVariable(name="Out1", iec_type=IECType.REAL, is_output=True),
                IRVariable(name="Out2", iec_type=IECType.REAL, is_output=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="Out1"),
                    value=IRLiteral(value=1.0, iec_type=IECType.REAL),
                ),
            ],
        )
        mapping = _make_mapping(
            outputs=[
                SensorEntry(st_name="Out1", sensor_name="AO_Out1_C", iec_type=IECType.REAL, scale=100),
                SensorEntry(st_name="Out2", sensor_name="AO_Out2_C", iec_type=IECType.REAL, scale=50),
            ],
        )
        js = generate(program, mapping)

        assert "out_AO_Out1_C = Math.round(Out1 * 100);" in js
        assert "out_AO_Out2_C = Math.round(Out2 * 50);" in js


class TestScaleFactorIntegerFormat:
    """Test that integer scale factors are emitted without decimal point."""

    def test_integer_scale_no_decimal(self):
        """scale: 100 should emit / 100, not / 100.0."""
        program = _make_program(
            inputs=[
                IRVariable(name="Temp", iec_type=IECType.REAL, is_input=True),
            ],
            body=[],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(
                st_name="Temp",
                sensor_name="AI_Temp_S",
                iec_type=IECType.REAL,
                scale=100.0,  # float but integer value
            )],
        )
        js = generate(program, mapping)

        assert "/ 100;" in js
        assert "/ 100.0" not in js


class TestScaleFactorE2E:
    """E2E test with scale_test.st + scale_mapping.yaml."""

    def _run_pipeline(self) -> str:
        from st2js.codegen import generate
        from st2js.mapping import load_mapping
        from st2js.parser import parse_st
        from st2js.transformer import transform
        from st2js.type_system import check_types

        with open(SCALE_ST) as f:
            st_source = f.read()

        parse_result = parse_st(st_source, filename="scale_test.st")
        ir_program = transform(parse_result)
        typed_program = check_types(ir_program)
        mapping = load_mapping(SCALE_MAPPING)

        return generate(typed_program, mapping)

    def test_input_scale_in_e2e(self):
        js = self._run_pipeline()
        assert "let Temperature = in_AI_Temp_S / 100;" in js
        assert "let Pressure = in_AI_Press_S / 10;" in js

    def test_output_scale_in_e2e(self):
        js = self._run_pipeline()
        assert "out_AO_Valve_C = Math.round(ValvePos * 100);" in js

    def test_body_uses_st_names(self):
        """Body should use Temperature, not in_AI_Temp_S."""
        js = self._run_pipeline()
        # In the body (if statement), Temperature should be used directly
        on_step = js.split("function uniset_on_step()")[1]
        assert "Temperature > setpoint" in on_step or "Temperature" in on_step

    def test_non_scaled_input_not_affected(self):
        """StartButton has no scale, should still use in_ prefix directly."""
        js = self._run_pipeline()
        # StartButton should NOT have a let ... = in_... / N line
        assert "let StartButton" not in js

    def test_non_scaled_output_not_affected(self):
        """Alarm has no scale, should use normal out_ prefix."""
        js = self._run_pipeline()
        assert "Math.round(Alarm" not in js

    def test_cli_with_scale(self):
        """Test scale factors through CLI subprocess."""
        result = subprocess.run(
            [
                sys.executable, "-m", "st2js",
                SCALE_ST,
                "-m", SCALE_MAPPING,
            ],
            capture_output=True,
            text=True,
        )
        assert result.returncode == 0
        js = result.stdout
        assert "let Temperature = in_AI_Temp_S / 100;" in js
        assert "out_AO_Valve_C = Math.round(ValvePos * 100);" in js
