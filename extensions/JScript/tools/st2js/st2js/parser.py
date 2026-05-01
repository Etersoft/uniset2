#!/usr/bin/env python3
"""ST parser wrapper around blark.

Provides parse_st() which parses IEC 61131-3 Structured Text source code
and returns a ParseResult containing the blark typed AST, original source,
and filename for error reporting.

Provides parse_file() which auto-detects file format by extension and
supports plain ST (.st, .txt) and TwinCAT/PLCopen XML (.xml, .TcPOU,
.TcDUT, .TcGVL) input files.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import blark
from blark.parse import parse as blark_parse

from st2js.errors import ParseError

# File extensions handled natively by blark (TwinCAT XML)
_TWINCAT_EXTENSIONS = frozenset(('.tcpou', '.tcdut', '.tcgvl'))

# PLCopen XML extension (requires custom ST extraction)
_PLCOPEN_EXTENSIONS = frozenset(('.xml',))


@dataclass
class GVLBlock:
    """A named Global Variable List extracted from PLCopen XML."""
    name: str
    variables: list  # list of (var_name, type_str, init_value_str)


@dataclass
class ParseResult:
    """Result of parsing ST source code.

    Attributes:
        tree: The blark transformed (typed) AST. This is a blark.transform.SourceCode
              object whose .items list contains Program/FunctionBlock nodes.
        source: The original ST source string.
        filename: The filename used for error reporting.
        gvl_blocks: Named GVL blocks extracted from PLCopen XML (empty for non-XML).
    """

    tree: Any
    source: str
    filename: str
    gvl_blocks: list[GVLBlock] = field(default_factory=list)


def _preprocess_st(source: str) -> str:
    """Preprocess ST source to work around blark grammar limitations.

    Transforms:
    - Typed literals: CHAR#'a' -> 'a', LDT#... -> ..., SINT#5 -> 5
    - Boolean function calls: AND(x1,x2) -> (x1 AND x2), OR(x1,x2) -> (x1 OR x2)
    """
    import re

    # Replace typed literals: TYPE#value -> value
    # Matches: CHAR#'a', SINT#5, USINT#16#FF, LDT#2024-..., DATE#..., etc.
    source = re.sub(
        r'\b(?:CHAR|WCHAR|SINT|USINT|BYTE|UINT|UDINT|ULINT|LINT|LREAL|'
        r'LDT|LDATE|LTIME|LTIME_OF_DAY|LTOD|DATE_AND_TIME|DT|DATE|'
        r'TIME_OF_DAY|TOD)#',
        '',
        source,
    )

    # Replace AND(x1,x2) -> (x1 AND x2) — only for boolean operator function form
    # Similarly OR(x1,x2) -> (x1 OR x2), XOR(x1,x2) -> (x1 XOR x2)
    for op in ('AND', 'OR', 'XOR'):
        pattern = re.compile(r'\b' + op + r'\s*\(\s*([^,)]+)\s*,\s*([^)]+)\s*\)')
        source = pattern.sub(r'(\1 ' + op + r' \2)', source)

    return source


def parse_st(source: str, filename: str = "<stdin>") -> ParseResult:
    """Parse IEC 61131-3 Structured Text source code.

    Uses blark to parse the source and transform the lark parse tree into
    blark's typed AST dataclasses (Program, FunctionBlock, etc.).

    Args:
        source: The ST source code string.
        filename: Filename for error messages (default: "<stdin>").

    Returns:
        ParseResult with the typed AST, source, and filename.

    Raises:
        ParseError: If the source cannot be parsed, with line/col when available.
    """
    # Preprocess: work around blark grammar limitations
    source = _preprocess_st(source)

    try:
        blark_result = blark.parse_source_code(source, fn=filename)
    except Exception as exc:
        raise ParseError(
            message=str(exc),
            file=filename,
        ) from exc

    # blark stores parse errors in result.exception instead of raising
    if blark_result.exception is not None:
        exc = blark_result.exception
        line = getattr(exc, "line", None)
        col = getattr(exc, "column", None)
        raise ParseError(
            message=str(exc),
            file=filename,
            line=line,
            col=col,
        ) from exc

    # Transform lark.Tree into blark typed AST dataclasses
    try:
        transformed = blark_result.transform()
    except Exception as exc:
        raise ParseError(
            message=f"AST transformation failed: {exc}",
            file=filename,
        ) from exc

    return ParseResult(
        tree=transformed,
        source=source,
        filename=filename,
    )


def parse_file(path: str) -> ParseResult:
    """Parse an ST source file, auto-detecting format by extension.

    Supports plain ST files (.st, .txt) and TwinCAT/PLCopen XML files
    (.xml, .TcPOU, .TcDUT, .TcGVL).  For XML files, blark.parse.parse()
    handles extraction of ST code from CDATA sections.

    Args:
        path: Filesystem path to the input file.

    Returns:
        ParseResult with the typed AST, source, and filename.

    Raises:
        ParseError: If the file cannot be read or parsed.
    """
    file_path = Path(path)
    ext = file_path.suffix.lower()

    if ext in _TWINCAT_EXTENSIONS:
        return _parse_twincat_file(file_path)

    if ext in _PLCOPEN_EXTENSIONS:
        return _parse_plcopen_xml(file_path)

    # Plain ST / text file
    try:
        source = file_path.read_text()
    except FileNotFoundError:
        raise ParseError(
            message=f"file not found: {file_path}",
            file=str(file_path),
        )
    except OSError as exc:
        raise ParseError(
            message=f"cannot read file: {exc}",
            file=str(file_path),
        )

    return parse_st(source, filename=str(file_path))


def _parse_twincat_file(file_path: Path) -> ParseResult:
    """Parse a TwinCAT XML file (.TcPOU, .TcDUT, .TcGVL) using blark.

    Collects all successfully parsed items from the XML file and merges
    them into a single SourceCode AST.  If there are parse errors in some
    items but at least one succeeds, the successful items are returned.

    Args:
        file_path: Path to the TwinCAT XML file.

    Returns:
        ParseResult with merged SourceCode tree.

    Raises:
        ParseError: If no items could be parsed from the file.
    """
    from blark.transform import SourceCode

    filename = str(file_path)

    try:
        blark_results = list(blark_parse(file_path))
    except Exception as exc:
        raise ParseError(
            message=f"XML parsing failed: {exc}",
            file=filename,
        ) from exc

    if not blark_results:
        raise ParseError(
            message="no POU definitions found in XML file",
            file=filename,
        )

    # Collect all successfully transformed items
    all_items = []
    errors = []
    source_parts = []

    for result in blark_results:
        if result.exception is not None:
            errors.append(str(result.exception))
            continue
        if result.tree is None:
            continue

        try:
            transformed = result.transform()
        except Exception as exc:
            errors.append(f"transform failed for {result.identifier}: {exc}")
            continue

        all_items.extend(transformed.items)
        source_parts.append(result.source_code)

    if not all_items:
        error_detail = "; ".join(errors) if errors else "unknown error"
        raise ParseError(
            message=f"all POU definitions failed to parse: {error_detail}",
            file=filename,
        )

    merged_source = "\n\n".join(source_parts)
    merged_tree = SourceCode(items=all_items)

    return ParseResult(
        tree=merged_tree,
        source=merged_source,
        filename=filename,
    )


def _parse_plcopen_xml(file_path: Path) -> ParseResult:
    """Parse a PLCopen XML file by reconstructing ST source from XML structure.

    Codesys PLCopen XML exports store POU declarations (PROGRAM,
    FUNCTION_BLOCK, FUNCTION), data types, global variables, and actions
    as XML elements.  The ST body code is embedded inside ``<ST>`` elements,
    either as direct text/CDATA or within an ``<xhtml>`` child element.

    This function reconstructs syntactically valid IEC 61131-3 Structured
    Text source code from the XML structure, then parses it with blark.

    Two styles are supported:

    1. **CDATA style** -- ``<ST><![CDATA[PROGRAM Main ... END_PROGRAM]]></ST>``
       The ST text already contains full POU wrappers; we just extract & join.

    2. **Codesys xhtml style** -- ``<pou pouType="program" name="Main">``
       with ``<interface>`` holding variable sections and ``<body><ST><xhtml>``
       holding only the body statements.  We reconstruct the ST wrapper from
       the XML structure.

    Args:
        file_path: Path to the PLCopen XML file.

    Returns:
        ParseResult with the typed AST.

    Raises:
        ParseError: If the file cannot be read, is not valid XML, or
            contains no parseable ST code.
    """
    import xml.etree.ElementTree as ET

    filename = str(file_path)

    try:
        xml_tree = ET.parse(file_path)
    except FileNotFoundError:
        raise ParseError(
            message=f"file not found: {file_path}",
            file=filename,
        )
    except ET.ParseError as exc:
        raise ParseError(
            message=f"invalid XML: {exc}",
            file=filename,
        )
    except OSError as exc:
        raise ParseError(
            message=f"cannot read file: {exc}",
            file=filename,
        )

    root = xml_tree.getroot()

    # --- Helper: get the local tag name stripping any namespace prefix ---
    def _local(tag: str) -> str:
        return tag.split("}")[-1] if "}" in tag else tag

    # --- Helper: extract ST text from an <ST> element ---
    def _st_text(st_elem) -> str:
        """Return the ST source from an <ST> element (CDATA or xhtml child)."""
        if st_elem.text and st_elem.text.strip():
            return st_elem.text.strip()
        for child in st_elem:
            if child.text and child.text.strip():
                return child.text.strip()
        return ""

    # --- Helper: extract type string from a <type> element ---
    def _type_str(type_elem) -> str:
        """Convert a PLCopen <type> element to an ST type string."""
        for child in type_elem:
            lt = _local(child.tag)
            if lt == "derived":
                return child.get("name", "UNKNOWN")
            elif lt == "array":
                # ARRAY [lo..hi] OF base_type
                dims = []
                base = "UNKNOWN"
                for ac in child:
                    acl = _local(ac.tag)
                    if acl == "dimension":
                        lo = ac.get("lower", "0")
                        hi = ac.get("upper", "0")
                        dims.append(f"{lo}..{hi}")
                    elif acl == "baseType":
                        base = _type_str(ac)
                return f"ARRAY [{', '.join(dims)}] OF {base}"
            elif lt == "string":
                length = child.get("length", "")
                return f"STRING[{length}]" if length else "STRING"
            elif lt == "wstring":
                length = child.get("length", "")
                return f"WSTRING[{length}]" if length else "WSTRING"
            elif lt == "pointer":
                inner = _type_str(child)
                return f"POINTER TO {inner}"
            elif lt == "enum":
                # inline enum -- just use INT as approximation
                return "INT"
            elif lt == "subrange":
                return "INT"
            else:
                # Simple type like <BOOL/>, <INT/>, <REAL/>, <TIME/>, etc.
                return lt.upper()
        return "UNKNOWN"

    # --- Helper: extract initial value from <initialValue> ---
    def _init_value(init_elem) -> str:
        """Return ST literal for a <simpleValue> initializer, or empty."""
        for child in init_elem:
            lt = _local(child.tag)
            if lt == "simpleValue":
                return child.get("value", "")
            # structValue / arrayValue are too complex; skip them
        return ""

    # --- Helper: reconstruct a VAR section ---
    def _var_section(section_elem) -> str:
        """Convert an <inputVars>, <localVars>, etc. to a VAR block."""
        lt = _local(section_elem.tag)
        kw_map = {
            "inputVars": "VAR_INPUT",
            "outputVars": "VAR_OUTPUT",
            "inOutVars": "VAR_IN_OUT",
            "localVars": "VAR",
            "tempVars": "VAR_TEMP",
            "externalVars": "VAR_EXTERNAL",
        }
        keyword = kw_map.get(lt)
        if keyword is None:
            return ""

        attrs = []
        if section_elem.get("constant", "false").lower() == "true":
            attrs.append("CONSTANT")
        if section_elem.get("retain", "false").lower() == "true":
            attrs.append("RETAIN")
        if section_elem.get("persistent", "false").lower() == "true":
            attrs.append("PERSISTENT")

        header = keyword
        if attrs:
            header += " " + " ".join(attrs)

        lines = [header]
        for var_elem in section_elem:
            if _local(var_elem.tag) != "variable":
                continue
            vname = var_elem.get("name", "???")
            vtype = "UNKNOWN"
            vinit = ""
            for vc in var_elem:
                vcl = _local(vc.tag)
                if vcl == "type":
                    vtype = _type_str(vc)
                elif vcl == "initialValue":
                    vinit = _init_value(vc)

            decl = f"    {vname} : {vtype}"
            if vinit:
                decl += f" := {vinit}"
            decl += ";"
            lines.append(decl)

        lines.append("END_VAR")
        # Only emit if there are actual variables
        if len(lines) <= 2:  # just header + END_VAR
            return ""
        return "\n".join(lines)

    # --- Helper: reconstruct a struct TYPE ---
    def _struct_type(dt_elem) -> str:
        """Convert a <dataType> with <struct> to a TYPE declaration."""
        name = dt_elem.get("name", "UNKNOWN")
        # Find <baseType>/<struct>
        for bt in dt_elem.iter():
            if _local(bt.tag) == "struct":
                lines = [f"TYPE {name} :", "STRUCT"]
                for var in bt:
                    if _local(var.tag) != "variable":
                        continue
                    vname = var.get("name", "???")
                    vtype = "UNKNOWN"
                    for vc in var:
                        if _local(vc.tag) == "type":
                            vtype = _type_str(vc)
                    lines.append(f"    {vname} : {vtype};")
                lines.append("END_STRUCT")
                lines.append("END_TYPE")
                return "\n".join(lines)

        # Enum dataType
        for bt in dt_elem.iter():
            if _local(bt.tag) == "enum":
                values = []
                for v in bt.iter():
                    if _local(v.tag) == "value":
                        vn = v.get("name", "")
                        vv = v.get("value", "")
                        if vn:
                            values.append(f"{vn} := {vv}" if vv else vn)
                if values:
                    return (
                        f"TYPE {name} :\n"
                        f"({', '.join(values)});\n"
                        f"END_TYPE"
                    )
        return ""

    # --- Helper: reconstruct a POU ---
    def _reconstruct_pou(pou_elem) -> str:
        """Reconstruct ST source for a single <pou> element."""
        pou_type = pou_elem.get("pouType", "program")
        name = pou_elem.get("name", "UNKNOWN")

        # Determine ST keyword and end keyword
        kw_map = {
            "program": ("PROGRAM", "END_PROGRAM"),
            "functionBlock": ("FUNCTION_BLOCK", "END_FUNCTION_BLOCK"),
            "function": ("FUNCTION", "END_FUNCTION"),
        }
        kw_start, kw_end = kw_map.get(pou_type, ("PROGRAM", "END_PROGRAM"))

        parts = []

        # For functions, find return type
        return_type = ""
        for iface in pou_elem:
            if _local(iface.tag) == "interface":
                for child in iface:
                    if _local(child.tag) == "returnType":
                        return_type = _type_str(child)

        # Header line
        if kw_start == "FUNCTION" and return_type:
            parts.append(f"{kw_start} {name} : {return_type}")
        else:
            parts.append(f"{kw_start} {name}")

        # Variable sections from <interface>
        for iface in pou_elem:
            if _local(iface.tag) == "interface":
                for section in iface:
                    sl = _local(section.tag)
                    if sl in (
                        "inputVars", "outputVars", "inOutVars",
                        "localVars", "tempVars", "externalVars",
                    ):
                        vs = _var_section(section)
                        if vs:
                            parts.append(vs)

        # Body from <body>/<ST>
        for body in pou_elem:
            if _local(body.tag) == "body":
                for st in body:
                    if _local(st.tag) == "ST":
                        text = _st_text(st)
                        if text:
                            parts.append(text)

        # Collect parent VAR sections for actions (actions share parent's scope)
        parent_var_sections = []
        for iface in pou_elem:
            if _local(iface.tag) == "interface":
                for section in iface:
                    sl = _local(section.tag)
                    if sl in ("localVars", "tempVars"):
                        vs = _var_section(section)
                        if vs:
                            parent_var_sections.append(vs)

        # Collect actions from <actions> — emitted as separate PROGRAM blocks
        # sharing the parent's local variables
        action_functions = []
        for actions in pou_elem:
            if _local(actions.tag) == "actions":
                for action in actions:
                    if _local(action.tag) == "action":
                        aname = action.get("name", "UNKNOWN")
                        abody = ""
                        for ab in action:
                            if _local(ab.tag) == "body":
                                for st in ab:
                                    if _local(st.tag) == "ST":
                                        abody = _st_text(st)
                        if abody:
                            var_block = "\n".join(parent_var_sections) if parent_var_sections else "VAR\nEND_VAR"
                            qualified_name = f"{name}_{aname}"
                            action_functions.append(
                                f"PROGRAM {qualified_name}\n"
                                f"{var_block}\n\n"
                                f"{abody}\n\n"
                                f"END_PROGRAM"
                            )

        parts.append(kw_end)

        result = "\n\n".join(parts)
        # Append action functions after the POU
        if action_functions:
            result += "\n\n" + "\n\n".join(action_functions)
        return result

    # ------------------------------------------------------------------
    # Phase 1: Try simple CDATA extraction (already-complete ST source)
    # ------------------------------------------------------------------
    st_texts_cdata = []
    for elem in root.iter():
        tag = elem.tag
        if tag == "ST" or (isinstance(tag, str) and tag.endswith("}ST")):
            if elem.text and elem.text.strip():
                text = elem.text.strip()
                # Heuristic: if it starts with a POU keyword, it's full ST
                first_word = text.lstrip("(* ").split()[0].upper() if text.strip() else ""
                if first_word in (
                    "PROGRAM", "FUNCTION_BLOCK", "FUNCTION",
                    "TYPE", "VAR_GLOBAL", "INTERFACE",
                ):
                    st_texts_cdata.append(text)

    if st_texts_cdata:
        combined_source = "\n\n".join(st_texts_cdata)
        return parse_st(combined_source, filename=filename)

    # ------------------------------------------------------------------
    # Phase 2: Reconstruct ST from XML POU structure (Codesys xhtml style)
    # ------------------------------------------------------------------
    source_parts = []

    # 2a. Data types (TYPE ... END_TYPE)
    for elem in root.iter():
        if _local(elem.tag) == "dataType" and elem.get("name"):
            st = _struct_type(elem)
            if st:
                source_parts.append(st)

    # 2b. Global variable lists — collect as structured metadata
    gvl_blocks: list[GVLBlock] = []
    for elem in root.iter():
        if _local(elem.tag) == "globalVars":
            gname = elem.get("name", "")
            gvl_vars = []
            for var in elem:
                if _local(var.tag) != "variable":
                    continue
                vname = var.get("name", "???")
                vtype = "UNKNOWN"
                vinit = ""
                for vc in var:
                    vcl = _local(vc.tag)
                    if vcl == "type":
                        vtype = _type_str(vc)
                    elif vcl == "initialValue":
                        vinit = _init_value(vc)
                gvl_vars.append((vname, vtype, vinit))
            if gvl_vars:
                gvl_blocks.append(GVLBlock(name=gname, variables=gvl_vars))

    # 2c. POUs
    for elem in root.iter():
        if _local(elem.tag) == "pou" and elem.get("pouType"):
            st = _reconstruct_pou(elem)
            if st:
                source_parts.append(st)

    if not source_parts:
        raise ParseError(
            message="no <ST> sections found in PLCopen XML file",
            file=filename,
        )

    combined_source = "\n\n".join(source_parts)
    result = parse_st(combined_source, filename=filename)
    result.gvl_blocks = gvl_blocks
    return result
