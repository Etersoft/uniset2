"""Tests for struct flatten mode in mapping + codegen (Task 8.2).

Tests cover:
- Struct fields become individual sensor entries in uniset_inputs/uniset_outputs
- Struct reconstruction: let sensor = { value: in_AI_Value_S, valid: in_DI_Valid_S };
- Output struct flattening: out_SensorName = varName.field;
- Non-struct entries are unaffected
- Mapping with struct_flatten: true parses correctly
"""
from __future__ import annotations

import os

import pytest

from st2js.codegen import generate
from st2js.ir import (
    IECType,
    IRAssignment,
    IRBinaryOp,
    IRFieldAccess,
    IRLiteral,
    IRProgram,
    IRVarRef,
    IRVariable,
)
from st2js.mapping import (
    MappingOptions,
    SensorEntry,
    SensorMapping,
    load_mapping_from_string,
)


FIXTURES_DIR = os.path.join(os.path.dirname(__file__), "fixtures")


def _make_program(
    name: str = "Main",
    inputs: list | None = None,
    outputs: list | None = None,
    locals_: list | None = None,
    body: list | None = None,
) -> IRProgram:
    return IRProgram(
        name=name,
        inputs=inputs or [],
        outputs=outputs or [],
        locals=locals_ or [],
        fb_instances=[],
        body=body or [],
        function_blocks=[],
    )


class TestMappingStructFlatten:
    """Test that mapping parser correctly handles struct-flattened entries."""

    def test_struct_flatten_option_parsed(self):
        content = """
options:
  struct_flatten: true
"""
        mapping = load_mapping_from_string(content)
        assert mapping.options.struct_flatten is True

    def test_dotted_names_in_inputs(self):
        content = """
inputs:
  sensor.value:
    sensor: AI_Value_S
  sensor.valid:
    sensor: DI_Valid_S
options:
  struct_flatten: true
"""
        mapping = load_mapping_from_string(content)
        assert len(mapping.inputs) == 2
        assert mapping.inputs[0].st_name == "sensor.value"
        assert mapping.inputs[0].sensor_name == "AI_Value_S"
        assert mapping.inputs[1].st_name == "sensor.valid"
        assert mapping.inputs[1].sensor_name == "DI_Valid_S"

    def test_dotted_names_in_outputs(self):
        content = """
outputs:
  result.value:
    sensor: AO_Result_C
  result.status:
    sensor: DO_Status_C
options:
  struct_flatten: true
"""
        mapping = load_mapping_from_string(content)
        assert len(mapping.outputs) == 2
        assert mapping.outputs[0].st_name == "result.value"
        assert mapping.outputs[1].st_name == "result.status"


class TestStructFlattenCodegenInputs:
    """Test struct flatten mode generates correct input sensor arrays and struct reconstruction."""

    def test_flattened_input_sensors_in_array(self):
        """Each struct field becomes a separate sensor entry."""
        program = _make_program(
            inputs=[
                IRVariable(name="sensor", iec_type=IECType.STRUCT, is_input=True,
                           struct_type_name="SensorData"),
            ],
            body=[],
        )
        mapping = SensorMapping(
            inputs=[
                SensorEntry(st_name="sensor.value", sensor_name="AI_Value_S"),
                SensorEntry(st_name="sensor.valid", sensor_name="DI_Valid_S"),
            ],
            outputs=[],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        assert '{ name: "AI_Value_S" }' in js
        assert '{ name: "DI_Valid_S" }' in js

    def test_struct_reconstruction_in_on_step(self):
        """Flattened input struct is reconstructed: let sensor = { value: in_AI_Value_S, valid: in_DI_Valid_S };"""
        program = _make_program(
            inputs=[
                IRVariable(name="sensor", iec_type=IECType.STRUCT, is_input=True,
                           struct_type_name="SensorData"),
            ],
            body=[],
        )
        mapping = SensorMapping(
            inputs=[
                SensorEntry(st_name="sensor.value", sensor_name="AI_Value_S"),
                SensorEntry(st_name="sensor.valid", sensor_name="DI_Valid_S"),
            ],
            outputs=[],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        assert "let sensor = { value: in_AI_Value_S, valid: in_DI_Valid_S };" in js

    def test_struct_reconstruction_at_top_of_on_step(self):
        """Struct reconstruction should appear at the top of uniset_on_step."""
        program = _make_program(
            inputs=[
                IRVariable(name="sensor", iec_type=IECType.STRUCT, is_input=True,
                           struct_type_name="SensorData"),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="x"),
                    value=IRLiteral(value=0, iec_type=IECType.INT),
                ),
            ],
            locals_=[
                IRVariable(name="x", iec_type=IECType.INT, is_local=True),
            ],
        )
        mapping = SensorMapping(
            inputs=[
                SensorEntry(st_name="sensor.value", sensor_name="AI_Value_S"),
                SensorEntry(st_name="sensor.valid", sensor_name="DI_Valid_S"),
            ],
            outputs=[],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        lines = js.split("\n")
        on_step_idx = next(
            i for i, l in enumerate(lines) if "function uniset_on_step()" in l
        )
        reconstruction_line = lines[on_step_idx + 1].strip()
        assert reconstruction_line == "let sensor = { value: in_AI_Value_S, valid: in_DI_Valid_S };"

    def test_mixed_struct_and_plain_inputs(self):
        """Non-struct entries are unaffected by struct flatten."""
        program = _make_program(
            inputs=[
                IRVariable(name="sensor", iec_type=IECType.STRUCT, is_input=True,
                           struct_type_name="SensorData"),
                IRVariable(name="threshold", iec_type=IECType.INT, is_input=True),
            ],
            body=[],
        )
        mapping = SensorMapping(
            inputs=[
                SensorEntry(st_name="sensor.value", sensor_name="AI_Value_S"),
                SensorEntry(st_name="sensor.valid", sensor_name="DI_Valid_S"),
                SensorEntry(st_name="threshold", sensor_name="AI_Threshold_S"),
            ],
            outputs=[],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        # All three sensors appear in inputs array
        assert '{ name: "AI_Value_S" }' in js
        assert '{ name: "DI_Valid_S" }' in js
        assert '{ name: "AI_Threshold_S" }' in js

        # Struct is reconstructed, threshold is not
        assert "let sensor = { value: in_AI_Value_S, valid: in_DI_Valid_S };" in js


class TestStructFlattenCodegenOutputs:
    """Test struct flatten mode generates correct output sensor arrays and field extraction."""

    def test_flattened_output_sensors_in_array(self):
        """Each struct field becomes a separate output sensor entry."""
        program = _make_program(
            outputs=[
                IRVariable(name="result", iec_type=IECType.STRUCT, is_output=True,
                           struct_type_name="ResultData"),
            ],
            body=[],
        )
        mapping = SensorMapping(
            inputs=[],
            outputs=[
                SensorEntry(st_name="result.value", sensor_name="AO_Result_C"),
                SensorEntry(st_name="result.status", sensor_name="DO_Status_C"),
            ],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        assert '{ name: "AO_Result_C" }' in js
        assert '{ name: "DO_Status_C" }' in js

    def test_output_field_extraction_at_bottom(self):
        """Flattened output struct fields are extracted: out_SensorName = result.field;"""
        program = _make_program(
            outputs=[
                IRVariable(name="result", iec_type=IECType.STRUCT, is_output=True,
                           struct_type_name="ResultData"),
            ],
            body=[
                IRAssignment(
                    target=IRFieldAccess(obj=IRVarRef(name="result"), field="value"),
                    value=IRLiteral(value=42, iec_type=IECType.INT),
                ),
            ],
        )
        mapping = SensorMapping(
            inputs=[],
            outputs=[
                SensorEntry(st_name="result.value", sensor_name="AO_Result_C"),
                SensorEntry(st_name="result.status", sensor_name="DO_Status_C"),
            ],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        assert "out_AO_Result_C = result.value;" in js
        assert "out_DO_Status_C = result.status;" in js

    def test_output_field_extraction_at_bottom_of_on_step(self):
        """Output extraction should be at the bottom of uniset_on_step."""
        program = _make_program(
            outputs=[
                IRVariable(name="result", iec_type=IECType.STRUCT, is_output=True,
                           struct_type_name="ResultData"),
            ],
            body=[
                IRAssignment(
                    target=IRFieldAccess(obj=IRVarRef(name="result"), field="value"),
                    value=IRLiteral(value=42, iec_type=IECType.INT),
                ),
            ],
        )
        mapping = SensorMapping(
            inputs=[],
            outputs=[
                SensorEntry(st_name="result.value", sensor_name="AO_Result_C"),
            ],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        lines = js.split("\n")
        on_step_idx = next(
            i for i, l in enumerate(lines) if "function uniset_on_step()" in l
        )
        closing_idx = next(
            i for i in range(len(lines) - 1, on_step_idx, -1)
            if lines[i].strip() == "}"
        )
        extract_line = lines[closing_idx - 1].strip()
        assert extract_line == "out_AO_Result_C = result.value;"

    def test_mixed_struct_and_plain_outputs(self):
        """Non-struct outputs are unaffected by struct flatten."""
        program = _make_program(
            outputs=[
                IRVariable(name="result", iec_type=IECType.STRUCT, is_output=True,
                           struct_type_name="ResultData"),
                IRVariable(name="alarm", iec_type=IECType.BOOL, is_output=True),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="alarm"),
                    value=IRLiteral(value=True, iec_type=IECType.BOOL),
                ),
            ],
        )
        mapping = SensorMapping(
            inputs=[],
            outputs=[
                SensorEntry(st_name="result.value", sensor_name="AO_Result_C"),
                SensorEntry(st_name="alarm", sensor_name="DO_Alarm_C"),
            ],
            options=MappingOptions(struct_flatten=True),
        )
        js = generate(program, mapping)

        assert '{ name: "AO_Result_C" }' in js
        assert '{ name: "DO_Alarm_C" }' in js
        # alarm should use normal out_ prefix
        assert "out_DO_Alarm_C = true;" in js


class TestStructFlattenDisabled:
    """When struct_flatten is not enabled, dotted names are treated as-is."""

    def test_no_flatten_no_reconstruction(self):
        """Without struct_flatten, no struct reconstruction happens."""
        program = _make_program(
            inputs=[
                IRVariable(name="sensor", iec_type=IECType.STRUCT, is_input=True),
            ],
            body=[],
        )
        mapping = SensorMapping(
            inputs=[
                SensorEntry(st_name="sensor.value", sensor_name="AI_Value_S"),
            ],
            outputs=[],
            options=MappingOptions(struct_flatten=False),
        )
        js = generate(program, mapping)

        # Without flatten, no struct reconstruction in on_step
        assert "let sensor = {" not in js
