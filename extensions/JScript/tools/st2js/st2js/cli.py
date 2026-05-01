#!/usr/bin/env python3
"""CLI entry point for the st2js converter.

Provides main() function that wires the full pipeline:
  read ST file -> parse_st() -> transform() -> check_types() -> generate() -> write output

Usage:
  python -m st2js input.st -m mapping.yaml -o output.js
  python -m st2js input.st -m mapping.yaml          # output to stdout
  python -m st2js input.st -m mapping.yaml --strict
  python -m st2js input.st -m mapping.yaml --struct-flatten
"""
from __future__ import annotations

import argparse
import sys
from typing import Any

from st2js import __version__
from st2js.errors import STError


def _build_parser() -> argparse.ArgumentParser:
    """Build and return the argument parser."""
    parser = argparse.ArgumentParser(
        prog="st2js",
        description="Convert IEC 61131-3 Structured Text to JavaScript for UniSet2 JScript runtime.",
    )
    parser.add_argument(
        "input",
        nargs="*",
        help="Path to ST source file(s) (.st, .txt, .xml, .TcPOU, .TcDUT, .TcGVL)",
    )
    parser.add_argument(
        "-m", "--mapping",
        help="Path to the YAML sensor mapping file",
    )
    parser.add_argument(
        "-o", "--output",
        help="Path to the output JS file (default: stdout)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Treat warnings as errors",
    )
    parser.add_argument(
        "--struct-flatten",
        action="store_true",
        help="Flatten struct fields into individual sensor entries",
    )
    parser.add_argument(
        "--programs",
        help="Comma-separated list of PROGRAM names to convert (default: all)",
    )
    parser.add_argument(
        "--main",
        default="Main",
        help="Name of the main program (entry point for uniset_on_step, default: Main)",
    )
    parser.add_argument(
        "--ignore-undefined",
        action="store_true",
        help="Treat undefined variables as warnings instead of errors (assume INT type)",
    )
    parser.add_argument(
        "--lib",
        action="append",
        default=[],
        help="Path to YAML library type definitions (can be repeated)",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Emit debug trace instrumentation (_dbg_if, _dbg_case, _program_meta)",
    )
    parser.add_argument(
        "--load-head",
        action="append",
        default=[],
        metavar="PATH",
        help="Extra load(\"PATH\") module emitted at the top of the "
             "generated file (repeatable; appended to mapping YAML)",
    )
    parser.add_argument(
        "--load-on-start",
        action="append",
        default=[],
        metavar="PATH",
        help="Extra load(\"PATH\") module emitted inside uniset_on_start() "
             "(repeatable; appended to mapping YAML)",
    )
    parser.add_argument(
        "--version",
        action="version",
        version=f"%(prog)s {__version__}",
    )
    return parser


def main(args: list[str] | None = None) -> int:
    """Main entry point for the st2js converter.

    Args:
        args: Command-line arguments (default: sys.argv[1:]).

    Returns:
        Exit code: 0 on success, 1 on error.
    """
    parser = _build_parser()
    parsed = parser.parse_args(args)

    if not parsed.input:
        parser.print_help()
        return 0

    program_filter = None
    if parsed.programs:
        program_filter = [p.strip() for p in parsed.programs.split(",")]

    # Load external library type definitions
    if parsed.lib:
        from st2js.fb_registry import load_lib_types
        for lib_path in parsed.lib:
            try:
                count = load_lib_types(lib_path)
                print(f"st2js: loaded {count} types from {lib_path}", file=sys.stderr)
            except Exception as exc:
                print(f"st2js: error loading --lib {lib_path}: {exc}", file=sys.stderr)
                return 1

    try:
        import warnings as _warnings
        with _warnings.catch_warnings(record=True) as caught_warnings:
            _warnings.simplefilter("always")

            ignore_undef = parsed.ignore_undefined and not parsed.strict

            if len(parsed.input) == 1:
                # Single file mode
                js_output, stats = _run_pipeline(
                    input_path=parsed.input[0],
                    mapping_path=parsed.mapping,
                    program_filter=program_filter,
                    main_program=parsed.main,
                    strict=parsed.strict,
                    struct_flatten=parsed.struct_flatten,
                    debug=parsed.debug,
                    ignore_undefined=ignore_undef,
                    cli_load_head=parsed.load_head,
                    cli_load_on_start=parsed.load_on_start,
                )
            else:
                # Multi-file mode: combine all files into one JS
                js_output, stats = _run_multi_pipeline(
                    input_paths=parsed.input,
                    mapping_path=parsed.mapping,
                    program_filter=program_filter,
                    main_program=parsed.main,
                    strict=parsed.strict,
                    struct_flatten=parsed.struct_flatten,
                    debug=parsed.debug,
                    ignore_undefined=ignore_undef,
                    cli_load_head=parsed.load_head,
                    cli_load_on_start=parsed.load_on_start,
                )

            stats.warnings = len(caught_warnings)
            if parsed.strict and caught_warnings:
                for w in caught_warnings:
                    print(f"st2js: warning: {w.message}", file=sys.stderr)
                print(f"st2js: error: {len(caught_warnings)} warning(s) with --strict", file=sys.stderr)
                return 1
    except STError as exc:
        print(f"st2js: {exc}", file=sys.stderr)
        return 1

    if parsed.output:
        try:
            with open(parsed.output, "w") as f:
                f.write(js_output)
        except OSError as exc:
            print(f"st2js: error writing output: {exc}", file=sys.stderr)
            return 1
    else:
        sys.stdout.write(js_output)

    # Print statistics to stderr
    if len(parsed.input) == 1:
        input_name = parsed.input[0].rsplit("/", 1)[-1] if "/" in parsed.input[0] else parsed.input[0]
    else:
        input_name = f"{len(parsed.input)} files"
    print(
        f"st2js: {input_name}: {stats.lines} lines, "
        f"{stats.programs} programs, {stats.function_blocks} FBs, "
        f"{stats.unsupported} unsupported, {stats.warnings} warnings",
        file=sys.stderr,
    )

    return 0


def _run_pipeline(
    input_path: str,
    mapping_path: str | None = None,
    program_filter: list[str] | None = None,
    main_program: str = "Main",
    strict: bool = False,
    struct_flatten: bool = False,
    debug: bool = False,
    ignore_undefined: bool = False,
    cli_load_head: list[str] | None = None,
    cli_load_on_start: list[str] | None = None,
):
    """Run the full ST-to-JS conversion pipeline.

    Returns:
        Tuple of (JavaScript source string, GenerateStats).
    """
    from st2js.codegen import generate_with_stats
    from st2js.mapping import SensorMapping, MappingOptions, load_mapping, _dedup_append
    from st2js.parser import parse_file
    from st2js.transformer import transform
    from st2js.type_system import check_types

    # Load mapping or create empty (ST variable names = sensor names)
    if mapping_path:
        mapping = load_mapping(mapping_path)
    else:
        mapping = SensorMapping(inputs=[], outputs=[], options=MappingOptions(struct_flatten=struct_flatten))

    # Merge CLI load directives (dedup, preserving first occurrence)
    if cli_load_head:
        mapping.load_head = _dedup_append(mapping.load_head, cli_load_head)
    if cli_load_on_start:
        mapping.load_on_start = _dedup_append(mapping.load_on_start, cli_load_on_start)

    # Apply per-file overrides
    file_mapping = mapping.for_file(input_path)

    # Parse (auto-detects format by extension) -> Transform -> Type Check -> Generate
    from st2js.transformer import resolve_io_by_prefix
    parse_result = parse_file(input_path)
    ir_program = transform(parse_result, program_filter=program_filter,
                           main_program=main_program)
    resolve_io_by_prefix(ir_program)
    typed_program = check_types(ir_program, ignore_undefined=ignore_undefined)
    js_output, stats = generate_with_stats(typed_program, file_mapping, debug=debug)

    return js_output, stats


def _apply_auto_prefix_if_needed(
    ir_programs: list[tuple[str, Any]],
    mapping: 'SensorMapping',
) -> None:
    """Check for name collisions across parsed IR and auto-assign prefixes.

    Mutates mapping.files in-place. Only assigns prefixes to files
    that don't already have them. Prefixes are p1_, p2_, ... by file order.
    """
    from st2js.mapping import FileMapping

    # Check if all files already have prefixes
    all_have_prefix = True
    for path, _ in ir_programs:
        basename = path.rsplit("/", 1)[-1] if "/" in path else path
        fm = mapping.files.get(basename)
        if fm is None or not fm.var_prefix:
            all_have_prefix = False
            break

    if all_have_prefix:
        return

    # Collect variable names from each file's IR
    file_names: list[set[str]] = []
    for _, ir in ir_programs:
        names = set()
        for v in ir.inputs + ir.outputs + ir.locals:
            names.add(v.name)
        for fb in ir.fb_instances:
            names.add(fb.name)
        file_names.append(names)

    # Check for collisions
    seen: set[str] = set()
    has_collision = False
    for names in file_names:
        if seen & names:
            has_collision = True
            break
        seen |= names

    if not has_collision:
        return

    # Assign auto-prefixes
    import sys
    print("st2js: auto-assigning prefixes due to name collisions between files",
          file=sys.stderr)

    for i, (path, _) in enumerate(ir_programs):
        basename = path.rsplit("/", 1)[-1] if "/" in path else path
        existing = mapping.files.get(basename)
        if existing is None or not existing.var_prefix:
            prefix = f"p{i + 1}_"
            mapping.files[basename] = FileMapping(
                var_prefix=prefix,
                func_prefix=prefix,
                inputs=existing.inputs if existing else [],
                outputs=existing.outputs if existing else [],
            )


def _run_multi_pipeline(
    input_paths: list[str],
    mapping_path: str | None = None,
    program_filter: list[str] | None = None,
    main_program: str = "Main",
    strict: bool = False,
    struct_flatten: bool = False,
    debug: bool = False,
    ignore_undefined: bool = False,
    cli_load_head: list[str] | None = None,
    cli_load_on_start: list[str] | None = None,
):
    """Run conversion for multiple files and combine into one JS output.

    Each file is converted independently with its own per-file mapping.
    Results are combined into one JS: shared header, merged sensor arrays,
    each file's body wrapped in a named function, single uniset_on_step
    calling all file functions.
    """
    from st2js.codegen import generate, generate_with_stats, GenerateStats
    from st2js.mapping import SensorMapping, MappingOptions, load_mapping, _dedup_append
    from st2js.parser import parse_file
    from st2js.transformer import transform
    from st2js.type_system import check_types

    if mapping_path:
        base_mapping = load_mapping(mapping_path)
    else:
        base_mapping = SensorMapping(inputs=[], outputs=[], options=MappingOptions(struct_flatten=struct_flatten))

    # Merge CLI load directives (dedup, preserving first occurrence)
    if cli_load_head:
        base_mapping.load_head = _dedup_append(base_mapping.load_head, cli_load_head)
    if cli_load_on_start:
        base_mapping.load_on_start = _dedup_append(base_mapping.load_on_start, cli_load_on_start)

    # Phase 1: Parse and transform all files
    ir_programs: list[tuple[str, Any]] = []  # (path, typed_ir)
    from st2js.transformer import resolve_io_by_prefix
    for path in input_paths:
        parse_result = parse_file(path)
        ir_program = transform(parse_result, program_filter=program_filter,
                               main_program=main_program)
        resolve_io_by_prefix(ir_program)
        typed_program = check_types(ir_program, ignore_undefined=ignore_undefined)
        ir_programs.append((path, typed_program))

    # Phase 2: Auto-prefix if name collisions detected and no explicit prefixes
    _apply_auto_prefix_if_needed(ir_programs, base_mapping)

    # Phase 3: Generate JS for each file
    file_results: list[tuple[str, str, GenerateStats]] = []
    total_stats = GenerateStats()

    for path, typed_program in ir_programs:
        file_mapping = base_mapping.for_file(path)
        js, stats = generate_with_stats(typed_program, file_mapping, debug=debug)
        file_results.append((path, js, stats))
        total_stats.programs += stats.programs
        total_stats.function_blocks += stats.function_blocks
        total_stats.fb_instances += stats.fb_instances
        total_stats.inputs += stats.inputs
        total_stats.outputs += stats.outputs
        total_stats.locals += stats.locals
        total_stats.unsupported += stats.unsupported

    # Extract sections from each file's JS
    def _extract_sections(js: str) -> dict:
        """Split generated JS into sections for merging."""
        lines = js.split("\n")
        sections = {
            "loads": [],
            "inputs": [],
            "outputs": [],
            "body": [],
            "on_step": [],
            "on_start_loads": [],   # load("...") calls inside uniset_on_start body
        }
        state = "header"
        for line in lines:
            if line == "function uniset_on_start() {}":
                # Empty stub — nothing to capture.
                continue
            if line == "function uniset_on_start() {":
                state = "on_start"
                continue
            if state == "on_start":
                if line == "}":
                    state = "done"
                else:
                    stripped = line.strip()
                    if stripped.startswith("load("):
                        sections["on_start_loads"].append(stripped)
                continue

            if line.startswith("load("):
                sections["loads"].append(line)
            elif "uniset_inputs" in line and "[" in line:
                state = "inputs"
            elif "uniset_outputs" in line and "[" in line:
                state = "outputs"
            elif state == "inputs":
                if line.strip() == "];":
                    state = "between"
                elif line.strip():
                    sections["inputs"].append(line.strip().rstrip(","))
            elif state == "outputs":
                if line.strip() == "];":
                    state = "between"
                elif line.strip():
                    sections["outputs"].append(line.strip().rstrip(","))
            elif line == "function uniset_on_step() {":
                state = "on_step"
            elif state == "on_step":
                if line == "}":
                    state = "done"
                else:
                    sections["on_step"].append(line)
            elif state in ("between", "header"):
                if line.startswith("//") and "Generated by" in line:
                    continue
                if line.strip():
                    sections["body"].append(line)
        return sections

    # Merge all files
    all_sections = [_extract_sections(js) for _, js, _ in file_results]

    out_lines: list[str] = []
    out_lines.append(f"// Generated by st2js from {len(input_paths)} files")

    # Deduplicated load() statements
    seen_loads: set[str] = set()
    for s in all_sections:
        for load_line in s["loads"]:
            if load_line not in seen_loads:
                out_lines.append(load_line)
                seen_loads.add(load_line)
    out_lines.append("")

    # Merged uniset_inputs
    all_inputs = []
    for s in all_sections:
        all_inputs.extend(s["inputs"])
    if all_inputs:
        out_lines.append("uniset_inputs = [")
        for i, entry in enumerate(all_inputs):
            comma = "," if i < len(all_inputs) - 1 else ""
            out_lines.append(f"    {entry}{comma}")
        out_lines.append("];")
    else:
        out_lines.append("uniset_inputs = [];")
    out_lines.append("")

    # Merged uniset_outputs
    all_outputs = []
    for s in all_sections:
        all_outputs.extend(s["outputs"])
    if all_outputs:
        out_lines.append("uniset_outputs = [")
        for i, entry in enumerate(all_outputs):
            comma = "," if i < len(all_outputs) - 1 else ""
            out_lines.append(f"    {entry}{comma}")
        out_lines.append("];")
    else:
        out_lines.append("uniset_outputs = [];")
    out_lines.append("")

    # Body sections from each file (vars, FB instances, classes, secondary programs)
    for (path, _, _), s in zip(file_results, all_sections):
        if s["body"]:
            basename = path.rsplit("/", 1)[-1] if "/" in path else path
            out_lines.append(f"// --- {basename} ---")
            out_lines.extend(s["body"])
            out_lines.append("")

    # Single uniset_on_step calling all file bodies
    out_lines.append("function uniset_on_step() {")
    for (path, _, _), s in zip(file_results, all_sections):
        if s["on_step"]:
            basename = path.rsplit("/", 1)[-1] if "/" in path else path
            out_lines.append(f"    // {basename}")
            out_lines.extend(s["on_step"])
    out_lines.append("}")
    out_lines.append("")

    # Combined uniset_on_start with deduplicated load() calls
    seen_on_start: set[str] = set()
    combined_on_start: list[str] = []
    for s in all_sections:
        for load_line in s["on_start_loads"]:
            if load_line not in seen_on_start:
                combined_on_start.append(load_line)
                seen_on_start.add(load_line)
    if combined_on_start:
        out_lines.append("function uniset_on_start() {")
        for load_line in combined_on_start:
            out_lines.append(f"    {load_line}")
        out_lines.append("}")
    else:
        out_lines.append("function uniset_on_start() {}")
    out_lines.append("function uniset_on_stop() {}")
    out_lines.append("")

    combined = "\n".join(out_lines)
    total_stats.lines = combined.count("\n") + 1
    return combined, total_stats
