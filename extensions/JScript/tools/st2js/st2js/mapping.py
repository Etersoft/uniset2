#!/usr/bin/env python3
"""YAML sensor mapping loader for the st2js converter.

Loads and validates YAML mapping configuration that maps ST variable names
to UniSet sensor names, with optional IEC type and scale factor information.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional

import yaml

from st2js.errors import MappingError
from st2js.ir import IECType


@dataclass
class SensorEntry:
    """A single sensor mapping entry.

    Maps an ST variable name to a UniSet sensor name, with optional
    IEC type and scale factor.
    """

    st_name: str
    sensor_name: str
    iec_type: IECType | None = None
    scale: float | None = None


@dataclass
class MappingOptions:
    """Options section of the mapping configuration."""

    struct_flatten: bool = False
    var_prefix: str = ""     # prefix for sensor names: "plc1_" → in_plc1_AI_Temp_S
    func_prefix: str = ""   # prefix for secondary programs/FBs (NOT uniset_on_step)


@dataclass
class FileMapping:
    """Per-file mapping overrides."""

    var_prefix: str = ""
    func_prefix: str = ""
    inputs: list[SensorEntry] = field(default_factory=list)
    outputs: list[SensorEntry] = field(default_factory=list)
    load_head: list[str] = field(default_factory=list)
    load_on_start: list[str] = field(default_factory=list)


@dataclass
class SensorMapping:
    """Complete sensor mapping configuration.

    Contains input and output sensor entries plus mapping options.
    Optionally contains per-file overrides in `files` dict.
    """

    inputs: list[SensorEntry]
    outputs: list[SensorEntry]
    options: MappingOptions = field(default_factory=MappingOptions)
    files: dict[str, FileMapping] = field(default_factory=dict)
    load_head: list[str] = field(default_factory=list)
    load_on_start: list[str] = field(default_factory=list)

    def for_file(self, filename: str) -> 'SensorMapping':
        """Get a SensorMapping specialized for a specific file.

        Merges per-file overrides on top of the global mapping:
        - inputs / outputs: concatenated (duplicates preserved — a per-file
          entry with the same ST name intentionally extends the list)
        - var_prefix / func_prefix: per-file value wins if non-empty
        - load_head / load_on_start: concatenated with first-occurrence
          dedup (exact string match), so a module listed both globally and
          per-file is loaded only once.
        """
        basename = filename.rsplit("/", 1)[-1] if "/" in filename else filename

        fm = self.files.get(basename)
        if fm is None:
            return self

        # Merge: per-file inputs/outputs extend global, per-file prefix overrides global
        merged_inputs = list(self.inputs) + fm.inputs
        merged_outputs = list(self.outputs) + fm.outputs
        merged_options = MappingOptions(
            struct_flatten=self.options.struct_flatten,
            var_prefix=fm.var_prefix or self.options.var_prefix,
            func_prefix=fm.func_prefix or self.options.func_prefix,
        )
        merged_load_head = _dedup_append(self.load_head, fm.load_head)
        merged_load_on_start = _dedup_append(self.load_on_start, fm.load_on_start)
        return SensorMapping(
            inputs=merged_inputs,
            outputs=merged_outputs,
            options=merged_options,
            load_head=merged_load_head,
            load_on_start=merged_load_on_start,
        )


def _parse_iec_type(type_name: str, st_name: str, section: str) -> IECType:
    """Parse and validate an IEC type name string.

    Args:
        type_name: The type name string from YAML.
        st_name: The ST variable name (for error messages).
        section: 'inputs' or 'outputs' (for error messages).

    Returns:
        The corresponding IECType enum value.

    Raises:
        MappingError: If the type name is not a valid IEC type.
    """
    try:
        return IECType(type_name)
    except ValueError:
        valid_types = ", ".join(t.value for t in IECType)
        raise MappingError(
            f"invalid type '{type_name}' for '{st_name}' in {section}; "
            f"valid types: {valid_types}"
        )


def _parse_scale(scale_value, st_name: str, section: str) -> float:
    """Parse and validate a scale factor value.

    Args:
        scale_value: The raw scale value from YAML.
        st_name: The ST variable name (for error messages).
        section: 'inputs' or 'outputs' (for error messages).

    Returns:
        The validated scale factor as a float.

    Raises:
        MappingError: If the scale is not a positive number.
    """
    try:
        scale = float(scale_value)
    except (TypeError, ValueError):
        raise MappingError(
            f"invalid scale '{scale_value}' for '{st_name}' in {section}; "
            f"scale must be a positive number"
        )
    if scale <= 0:
        raise MappingError(
            f"invalid scale '{scale_value}' for '{st_name}' in {section}; "
            f"scale must be a positive number"
        )
    return scale


def _parse_entries(
    raw_section: dict | None, section_name: str
) -> list[SensorEntry]:
    """Parse a section (inputs or outputs) into a list of SensorEntry.

    Args:
        raw_section: The raw dict from YAML for this section.
        section_name: 'inputs' or 'outputs' (for error messages).

    Returns:
        List of SensorEntry objects.

    Raises:
        MappingError: If any entry is missing required fields or has invalid values.
    """
    if not raw_section:
        return []

    entries = []
    for st_name, entry_data in raw_section.items():
        if not isinstance(entry_data, dict) or "sensor" not in entry_data:
            raise MappingError(
                f"missing required 'sensor' field for '{st_name}' in {section_name}"
            )

        sensor_name = entry_data["sensor"]

        iec_type = None
        if "type" in entry_data:
            iec_type = _parse_iec_type(entry_data["type"], st_name, section_name)

        scale = None
        if "scale" in entry_data:
            scale = _parse_scale(entry_data["scale"], st_name, section_name)

        entries.append(
            SensorEntry(
                st_name=st_name,
                sensor_name=sensor_name,
                iec_type=iec_type,
                scale=scale,
            )
        )

    return entries


def _parse_options(raw_options: dict | None) -> MappingOptions:
    """Parse the options section into MappingOptions.

    Args:
        raw_options: The raw dict from YAML for the options section.

    Returns:
        MappingOptions with values from YAML or defaults.
    """
    if not raw_options:
        return MappingOptions()

    return MappingOptions(
        struct_flatten=bool(raw_options.get("struct_flatten", False)),
        var_prefix=str(raw_options.get("var_prefix", "")),
        func_prefix=str(raw_options.get("func_prefix", "")),
    )


def _parse_loads(raw_value, section_name: str) -> list[str]:
    """Parse a load_head / load_on_start list from YAML.

    Args:
        raw_value: The raw value for the section (list or None).
        section_name: Section key name (for error messages).

    Returns:
        List of validated path strings.

    Raises:
        MappingError: If the value is not a list, or any element is not
            a non-empty string.
    """
    if raw_value is None:
        return []
    if not isinstance(raw_value, list):
        raise MappingError(
            f"'{section_name}' must be a list, got {type(raw_value).__name__}"
        )
    result = []
    for idx, item in enumerate(raw_value):
        if not isinstance(item, str):
            raise MappingError(
                f"'{section_name}[{idx}]' must be a string, got "
                f"{type(item).__name__}"
            )
        if not item:
            raise MappingError(
                f"'{section_name}[{idx}]' must be a non-empty string"
            )
        result.append(item)
    return result


def _dedup_append(base: list[str], extra: list[str]) -> list[str]:
    """Append items from `extra` to `base`, skipping items already present.

    Membership is tested by exact string equality (no path normalization);
    `"sim.js"` and `"./sim.js"` are treated as distinct. First occurrence
    wins; ordering of the original sequences is preserved.
    """
    result = list(base)
    seen = set(base)
    for item in extra:
        if item not in seen:
            result.append(item)
            seen.add(item)
    return result


def load_mapping_from_string(content: str) -> SensorMapping:
    """Load and validate a sensor mapping from a YAML string.

    Args:
        content: The YAML content as a string.

    Returns:
        A validated SensorMapping object.

    Raises:
        MappingError: If the YAML is invalid or fails validation.
    """
    try:
        data = yaml.safe_load(content)
    except yaml.YAMLError as exc:
        raise MappingError(f"invalid YAML: {exc}")

    if data is None:
        data = {}

    if not isinstance(data, dict):
        raise MappingError("mapping YAML must be a dictionary at top level")

    inputs = _parse_entries(data.get("inputs"), "inputs")
    outputs = _parse_entries(data.get("outputs"), "outputs")
    options = _parse_options(data.get("options"))
    load_head = _parse_loads(data.get("load_head"), "load_head")
    load_on_start = _parse_loads(data.get("load_on_start"), "load_on_start")

    # Parse per-file overrides
    files: dict[str, FileMapping] = {}
    raw_files = data.get("files")
    if raw_files and isinstance(raw_files, dict):
        for fname, fdata in raw_files.items():
            if not isinstance(fdata, dict):
                continue
            files[fname] = FileMapping(
                var_prefix=str(fdata.get("var_prefix", "")),
                func_prefix=str(fdata.get("func_prefix", "")),
                inputs=_parse_entries(fdata.get("inputs"), f"files.{fname}.inputs"),
                outputs=_parse_entries(fdata.get("outputs"), f"files.{fname}.outputs"),
                load_head=_parse_loads(fdata.get("load_head"), f"files.{fname}.load_head"),
                load_on_start=_parse_loads(fdata.get("load_on_start"), f"files.{fname}.load_on_start"),
            )

    return SensorMapping(
        inputs=inputs,
        outputs=outputs,
        options=options,
        files=files,
        load_head=load_head,
        load_on_start=load_on_start,
    )


def load_mapping(path: str) -> SensorMapping:
    """Load and validate a sensor mapping from a YAML file.

    Args:
        path: Path to the YAML mapping file.

    Returns:
        A validated SensorMapping object.

    Raises:
        MappingError: If the file cannot be read or the YAML is invalid.
    """
    try:
        with open(path, "r") as f:
            content = f.read()
    except FileNotFoundError:
        raise MappingError(f"mapping file not found: {path}")
    except OSError as exc:
        raise MappingError(f"cannot read mapping file: {exc}")

    return load_mapping_from_string(content)
