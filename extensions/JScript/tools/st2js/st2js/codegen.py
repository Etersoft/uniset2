#!/usr/bin/env python3
"""JavaScript code generator for the st2js converter.

Walks the typed IR and emits JavaScript source code compatible with the
UniSet2 JScript extension. Generates:
  - load("uniset2-iec61131.js") import
  - uniset_inputs / uniset_outputs sensor arrays
  - Local variable declarations
  - uniset_on_step() function wrapping the PROGRAM body

Sensor variable substitution:
  - VAR_INPUT variables become in_<SensorName>
  - VAR_OUTPUT variables become out_<SensorName>
  - VAR local variables keep their original names
"""
from __future__ import annotations

import json
import warnings
from dataclasses import dataclass
from typing import Any

from st2js.fb_registry import get_fb_info, needs_codesys_library
from st2js.ir import (
    IECType,
    IRArrayAccess,
    IRAssignment,
    IRBinaryOp,
    IRCase,
    IRContinueStatement,
    IRExitStatement,
    IRExpression,
    IRFBCall,
    IRFBInstance,
    IRFieldAccess,
    IRForLoop,
    IRFunctionBlock,
    IRFunctionCall,
    IRIfElse,
    IRLiteral,
    IRProgram,
    IRProgramCall,
    IRRepeatLoop,
    IRReturnStatement,
    IRStatement,
    IRStructField,
    IRTypeCoercion,
    IRUnaryOp,
    IRVarRef,
    IRVariable,
    IRWhileLoop,
)
from st2js.mapping import SensorEntry, SensorMapping


# ST operator -> JS operator mapping
_OPERATOR_MAP: dict[str, str] = {
    "=": "===",
    "<>": "!==",
    "AND": "&&",
    "OR": "||",
    "XOR": "!==",
    "MOD": "%",
    "+": "+",
    "-": "-",
    "*": "*",
    "/": "/",
    ">": ">",
    "<": "<",
    ">=": ">=",
    "<=": "<=",
}

# Default initial values by IEC type
_DEFAULT_VALUES: dict[IECType, Any] = {
    IECType.BOOL: False,
    # Signed integers
    IECType.SINT: 0,
    IECType.INT: 0,
    IECType.DINT: 0,
    IECType.LINT: 0,
    # Unsigned integers
    IECType.USINT: 0,
    IECType.UINT: 0,
    IECType.UDINT: 0,
    IECType.ULINT: 0,
    # Bit-string types
    IECType.BYTE: 0,
    IECType.WORD: 0,
    IECType.DWORD: 0,
    IECType.LWORD: 0,
    # Floating point
    IECType.REAL: 0.0,
    IECType.LREAL: 0.0,
    # Other
    IECType.TIME: 0,
    IECType.STRING: '""',
    IECType.WSTRING: '""',
}


@dataclass
class GenerateStats:
    """Statistics about the generated JS output."""

    lines: int = 0
    programs: int = 0
    function_blocks: int = 0
    fb_instances: int = 0
    inputs: int = 0
    outputs: int = 0
    locals: int = 0
    unsupported: int = 0
    warnings: int = 0


def generate(program: IRProgram, mapping: SensorMapping, debug: bool = False) -> str:
    """Generate JavaScript source from a typed IRProgram and sensor mapping.

    Args:
        program: The typed IRProgram (after type checking).
        mapping: The sensor mapping configuration.
        debug: If True, emit trace instrumentation and program metadata.

    Returns:
        A complete JavaScript source string.
    """
    gen = CodeGenerator(program, mapping, debug=debug)
    return gen.generate()


def generate_with_stats(
    program: IRProgram, mapping: SensorMapping, debug: bool = False,
) -> tuple[str, GenerateStats]:
    """Generate JavaScript source and collect statistics.

    Returns:
        Tuple of (JavaScript source string, GenerateStats).
    """
    js = generate(program, mapping, debug=debug)
    stats = GenerateStats(
        lines=js.count("\n") + 1,
        programs=1 + len(program.secondary_programs),
        function_blocks=len(program.function_blocks),
        fb_instances=len(program.fb_instances) + sum(
            len(p.fb_instances) for p in program.secondary_programs),
        inputs=len(program.inputs),
        outputs=len(program.outputs),
        locals=len(program.locals),
        unsupported=js.count("[UNSUPPORTED"),
    )
    return js, stats


class CodeGenerator:
    """Walks IR and emits JavaScript source code.

    Builds lookup tables for input/output variable-to-sensor mapping,
    then emits the JS file sections in order.
    """

    def __init__(self, program: IRProgram, mapping: SensorMapping, debug: bool = False):
        self.program = program
        self.mapping = mapping
        self._debug = debug

        # Counter for debug line number placeholders (replaced in final pass)

        # Track whether an explicit mapping file was provided
        self._has_mapping = bool(mapping.inputs or mapping.outputs)

        # Track unsupported FB instances
        self._unsupported_fbs: set[str] = set()
        # FB types that need auto-generated stub classes
        self._stub_fb_types: set[str] = set()

        # Known user-defined FUNCTION_BLOCK names
        self._user_fb_types: set[str] = {fb.name for fb in program.function_blocks}

        # Prefixes for multi-program isolation
        self._var_prefix = mapping.options.var_prefix
        self._func_prefix = mapping.options.func_prefix

        # Build lookup: ST variable name -> SensorEntry
        self._input_map: dict[str, SensorEntry] = {
            entry.st_name: entry for entry in mapping.inputs
        }
        self._output_map: dict[str, SensorEntry] = {
            entry.st_name: entry for entry in mapping.outputs
        }

        # Build sets of input/output variable names for quick lookup
        self._input_names: set[str] = {v.name for v in program.inputs}
        self._output_names: set[str] = {v.name for v in program.outputs}
        self._global_names: set[str] = {v.name for v in program.globals}

        # Build sets of scaled input/output variable names
        self._scaled_inputs: dict[str, SensorEntry] = {
            entry.st_name: entry
            for entry in mapping.inputs
            if entry.scale is not None
        }
        self._scaled_outputs: dict[str, SensorEntry] = {
            entry.st_name: entry
            for entry in mapping.outputs
            if entry.scale is not None
        }

        # Struct flatten support: group dotted entries by struct name
        self._struct_flatten = mapping.options.struct_flatten
        self._flat_input_structs: dict[str, list[tuple[str, SensorEntry]]] = {}
        self._flat_output_structs: dict[str, list[tuple[str, SensorEntry]]] = {}
        if self._struct_flatten:
            for entry in mapping.inputs:
                if "." in entry.st_name:
                    struct_name, field_name = entry.st_name.split(".", 1)
                    self._flat_input_structs.setdefault(struct_name, []).append(
                        (field_name, entry)
                    )
            for entry in mapping.outputs:
                if "." in entry.st_name:
                    struct_name, field_name = entry.st_name.split(".", 1)
                    self._flat_output_structs.setdefault(struct_name, []).append(
                        (field_name, entry)
                    )

    def generate(self) -> str:
        """Generate the complete JavaScript source string."""
        lines: list[str] = []

        # Header comment
        lines.append(f"// Generated by st2js from {self.program.name}")

        # Debug stubs (before load statements)
        if self._debug:
            lines.append("")
            lines.extend(self._emit_debug_stubs())
            lines.append("")
            lines.extend(self._emit_program_meta())

        # Load statements
        lines.append('load("uniset2-iec61131.js");')

        # Check if Codesys library needed (any FB instance from codesys lib)
        all_fb_instances = list(self.program.fb_instances)
        for sec in self.program.secondary_programs:
            all_fb_instances.extend(sec.fb_instances)
        if needs_codesys_library(all_fb_instances):
            lines.append('load("uniset2-iec61131-codesys.js");')

        # Extra load_head modules from mapping (skip duplicates of core loads)
        _core_loads = {"uniset2-iec61131.js", "uniset2-iec61131-codesys.js"}
        for mod in self.mapping.load_head:
            if mod in _core_loads:
                continue
            lines.append(f'load({json.dumps(mod)});')

        lines.append("")

        # Sensor arrays
        lines.extend(self._emit_sensor_arrays())
        lines.append("")

        # FUNCTION_BLOCK class declarations
        for fb in self.program.function_blocks:
            lines.extend(self._emit_function_block_class(fb))
            lines.append("")

        # Auto-stub classes for unknown external FB types
        # Pre-scan all FB instances to find types needing stubs
        self._prescan_stub_types()
        stub_lines = self._emit_auto_stubs()
        if stub_lines:
            lines.extend(stub_lines)
            lines.append("")

        # Global variable declarations (from VAR_GLOBAL)
        global_lines = self._emit_global_vars()
        if global_lines:
            lines.append("// Global variables")
            lines.extend(global_lines)
            lines.append("")

        # Local variable declarations
        local_lines = self._emit_local_vars()
        if local_lines:
            lines.append("// Local variables")
            lines.extend(local_lines)
            lines.append("")

        # FB instance declarations
        fb_lines = self._emit_fb_instances()
        if fb_lines:
            lines.append("// Function block instances")
            lines.extend(fb_lines)
            lines.append("")

        # Debug: export globals, locals and FB instances to globalThis for debug access
        if self._debug:
            export_lines = []
            vp = self._var_prefix
            for var in self.program.globals:
                export_lines.append(f"globalThis.{var.name} = {var.name};")
            for var in self.program.locals:
                export_lines.append(f"globalThis.{vp}{var.name} = {vp}{var.name};")
            for fb_inst in self.program.fb_instances:
                if fb_inst.name not in self._unsupported_fbs:
                    export_lines.append(f"globalThis.{vp}{fb_inst.name} = {vp}{fb_inst.name};")
            if export_lines:
                lines.append("// Debug: export to globalThis for debug visualizer")
                lines.extend(export_lines)
                lines.append("")

        # Secondary program functions
        for prog in self.program.secondary_programs:
            lines.extend(self._emit_program_function(prog))
            lines.append("")

        # uniset_on_step function — calls main program
        lines.append("function uniset_on_step() {")

        # Scale factor input conversions at the top
        for var in self.program.inputs:
            entry = self._scaled_inputs.get(var.name)
            if entry is not None:
                scale_str = self._format_scale(entry.scale)
                lines.append(f"    let {var.name} = in_{self._prefixed_sensor(entry.sensor_name)} / {scale_str};")

        # Struct flatten: reconstruct input structs from flattened sensor values
        if self._struct_flatten:
            for var in self.program.inputs:
                if var.name in self._flat_input_structs:
                    fields = self._flat_input_structs[var.name]
                    field_parts = [
                        f"{fname}: in_{self._prefixed_sensor(fentry.sensor_name)}"
                        for fname, fentry in fields
                    ]
                    fields_str = ", ".join(field_parts)
                    lines.append(f"    let {var.name} = {{ {fields_str} }};")

        for stmt in self.program.body:
            stmt_lines = self._emit_statement(stmt, indent=1)
            lines.extend(stmt_lines)

        # Struct flatten: extract output struct fields to individual sensors
        if self._struct_flatten:
            for var in self.program.outputs:
                if var.name in self._flat_output_structs:
                    for fname, fentry in self._flat_output_structs[var.name]:
                        lines.append(f"    out_{self._prefixed_sensor(fentry.sensor_name)} = {var.name}.{fname};")

        # Scale factor output conversions at the bottom
        for var in self.program.outputs:
            entry = self._scaled_outputs.get(var.name)
            if entry is not None:
                scale_str = self._format_scale(entry.scale)
                lines.append(f"    out_{self._prefixed_sensor(entry.sensor_name)} = Math.round({var.name} * {scale_str});")

        lines.append("}")
        lines.append("")

        # Optional lifecycle stubs (suppress runtime warnings)
        if self.mapping.load_on_start:
            lines.append("function uniset_on_start() {")
            for mod in self.mapping.load_on_start:
                lines.append(f"    load({json.dumps(mod)});")
            lines.append("}")
        else:
            lines.append("function uniset_on_start() {}")
        lines.append("function uniset_on_stop() {}")
        lines.append("function uniset_process_timers() {}")
        lines.append("function uniset_on_sensor() {}")
        lines.append("")

        result = "\n".join(lines)

        return result

    def _emit_sensor_arrays(self) -> list[str]:
        """Emit uniset_inputs and uniset_outputs array declarations."""
        lines: list[str] = []

        # Inputs
        input_entries = self._collect_input_entries()
        if input_entries:
            lines.append("uniset_inputs = [")
            for i, entry in enumerate(input_entries):
                comma = "," if i < len(input_entries) - 1 else ""
                lines.append(f'    {{ name: "{self._prefixed_sensor(entry.sensor_name)}" }}{comma}')
            lines.append("];")
        else:
            lines.append("uniset_inputs = [];")

        lines.append("")

        # Outputs
        output_entries = self._collect_output_entries()
        if output_entries:
            lines.append("uniset_outputs = [")
            for i, entry in enumerate(output_entries):
                comma = "," if i < len(output_entries) - 1 else ""
                lines.append(f'    {{ name: "{self._prefixed_sensor(entry.sensor_name)}" }}{comma}')
            lines.append("];")
        else:
            lines.append("uniset_outputs = [];")

        return lines

    def _collect_input_entries(self) -> list[SensorEntry]:
        """Collect sensor entries for all input variables.

        When struct_flatten is enabled, struct variables expand into their
        individual field entries (from dotted mapping names).
        """
        entries: list[SensorEntry] = []
        for var in self.program.inputs:
            # Check for flattened struct fields
            if self._struct_flatten and var.name in self._flat_input_structs:
                for _field_name, field_entry in self._flat_input_structs[var.name]:
                    entries.append(field_entry)
                continue

            entry = self._input_map.get(var.name)
            if entry is not None:
                entries.append(entry)
            else:
                # Unmapped input: use ST variable name as sensor name
                if self._has_mapping:
                    warnings.warn(
                        f"No mapping found for input variable '{var.name}'; "
                        f"using ST variable name as sensor name",
                        stacklevel=2,
                    )
                entries.append(SensorEntry(st_name=var.name, sensor_name=var.name))
        return entries

    def _collect_output_entries(self) -> list[SensorEntry]:
        """Collect sensor entries for all output variables.

        When struct_flatten is enabled, struct variables expand into their
        individual field entries (from dotted mapping names).
        """
        entries: list[SensorEntry] = []
        for var in self.program.outputs:
            # Check for flattened struct fields
            if self._struct_flatten and var.name in self._flat_output_structs:
                for _field_name, field_entry in self._flat_output_structs[var.name]:
                    entries.append(field_entry)
                continue

            entry = self._output_map.get(var.name)
            if entry is not None:
                entries.append(entry)
            else:
                if self._has_mapping:
                    warnings.warn(
                        f"No mapping found for output variable '{var.name}'; "
                        f"using ST variable name as sensor name",
                        stacklevel=2,
                    )
                entries.append(SensorEntry(st_name=var.name, sensor_name=var.name))
        return entries

    def _emit_global_vars(self) -> list[str]:
        """Emit global variable declarations (from VAR_GLOBAL) as let statements.

        Skips globals that are shadowed by local/input/output variables.
        """
        local_names = (
            {v.name for v in self.program.locals}
            | self._input_names
            | self._output_names
        )
        lines: list[str] = []
        seen: set[str] = set()
        for var in self.program.globals:
            if var.name not in local_names and var.name not in seen:
                seen.add(var.name)
                value = self._format_initial_value(var)
                lines.append(f"let {var.name} = {value};")
        return lines

    def _emit_local_vars(self) -> list[str]:
        """Emit local variable declarations as let statements.

        Deduplicates by name (actions copy parent locals, causing repeats).
        """
        lines: list[str] = []
        seen: set[str] = set()
        vp = self._var_prefix
        for var in self.program.locals:
            key = f"{vp}{var.name}"
            if key not in seen:
                seen.add(key)
                value = self._format_initial_value(var)
                lines.append(f"let {key} = {value};")
        return lines

    def _prescan_stub_types(self) -> None:
        """Pre-scan all FB instances and GVL struct fields to find types needing auto-stubs."""
        import re
        all_instances = list(self.program.fb_instances)
        for sec in self.program.secondary_programs:
            all_instances.extend(sec.fb_instances)
        for fb_inst in all_instances:
            fb_info = get_fb_info(fb_inst.fb_type)
            if fb_info is None and fb_inst.fb_type not in self._user_fb_types:
                js_type = fb_inst.fb_type.replace('.', '_')
                self._stub_fb_types.add((fb_inst.fb_type, js_type))
        # Also scan GVL struct fields for __raw:new ClassName() patterns
        for var in self.program.globals:
            if var.struct_fields:
                for field in var.struct_fields:
                    if isinstance(field.initial_value, str) and field.initial_value.startswith("__raw:new "):
                        m = re.match(r'__raw:new (\w+)\(', field.initial_value)
                        if m:
                            js_type = m.group(1)
                            if not get_fb_info(js_type) and js_type not in self._user_fb_types:
                                self._stub_fb_types.add((js_type, js_type))

    def _emit_auto_stubs(self) -> list[str]:
        """Emit auto-generated stub classes for unknown external FB types."""
        if not self._stub_fb_types:
            return []
        lines = ["// Auto-generated stubs for external library FB types"]
        for orig_type, js_type in sorted(self._stub_fb_types):
            lines.append(f"class {js_type} {{ constructor() {{}} execute() {{}} }} // stub for {orig_type}")
        return lines

    def _emit_fb_instances(self) -> list[str]:
        """Emit function block instance declarations as const statements.

        Timers (TON/TOF/TP): constructor takes PT value -> new TON(ptValue)
        Counters (CTU/CTD/CTUD): constructor takes PV value -> new CTU(pvValue)
        Bistable/Edge (RS/SR/R_TRIG/F_TRIG): no constructor args -> new RS()
        """
        lines: list[str] = []
        seen: set[str] = set()
        vp = self._var_prefix
        for fb_inst in self.program.fb_instances:
            if fb_inst.name in seen:
                continue
            seen.add(fb_inst.name)
            fb_info = get_fb_info(fb_inst.fb_type)
            if fb_info is not None and fb_info.constructor_args:
                args: list[str] = []
                for param_name, _ in fb_info.constructor_args:
                    if param_name in fb_inst.constructor_args:
                        args.append(self._emit_expression(fb_inst.constructor_args[param_name]))
                    else:
                        args.append("0")
                args_str = ", ".join(args)
                lines.append(f"const {vp}{fb_inst.name} = new {fb_inst.fb_type}({args_str});")
            elif fb_info is not None or fb_inst.fb_type in self._user_fb_types:
                lines.append(f"const {vp}{fb_inst.name} = new {self._prefixed_func(fb_inst.fb_type) if fb_inst.fb_type in self._user_fb_types else fb_inst.fb_type}();")
            else:
                # Unknown external FB type — generate auto-stub class
                js_type = fb_inst.fb_type.replace('.', '_')
                self._stub_fb_types.add((fb_inst.fb_type, js_type))
                lines.append(f"const {vp}{fb_inst.name} = new {js_type}();")
        return lines

    def _emit_program_function(self, prog: IRProgram) -> list[str]:
        """Emit a secondary PROGRAM as a JS class with execute() method.

        Same pattern as FUNCTION_BLOCK: local variables and FB instances
        stored as properties (this.xxx), body in execute(). This ensures
        state persists between calls.

        Called from main: `ProgramName.execute();`
        """
        # Reuse FUNCTION_BLOCK emission by wrapping IRProgram as IRFunctionBlock-like
        lines: list[str] = []

        pname = self._prefixed_func(prog.name)
        lines.append(f"// PROGRAM {prog.name}")
        lines.append(f"const {pname} = (function() {{")

        # Local variables
        for var in prog.locals:
            value = self._format_initial_value(var)
            lines.append(f"    let {var.name} = {value};")

        # FB instances
        for fb_inst in prog.fb_instances:
            fb_info = get_fb_info(fb_inst.fb_type)
            if fb_info is not None and fb_info.constructor_args:
                args: list[str] = []
                for param_name, _ in fb_info.constructor_args:
                    if param_name in fb_inst.constructor_args:
                        args.append(self._emit_expression(fb_inst.constructor_args[param_name]))
                    else:
                        args.append("0")
                args_str = ", ".join(args)
                lines.append(f"    const {fb_inst.name} = new {fb_inst.fb_type}({args_str});")
            elif fb_info is not None or fb_inst.fb_type in self._user_fb_types:
                lines.append(f"    const {fb_inst.name} = new {fb_inst.fb_type}();")
            else:
                js_type = fb_inst.fb_type.replace('.', '_')
                self._stub_fb_types.add((fb_inst.fb_type, js_type))
                lines.append(f"    const {fb_inst.name} = new {js_type}();")

        # Debug: export FB instances from this IIFE to globalThis for debug visualizer
        if self._debug:
            for fb_inst in prog.fb_instances:
                lines.append(f"    globalThis.{fb_inst.name} = {fb_inst.name};")

        lines.append(f"    return function {pname}() {{")

        for stmt in prog.body:
            stmt_lines = self._emit_statement(stmt, indent=2)
            lines.extend(stmt_lines)

        lines.append("    };")
        lines.append("})();")
        return lines

    def _emit_function_block_class(self, fb: IRFunctionBlock) -> list[str]:
        """Emit a FUNCTION_BLOCK as a JavaScript class with constructor and execute()."""
        lines: list[str] = []

        lines.append(f"class {self._prefixed_func(fb.name)} {{")

        # Constructor
        lines.append("    constructor() {")
        # Emit all variables (inputs, outputs, locals) as this.name = value
        all_vars = list(fb.inputs) + list(fb.outputs) + list(fb.locals)
        for var in all_vars:
            value = self._format_initial_value(var)
            lines.append(f"        this.{var.name} = {value};")
        lines.append("    }")
        lines.append("")

        # execute() method
        lines.append("    execute() {")
        for stmt in fb.body:
            stmt_lines = self._emit_statement(stmt, indent=2, fb_context=fb)
            lines.extend(stmt_lines)
        lines.append("    }")

        lines.append("}")

        # Debug: emit _debug_meta listing all watchable fields (inputs + outputs
        # + locals). The runtime in uniset2-debug.js uses this to register each
        # field via uniset_debug_watch() so live values appear in the snapshot.
        if self._debug:
            class_name = self._prefixed_func(fb.name)
            field_names = [v.name for v in all_vars]
            lines.append(f"{class_name}._debug_meta = {{ fields: {{")
            for name in field_names:
                lines.append(f'    "{name}": {{}},')
            lines.append("} };")
        return lines

    def _format_initial_value(self, var: IRVariable) -> str:
        """Format the initial value for a local variable declaration."""
        # STRUCT -> JS object literal
        if var.iec_type == IECType.STRUCT and var.struct_fields:
            return self._format_struct_literal(var.struct_fields)

        # ARRAY -> new Array(size).fill(default)
        if var.iec_type == IECType.ARRAY and var.array_size is not None:
            return self._format_array_init(var)

        if var.initial_value is not None:
            return self._format_python_value(var.initial_value, var.iec_type)

        # Use type-appropriate default
        default = _DEFAULT_VALUES.get(var.iec_type, 0)
        return self._format_python_value(default, var.iec_type)

    def _format_struct_literal(self, fields: list[IRStructField]) -> str:
        """Format a STRUCT as a JS object literal with default values per field."""
        parts: list[str] = []
        for field in fields:
            if isinstance(field.initial_value, str) and field.initial_value.startswith("__raw:"):
                val = field.initial_value[6:]  # strip __raw: prefix
            elif field.initial_value is not None:
                val = self._format_python_value(field.initial_value, field.iec_type)
            else:
                default = _DEFAULT_VALUES.get(field.iec_type, 0)
                val = self._format_python_value(default, field.iec_type)
            parts.append(f"{field.name}: {val}")
        return "{ " + ", ".join(parts) + " }"

    def _format_array_init(self, var: IRVariable) -> str:
        """Format an ARRAY as a JS array literal."""
        elem_type = var.array_element_type or IECType.INT
        default = _DEFAULT_VALUES.get(elem_type, 0)
        default_str = self._format_python_value(default, elem_type)
        return self._format_array_literal(var.array_size, default_str)

    @staticmethod
    def _format_array_literal(size: int, default_str: str) -> str:
        """Format an array as a JS literal [val, val, ...] for QuickJS compatibility."""
        return "[" + ", ".join([default_str] * size) + "]"

    def _format_python_value(self, value: Any, iec_type: IECType) -> str:
        """Format a Python value as a JS literal string."""
        if iec_type == IECType.BOOL:
            return "true" if value else "false"
        if iec_type == IECType.REAL:
            # Ensure REAL values have a decimal point
            s = str(value)
            if "." not in s and "e" not in s.lower():
                s += ".0"
            return s
        if iec_type == IECType.STRING:
            # STRING values: wrap in JS double quotes
            s = str(value)
            # If already wrapped in double quotes (from _DEFAULT_VALUES), return as-is
            if s.startswith('"') and s.endswith('"'):
                return s
            return f'"{s}"'
        return str(value)

    def _emit_statement(
        self,
        stmt: IRStatement,
        indent: int,
        fb_context: IRFunctionBlock | None = None,
    ) -> list[str]:
        """Emit a single statement as a list of indented lines.

        Args:
            stmt: The IR statement to emit.
            indent: Current indentation level.
            fb_context: If set, we are inside a FUNCTION_BLOCK body and
                variable references should use this.name prefix.
        """
        prefix = "    " * indent

        if isinstance(stmt, IRAssignment):
            target = self._emit_expression(stmt.target, fb_context=fb_context)
            value = self._emit_expression(stmt.value, fb_context=fb_context)
            return [f"{prefix}{target} = {value};"]

        if isinstance(stmt, IRProgramCall):
            return [f"{prefix}{self._prefixed_func(stmt.program_name)}();"]

        if isinstance(stmt, IRFBCall):
            return self._emit_fb_call(stmt, indent, fb_context=fb_context)

        if isinstance(stmt, IRIfElse):
            return self._emit_if_else(stmt, indent, fb_context=fb_context)

        if isinstance(stmt, IRCase):
            return self._emit_case(stmt, indent)

        if isinstance(stmt, IRForLoop):
            return self._emit_for_loop(stmt, indent)

        if isinstance(stmt, IRWhileLoop):
            return self._emit_while_loop(stmt, indent)

        if isinstance(stmt, IRRepeatLoop):
            return self._emit_repeat_loop(stmt, indent)

        if isinstance(stmt, IRExitStatement):
            return [f"{prefix}break;"]

        if isinstance(stmt, IRContinueStatement):
            return [f"{prefix}continue;"]

        if isinstance(stmt, IRReturnStatement):
            return [f"{prefix}return;"]

        return [f"{prefix}// unsupported statement: {type(stmt).__name__}"]

    def _emit_fb_call(self, call: IRFBCall, indent: int,
                      fb_context: IRFunctionBlock | None = None) -> list[str]:
        """Emit an FB call as instance.update(arg1, arg2, ...).

        Arguments are ordered according to the fb_registry update_params order.
        When inside a FUNCTION_BLOCK body (fb_context), uses this. prefix.
        """
        prefix = "    " * indent
        fb_info = get_fb_info(self._get_fb_type(call.instance))

        # Determine instance reference (this.name inside FB, prefixed otherwise)
        inst_ref = f"{self._var_prefix}{call.instance}" if self._var_prefix else call.instance
        if fb_context is not None:
            fb_var_names = {v.name for v in fb_context.inputs + fb_context.outputs + fb_context.locals}
            fb_inst_names = {fi.name for fi in fb_context.fb_instances}
            if call.instance in fb_inst_names or call.instance in fb_var_names:
                inst_ref = f"this.{call.instance}"

        if fb_info is not None:
            # Order arguments according to registry-defined update_params
            args: list[str] = []
            for param_name, _ in fb_info.update_params:
                if param_name in call.arguments:
                    args.append(self._emit_expression(call.arguments[param_name], fb_context=fb_context))
                # Skip params not provided (partial call)
            args_str = ", ".join(args)
            return [f"{prefix}{inst_ref}.update({args_str});"]

        # Check if it's a user-defined FUNCTION_BLOCK
        fb_type = self._get_fb_type(call.instance)
        if fb_type in self._user_fb_types:
            # Set input properties, then call execute()
            lines: list[str] = []
            for param_name, value in call.arguments.items():
                lines.append(f"{prefix}{inst_ref}.{param_name} = {self._emit_expression(value, fb_context=fb_context)};")
            lines.append(f"{prefix}{inst_ref}.execute();")
            return lines

        # Unknown/unsupported FB: emit as commented-out code
        args_parts = [f"{k} := {self._emit_expression(v, fb_context=fb_context)}" for k, v in call.arguments.items()]
        args_str = ", ".join(args_parts)
        return [f"{prefix}// [UNSUPPORTED] {inst_ref}({args_str});"]

    def _get_fb_type(self, instance_name: str) -> str:
        """Look up the FB type for an instance name from all programs and FBs."""
        for fb_inst in self.program.fb_instances:
            if fb_inst.name == instance_name:
                return fb_inst.fb_type
        for prog in self.program.secondary_programs:
            for fb_inst in prog.fb_instances:
                if fb_inst.name == instance_name:
                    return fb_inst.fb_type
        # Search inside user-defined FUNCTION_BLOCK declarations
        for fb_decl in self.program.function_blocks:
            for fb_inst in fb_decl.fb_instances:
                if fb_inst.name == instance_name:
                    return fb_inst.fb_type
        return ""

    def _emit_if_else(
        self,
        stmt: IRIfElse,
        indent: int,
        fb_context: IRFunctionBlock | None = None,
    ) -> list[str]:
        """Emit an if/else if/else statement."""
        prefix = "    " * indent
        lines: list[str] = []

        # if (condition) {
        cond = self._emit_expression(stmt.condition, fb_context=fb_context)
        lines.append(f"{prefix}if ({cond}) {{")

        # then body
        for s in stmt.then_body:
            lines.extend(self._emit_statement(s, indent + 1, fb_context=fb_context))

        # elsif branches
        for elsif_cond, elsif_body in stmt.elsif_branches:
            cond = self._emit_expression(elsif_cond, fb_context=fb_context)
            lines.append(f"{prefix}}} else if ({cond}) {{")
            for s in elsif_body:
                lines.extend(self._emit_statement(s, indent + 1, fb_context=fb_context))

        # else body
        if stmt.else_body is not None:
            lines.append(f"{prefix}}} else {{")
            for s in stmt.else_body:
                lines.extend(self._emit_statement(s, indent + 1, fb_context=fb_context))

        lines.append(f"{prefix}}}")
        return lines

    def _emit_case(self, stmt: IRCase, indent: int) -> list[str]:
        """Emit a switch/case statement."""
        prefix = "    " * indent
        inner_prefix = "    " * (indent + 1)
        body_prefix = "    " * (indent + 2)
        lines: list[str] = []

        selector = self._emit_expression(stmt.selector)
        lines.append(f"{prefix}switch ({selector}) {{")

        for case_values, case_body in stmt.branches:
            for val in case_values:
                val_str = self._emit_expression(val)
                lines.append(f"{inner_prefix}case {val_str}:")
            for s in case_body:
                lines.extend(self._emit_statement(s, indent + 2))
            lines.append(f"{body_prefix}break;")

        if stmt.else_body is not None:
            lines.append(f"{inner_prefix}default:")
            for s in stmt.else_body:
                lines.extend(self._emit_statement(s, indent + 2))
            lines.append(f"{body_prefix}break;")

        lines.append(f"{prefix}}}")
        return lines

    def _emit_for_loop(self, stmt: IRForLoop, indent: int) -> list[str]:
        """Emit a for loop statement."""
        prefix = "    " * indent
        lines: list[str] = []

        var = stmt.var
        start = self._emit_expression(stmt.start)
        end = self._emit_expression(stmt.end)

        if stmt.step is not None:
            step = self._emit_expression(stmt.step)
        else:
            step = "1"

        lines.append(
            f"{prefix}for (let {var} = {start}; {var} <= {end}; {var} += {step}) {{"
        )
        for s in stmt.body:
            lines.extend(self._emit_statement(s, indent + 1))
        lines.append(f"{prefix}}}")
        return lines

    def _emit_while_loop(self, stmt: IRWhileLoop, indent: int) -> list[str]:
        """Emit a while loop statement."""
        prefix = "    " * indent
        lines: list[str] = []

        cond = self._emit_expression(stmt.condition)
        lines.append(f"{prefix}while ({cond}) {{")
        for s in stmt.body:
            lines.extend(self._emit_statement(s, indent + 1))
        lines.append(f"{prefix}}}")
        return lines

    def _emit_repeat_loop(self, stmt: IRRepeatLoop, indent: int) -> list[str]:
        """Emit a do..while loop statement (REPEAT..UNTIL)."""
        prefix = "    " * indent
        lines: list[str] = []

        cond = self._emit_expression(stmt.condition)
        lines.append(f"{prefix}do {{")
        for s in stmt.body:
            lines.extend(self._emit_statement(s, indent + 1))
        lines.append(f"{prefix}}} while (!({cond}));")
        return lines

    def _emit_expression(
        self,
        expr: IRExpression,
        fb_context: IRFunctionBlock | None = None,
    ) -> str:
        """Emit an expression as a JavaScript string.

        Args:
            expr: The IR expression to emit.
            fb_context: If set, variable references within a FUNCTION_BLOCK
                body should use this.name prefix for the FB's own variables.
        """
        if isinstance(expr, IRLiteral):
            return self._emit_literal(expr)

        if isinstance(expr, IRVarRef):
            return self._emit_var_ref(expr, fb_context=fb_context)

        if isinstance(expr, IRArrayAccess):
            return self._emit_array_access(expr, fb_context=fb_context)

        if isinstance(expr, IRFieldAccess):
            obj = self._emit_expression(expr.obj, fb_context=fb_context)
            # Check if accessing a field on an unsupported FB
            if isinstance(expr.obj, IRVarRef) and expr.obj.name in self._unsupported_fbs:
                return f"undefined /* [UNSUPPORTED] {obj}.{expr.field} */"
            # Use bracket notation for numeric or non-identifier field names
            if not expr.field.isidentifier():
                return f'{obj}["{expr.field}"]'
            return f"{obj}.{expr.field}"

        if isinstance(expr, IRBinaryOp):
            left = self._emit_expression(expr.left, fb_context=fb_context)
            right = self._emit_expression(expr.right, fb_context=fb_context)
            js_op = _OPERATOR_MAP.get(expr.op, expr.op)
            return f"({left} {js_op} {right})"

        if isinstance(expr, IRUnaryOp):
            operand = self._emit_expression(expr.operand, fb_context=fb_context)
            if expr.op == "NOT":
                return f"!{operand}"
            return f"-{operand}"

        if isinstance(expr, IRTypeCoercion):
            return self._emit_type_coercion(expr)

        if isinstance(expr, IRFunctionCall):
            return self._emit_function_call_expr(expr, fb_context=fb_context)

        return f"/* unsupported: {type(expr).__name__} */"

    def _emit_function_call_expr(
        self,
        expr: IRFunctionCall,
        fb_context: IRFunctionBlock | None = None,
    ) -> str:
        """Emit a FUNCTION call expression as ``funcName(arg1, arg2, ...)``."""
        args = []
        for param_name, value in expr.arguments:
            args.append(self._emit_expression(value, fb_context=fb_context))
        return f"{expr.name}({', '.join(args)})"

    def _emit_array_access(
        self,
        expr: IRArrayAccess,
        fb_context: IRFunctionBlock | None = None,
    ) -> str:
        """Emit an array access with 1-based to 0-based index conversion.

        Looks up the array variable to determine the lower bound,
        then emits arr[index - lower_bound].
        """
        array_str = self._emit_expression(expr.array, fb_context=fb_context)
        index_str = self._emit_expression(expr.index, fb_context=fb_context)

        # Determine the lower bound from the array variable
        lower_bound = 1
        if isinstance(expr.array, IRVarRef):
            var = self._find_variable(expr.array.name)
            if var is not None and var.array_lower_bound is not None:
                lower_bound = var.array_lower_bound

        if lower_bound != 0:
            return f"{array_str}[{index_str} - {lower_bound}]"
        return f"{array_str}[{index_str}]"

    def _find_variable(self, name: str) -> IRVariable | None:
        """Find a variable by name across all program variable sections."""
        for var in self.program.globals + self.program.inputs + self.program.outputs + self.program.locals:
            if var.name == name:
                return var
        return None

    def _emit_literal(self, lit: IRLiteral) -> str:
        """Emit a literal value."""
        if lit.iec_type == IECType.BOOL:
            return "true" if lit.value else "false"
        if lit.iec_type == IECType.STRING:
            return f'"{lit.value}"'
        return str(lit.value)

    def _emit_var_ref(
        self,
        ref: IRVarRef,
        fb_context: IRFunctionBlock | None = None,
    ) -> str:
        """Emit a variable reference with input/output prefix substitution.

        When fb_context is set, variables belonging to the FUNCTION_BLOCK
        are prefixed with 'this.' instead of using sensor mapping.
        """
        name = ref.name

        # Inside a FUNCTION_BLOCK body: use this.name for the FB's own variables
        if fb_context is not None:
            fb_var_names = {v.name for v in fb_context.inputs + fb_context.outputs + fb_context.locals}
            if name in fb_var_names:
                return f"this.{name}"

        if name in self._input_names:
            # Scaled inputs are declared as local let variables in uniset_on_step;
            # use the ST name directly in the body
            if name in self._scaled_inputs:
                return name
            # Flattened struct inputs are reconstructed as local let variables
            if self._struct_flatten and name in self._flat_input_structs:
                return name
            entry = self._input_map.get(name)
            if entry is not None:
                return f"in_{self._prefixed_sensor(entry.sensor_name)}"
            # Unmapped: use ST name with prefix
            return f"in_{self._prefixed_sensor(name)}"

        if name in self._output_names:
            # Scaled outputs are written via Math.round at the bottom;
            # use the ST name directly in the body
            if name in self._scaled_outputs:
                return name
            # Flattened struct outputs: use ST name, field extraction happens at bottom
            if self._struct_flatten and name in self._flat_output_structs:
                return name
            entry = self._output_map.get(name)
            if entry is not None:
                return f"out_{self._prefixed_sensor(entry.sensor_name)}"
            return f"out_{self._prefixed_sensor(name)}"

        # Global variable: no prefix (shared across programs)
        if name in self._global_names:
            return name

        # GVL field accessed without qualifier: rewrite to GVL_Name.VarName
        if name in self.program.gvl_field_map:
            gvl_name, _ = self.program.gvl_field_map[name]
            return f"{gvl_name}.{name}"

        # Local variable: use prefixed name
        return f"{self._var_prefix}{name}" if self._var_prefix else name

    def _prefixed_sensor(self, sensor_name: str) -> str:
        """Apply var_prefix to a sensor name."""
        return f"{self._var_prefix}{sensor_name}" if self._var_prefix else sensor_name

    def _prefixed_func(self, func_name: str) -> str:
        """Apply func_prefix to a function/program/FB name."""
        return f"{self._func_prefix}{func_name}" if self._func_prefix else func_name

    @staticmethod
    def _format_scale(scale: float) -> str:
        """Format a scale factor value, omitting .0 for integer values."""
        if scale == int(scale):
            return str(int(scale))
        return str(scale)

    _FLOAT_TYPES = {IECType.REAL, IECType.LREAL}
    _ALL_INTEGER_TYPES = {
        IECType.SINT, IECType.INT, IECType.DINT, IECType.LINT,
        IECType.USINT, IECType.UINT, IECType.UDINT, IECType.ULINT,
        IECType.BYTE, IECType.WORD, IECType.DWORD, IECType.LWORD,
    }

    # (Debug trace instrumentation (_dbg_if/_dbg_case) removed — the
    # Execution Trace panel was dropped from the UI and the per-branch
    # function-call overhead was not justified.)

    def _emit_debug_stubs(self) -> list[str]:
        """Emit debug initialization: load uniset2-debug.js and start debug server."""
        return [
            "// Debug: load visualizer and start HTTP server",
            "try {",
            '    load("uniset2-debug.js");',
            '    if (typeof uniset_debug_set_ui === "function") {',
            '        try { uniset_debug_set_ui(loadFile("uniset2-debug-ui.html")); } catch(e2) {}',
            "    }",
            "    uniset_debug_start(8088);",
            "} catch(e) {",
            '    // uniset2-debug.js not found — debug visualizer unavailable',
            "}",
        ]

    def _collect_fb_type_ports(self) -> dict[str, tuple[list[str], list[str]]]:
        """Build fb_type -> (input_names, output_names) lookup.

        Covers user-defined FBs from the ST source (IRFunctionBlock),
        including nested FBs declared inside other FBs, and standard/library
        FBs from the fb_registry (IEC 61131-3 primitives + YAML lib types).
        """
        ports: dict[str, tuple[list[str], list[str]]] = {}

        # User-defined FBs from this program (and nested FBs they declare)
        def _walk(fb: IRFunctionBlock) -> None:
            in_names = [v.name for v in fb.inputs]
            out_names = [v.name for v in fb.outputs]
            ports[fb.name] = (in_names, out_names)
            # FBs can be declared inside other FBs via fb_instances (type references),
            # but the declaration itself is only in program.function_blocks, so no recursion needed.

        for fb in self.program.function_blocks:
            _walk(fb)

        # Standard IEC FBs + YAML library FBs (via fb_registry)
        # Collect the unique set of fb_types referenced by instances
        referenced: set[str] = set()
        for fi in self.program.fb_instances:
            referenced.add(fi.fb_type)
        for sec in self.program.secondary_programs:
            for fi in sec.fb_instances:
                referenced.add(fi.fb_type)
        for fb in self.program.function_blocks:
            for fi in fb.fb_instances:
                referenced.add(fi.fb_type)

        for fb_type in referenced:
            if fb_type in ports:
                continue
            info = get_fb_info(fb_type)
            if info is None:
                continue
            # Inputs on the FBD diagram: update_params first (dynamic signals),
            # then constructor_args (compile-time parameters like PT, PV).
            in_names = [n for n, _ in info.update_params] + [n for n, _ in info.constructor_args]
            out_names = [n for n, _ in info.outputs]
            ports[fb_type] = (in_names, out_names)

        return ports

    def _emit_program_meta(self) -> list[str]:
        """Emit globalThis._program_meta with program structure info."""
        lines: list[str] = []
        lines.append("globalThis._program_meta = {")
        lines.append(f'    name: "{self.program.name}",')

        # Inputs
        lines.append("    inputs: [")
        for var in self.program.inputs:
            entry = self._input_map.get(var.name)
            sensor = entry.sensor_name if entry else var.name
            iec_type = var.iec_type.name if var.iec_type else "INT"
            parts = [
                f'name: "{var.name}"',
                f'sensor: "{sensor}"',
                f'type: "{iec_type}"',
            ]
            if entry and entry.scale is not None:
                scale_str = self._format_scale(entry.scale)
                parts.append(f"scale: {scale_str}")
            lines.append(f"        {{ {', '.join(parts)} }},")
        lines.append("    ],")

        # Outputs
        lines.append("    outputs: [")
        for var in self.program.outputs:
            entry = self._output_map.get(var.name)
            sensor = entry.sensor_name if entry else var.name
            iec_type = var.iec_type.name if var.iec_type else "INT"
            parts = [
                f'name: "{var.name}"',
                f'sensor: "{sensor}"',
                f'type: "{iec_type}"',
            ]
            if entry and entry.scale is not None:
                scale_str = self._format_scale(entry.scale)
                parts.append(f"scale: {scale_str}")
            lines.append(f"        {{ {', '.join(parts)} }},")
        lines.append("    ],")

        # Locals
        lines.append("    locals: [")
        for var in self.program.locals:
            iec_type = var.iec_type.name if var.iec_type else "INT"
            lines.append(f'        {{ name: "{var.name}", type: "{iec_type}" }},')
        lines.append("    ],")

        # Globals (GVL objects)
        lines.append("    globals: [")
        for var in self.program.globals:
            iec_type = var.iec_type.name if var.iec_type else "INT"
            lines.append(f'        {{ name: "{var.name}", type: "{iec_type}" }},')
        lines.append("    ],")

        # FB instances (from all programs, deduplicated)
        fb_type_ports = self._collect_fb_type_ports()
        lines.append("    fb_instances: [")
        seen_fbs: set[str] = set()
        all_fb_insts = list(self.program.fb_instances)
        for sec in self.program.secondary_programs:
            all_fb_insts.extend(sec.fb_instances)
        for fb_inst in all_fb_insts:
            if fb_inst.name in seen_fbs:
                continue
            seen_fbs.add(fb_inst.name)
            parts = [f'name: "{fb_inst.name}"', f'type: "{fb_inst.fb_type}"']
            port_def = fb_type_ports.get(fb_inst.fb_type)
            if port_def:
                in_js = '[' + ', '.join(f'"{n}"' for n in port_def[0]) + ']'
                out_js = '[' + ', '.join(f'"{n}"' for n in port_def[1]) + ']'
                parts.append(f'inputs: {in_js}')
                parts.append(f'outputs: {out_js}')
            lines.append(f'        {{ {", ".join(parts)} }},')
        lines.append("    ],")

        # Connections and operator nodes (extracted from all program bodies)
        operator_nodes: list = []
        self._op_counter = 0
        main_conns = self._extract_connections(self.program.body, operator_nodes)
        for c in main_conns:
            c["program"] = self.program.name
        connections = list(main_conns)
        for sec in self.program.secondary_programs:
            sec_conns = self._extract_connections(sec.body, operator_nodes)
            for c in sec_conns:
                c["program"] = sec.name
            connections.extend(sec_conns)

        # Collect program names for filter dropdown
        programs_list = [self.program.name] + [s.name for s in self.program.secondary_programs]

        # Operator nodes (virtual FB-like blocks for complex expressions)
        lines.append("    operators: [")
        seen_ops: set = set()
        for op in operator_nodes:
            if op["id"] in seen_ops:
                continue
            seen_ops.add(op["id"])
            inputs_js = '[' + ', '.join(f'"{i}"' for i in op["inputs"]) + ']'
            lines.append(f'        {{ id: "{op["id"]}", op: "{op["op"]}", inputs: {inputs_js} }},')
        lines.append("    ],")

        # Program names for filter dropdown
        lines.append("    programs: [")
        for pn in programs_list:
            lines.append(f'        "{pn}",')
        lines.append("    ],")

        lines.append("    connections: [")
        for conn in connections:
            def _esc(s):
                return s.replace('\\', '\\\\').replace('"', '\\"')
            parts = [f'from: "{_esc(conn["from"])}"', f'to: "{_esc(conn["to"])}"', f'toParam: "{_esc(conn["toParam"])}"']
            if conn.get("fromField"):
                parts.append(f'fromField: "{_esc(conn["fromField"])}"')
            if conn.get("toField"):
                parts.append(f'toField: "{_esc(conn["toField"])}"')
            if conn.get("kind"):
                parts.append(f'kind: "{_esc(conn["kind"])}"')
            if conn.get("program"):
                parts.append(f'program: "{_esc(conn["program"])}"')
            lines.append(f"        {{ {', '.join(parts)} }},")
        lines.append("    ],")

        lines.append("};")
        lines.append("")
        return lines

    def _extract_connections(self, stmts: list, operator_nodes: list = None) -> list[dict]:
        """Extract FB connections from IR statements for the schema graph.

        Extracts:
        1. FB call inputs: fb(PARAM := source) → {from: source, to: fb, toParam: PARAM}
        2. Output assignments: var := fb.field → {from: fb, fromField: field, to: var, toParam: "="}
        3. Operator nodes: complex expressions (A AND B, NOT C) → virtual nodes with connections

        Args:
            stmts: IR statement list
            operator_nodes: accumulator for generated operator nodes
        """
        if operator_nodes is None:
            operator_nodes = []
            self._op_counter = 0
        connections = []
        for stmt in stmts:
            if isinstance(stmt, IRFBCall):
                for param_name, expr in stmt.arguments.items():
                    conn = self._classify_connection(stmt.instance, param_name, expr, operator_nodes, connections)
                    if conn:
                        connections.append(conn)
            # Output assignments: target := fb.field
            if isinstance(stmt, IRAssignment):
                if isinstance(stmt.value, IRFieldAccess) and isinstance(stmt.value.obj, IRVarRef):
                    target = None
                    target_field = None
                    if isinstance(stmt.target, IRVarRef):
                        target = stmt.target.name
                    elif isinstance(stmt.target, IRFieldAccess) and isinstance(stmt.target.obj, IRVarRef):
                        target = stmt.target.obj.name
                        target_field = stmt.target.field
                    if target:
                        conn = {
                            "from": stmt.value.obj.name,
                            "fromField": stmt.value.field,
                            "to": target,
                            "toParam": "=",
                            "kind": "assign",
                        }
                        if target_field:
                            conn["toField"] = target_field
                        connections.append(conn)
            # Recurse into control flow
            for attr in ('then_body', 'else_body', 'body'):
                sub = getattr(stmt, attr, None)
                if isinstance(sub, list):
                    connections.extend(self._extract_connections(sub, operator_nodes))
            if hasattr(stmt, 'branches'):
                for _, body in stmt.branches:
                    connections.extend(self._extract_connections(body, operator_nodes))
            if hasattr(stmt, 'elsif_branches'):
                for _, body in stmt.elsif_branches:
                    connections.extend(self._extract_connections(body, operator_nodes))
        self._extracted_operator_nodes = operator_nodes
        return connections

    def _classify_connection(self, instance: str, param: str, expr,
                              operator_nodes: list = None, connections: list = None) -> dict | None:
        """Classify an FB call argument as a connection.

        For complex expressions, creates operator nodes with inner connections.
        Returns the final connection linking to the target FB.param.
        """
        if isinstance(expr, IRLiteral):
            return None

        if isinstance(expr, IRFieldAccess) and isinstance(expr.obj, IRVarRef):
            return {"from": expr.obj.name, "to": instance, "toParam": param, "fromField": expr.field, "kind": "call"}

        if isinstance(expr, IRVarRef):
            return {"from": expr.name, "to": instance, "toParam": param, "kind": "call"}

        if isinstance(expr, IRTypeCoercion):
            return self._classify_connection(instance, param, expr.expr, operator_nodes, connections)

        # Complex expression — build operator node subgraph
        if operator_nodes is not None and connections is not None:
            op_id = self._build_operator_subgraph(expr, operator_nodes, connections)
            if op_id:
                return {"from": op_id, "to": instance, "toParam": param, "fromField": "OUT", "kind": "call"}

        return None

    def _build_operator_subgraph(self, expr, operator_nodes: list, connections: list) -> str | None:
        """Recursively build operator nodes for a complex expression.

        Returns the output node ID of the subgraph, or None for literals.
        """
        if isinstance(expr, IRLiteral):
            return None

        if isinstance(expr, IRVarRef):
            return expr.name

        if isinstance(expr, IRFieldAccess) and isinstance(expr.obj, IRVarRef):
            # Return a virtual id that edges reference as source.field
            return expr.obj.name  # edges will use fromField separately

        if isinstance(expr, IRTypeCoercion):
            return self._build_operator_subgraph(expr.expr, operator_nodes, connections)

        if isinstance(expr, IRBinaryOp):
            self._op_counter += 1
            op_id = f"__op{self._op_counter}_{expr.op}"
            operator_nodes.append({"id": op_id, "op": expr.op, "inputs": ["IN1", "IN2"]})
            left_id = self._build_operator_subgraph(expr.left, operator_nodes, connections)
            right_id = self._build_operator_subgraph(expr.right, operator_nodes, connections)
            if left_id:
                conn = {"from": left_id, "to": op_id, "toParam": "IN1", "kind": "call"}
                if isinstance(expr.left, IRFieldAccess):
                    conn["fromField"] = expr.left.field
                connections.append(conn)
            if right_id:
                conn = {"from": right_id, "to": op_id, "toParam": "IN2", "kind": "call"}
                if isinstance(expr.right, IRFieldAccess):
                    conn["fromField"] = expr.right.field
                connections.append(conn)
            return op_id

        if isinstance(expr, IRUnaryOp):
            self._op_counter += 1
            op_id = f"__op{self._op_counter}_{expr.op}"
            operator_nodes.append({"id": op_id, "op": expr.op, "inputs": ["IN"]})
            operand_id = self._build_operator_subgraph(expr.operand, operator_nodes, connections)
            if operand_id:
                conn = {"from": operand_id, "to": op_id, "toParam": "IN", "kind": "call"}
                if isinstance(expr.operand, IRFieldAccess):
                    conn["fromField"] = expr.operand.field
                connections.append(conn)
            return op_id

        return None

    def _emit_type_coercion(self, coercion: IRTypeCoercion) -> str:
        """Emit a type coercion expression."""
        inner = self._emit_expression(coercion.expr)

        # Float -> integer: truncate
        if (coercion.from_type in self._FLOAT_TYPES
                and coercion.to_type in self._ALL_INTEGER_TYPES):
            return f"Math.round({inner})"

        # BOOL -> integer
        if (coercion.from_type == IECType.BOOL
                and coercion.to_type in self._ALL_INTEGER_TYPES):
            return f"({inner} ? 1 : 0)"

        # Default: pass through
        return inner
