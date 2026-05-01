"""Tests for st2js YAML sensor mapping loader."""
import os

import pytest

from st2js.errors import MappingError
from st2js.ir import IECType
from st2js.mapping import (
    SensorEntry,
    SensorMapping,
    MappingOptions,
    load_mapping,
    load_mapping_from_string,
)

FIXTURES_DIR = os.path.join(os.path.dirname(__file__), "fixtures")


class TestLoadValidYAML:
    """Tests for loading valid YAML with all fields."""

    def test_load_full_mapping(self):
        content = """\
inputs:
  Temperature:
    sensor: AI_Temp_S
    type: REAL
    scale: 100
  StartButton:
    sensor: DI_Start_S

outputs:
  Heater:
    sensor: DO_Heater_C
  Alarm:
    sensor: AO_Alarm_C
    type: REAL
    scale: 10

options:
  struct_flatten: false
"""
        mapping = load_mapping_from_string(content)

        assert isinstance(mapping, SensorMapping)
        assert len(mapping.inputs) == 2
        assert len(mapping.outputs) == 2

        temp = mapping.inputs[0]
        assert temp.st_name == "Temperature"
        assert temp.sensor_name == "AI_Temp_S"
        assert temp.iec_type == IECType.REAL
        assert temp.scale == 100

        start = mapping.inputs[1]
        assert start.st_name == "StartButton"
        assert start.sensor_name == "DI_Start_S"
        assert start.iec_type is None
        assert start.scale is None

        heater = mapping.outputs[0]
        assert heater.st_name == "Heater"
        assert heater.sensor_name == "DO_Heater_C"
        assert heater.iec_type is None
        assert heater.scale is None

        alarm = mapping.outputs[1]
        assert alarm.st_name == "Alarm"
        assert alarm.sensor_name == "AO_Alarm_C"
        assert alarm.iec_type == IECType.REAL
        assert alarm.scale == 10

    def test_load_minimal_mapping(self):
        content = """\
inputs:
  Temp:
    sensor: AI_Temp

outputs:
  Out1:
    sensor: AO_Out1
"""
        mapping = load_mapping_from_string(content)

        assert len(mapping.inputs) == 1
        assert mapping.inputs[0].st_name == "Temp"
        assert mapping.inputs[0].sensor_name == "AI_Temp"
        assert mapping.inputs[0].iec_type is None
        assert mapping.inputs[0].scale is None

        assert len(mapping.outputs) == 1
        assert mapping.outputs[0].st_name == "Out1"
        assert mapping.outputs[0].sensor_name == "AO_Out1"


class TestMappingErrorMissingSensor:
    """Tests for MappingError when sensor field is missing."""

    def test_missing_sensor_in_input(self):
        content = """\
inputs:
  Temperature:
    type: REAL
outputs: {}
"""
        with pytest.raises(MappingError, match="sensor"):
            load_mapping_from_string(content)

    def test_missing_sensor_in_output(self):
        content = """\
inputs: {}
outputs:
  Heater:
    type: BOOL
"""
        with pytest.raises(MappingError, match="sensor"):
            load_mapping_from_string(content)


class TestMappingErrorInvalidType:
    """Tests for MappingError when type field has invalid IEC type."""

    def test_invalid_type_name(self):
        content = """\
inputs:
  Temp:
    sensor: AI_Temp
    type: FLOAT
outputs: {}
"""
        with pytest.raises(MappingError, match="type"):
            load_mapping_from_string(content)


class TestMappingErrorInvalidScale:
    """Tests for MappingError when scale field is invalid."""

    def test_negative_scale(self):
        content = """\
inputs:
  Temp:
    sensor: AI_Temp
    scale: -5
outputs: {}
"""
        with pytest.raises(MappingError, match="scale"):
            load_mapping_from_string(content)

    def test_zero_scale(self):
        content = """\
inputs:
  Temp:
    sensor: AI_Temp
    scale: 0
outputs: {}
"""
        with pytest.raises(MappingError, match="scale"):
            load_mapping_from_string(content)

    def test_string_scale(self):
        content = """\
inputs:
  Temp:
    sensor: AI_Temp
    scale: abc
outputs: {}
"""
        with pytest.raises(MappingError, match="scale"):
            load_mapping_from_string(content)


class TestOptionsDefaults:
    """Tests for default option values."""

    def test_options_defaults_when_absent(self):
        content = """\
inputs: {}
outputs: {}
"""
        mapping = load_mapping_from_string(content)
        assert mapping.options.struct_flatten is False

    def test_options_defaults_when_empty(self):
        content = """\
inputs: {}
outputs: {}
options: {}
"""
        mapping = load_mapping_from_string(content)
        assert mapping.options.struct_flatten is False


class TestOptionsOverride:
    """Tests for overriding option values."""

    def test_struct_flatten_true(self):
        content = """\
inputs: {}
outputs: {}
options:
  struct_flatten: true
"""
        mapping = load_mapping_from_string(content)
        assert mapping.options.struct_flatten is True


class TestEmptySections:
    """Tests for empty inputs/outputs sections."""

    def test_empty_inputs(self):
        content = """\
inputs: {}
outputs:
  Out1:
    sensor: AO_Out1
"""
        mapping = load_mapping_from_string(content)
        assert len(mapping.inputs) == 0
        assert len(mapping.outputs) == 1

    def test_empty_outputs(self):
        content = """\
inputs:
  In1:
    sensor: AI_In1
outputs: {}
"""
        mapping = load_mapping_from_string(content)
        assert len(mapping.inputs) == 1
        assert len(mapping.outputs) == 0

    def test_both_empty(self):
        content = """\
inputs: {}
outputs: {}
"""
        mapping = load_mapping_from_string(content)
        assert len(mapping.inputs) == 0
        assert len(mapping.outputs) == 0

    def test_null_inputs(self):
        content = """\
inputs:
outputs:
  Out1:
    sensor: AO_Out1
"""
        mapping = load_mapping_from_string(content)
        assert len(mapping.inputs) == 0
        assert len(mapping.outputs) == 1

    def test_null_outputs(self):
        content = """\
inputs:
  In1:
    sensor: AI_In1
outputs:
"""
        mapping = load_mapping_from_string(content)
        assert len(mapping.inputs) == 1
        assert len(mapping.outputs) == 0


class TestLoadFromFile:
    """Tests for load_mapping() from file path."""

    def test_load_fixture_file(self):
        path = os.path.join(FIXTURES_DIR, "minimal_mapping.yaml")
        mapping = load_mapping(path)

        assert isinstance(mapping, SensorMapping)
        assert len(mapping.inputs) == 2
        assert len(mapping.outputs) == 2

        assert mapping.inputs[0].st_name == "Temperature"
        assert mapping.inputs[0].sensor_name == "AI_Temp_S"
        assert mapping.inputs[0].iec_type == IECType.REAL
        assert mapping.inputs[0].scale == 100

        assert mapping.inputs[1].st_name == "StartButton"
        assert mapping.inputs[1].sensor_name == "DI_Start_S"

        assert mapping.outputs[0].st_name == "Heater"
        assert mapping.outputs[0].sensor_name == "DO_Heater_C"

        assert mapping.outputs[1].st_name == "Alarm"
        assert mapping.outputs[1].sensor_name == "AO_Alarm_C"
        assert mapping.outputs[1].iec_type == IECType.REAL
        assert mapping.outputs[1].scale == 10

        assert mapping.options.struct_flatten is False

    def test_load_nonexistent_file(self):
        with pytest.raises(MappingError, match="not found|No such file"):
            load_mapping("/nonexistent/path/mapping.yaml")


class TestLoadDirectives:
    """Tests for the load_head / load_on_start mapping sections."""

    def test_load_directives_parsed_at_top_level(self):
        content = """\
load_head:
  - helpers/utils.js
load_on_start:
  - sim.js
  - post.js
"""
        mapping = load_mapping_from_string(content)
        assert mapping.load_head == ["helpers/utils.js"]
        assert mapping.load_on_start == ["sim.js", "post.js"]

    def test_load_directives_default_empty(self):
        mapping = load_mapping_from_string("inputs: {}\noutputs: {}\n")
        assert mapping.load_head == []
        assert mapping.load_on_start == []

    def test_load_head_non_list_rejected(self):
        content = "load_head: helpers/utils.js\n"
        with pytest.raises(MappingError, match="'load_head' must be a list"):
            load_mapping_from_string(content)

    def test_load_on_start_non_string_element_rejected(self):
        content = "load_on_start:\n  - 42\n"
        with pytest.raises(MappingError, match="'load_on_start\\[0\\]' must be a string"):
            load_mapping_from_string(content)

    def test_load_head_empty_string_rejected(self):
        content = "load_head:\n  - ''\n"
        with pytest.raises(MappingError, match="'load_head\\[0\\]' must be a non-empty string"):
            load_mapping_from_string(content)

    def test_per_file_load_directives_appended_to_global(self):
        content = """\
load_head:
  - global_head.js
load_on_start:
  - global_sim.js

files:
  plant1.st:
    load_head:
      - plant1_head.js
    load_on_start:
      - plant1_sim.js
"""
        mapping = load_mapping_from_string(content)
        fm = mapping.for_file("plant1.st")
        assert fm.load_head == ["global_head.js", "plant1_head.js"]
        assert fm.load_on_start == ["global_sim.js", "plant1_sim.js"]

    def test_per_file_load_directives_deduplicate(self):
        content = """\
load_head:
  - shared.js
files:
  plant1.st:
    load_head:
      - shared.js
      - plant1.js
"""
        mapping = load_mapping_from_string(content)
        fm = mapping.for_file("plant1.st")
        assert fm.load_head == ["shared.js", "plant1.js"]  # shared.js not duplicated

    def test_for_file_unknown_returns_global_only(self):
        content = "load_head:\n  - global.js\n"
        mapping = load_mapping_from_string(content)
        fm = mapping.for_file("other.st")
        assert fm.load_head == ["global.js"]
